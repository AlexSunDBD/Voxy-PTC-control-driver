// main.c

// ============================================================================
// v. 6.1_003 - Исправлены ошибки компиляции
// ============================================================================

#include "hardware.h"
#include "pwm_input.h"
#include "validation.h"
#include "heater_logic.h"
#include "core_fsm.h"
#include "voltage_monitoring.h"
#include "output_manager.h"
#include "safety_signal.h"
#include "safety_timeout.h"
#include "error_handler.h"
#include "led_fsm.h"
#include "bluetooth.h"
#include <string.h>

// ============================================================================
// Константы системы
// ============================================================================


// ============================================================================
// Глобальные переменные системы
// ============================================================================

typedef struct {
    bool init_phase;
} system_global_state_t;

static system_global_state_t system_state;
// conflicting types for 'system_state'

// ============================================================================
// Вспомогательные функции
// ============================================================================

// ============================================================================
// Функции инициализации
// ============================================================================

static void system_initialize(void) {
    // 1. Очищаем состояние системы
    memset(&system_state, 0, sizeof(system_state));
    
    // 2. Инициализация железа (включает UART)
    hardware_init();
    
    // 3. Небольшая задержка для стабилизации UART
    HAL_Delay(50);
    
    // 4. Инициализация модулей в правильном порядке
    bluetooth_init();           // UART уже готов

    send_line("BT OK");
    
    pwm_input_init();
    send_line("PWM OK");
    
    safety_signal_init();
    send_line("SS OK");
    
    core_fsm_init();
    voltage_monitoring_init();
    send_line("VM OK");

    output_manager_init();
    safety_timeout_init();
    error_handler_init();
    led_fsm_init();
    
    // Устанавливаем начальное состояние
    system_state.init_phase = true;
    
    // Небольшая пауза перед финальным READY
    HAL_Delay(10);

    // Отправляем READY сообщение
    send_line("\r\nREADY\r\n");
}

static void MX_IWDG_Init(void) {
    extern IWDG_HandleTypeDef hiwdg;
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = IWDG_TIMEOUT_MS;  // 750 мс таймаут
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}

// ============================================================================
// Фоновые задачи
// ============================================================================

static void system_background_tasks(void) {
    static uint32_t last_background_run = 0;
    uint32_t current_time = time_ms();

    
    if ((current_time - last_background_run) >= 10) {
        // ВСЕГДА выполняем:
        bluetooth_process();
        led_fsm_update();
                
        // Проверка PB15 (только после grace периода)
        safety_signal_error_t safety_error = safety_signal_process();
    
        // Если обнаружена ошибка5 (PB15 ≠ 0 при отсутствии НОРМЫ)
        if (safety_error == SAFETY_SIGNAL_ERROR_LEVEL) {
            // НЕМЕДЛЕННЫЙ FATAL согласно ТЗ 6.1
            error_handler_enter_fatal(ERR_SAFETY_SIGNAL_FAIL);
            pwm_input_stop();  // Останавливаем измерения в FATAL

            // LED режим установится автоматически через led_fsm_auto_update
        }
            
        // В FATAL режиме остальное НЕ выполняем
        if (error_handler_get_state()->fatal_state_active) {
            last_background_run = current_time;
            return;
        }

        // Автоматическое обновление LED режима
        const error_state_t* error_state = error_handler_get_state();
        uint8_t current_output_state = output_read_bitmask();        
        
        led_fsm_auto_update(error_state, current_output_state);
        
        last_background_run = current_time;
    }
}

// ============================================================================
// Основные задачи
// ============================================================================

static void system_main_tasks(void) {
    if (error_handler_get_state()->fatal_state_active) {
        return;
    }
    
    const error_state_t* error_state = error_handler_get_state();
    
    if (error_state->error_state_active)
{
    if (error_handler_timeout_expired())
    {
        error_handler_resume();

        pwm_input_reset();
        core_reset_state();
        
        system_state.init_phase = true;
    }

    return;
}
}

// ============================================================================
// Точка входа
// ============================================================================

int main(void) {
    system_initialize();
    
    MX_IWDG_Init();

    //====================================== WHILE(1) ================================================
    while (1)
    {
        if (system_state.init_phase)
        {
            measurement_window_t window;

            if (pwm_try_get_window(&window))
            {
                processing_result_t result = core_process_cycle(&window);

                if (result.success)
                {
                    system_state.init_phase = false;
                }
            }

            continue;
        }

        system_background_tasks();

        if (!error_handler_get_state()->fatal_state_active)
        {
            system_main_tasks();

            measurement_window_t window;

            if (pwm_try_get_window(&window))
            {
                core_process_cycle(&window);
            }

            if (error_handler_check_processing_timeout(core_get_context()->last_processing_time))
            {
                error_handler_process(ERR_PROCESSING_TIMEOUT, 0);
            }
        }

        iwdg_refresh();
    }
    
    return 0;
}


