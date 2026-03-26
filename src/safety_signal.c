// safety_signal.c

#include "safety_signal.h"
#include "hardware.h"
#include <string.h>

// ============================================================================
// Внутренние переменные
// ============================================================================

static safety_signal_state_t safety_ctx;

// ============================================================================
// Вспомогательные функции
// ============================================================================

static void set_safety_signal_physical(bool state) {
    if (state) {
        SAFETY_SIGNAL_ON;    // PB15 = 1
    } else {
        SAFETY_SIGNAL_OFF;   // PB15 = 0
    }
    
    safety_ctx.last_change_time = time_ms();
}

static bool read_safety_signal_physical(void) {
    return (gpio_read(PORT_SAFETY_SIGNAL, PIN_SAFETY_SIGNAL) == GPIO_PIN_SET);
}

// ============================================================================
// Публичные функции
// ============================================================================

void safety_signal_init(void) {
    memset(&safety_ctx, 0, sizeof(safety_ctx));
    
    // Начальное состояние согласно ТЗ: PB15 = 0
    safety_ctx.target_state = false;
    safety_ctx.actual_state = false;
    safety_ctx.last_confirmed = false;
    safety_ctx.error_count = 0;
    safety_ctx.check_count = 0;
    
    // Устанавливаем физическое состояние
    set_safety_signal_physical(false);
}


safety_signal_result_t safety_signal_set(void)
{
    safety_signal_result_t result = {
        .success = false,
        .state_changed = false,
        .error = SAFETY_SIGNAL_ERROR_NONE,
        .timestamp_ms = time_ms()
    };

    bool current_state = read_safety_signal_physical();

    safety_ctx.target_state = true;

    set_safety_signal_physical(true);

    result.state_changed = (current_state != true);

    safety_ctx.actual_state = true;
    safety_ctx.last_confirmed = true;

    // --- ОТКЛЮЧАЕМ ПРОВЕРКУ ---
    safety_ctx.first_check_pending = false;
    safety_ctx.last_periodic_check_ms = 0;

    result.success = true;

    return result;
}

safety_signal_result_t safety_signal_clear(void)
{
    safety_signal_result_t result = {
        .success = false,
        .state_changed = false,
        .error = SAFETY_SIGNAL_ERROR_NONE,
        .timestamp_ms = time_ms()
    };

    bool current_state = read_safety_signal_physical();

    safety_ctx.target_state = false;

    set_safety_signal_physical(false);

    result.state_changed = (current_state != false);

    safety_ctx.actual_state = false;
    safety_ctx.last_confirmed = false;

    // --- ЗАПУСК СОБЫТИЙНОЙ ПРОВЕРКИ ---
    safety_ctx.clear_time_ms = time_ms();
    safety_ctx.first_check_pending = true;
    safety_ctx.last_periodic_check_ms = 0;

    result.success = true;

    return result;
}

safety_signal_error_t safety_signal_process(void)
{
    uint32_t now = time_ms();
    bool physical = read_safety_signal_physical();
    safety_ctx.actual_state = physical;
    
    // Проверяем ТОЛЬКО если НОРМА снята
    if (safety_ctx.target_state == false)
    {
        // --- Первая проверка ---
        if (safety_ctx.first_check_pending)
        {
            if (now - safety_ctx.clear_time_ms >= OUT_STABILIZATION_MS)
            {
                if (safety_ctx.check_count < UINT32_MAX) {
                    safety_ctx.check_count++;
                }

                if (physical != false)
                {
                    if (safety_ctx.error_count < UINT32_MAX) {
                        safety_ctx.error_count++;
                    }
                    return SAFETY_SIGNAL_ERROR_LEVEL;
                }

                safety_ctx.first_check_pending = false;
                safety_ctx.last_periodic_check_ms = now;
                safety_ctx.last_confirmed = false;
            }
        }
        // --- Периодическая проверка ---
        else
        {
            if (now - safety_ctx.last_periodic_check_ms >= SAFETY_CHECK_INTERVAL_MS)
            {
                if (safety_ctx.check_count < UINT32_MAX) {
                    safety_ctx.check_count++;
                }

                if (physical != false)
                {
                    if (safety_ctx.error_count < UINT32_MAX) {
                        safety_ctx.error_count++;
                    }
                    return SAFETY_SIGNAL_ERROR_LEVEL;
                }

                safety_ctx.last_periodic_check_ms = now;
            }
        }
    }

    return SAFETY_SIGNAL_ERROR_NONE;
}

bool safety_signal_read_state(void) {
    bool state = safety_ctx.last_confirmed;
    return state;
}

bool safety_signal_read_physical_state(void) {
    return read_safety_signal_physical();
}

const safety_signal_state_t* safety_signal_get_state(void) {
    return &safety_ctx;
}


