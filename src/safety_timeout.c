// safety_timeout.c

#include "safety_timeout.h"
#include "hardware.h"
#include "output_manager.h"
#include <string.h>

// ============================================================================
// Внутренние переменные
// ============================================================================

static safety_timer_state_t safety_ctx;

// ============================================================================
// Вспомогательные функции
// ============================================================================

/**
 * @brief Проверить, отключены ли все выходы (новые пины)
 */
static bool is_physical_off(GPIO_TypeDef* port, uint16_t pin)
{
    GPIO_PinState state = gpio_read(port, pin);

#if HEATER_OUTPUT_INVERTED_LOGIC
    return (state == GPIO_PIN_SET);     // OFF = 1
#else
    return (state == GPIO_PIN_RESET);   // OFF = 0
#endif
}

bool safety_outputs_are_physically_off(void)
{
    return
        is_physical_off(PORT_HEATER_1, PIN_HEATER_1) &&
        is_physical_off(PORT_HEATER_2, PIN_HEATER_2) &&
        is_physical_off(PORT_HEATER_3, PIN_HEATER_3);
}

static bool safety_timeout_expired(void)
{
    if (!safety_ctx.active)
        return false;

    uint32_t elapsed = time_ms() - safety_ctx.start_time;
    return (elapsed >= SAFETY_TIMEOUT_MS);
}

// ============================================================================
// Публичные функции
// ============================================================================

void safety_timeout_init(void) {
    memset(&safety_ctx, 0, sizeof(safety_ctx));
    
    safety_ctx.active = false;
    safety_ctx.timeout_triggered = false;
    safety_ctx.timeout_count = 0;
    
}

bool safety_timeout_update(heater_level_t target_level) {
    // Проверяем целевое состояние
    bool target_is_zero = (target_level == HEATER_STATE_0);
    
    if (target_is_zero) {
        // Целевое состояние = 0
        if (!safety_ctx.active) {
            // Стартуем таймер при первом обнаружении нулевого состояния
            safety_ctx.active = true;
            safety_ctx.start_time = time_ms();
            safety_ctx.timeout_triggered = false;
        }
        // Повторные нулевые состояния не перезапускают таймер
    } 
    /* При target ≠ 0 проверка не производится.
       Таймер НЕ сбрасывается принудительно. */

    bool timer_active = safety_ctx.active;
    return timer_active;
}

bool safety_timeout_check(void) {

    /*
    * SAFETY_TIMEOUT стартует при первом логическом запросе на 0.
    * Он контролирует задержку между вычисленным target=0
    * и фактическим отключением физических выходов.
    * Повторные нули таймер не перезапускают.
    */

    if (!safety_ctx.active || safety_ctx.timeout_triggered) {
        // Таймер не активен или уже сработал
        return false;
    }

    // Проверяем, истекло ли время
    if (!safety_timeout_expired()) {
        // Время ещё не истекло
        return false;
    }
    
    if (safety_outputs_are_physically_off())
    // Выходы отключены - безопасно
    {
        safety_ctx.active = false;
        return false;
    }

    safety_ctx.timeout_triggered = true;
    safety_ctx.timeout_count++;

    return true;
}

void safety_timeout_reset(void) {
    // Проверяем, что все выходы действительно отключены
    bool outputs_off = safety_outputs_are_physically_off();
    
    if (outputs_off) {
        // Выходы отключены - сбрасываем таймер
        safety_ctx.active = false;
        safety_ctx.timeout_triggered = false;
    }
    // Если выходы не отключены - не сбрасываем таймер
    
}

const safety_timer_state_t* safety_get_state(void) {
    return &safety_ctx;
}

safety_check_result_t safety_get_check_result(void) {
    safety_check_result_t result = {
        .safety_ok = true,
        .timer_active = false,
        .timeout_detected = false,
        .elapsed_ms = 0,
        .remaining_ms = 0,
        .timestamp_ms = time_ms()
    };
    
    result.timer_active = safety_ctx.active;
    result.timeout_detected = safety_ctx.timeout_triggered;
    
    if (safety_ctx.active) {
        uint32_t elapsed = time_ms() - safety_ctx.start_time;
        result.elapsed_ms = elapsed;
        
        if (elapsed < SAFETY_TIMEOUT_MS) {
            result.remaining_ms = SAFETY_TIMEOUT_MS - elapsed;
        } else {
            result.remaining_ms = 0;
        }
        
        // Проверяем безопасность
        if (safety_timeout_expired()) {
            // Время истекло, проверяем выходы
            bool outputs_off = safety_outputs_are_physically_off();
            result.safety_ok = outputs_off;
        } else {
            // Время ещё не истекло
            result.safety_ok = true;
        }
    }
    
    return result;
}


