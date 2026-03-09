// integration_check.h

#ifndef INTEGRATION_CHECK_H
#define INTEGRATION_CHECK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Структуры данных
// ============================================================================

typedef struct {
    uint32_t integration_errors;
    uint32_t last_check_time;
    uint32_t check_interval_ms;
    bool enabled;
} integration_state_t;

// ============================================================================
// Публичные функции
// ============================================================================

void integration_check_init(void);
void integration_check_background_task(void);
const integration_state_t* integration_get_state(void);

// Функции для тестирования (объявляем их, чтобы убрать предупреждения)
bool integration_check_data_flow(void);
bool integration_check_error_flow(void);
bool integration_check_timing(void);

#ifdef __cplusplus
}
#endif

#endif // INTEGRATION_CHECK_H