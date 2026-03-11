//core_fsm.h

#ifndef CORE_FSM_H
#define CORE_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "validation.h"
#include "heater_logic.h"

// ============================================================================
// Константы из ТЗ 6.1
// ============================================================================

#define PROCESSING_INTERVAL_MS      500    // Период основного цикла
#define PROCESSING_TIMEOUT_MS      1500    // Таймаут отсутствия обработки

// ============================================================================
// Структуры данных
// ============================================================================

/**
 * @brief Результат выполнения основного цикла
 */
typedef struct {
    bool success;
    heater_mask_t actual_state;
    heater_level_t target_level;
} processing_result_t;

/**
 * @brief Контекст основного цикла
 */
typedef struct {
    // Текущее состояние
    heater_level_t current_state;     // Текущее подтверждённое состояние
    heater_level_t target_level;      // Целевое состояние (последнее рассчитанное)

    // Таймеры и задержки
    uint32_t last_processing_time;   // Время последней обработки

    // Статистика
    uint32_t cycle_count;            // Счётчик циклов
    uint32_t success_count;          // Счётчик успешных циклов
    uint32_t error_count;            // Счётчик ошибок

} core_context_t;

typedef enum
{
    SYSTEM_INIT = 0,
    SYSTEM_WAIT_FIRST_WINDOW,
    SYSTEM_NORMAL,
    SYSTEM_ERROR_WAIT,
    SYSTEM_FATAL_LOCK

} system_state_t;

static system_state_t system_state = SYSTEM_INIT;

// ============================================================================
// Публичные функции
// ============================================================================

/**
 * @brief Инициализация основного цикла
 */
void core_fsm_init(void);

/**
 * @brief Выполнить один атомарный цикл обработки
 */
processing_result_t core_process_cycle(const measurement_window_t* window);

/**
 * @brief Получить текущий контекст
 */
const core_context_t* core_get_context(void);

/**
 * @brief Сбросить состояние основного цикла
 */
void core_reset_state(void);

#ifdef __cplusplus
}
#endif

#endif // CORE_FSM_H


