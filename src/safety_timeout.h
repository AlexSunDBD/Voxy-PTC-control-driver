// safety_timeout.h

#ifndef SAFETY_TIMEOUT_H
#define SAFETY_TIMEOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "heater_logic.h"

// ============================================================================
// Структуры данных
// ============================================================================

/**
 * @brief Состояние таймера безопасности
 */
typedef struct {
    bool active;                    // Таймер активен
    uint32_t start_time;           // Время старта таймера
    uint32_t timeout_count;        // Счётчик таймаутов
    bool timeout_triggered;        // Флаг срабатывания таймаута
} safety_timer_state_t;

/**
 * @brief Результат проверки безопасности
 */
typedef struct {
    bool safety_ok;                // Безопасность в норме
    bool timer_active;             // Таймер активен
    bool timeout_detected;         // Обнаружен таймаут безопасности
    uint32_t elapsed_ms;          // Прошедшее время (если таймер активен)
    uint32_t remaining_ms;        // Оставшееся время до таймаута
    uint32_t timestamp_ms;        // Время проверки
} safety_check_result_t;

// ============================================================================
// Публичные функции
// ============================================================================

bool safety_outputs_are_physically_off(void);

/**
 * @brief Инициализация модуля безопасности
 */
void safety_timeout_init(void);

/**
 * @brief Обновить состояние таймера безопасности
 * 
 * Согласно ТЗ п.4.7:
 * - Проверка SAFETY_TIMEOUT выполняется только при целевом состоянии = 0
 * - Таймер стартует при первом обнаружении нулевого состояния
 * - Автоматически сбрасывается через SAFETY_TIMEOUT_MS после установки всех выходов в 0
 * - Повторные нулевые состояния не перезапускают таймер
 * 
 * @param target_state Текущее целевое состояние нагревателей
 * @return true - таймер безопасности активен, false - неактивен
 */
bool safety_timeout_update(heater_level_t target_level);

/**
 * @brief Проверить таймаут безопасности
 * 
 * Проверяет, не превышен ли SAFETY_TIMEOUT при целевом состоянии 0.
 * Формирует ERR_SAFETY_TIMEOUT если по истечении SAFETY_TIMEOUT_MS
 * любой из выходов PB14, PB13 или PB12 остаётся активным.
 * 
 * @return true - таймаут безопасности превышен, false - безопасность в норме
 */
bool safety_timeout_check(void);

/**
 * @brief Принудительно сбросить таймер безопасности
 * 
 * Используется при подтверждённом отключении всех выходов
 */
void safety_timeout_reset(void);

/**
 * @brief Получить текущее состояние модуля безопасности
 */
const safety_timer_state_t* safety_get_state(void);

/**
 * @brief Получить результат проверки безопасности
 */
safety_check_result_t safety_get_check_result(void);


#ifdef __cplusplus
}
#endif

#endif // SAFETY_TIMEOUT_H


