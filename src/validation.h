// validation.h

#ifndef VALIDATION_H
#define VALIDATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "pwm_input.h"

// ============================================================================
// Константы валидации из ТЗ
// ============================================================================

#define MIN_VALID_SAMPLES             25    // Минимальное количество валидных измерений
#define MIN_ACCUMULATED_DATA           3    // Минимально достаточное количество измерений

// Пороги управления (для информации)
#define THRESHOLD_1                  450
#define THRESHOLD_2                  727
#define THRESHOLD_3                 1022

// ============================================================================
// Структуры данных
// ============================================================================

/**
 * @brief Результат валидации окна измерений
 */
typedef struct {
    bool is_valid;                  // Общая валидность окна
    uint32_t median_pulse_width;    // Медианное значение pulse_width
    uint8_t valid_samples_count;    // Количество валидных измерений
    uint8_t total_samples_count;    // Общее количество измерений
    uint32_t timestamp_ms;          // Время закрытия окна
} validation_result_t;

/**
 * @brief Коды ошибок валидации
 */
typedef enum {
    VALIDATION_OK = 0,              // Окно валидно
    VALIDATION_ZERO_WINDOW,         // Нулевое окно (< MIN_ACCUMULATED_DATA)
    VALIDATION_INSUFFICIENT_VALID,  // Недостаточно валидных измерений
    VALIDATION_NO_VALID_DATA,       // Нет валидных данных вообще
} validation_error_t;

// ============================================================================
// Публичные функции
// ============================================================================

/**
 * @brief Проверить валидность одного измерения
 * 
 * Проверяет период и длительность импульса согласно ТЗ п.4.2
 * Порядок проверки: сначала период, потом pulse_width
 * 
 * @param measurement Измерение для проверки
 * @return true - измерение валидно, false - невалидно
 */
bool validate_single_measurement(const pwm_measurement_t* measurement);

/**
 * @brief Валидация всего измерительного окна
 * 
 * Выполняет все проверки согласно ТЗ п.4.2:
 * 1. Определение нулевого окна
 * 2. Проверка каждого измерения
 * 3. Подсчёт валидных измерений
 * 4. Проверка MIN_VALID_SAMPLES
 * 5. Вычисление медиан для валидных измерений
 * 
 * @param window Измерительное окно для валидации
 * @return Результат валидации
 */
validation_result_t validate_window(const measurement_window_t* window);

/**
 * @brief Получить текстовое описание ошибки валидации
 * 
 * @param error Код ошибки
 * @return Строка с описанием
 */
const char* validation_error_to_string(validation_error_t error);

// ============================================================================
// Вспомогательные функции
// ============================================================================

/**
 * @brief Вычислить медиану массива значений
 * 
 * Функция модифицирует входной массив (сортирует)
 * 
 * @param values Массив значений
 * @param count Количество элементов
 * @return Медианное значение
 */
uint32_t calculate_median(uint32_t* values, uint8_t count);

#ifdef __cplusplus
}
#endif

#endif // VALIDATION_H


