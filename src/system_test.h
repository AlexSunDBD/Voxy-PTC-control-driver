#ifndef SYSTEM_TEST_H
#define SYSTEM_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Типы тестов
// ============================================================================

typedef enum {
    TEST_CATEGORY_UNIT,          // Модульные тесты
    TEST_CATEGORY_INTEGRATION,   // Интеграционные тесты
    TEST_CATEGORY_SYSTEM,        // Системные тесты
    TEST_CATEGORY_SAFETY,        // Тесты безопасности
    TEST_CATEGORY_PERFORMANCE,   // Тесты производительности
} test_category_t;

typedef enum {
    TEST_STATE_NOT_RUN,          // Тест не запускался
    TEST_STATE_RUNNING,          // Тест выполняется
    TEST_STATE_PASSED,           // Тест пройден
    TEST_STATE_FAILED,           // Тест не пройден
    TEST_STATE_SKIPPED,          // Тест пропущен
} test_state_t;

// ============================================================================
// Структуры данных
// ============================================================================

typedef struct {
    const char* name;            // Название теста
    test_category_t category;    // Категория теста
    test_state_t state;          // Состояние теста
    uint32_t start_time;         // Время начала теста
    uint32_t duration_ms;        // Длительность выполнения
    bool (*test_function)(void); // Функция теста
    const char* description;     // Описание теста
} test_case_t;

typedef struct {
    test_case_t* tests;          // Массив тестов
    uint16_t test_count;         // Количество тестов
    uint16_t tests_passed;       // Пройдено тестов
    uint16_t tests_failed;       // Не пройдено тестов
    uint16_t tests_skipped;      // Пропущено тестов
    uint32_t total_duration_ms;  // Общее время тестирования
    bool test_in_progress;       // Тест выполняется
    uint16_t current_test_index; // Текущий тест
} test_suite_t;

// ============================================================================
// Публичные функции
// ============================================================================

/**
 * @brief Инициализация системы тестирования
 */
void system_test_init(void);

/**
 * @brief Запуск всех тестов
 */
void system_test_run_all(void);

/**
 * @brief Запуск тестов определённой категории
 * 
 * @param category Категория тестов для запуска
 */
void system_test_run_category(test_category_t category);

/**
 * @brief Выполнить один тестовый случай
 * 
 * @param test_index Индекс теста
 * @return true - тест пройден, false - тест не пройден
 */
bool system_test_run_single(uint16_t test_index);

/**
 * @brief Генерация отчёта о тестировании
 */
void system_test_generate_report(void);

/**
 * @brief Фоновая задача выполнения тестов
 */
void system_test_background_task(void);

/**
 * @brief Получить состояние системы тестирования
 */
const test_suite_t* system_test_get_state(void);

// ============================================================================
// Функции тестов (объявления)
// ============================================================================

// Модульные тесты
bool test_hardware_init(void);
bool test_pwm_input_measurement(void);
bool test_validation_logic(void);
bool test_heater_logic(void);
bool test_output_manager(void);
bool test_pb0_signal(void);
bool test_safety_timeout(void);
bool test_error_handler(void);
bool test_led_fsm(void);
bool test_bluetooth_commands(void);

// Интеграционные тесты
bool test_integration_data_flow(void);
bool test_integration_error_flow(void);
bool test_integration_timing(void);

// Системные тесты
bool test_system_normal_operation(void);
bool test_system_pwm_loss(void);
bool test_system_error_recovery(void);
bool test_system_fatal_handling(void);

// Тесты безопасности
bool test_safety_integrity_check(void);
bool test_safety_timeout_response(void);
bool test_safety_pb0_confirmation(void);
bool test_safety_error_window(void);

// Тесты производительности
bool test_performance_processing_time(void);
bool test_performance_memory_usage(void);
bool test_performance_response_time(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_TEST_H