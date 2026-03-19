// heater_logic.h

#ifndef HEATER_LOGIC_H
#define HEATER_LOGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Константы из ТЗ
// ============================================================================

#define THRESHOLD_1                  450    // Порог включения ТЭН 1
#define THRESHOLD_2                  727    // Порог включения ТЭН 2
#define THRESHOLD_3                 1022    // Порог включения ТЭН 3

// Определения состояний
#define HEATER_STATE_0              0x00    // Все выключены
#define HEATER_STATE_1              0x01    // ТЭН 1 включен (PB14)
#define HEATER_STATE_2              0x03    // ТЭН 1+2 включены (PB14 + PB13)
#define HEATER_STATE_3              0x07    // Все три ТЭНа включены (PB14 + PB13 + PB12)

// Маски выходов
#define HEATER_MASK_1               (1 << 0)  // PB14
#define HEATER_MASK_2               (1 << 1)  // PB13  
#define HEATER_MASK_3               (1 << 2)  // PB12

// ============================================================================
// Структуры данных
// ============================================================================

/**
 * @brief Код состояния нагревателей (логический уровень)
 *
 * Это **код состояния** в диапазоне 0..3:
 *  - 0 — все выключены
 *  - 1 — 1 нагреватель (уровень 1)
 *  - 2 — 2 нагревателя (уровень 2)
 *  - 3 — 3 нагревателя (уровень 3)
 *
 * Для представления в виде битовой маски (для портов) используйте
 * значения HEATER_STATE_0 .. HEATER_STATE_3 (0x00,0x01,0x03,0x07).
 */
typedef uint8_t heater_level_t;   // 0..3 уровень мощности
typedef uint8_t heater_mask_t;    // 000 / 001 / 011 / 111

/**
 * @brief Результат расчёта состояния
 */
typedef struct {
    heater_level_t target_level;     // Код состояния (0..3)
    uint32_t pulse_width;           // Входное значение pulse_width
    bool is_zero_state;             // Флаг нулевого состояния
} heater_result_t;

// ============================================================================
// Публичные функции
// ============================================================================

/**
 * @brief Рассчитать целевое состояние нагревателей по медианному pulse_width
 * 
 * Логика согласно ТЗ:
 * - pulse_width < THRESHOLD_1 → состояние 0 (все выключены)
 * - THRESHOLD_1 ≤ pulse_width < THRESHOLD_2 → состояние 1 (ТЭН 1)
 * - THRESHOLD_2 ≤ pulse_width < THRESHOLD_3 → состояние 2 (ТЭН 1+2)
 * - pulse_width ≥ THRESHOLD_3 → состояние 3 (все три ТЭНа)
 * 
 * @param pulse_width_us Медианное значение длительности LOW-импульса
 * @return Целевое состояние нагревателей (0-3)
 */
heater_level_t calculate_target_level(uint32_t pulse_width_us);

/**
 * @brief Расширенный расчёт состояния с дополнительной информацией
 * 
 * @param pulse_width_us Медианное значение длительности LOW-импульса
 * @return heater_result_t с полной информацией о состоянии
 */
heater_result_t calculate_target_level_ex(uint32_t pulse_width_us);

/**
 * @brief Преобразовать код состояния (0..3) в битовую маску выходов
 * 
 * @param state Код состояния (0..3)
 * @return Битовая маска (0x00,0x01,0x03,0x07) для PB14, PB13, PB12
 */
uint8_t state_to_bitmask(uint8_t state);

/**
 * @brief Проверить корректность битовой маски состояния
 * 
 * Допустимые маски:
 * - 0b000 (0x00) - состояние 0
 * - 0b001 (0x01) - состояние 1
 * - 0b011 (0x03) - состояние 2
 * - 0b111 (0x07) - состояние 3
 * 
 * @param bitmask Битовая маска для проверки
 * @return true - маска корректна, false - некорректна
 */
bool is_valid_heater_bitmask(uint8_t bitmask);

/**
 * @brief Получить текстовое описание кода состояния
 * 
 * @param state Код состояния (0-3)
 * @return Строка с описанием
 */
const char* heater_state_to_string(heater_level_t state);

/**
 * @brief Получить текстовое описание битовой маски
 * 
 * @param bitmask Битовая маска (0x00,0x01,0x03,0x07)
 * @return Строка с описанием
 */
const char* heater_bitmask_to_string(heater_mask_t bitmask);

#ifdef __cplusplus
}
#endif

#endif // HEATER_LOGIC_H


