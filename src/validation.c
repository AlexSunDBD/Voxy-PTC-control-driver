// validation.c

#include "validation.h"
#include "pwm_input.h"
#include <string.h>
#include <stdlib.h>

// ============================================================================
// Вспомогательные функции (внутренние)
// ============================================================================

/**
 * @brief Сравнение для сортировки (qsort)
 */
static int compare_uint32(const void* a, const void* b) {
    uint32_t val_a = *(const uint32_t*)a;
    uint32_t val_b = *(const uint32_t*)b;
    
    if (val_a < val_b) return -1;
    if (val_a > val_b) return 1;
    return 0;
}

/**
 * @brief Проверка валидности периода
 */
static bool is_period_valid(uint32_t period_us) {
    return (period_us >= MIN_PERIOD_US) && (period_us <= MAX_PERIOD_US);
}

/**
 * @brief Проверка валидности длительности импульса
 */
static bool is_pulse_width_valid(uint32_t pulse_width_us) {
    return (pulse_width_us >= MIN_PULSE_WIDTH) && (pulse_width_us <= MAX_PULSE_WIDTH);
}

// ============================================================================
// Публичные функции
// ============================================================================

bool validate_single_measurement(const pwm_measurement_t* measurement) {
    // Сначала проверяем период (согласно ТЗ п.4.2)
    if (!is_period_valid(measurement->period_us)) {
        return false;
    }
    
    // Если период валиден, проверяем pulse_width
    if (!is_pulse_width_valid(measurement->pulse_width_us)) {
        return false;
    }
    
    return true;
}

validation_result_t validate_window(const measurement_window_t* window)
{
    validation_result_t result = {
        .is_valid = false,
        .median_pulse_width = 0,
        .valid_samples_count = 0,
        .total_samples_count = window->count,
        .timestamp_ms = window->timestamp_ms
    };

    // ====================================================
    // Шаг 1: ZERO по количеству накопленных данных
    // ====================================================
    if (window->count < MIN_ACCUMULATED_DATA) {
        result.is_valid = true;
        result.median_pulse_width = 0;
        return result;
    }

    // ====================================================
    // Шаг 2: Фильтрация валидных измерений
    // ====================================================
    uint32_t valid_pulse[PWM_BUFFER_SIZE];
    uint8_t valid_count = 0;

    for (uint8_t i = 0; i < window->count; i++) {
        if (validate_single_measurement(&window->samples[i])) {
            valid_pulse[valid_count]  = window->samples[i].pulse_width_us;
            valid_count++;
        }
    }

    result.valid_samples_count = valid_count;

    // ====================================================
    // Шаг 3: Проверка MIN_VALID_SAMPLES
    // ====================================================
    if (valid_count < MIN_VALID_SAMPLES) {
        return result;   // is_valid = false
    }

    // ====================================================
    // Шаг 4: Медиана
    // ====================================================
    qsort(valid_pulse,  valid_count, sizeof(uint32_t), compare_uint32);
    
    result.median_pulse_width = calculate_median(valid_pulse, valid_count);
    result.is_valid = true;

    return result;
}

uint32_t calculate_median(uint32_t* values, uint8_t count) {
    if (count == 0) {
        return 0;
    }
    
    // Массив уже должен быть отсортирован
    if (count % 2 == 1) {
        // Нечётное количество элементов
        return values[count / 2];
    } else {
        // Чётное количество элементов - беррм меньшее
        return values[count / 2 - 1];
    }
}

const char* validation_error_to_string(validation_error_t error) {
    switch (error) {
        case VALIDATION_OK:
            return "OK";
        case VALIDATION_ZERO_WINDOW:
            return "Zero window";
        case VALIDATION_INSUFFICIENT_VALID:
            return "Insufficient valid samples";
        case VALIDATION_NO_VALID_DATA:
            return "No valid data";
        default:
            return "Unknown error";
    }
}

// ============================================================================
// Дополнительные функции для анализа
// ============================================================================

#ifdef DEBUG_VALIDATION

/**
 * @brief Детальный анализ окна (для отладки)
 */
void analyze_window_debug(const measurement_window_t* window) {
    char buffer[512];
    uint8_t valid_by_period = 0;
    uint8_t valid_by_pulse = 0;
    uint8_t fully_valid = 0;
    uint32_t min_period = UINT32_MAX;
    uint32_t max_period = 0;
    uint32_t min_pulse = UINT32_MAX;
    uint32_t max_pulse = 0;
    
    snprintf(buffer, sizeof(buffer),
            "\r\n=== VALIDATION ANALYSIS ===\r\n"
            "Window: %s, Samples: %u\r\n",
            (window->count == 0) ? "ZERO" : "NORMAL",
            window->count);
    
    for (uint8_t i = 0; i < window->count; i++) {
        bool period_ok = is_period_valid(window->samples[i].period_us);
        bool pulse_ok = is_pulse_width_valid(window->samples[i].pulse_width_us);
        
        if (period_ok) valid_by_period++;
        if (pulse_ok) valid_by_pulse++;
        if (period_ok && pulse_ok) fully_valid++;
        
        // Статистика
        if (window->samples[i].period_us < min_period) min_period = window->samples[i].period_us;
        if (window->samples[i].period_us > max_period) max_period = window->samples[i].period_us;
        if (window->samples[i].pulse_width_us < min_pulse) min_pulse = window->samples[i].pulse_width_us;
        if (window->samples[i].pulse_width_us > max_pulse) max_pulse = window->samples[i].pulse_width_us;
    }
    
    snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
            "Valid by period: %u/%u\r\n"
            "Valid by pulse: %u/%u\r\n"
            "Fully valid: %u/%u\r\n"
            "Period range: %lu-%lu us\r\n"
            "Pulse range: %lu-%lu us\r\n",
            valid_by_period, window->count,
            valid_by_pulse, window->count,
            fully_valid, window->count,
            min_period, max_period,
            min_pulse, max_pulse);
    
    if (fully_valid >= MIN_VALID_SAMPLES) {
        validation_result_t result = validate_window(window);
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                "Median period: %lu us\r\n"
                "Median pulse: %lu us\r\n"
                "Duty cycle: %.1f%%\r\n",
                result.median_period,
                result.median_pulse_width,
                (float)result.median_pulse_width * 100.0f / result.median_period);
    }
    
    uart_send_string(buffer);
}

#endif // DEBUG_VALIDATION


