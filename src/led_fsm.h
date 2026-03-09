// led_fsm.h

#ifndef LED_FSM_H
#define LED_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "error_handler.h"
#include "heater_logic.h"

// ============================================================================
// Константы из ТЗ 6.1 (Приложение А)
// ============================================================================

#define LED_SHORT_PULSE_MS         300    // Короткая вспышка - 0.3 сек
#define LED_LONG_PULSE_MS          800    // Длинная вспышка - 0.8 сек
#define LED_SHORT_PAUSE_MS         300    // Короткая пауза - 0.3 сек
#define LED_LONG_PAUSE_MS          800    // Длинная пауза - 0.8 сек
#define LED_CYCLE_PAUSE_MS        5000    // Пауза между циклами - 5 сек

#define OUTPUT_STATE_TO_FLASHES(x) \
    ((x) == 1 ? 1 : (x) == 3 ? 2 : (x) == 7 ? 3 : 0)

// ============================================================================
// Типы режимов индикации
// ============================================================================

typedef enum {
    LED_MODE_INIT,      // Режим инициализации
    LED_MODE_ACTIVE,    // Основной режим работы
    LED_MODE_ERROR,     // Режим обработки ошибки
    LED_MODE_FATAL,     // Фатальная блокировка системы
} led_mode_t;

// ============================================================================
// Структура состояния FSM
// ============================================================================

typedef struct {
    led_mode_t current_mode;
    uint8_t error_code;         // Код ошибки для индикации
    uint8_t output_state;       // Состояние выходов (0-3)
    
    uint8_t step;               // Текущий шаг в последовательности
    uint32_t timer;             // Таймер для отсчёта времени
    uint8_t flash_count;        // Счётчик вспышек в текущей фазе

    uint8_t fatal_code_index;   // Индекс текущего FATAL-кода
    
    bool led_on;                // Светодиод включен
} led_fsm_state_t;

// ============================================================================
// Публичные функции
// ============================================================================

/**
 * @brief Инициализация FSM светодиодной индикации
 */
void led_fsm_init(void);

/**
 * @brief Установить режим индикации
 * 
 * @param mode Режим индикации
 * @param error_code Код ошибки (для ERROR/FATAL режимов)
 * @param output_state Состояние выходов (для ACTIVE режима)
 */
void led_fsm_set_mode(led_mode_t mode, uint8_t error_code, uint8_t output_state);

/**
 * @brief Обновить состояние FSM
 * 
 * Должен вызываться периодически (например, каждые 10-100мс)
 */
void led_fsm_update(void);

/**
 * @brief Автоматическое обновление режима на основе состояния системы
 * 
 * @param error_state Состояние обработчика ошибок
 * @param current_output_state Текущее состояние выходов (0-3)
 */
void led_fsm_auto_update(const error_state_t* error_state, uint8_t current_output_state);

/**
 * @brief Получить текущее состояние FSM
 * 
 * @return Указатель на структуру состояния FSM
 */
const led_fsm_state_t* led_fsm_get_state(void);

#ifdef __cplusplus
}
#endif

#endif // LED_FSM_H


