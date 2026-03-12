//core_fsm.c

#include "core_fsm.h"
#include "hardware.h"
#include "pwm_input.h"
#include "validation.h"
#include "output_manager.h"
#include "safety_signal.h"  
#include "safety_timeout.h"
#include "error_handler.h"
#include "heater_logic.h"
#include "voltage_monitoring.h"
#include <string.h>

// ============================================================================
// Внутренние переменные
// ============================================================================

static core_context_t core_ctx;
static uint8_t zero_confirm_counter = 0;
static system_state_t system_state = SYSTEM_INIT;

// ============================================================================
// Публичные функции
// ============================================================================

void core_fsm_init(void) {
    memset(&core_ctx, 0, sizeof(core_ctx));
    
    core_ctx.target_level = 0;
    
    core_ctx.last_processing_time = time_ms();

    voltage_monitoring_reset_limit();
}

processing_result_t core_process_cycle(const measurement_window_t* window) {
    processing_result_t result = {
        .success = false,
    };
    
    // ====================================================
    // 1. FATAL режим
    // ====================================================
    if (error_handler_get_state()->fatal_state_active)
        return result;

    // ====================================================
    // 2. Контроль целостности выходов (ТЗ 4.6)
    // Проверяем состояние, применённое в предыдущем цикле
    // ====================================================
    const output_state_t* out_state = output_get_state();
    heater_mask_t actual_now = output_read_bitmask();

    if (actual_now != out_state->last_confirmed)
    {
        error_handler_process(ERR_INTEGRITY, 0);
        return result;
    }

    /* Соответствие подтверждено - НОРМА */
    safety_signal_set();

    // ====================================================
    // 3. Обновление служебных параметров
    // ====================================================
    core_ctx.last_processing_time = time_ms();
    core_ctx.cycle_count++;
    
    // ====================================================
    // 4. Валидация измерений
    // ====================================================
    validation_result_t validation = validate_window(window);
    
    if (!validation.is_valid) {
        error_handler_process(ERR_INSUFFICIENT_VALID,
                              validation.valid_samples_count);
        core_ctx.error_count++;
        return result;
    }
    
 
    // ====================================================
    // 5. Расчёт целевого количества нагревателей
    // ====================================================
    
    /* 1. Расчёт требуемого количества тэнов по PWM */
    heater_result_t heater = calculate_target_level_ex(validation.median_pulse_width);

    /* 2  Вычисление лимита */
    int8_t limit = voltage_monitoring_task(heater.target_level);
    //  Отрицательный лимит трактуем, как ошибку.
    if (limit < 0) {
        error_handler_process(ERR_VOLTAGE_NO_NOMINAL, 0);
        return result;
    }

    /* 3. Расчёт итогового количества тэнов */
    int8_t final_state = (int8_t)heater.target_level - (int8_t)limit;
    if (final_state < 0) final_state = 0;
    if (final_state > 3) final_state = 3;

    bluetooth_send_auto_message(heater.target_level, limit, final_state);
            

    /* 6. Преобразование в битовую маску */
    heater_mask_t target_bitmask = state_to_bitmask((heater_level_t)final_state);
    
    // ====================================================
    // 6. Двойное подтверждение нуля
    // ====================================================
    if (heater.is_zero_state)
    {
        zero_confirm_counter++;

        if (zero_confirm_counter < 2)
        {
            /* SAFETY_TIMEOUT стартует сразу при расчёте target=0 */
            safety_timeout_update(target_bitmask);
            /* Ждём второго цикла */
            target_bitmask = out_state->last_confirmed;
        }
    }
    else
    {
        zero_confirm_counter = 0;
    }
    
    core_ctx.target_level = final_state;

    // ====================================================
    // 7. Установка целевого состояния выходов
    // ====================================================
    apply_output_state(target_bitmask);

    // ====================================================
    // 8. SAFETY_TIMEOUT логика (защита от не выключения тэнов)
    // ====================================================
    if (heater.is_zero_state)
    {
        if (safety_timeout_check())
        {
            error_handler_process(ERR_SAFETY_TIMEOUT, 0);
            return result;
        }

        /* Если физически уже отключились — сбрасываем таймер */
        if (are_all_outputs_off())
        {
            safety_timeout_reset();
        }
    }

    // ====================================================
    // 9. Завершение цикла
    // ====================================================
    result.success = true;
    safety_timeout_update_activity();

    core_ctx.success_count++;

    result.actual_state = actual_now;

    result.target_level = final_state;
    // а нужен ли этот "snapshot наружу"? Может есть смысл оптимизировать структуру и обойтись одной переменной?

    return result;
}

const core_context_t* core_get_context(void) {
    return &core_ctx;
}

void core_reset_state(void) {
    
    core_ctx.target_level = 0;
}


