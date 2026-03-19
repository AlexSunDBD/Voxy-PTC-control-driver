// output_manager.h

#ifndef OUTPUT_MANAGER_H
#define OUTPUT_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "heater_logic.h"

// ============================================================================
// Константы из ТЗ 6.1
// ============================================================================

#define MIN_SWITCH_INTERVAL_MS      5000    // Запрет переключения выходов (5 сек стабильности)
#define SAFETY_TIMEOUT_MS           5000    // Таймаут безопасного отключения (выключено своевременно)

// ============================================================================
// Структуры данных
// ============================================================================

/**
 * @brief Состояние управления выходами
 */
typedef struct
{
    heater_mask_t commanded_state;
    heater_mask_t last_confirmed;
    uint32_t last_switch_time;
} output_state_t;


// ============================================================================
// Публичные функции
// ============================================================================

/**
 * @brief Прочитать текущую битовую маску с выходов (НОВЫЕ ПИНЫ)
 */
heater_mask_t output_read_bitmask(void);

/**
 * @brief Инициализация модуля управления выходами
 */
void output_manager_init(void);

/**
 * @brief Применить целевое состояние к выходам
 * 
 * В FATAL режиме возвращает ошибку для любых команд включения
 * 
 * @param target_level Целевое состояние (битовая маска)
 * @return Результат операции
 */
void apply_output_state(heater_mask_t target_level);

/**
 * @brief Немедленное отключение всех выходов
 * 
 * Используется в аварийных ситуациях (FATAL, ERR_SAFETY_TIMEOUT)
 * Работает даже в FATAL режиме
 * 
 * @return true - успешно отключено, false - ошибка
 */
bool emergency_shutdown(void);

/**
 * @brief Получить текущее состояние управления выходами
 */
const output_state_t* output_get_state(void);

/**
 * @brief Сбросить состояние модуля
 */
void output_reset(void);

bool are_all_outputs_off(void);

#ifdef __cplusplus
}
#endif

#endif // OUTPUT_MANAGER_H


