// error_handler.c

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Константы из ТЗ 6.1
// ============================================================================

#define ERROR_TIMEOUT_MS           10000     // Пауза после ошибки ERROR
#define ERROR_WINDOW_MS            60000     // Окно подсчета ошибок для FATAL
#define PROCESSING_TIMEOUT_MS       1500     // Таймаут отсутствия обработки данных
#define MAX_ERRORS_IN_WINDOW         3       // Максимальное количество ошибок в окне
#define ERROR_HISTORY_SIZE          10       // Размер истории ошибок

// ============================================================================
// Типы ошибок системы (согласно ТЗ 6.1)
// ============================================================================

typedef enum {
    ERR_NONE = 0,                           // Нет ошибки
    
    // Обычные ошибки (учитываются в лимите)
    ERR_INSUFFICIENT_VALID,                 // Недостаточно валидных измерений
    ERR_INTEGRITY,                          // Ошибка целостности выходов
    ERR_SAFETY_TIMEOUT,                     // Таймаут безопасного отключения
    ERR_PROCESSING_TIMEOUT,                 // Таймаут обработки данных
    ERR_VOLTAGE_NO_NOMINAL,                 // Низкое напряжение сети  
    
    // Потеря управления защитой (немедленный FATAL)
    ERR_SAFETY_SIGNAL_FAIL,                 // Ошибка сигнала безопасности (PB15)
    
    // Критическая ошибка (состояние)
    ERR_FATAL                               // Критическая ошибка (FATAL)
} error_type_t;

// ============================================================================
// Структуры данных
// ============================================================================

typedef struct {
    error_type_t type;              // Тип ошибки
    uint32_t data;                 // Дополнительные данные
    bool is_fatal;                 // Привела ли ошибка к FATAL
} error_record_t;

typedef struct {
    // Флаги состояния
    bool error_state_active;       // Состояние ERROR активно
    bool fatal_state_active;       // Состояние FATAL активно
    bool outputs_locked;           // Выходы заблокированы
    bool processing_locked;        // Обработка данных заблокирована
    
    // Таймеры
    uint32_t error_start_time;     // Время входа в ERROR
    uint32_t error_window_start;   // Начало плавающего окна ошибок
    uint32_t last_error_time;      // Время последней ошибки
    
    // Счётчики
    uint32_t total_errors;         // Всего ошибок за время работы
    uint32_t total_fatals;         // Всего переходов в FATAL
    uint32_t error_resets;         // Количество выходов из ERROR
    
    // Плавающее окно ошибок
    uint8_t error_count_in_window; // Количество ошибок в текущем окне
    uint8_t window_error_mask;     // битовая маска видов ошибок окна
    
    // История ошибок
    error_record_t error_history[ERROR_HISTORY_SIZE];
    uint8_t error_history_index;
    uint8_t error_history_count;
} error_state_t;

typedef struct {
    bool error_handled;            // Ошибка обработана
    bool system_locked;            // Система заблокирована (FATAL)
    bool should_pause;             // Нужна пауза (ERROR)
    uint32_t pause_duration_ms;    // Длительность паузы
    error_type_t error_type;       // Тип обработанной ошибки
} error_result_t;

// ============================================================================
// Публичные функции
// ============================================================================

void error_handler_init(void);
error_result_t error_handler_process(error_type_t error_type, uint32_t error_data);
bool error_handler_timeout_expired(void);
void error_handler_resume(void);
bool error_handler_check_processing_timeout(uint32_t last_processing_time);
void error_handler_update_window(void);
bool error_handler_check_error_limit(void);
void error_handler_enter_fatal(error_type_t fatal_error_type);
const error_state_t* error_handler_get_state(void);
const char* error_type_to_string(error_type_t error_type);
uint8_t error_type_to_code(error_type_t error_type);

#ifdef __cplusplus
}
#endif

#endif // ERROR_HANDLER_H


