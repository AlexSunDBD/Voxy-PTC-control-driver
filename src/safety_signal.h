// safety_signal.h

#ifndef SAFETY_SIGNAL_H
#define SAFETY_SIGNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Константы из ТЗ 6.1
// ============================================================================

#define OUT_STABILIZATION_MS          50    // Время стабилизации выхода
#define SAFETY_CHECK_INTERVAL_MS     5000    // Период проверки PB15 при отсутствии НОРМЫ

// ============================================================================
// Ошибки модуля сигнала безопасности (PB15)
// ============================================================================

typedef enum {
    SAFETY_SIGNAL_ERROR_NONE = 0,
    SAFETY_SIGNAL_ERROR_LEVEL,              // PB15 ≠ 0 при отсутствии НОРМЫ
} safety_signal_error_t;

// ============================================================================
// Структуры данных
// ============================================================================

/**
 * @brief Состояние сигнала безопасности (PB15)
 */
typedef struct {
    bool target_state;
    bool actual_state;
    bool last_confirmed;
    bool first_check_pending;

    uint32_t last_change_time;
    uint32_t error_count;
    uint32_t check_count;
    uint32_t clear_time_ms;
    uint32_t last_periodic_check_ms;
} safety_signal_state_t;

/**
 * @brief Результат операции с сигналом безопасности
 */
typedef struct {
    bool success;                     // Успешное выполнение
    bool state_changed;               // Состояние изменилось
    safety_signal_error_t error;      // Код ошибки (если есть)
    uint32_t timestamp_ms;           // Время операции
} safety_signal_result_t;

// ============================================================================
// Публичные функции
// ============================================================================

/**
 * @brief Инициализация сигнала безопасности (PB15)
 */
void safety_signal_init(void);

/**
 * @brief Установить сигнал НОРМА (PB15 = 1)
 * @return Результат операции
 */
safety_signal_result_t safety_signal_set(void);

/**
 * @brief Снять сигнал НОРМА
 * @return Результат операции
 */
safety_signal_result_t safety_signal_clear(void);

/**
 * @brief Прочитать текущее состояние сигнала (подтверждённое)
 * @return true - НОРМА присутствует (PB15=1), false - НОРМА отсутствует (PB15=0)
 */
bool safety_signal_read_state(void);

/**
 * @brief Прочитать физическое состояние PB15
 * @return Физическое состояние пина
 */
bool safety_signal_read_physical_state(void);

/**
 * @brief Получить текущее состояние модуля
 */
const safety_signal_state_t* safety_signal_get_state(void);

/**
 * @brief Сбросить состояние модуля
 */
safety_signal_error_t safety_signal_process(void);


#ifdef __cplusplus
}
#endif

#endif // SAFETY_SIGNAL_H


