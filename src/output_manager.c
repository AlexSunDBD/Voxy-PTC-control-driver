// output_manager.c

#include "output_manager.h"
#include "hardware.h"
#include "safety_timeout.h"
#include "safety_signal.h"  // OUT_STABILIZATION_MS
#include <string.h>

// ============================================================================
// Внутренние переменные
// ============================================================================

static output_state_t output_ctx;

// ============================================================================
// Локальные прототипы
// ============================================================================

static void force_all_outputs_off(void);

// ============================================================================
// Вспомогательные функции
// ============================================================================

/**
 * @brief Преобразовать логическое состояние ТЭНов (маску) в физические уровни GPIO
 */
static bool logical_to_physical(bool state)
{
#if HEATER_OUTPUT_INVERTED_LOGIC
    return !state;
#else
    return state;
#endif
}

/**
 * @brief Установить битовую маску на выходах 
 */
static void gpio_write(bool level, GPIO_TypeDef* port, uint16_t pin)
{
    if (level)
        gpio_set(port, pin);
    else
        gpio_clear(port, pin);
}

static void set_output_bitmask(heater_mask_t bitmask) {
    // ТЭН 1 (PB14) - бит 0
    bool heater1_on = (bitmask & HEATER_MASK_1) != 0;
    bool gpio1_level = logical_to_physical(heater1_on);
    
    gpio_write(gpio1_level, PORT_HEATER_1, PIN_HEATER_1);

    // ТЭН 2 (PB13) - бит 1
    bool heater2_on = (bitmask & HEATER_MASK_2) != 0;
    bool gpio2_level = logical_to_physical(heater2_on);
    
    gpio_write(gpio2_level, PORT_HEATER_2, PIN_HEATER_2);    
    
    // ТЭН 3 (PB12) - бит 2
    bool heater3_on = (bitmask & HEATER_MASK_3) != 0;
    bool gpio3_level = logical_to_physical(heater3_on);
    
    gpio_write(gpio3_level, PORT_HEATER_3, PIN_HEATER_3);
}

static bool physical_to_logical(bool state) {
    #if HEATER_OUTPUT_INVERTED_LOGIC
        return !state;
    #else
        return state;
    #endif
}

/**
 * @brief Прочитать текущую битовую маску с выходов
 */
heater_mask_t output_read_bitmask(void) {
    heater_level_t bitmask = 0;
    
    // ТЭН 1 (PB14) - бит 0
    bool gpio1_state = (gpio_read(PORT_HEATER_1, PIN_HEATER_1) == GPIO_PIN_SET);
    bool heater1_on = physical_to_logical(gpio1_state);
    
    if (heater1_on) {
        bitmask |= HEATER_MASK_1;
    }
    
    // ТЭН 2 (PB13) - бит 1
    bool gpio2_state = (gpio_read(PORT_HEATER_2, PIN_HEATER_2) == GPIO_PIN_SET);
    bool heater2_on = physical_to_logical(gpio2_state);
    
    if (heater2_on) {
        bitmask |= HEATER_MASK_2;
    }
    
    // ТЭН 3 (PB12) - бит 2
    bool gpio3_state = (gpio_read(PORT_HEATER_3, PIN_HEATER_3) == GPIO_PIN_SET);
    bool heater3_on = physical_to_logical(gpio3_state);
    
    if (heater3_on) {
        bitmask |= HEATER_MASK_3;
    }
    
    return bitmask;
}

// ============================================================================
// Публичные функции
// ============================================================================

void output_manager_init(void)
{
    memset(&output_ctx, 0, sizeof(output_ctx));

    output_ctx.last_confirmed = HEATER_STATE_0;

    force_all_outputs_off();

    output_ctx.last_switch_time = 0;
}

void apply_output_state(heater_mask_t target_level)
{
    uint32_t now = time_ms();

    /*
     * При отсутствии активной блокировки
     * выполнить переключение.
     * Механизм switch_lock запрещает дальнейшие
     * переключения в течение MIN_SWITCH_INTERVAL_MS.
     * 
     * Исключение:
     * Пререключение при target_level == HEATER_STATE_0
     * не блокируется.
    */

    /* защита от недопустимой маски */
    if (!is_valid_heater_bitmask(target_level))
        return;

    /* Если состояние не изменилось — ничего не делаем */
    if (target_level == output_ctx.last_confirmed)
        return;

    /* Ноль — всегда немедленно */
    if (target_level == HEATER_STATE_0)
    {
        force_all_outputs_off();
        output_ctx.last_switch_time = now;
        return;
    }

    /* Любое ненулевое состояние — только после интервала */
    if ((now - output_ctx.last_switch_time) < MIN_SWITCH_INTERVAL_MS)
        return;

    /* Применение нового состояния */
    set_output_bitmask(target_level);
    output_ctx.last_confirmed = target_level;
    output_ctx.last_switch_time = now;
}


bool are_all_outputs_off(void)
{
    GPIO_PinState off_level =
        HEATER_OUTPUT_INVERTED_LOGIC ? GPIO_PIN_SET : GPIO_PIN_RESET;

    if (gpio_read(PORT_HEATER_1, PIN_HEATER_1) != off_level)
        return false;

    if (gpio_read(PORT_HEATER_2, PIN_HEATER_2) != off_level)
        return false;

    if (gpio_read(PORT_HEATER_3, PIN_HEATER_3) != off_level)
        return false;

    return true;
}

bool emergency_shutdown(void) {
    force_all_outputs_off();

    /* блокирующая задержка здесь требует отдельного подтверждения по ТЗ */
    delay_ms(OUT_STABILIZATION_MS);
    heater_mask_t actual = output_read_bitmask();

    return (actual == HEATER_STATE_0);
}

const output_state_t* output_get_state(void) {
    return &output_ctx;
}

void output_reset(void) {
        output_ctx.last_confirmed = HEATER_STATE_0;
        force_all_outputs_off();
    
    safety_timeout_reset();
}

static void force_all_outputs_off(void)
{
    set_output_bitmask(HEATER_STATE_0);
    output_ctx.last_confirmed = HEATER_STATE_0;
}


