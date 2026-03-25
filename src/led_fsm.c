// led_fsm.с

#include "led_fsm.h"
#include "hardware.h"
#include <string.h>

// ============================================================================
// Внутренние переменные
// ============================================================================

#define FATAL_MAP_SIZE (sizeof(fatal_map) / sizeof(fatal_map[0]))

static led_fsm_state_t led_fsm;

// Таблица соответствия типов ошибок и кодов индикации
typedef struct {
    error_type_t type;
    uint8_t code;
} fatal_map_t;

static const fatal_map_t fatal_map[] = {
    { ERR_INSUFFICIENT_VALID,   1 },
    { ERR_INTEGRITY,            2 },
    { ERR_SAFETY_TIMEOUT,       3 },
    { ERR_PROCESSING_TIMEOUT,   4 },
    { ERR_SAFETY_SIGNAL_FAIL,   5 },
    { ERR_FATAL,                6 },
    { ERR_VOLTAGE_NO_NOMINAL,   7 },
};

// ============================================================================
// Вспомогательные функции
// ============================================================================

static void led_off(void) {
    HAL_GPIO_WritePin(PORT_LED, PIN_LED, GPIO_PIN_SET);
    led_fsm.led_on = false;
}

static void led_on(void) {
    HAL_GPIO_WritePin(PORT_LED, PIN_LED, GPIO_PIN_RESET);
    led_fsm.led_on = true;
}

static uint8_t get_target_flashes(void) {
    switch (led_fsm.current_mode) {
        case LED_MODE_ACTIVE:
            return OUTPUT_STATE_TO_FLASHES(led_fsm.output_state);
            
        case LED_MODE_ERROR:
            return led_fsm.error_code;
            
        case LED_MODE_FATAL:
        {
            const error_state_t* es = error_handler_get_state();
            uint8_t visible = 0;

            for (uint8_t i = 0; i < es->error_history_count; i++) {
                uint8_t idx = (es->error_history_index + ERROR_HISTORY_SIZE - es->error_history_count + i) % ERROR_HISTORY_SIZE;
                error_type_t t = es->error_history[idx].type;
                bool is_fatal = es->error_history[idx].is_fatal;

                if (!is_fatal) continue;

                for (uint8_t j = 0; j < FATAL_MAP_SIZE; j++) {
                    if (fatal_map[j].type == t) {
                        if (visible == led_fsm.fatal_code_index) return fatal_map[j].code;
                        visible++;
                        break;
                    }
                }
            }

            return 0;
        }

        default:
            return 0;
    }
}

static uint8_t fatal_visible_count(void)
{
    const error_state_t* es = error_handler_get_state();
    uint8_t count = 0;

    for (uint8_t i = 0; i < es->error_history_count; i++) {
        uint8_t idx = (es->error_history_index + ERROR_HISTORY_SIZE - es->error_history_count + i) % ERROR_HISTORY_SIZE;
        if (es->error_history[idx].is_fatal) {
            count++;
        }
    }

    return count;
}

// ============================================================================
// Публичные функции
// ============================================================================

void led_fsm_init(void) {
    memset(&led_fsm, 0, sizeof(led_fsm));
    
    led_fsm.current_mode = LED_MODE_INIT;
    led_fsm.error_code = 0;
    led_fsm.output_state = 0;
    led_fsm.step = 0;
    led_fsm.timer = time_ms();
    led_fsm.flash_count = 0;
    led_fsm.fatal_code_index = 0;
    led_fsm.led_on = false;
    
    led_off();
}

void led_fsm_set_mode(led_mode_t mode, uint8_t error_code, uint8_t output_state) {
    // FATAL - терминальный режим, нельзя выйти без сброса
    if (led_fsm.current_mode == LED_MODE_FATAL && mode != LED_MODE_FATAL) {
        return;
    }
    
    // Обновляем состояние
    led_fsm.current_mode = mode;
    led_fsm.error_code = error_code;
    led_fsm.output_state = output_state;
    led_fsm.step = 0;
    led_fsm.timer = time_ms();
    led_fsm.flash_count = 0;
    led_fsm.fatal_code_index = 0;
    led_fsm.led_on = false;

    led_off();
}

void led_fsm_update(void) {
    uint32_t current_time = time_ms();
    
    switch (led_fsm.step) {
        case 0:
            {
                uint32_t pulse =
                    (led_fsm.current_mode == LED_MODE_ACTIVE)
                    ? LED_SHORT_PULSE_MS
                    : LED_LONG_PULSE_MS;

                led_on();

                if ((current_time - led_fsm.timer) >= pulse) {
                    led_off();
                    led_fsm.step = 1;
                    led_fsm.timer = current_time;
                }
            }
            break;
            
        case 1:
            switch (led_fsm.current_mode) {
                case LED_MODE_INIT:
                    if ((current_time - led_fsm.timer) >= LED_CYCLE_PAUSE_MS) {
                        led_fsm.step = 3;
                        led_fsm.timer = current_time;
                    }
                    break;
                    
                case LED_MODE_ACTIVE:
                case LED_MODE_ERROR:
                {
                    if ((current_time - led_fsm.timer) >= LED_LONG_PAUSE_MS) {

                        uint8_t target = get_target_flashes();

                        if (target == 0) {
                            led_fsm.step = 3;
                        } else {
                            led_fsm.step = 2;
                            led_fsm.flash_count = 0;
                        }

                        led_fsm.timer = current_time;
                    }
                }
                break;
                    
                case LED_MODE_FATAL:
                    if ((current_time - led_fsm.timer) >= LED_LONG_PAUSE_MS) {
                        if (fatal_visible_count() > 0) {
                            led_fsm.step = 2;
                            led_fsm.flash_count = 0;
                        } else {
                            led_fsm.step = 3;
                        }
                        led_fsm.timer = current_time;
                    }
                    break;
            }
            break;
            
        case 2:
        {
            uint8_t target_flashes = get_target_flashes();
            
            if (target_flashes == 0) {
                led_fsm.step = 3;
                led_fsm.timer = current_time;
                break;
            }
            
            if (!led_fsm.led_on && led_fsm.flash_count == 0) {
                led_on();
                led_fsm.timer = current_time;
            }

            if ((current_time - led_fsm.timer) >= LED_SHORT_PULSE_MS) {
                if (led_fsm.led_on) {
                    led_off();
                    led_fsm.flash_count++;
                    
                    if (led_fsm.flash_count >= target_flashes) {
                        if (led_fsm.current_mode == LED_MODE_FATAL) {

                            uint8_t count = fatal_visible_count();

                            led_fsm.fatal_code_index++;
                            led_fsm.flash_count = 0;

                            if (led_fsm.fatal_code_index >= count) {
                                led_fsm.fatal_code_index = 0;
                                led_fsm.step = 3;   // завершили список кодов
                            } else {
                                led_fsm.step = 4;   // пауза между кодами
                            }

                        } else {
                            led_fsm.step = 3;
                        }
                    } else {
                        led_fsm.timer = current_time;
                    }
                } else {
                    led_on();
                    led_fsm.timer = current_time;
                }
            }
        }
        break;
            
        case 3:
        {
            uint32_t cycle_pause = (led_fsm.current_mode == LED_MODE_ERROR) ? 
                                    LED_LONG_PAUSE_MS : LED_CYCLE_PAUSE_MS;
            
            if ((current_time - led_fsm.timer) >= cycle_pause) {
                led_fsm.step = 0;
                led_fsm.flash_count = 0;
                led_fsm.timer = current_time;
            }
        }
        break;
        
        case 4:
            if ((current_time - led_fsm.timer) >= LED_SHORT_PAUSE_MS) {
                led_fsm.step = 2;
                led_fsm.timer = current_time;
            }
            break;
    }
}

void led_fsm_auto_update(const error_state_t* error_state, uint8_t current_output_state) {
 
    led_mode_t new_mode;
    uint8_t error_code = 0;
    uint8_t output_state = current_output_state;
    
    if (error_state->fatal_state_active) {

        new_mode = LED_MODE_FATAL;
        error_code = 0;

    } else if (error_state->error_state_active) {
        new_mode = LED_MODE_ERROR;
        
        // Берём код последней ошибки
        if (error_state->error_history_count > 0) {
            uint8_t idx =
                (error_state->error_history_index - 1 + ERROR_HISTORY_SIZE)
                % ERROR_HISTORY_SIZE;

            error_code =
                error_type_to_code(error_state->error_history[idx].type);
        }
    } else if (led_fsm.current_mode == LED_MODE_ERROR &&
            !error_state->error_state_active) 
        {
            new_mode = LED_MODE_ACTIVE;
        } else {
            new_mode = LED_MODE_ACTIVE;
    }
    
    // Устанавливаем режим если он изменился
    if (new_mode != led_fsm.current_mode || 
        error_code != led_fsm.error_code || 
        output_state != led_fsm.output_state) {
        led_fsm_set_mode(new_mode, error_code, output_state);
    }
}

const led_fsm_state_t* led_fsm_get_state(void) {
    return &led_fsm;
}


