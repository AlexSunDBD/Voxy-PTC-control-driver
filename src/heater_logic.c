// heater_logic.c

#include "heater_logic.h"
#include <string.h>

// ============================================================================
// Внутренние функции
// ============================================================================

/**
 * @brief Определение состояния по порогам
 */
static uint8_t determine_state_by_threshold(uint32_t pulse_width_us) {
    if (pulse_width_us < THRESHOLD_1) {
        return 0;
    } else if (pulse_width_us < THRESHOLD_2) {
        return 1;
    } else if (pulse_width_us < THRESHOLD_3) {
        return 2;
    } else {
        return 3;
    }
}

// ============================================================================
// Публичные функции
// ============================================================================

heater_level_t calculate_target_level(uint32_t pulse_width_us) {
    return determine_state_by_threshold(pulse_width_us);
}

heater_result_t calculate_target_level_ex(uint32_t pulse_width_us)
{
    heater_result_t result;

    result.target_level = determine_state_by_threshold(pulse_width_us);
    result.pulse_width = pulse_width_us;
    result.is_zero_state = (result.target_level == 0);

    return result;
}

uint8_t state_to_bitmask(uint8_t state) {
    if (state > 3)
        state = 0;

    return (1 << state) - 1;
    /*Это даёт:
        0 -> 000
        1 -> 001
        2 -> 011
        3 -> 111
    */
}

bool is_valid_heater_bitmask(uint8_t bitmask) {
    // Допустимые значения согласно ТЗ
    return (bitmask == HEATER_STATE_0) ||  // 0b000
           (bitmask == HEATER_STATE_1) ||  // 0b001
           (bitmask == HEATER_STATE_2) ||  // 0b011
           (bitmask == HEATER_STATE_3);    // 0b111
}

const char* heater_state_to_string(heater_level_t state) {
    switch (state) {
        case 0: return "STATE_0 (All OFF)";
        case 1: return "STATE_1 (Heater 1 ON)";
        case 2: return "STATE_2 (Heaters 1+2 ON)";
        case 3: return "STATE_3 (All heaters ON)";
        default: return "UNKNOWN";
    }
}

const char* heater_bitmask_to_string(heater_mask_t bitmask) {
    switch (bitmask) {
        case HEATER_STATE_0: return "STATE_0 (All OFF)";
        case HEATER_STATE_1: return "STATE_1 (Heater 1 ON)";
        case HEATER_STATE_2: return "STATE_2 (Heaters 1+2 ON)";
        case HEATER_STATE_3: return "STATE_3 (All heaters ON)";
        default: return "INVALID";
    }
}

// ============================================================================
// Дополнительные функции для тестирования
// ============================================================================

#ifdef DEBUG_HEATER_LOGIC

/**
 * @brief Тестирование логики с различными входными значениями
 */
void test_heater_logic(void) {
    struct {
        uint32_t pulse_width;
        uint8_t expected_state;
        const char* description;
    } test_cases[] = {
        {0, 0, "Zero pulse width"},
        {100, 0, "Below threshold 1"},
        {449, 0, "Just below threshold 1"},
        {450, 1, "At threshold 1"},
        {451, 1, "Just above threshold 1"},
        {600, 1, "Between 1 and 2"},
        {726, 1, "Just below threshold 2"},
        {727, 2, "At threshold 2"},
        {728, 2, "Just above threshold 2"},
        {800, 2, "Between 2 and 3"},
        {1021, 2, "Just below threshold 3"},
        {1022, 3, "At threshold 3"},
        {1023, 3, "Just above threshold 3"},
        {1500, 3, "Max valid pulse width"},
        {2000, 3, "Above max (still state 3)"},
    };
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        uint8_t state = calculate_target_level(test_cases[i].pulse_width);
        uint8_t bitmask = state_to_bitmask(state);
        bool valid = is_valid_heater_bitmask(bitmask);
        
        // Здесь можно вывести результаты теста
        // printf("Test %zu: %s\n", i, 
        //        (state == test_cases[i].expected_state && valid) ? "PASS" : "FAIL");
    }
}

#endif // DEBUG_HEATER_LOGIC


