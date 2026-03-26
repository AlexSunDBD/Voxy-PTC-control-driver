// error_handler.c

#include "error_handler.h"
#include "hardware.h"
#include "safety_signal.h"
#include "output_manager.h"
#include <string.h>

// ============================================================================
// Внутренние переменные
// ============================================================================

static error_state_t error_ctx;

// ============================================================================
// Вспомогательные функции
// ============================================================================

static void add_error_to_history(error_type_t type, uint32_t data, bool is_fatal) {
    error_record_t record = {
        .type = type,
        .timestamp_ms = time_ms(),
        .data = data,
        .is_fatal = is_fatal
    };
    
    error_ctx.error_history[error_ctx.error_history_index] = record;
    error_ctx.error_history_index = (error_ctx.error_history_index + 1) % ERROR_HISTORY_SIZE;
    
    if (error_ctx.error_history_count < ERROR_HISTORY_SIZE) {
        error_ctx.error_history_count++;
    }
}

static bool is_error_counted_in_window(error_type_t error_type) {
    // Обычные ошибки, учитываемые в окне
    return (error_type == ERR_INSUFFICIENT_VALID ||
            error_type == ERR_INTEGRITY ||
            error_type == ERR_SAFETY_TIMEOUT ||
            error_type == ERR_PROCESSING_TIMEOUT || 
            error_type == ERR_VOLTAGE_NO_NOMINAL);
}

static void enter_fatal_state(error_type_t error_type, uint32_t error_data, bool from_window_limit)
{
    error_ctx.fatal_state_active = true;
    error_ctx.outputs_locked = true;
    error_ctx.processing_locked = true;

    safety_signal_clear();

    bool shutdown_ok = emergency_shutdown();
    (void)shutdown_ok; // можно позже усилить проверку

    // Очистка истории перед формированием "фатальной истории"
    error_ctx.error_history_index = 0;
    error_ctx.error_history_count = 0;

    // Вариант с явным признаком источника FATAL:

    if (from_window_limit) {
        for (uint8_t code = ERR_INSUFFICIENT_VALID; code <= ERR_VOLTAGE_NO_NOMINAL; code++) {
            if (error_ctx.window_error_mask & (1UL << (uint32_t)code)) {
                add_error_to_history((error_type_t)code, 0, true);
            }
        }
    } else {
        // FATAL не из окна: история начинается с текущего фатального кода
        add_error_to_history(error_type, error_data, true);
    }


    error_ctx.total_errors++;
    error_ctx.total_fatals++;
    error_ctx.last_error_time = time_ms();
}

static error_result_t handle_normal_error(error_type_t error_type, uint32_t error_data) {
    error_result_t result = {
        .error_handled = true,
        .system_locked = false,
        .should_pause = true,
        .pause_duration_ms = ERROR_TIMEOUT_MS,
        .error_type = error_type
    };
    
    if (!error_ctx.error_state_active) {
        // Входим в состояние ERROR
        safety_signal_clear();
        
        error_ctx.outputs_locked = true;
        error_ctx.processing_locked = true;
        
        error_ctx.error_start_time = time_ms();
        error_ctx.error_state_active = true;
        
        if (is_error_counted_in_window(error_type)) {

        error_ctx.window_error_mask |= (1UL << (uint32_t)error_type);

        uint32_t now = time_ms();

            if (error_ctx.error_count_in_window == 0) {
                // первая ошибка в окне
                error_ctx.error_window_start = now;
                error_ctx.error_count_in_window = 1;
            } else {
                uint32_t window_age = now - error_ctx.error_window_start;

                if (window_age <= ERROR_WINDOW_MS) {
                    error_ctx.error_count_in_window++;
                } else {
                    // окно истекло — начинаем заново
                    error_ctx.error_window_start = now;
                    error_ctx.error_count_in_window = 1;
                    error_ctx.window_error_mask = (1UL << (uint32_t)error_type);
                }
            }
        }
       
        if (error_ctx.error_count_in_window >= MAX_ERRORS_IN_WINDOW)
        {
            enter_fatal_state(error_type, error_data, true);
            result.system_locked = true;
            result.should_pause = false;
            return result;
        }
        add_error_to_history(error_type, error_data, false);
        error_ctx.total_errors++;
        error_ctx.last_error_time = time_ms();
    } else {
        result.should_pause = false;
    }

    return result;
}

// ============================================================================
// Публичные функции
// ============================================================================

void error_handler_init(void) {
    
    memset(&error_ctx, 0, sizeof(error_ctx));
    
    error_ctx.error_state_active = false;
    error_ctx.fatal_state_active = false;
    error_ctx.outputs_locked = false;
    error_ctx.processing_locked = false;
    
    error_ctx.error_window_start = time_ms();
    error_ctx.error_count_in_window = 0;
    error_ctx.window_error_mask = 0;
    
}

error_result_t error_handler_process(error_type_t error_type, uint32_t error_data) {
    error_result_t result = {
        .error_handled = false,
        .system_locked = false,
        .should_pause = false,
        .pause_duration_ms = 0,
        .error_type = error_type
    };
    
    bool fatal_active = error_ctx.fatal_state_active;
 
    if (fatal_active) {
        if (error_type == ERR_SAFETY_SIGNAL_FAIL) {
            add_error_to_history(error_type, error_data, true);
            error_ctx.last_error_time = time_ms();
        }
        
        result.error_handled = true;
        result.system_locked = true;
        return result;
    }


    if (error_type == ERR_NONE) {
        return result;
    }
    
    switch (error_type) {
        case ERR_SAFETY_SIGNAL_FAIL:
            add_error_to_history(error_type, error_data, false);
            error_ctx.total_errors++;
            error_ctx.last_error_time = time_ms();
            enter_fatal_state(error_type, error_data, false);
            result.error_handled = true;
            result.system_locked = true;
            result.should_pause = false;
            return result;
                    
        case ERR_INSUFFICIENT_VALID:
        case ERR_INTEGRITY:
        case ERR_SAFETY_TIMEOUT:
        case ERR_PROCESSING_TIMEOUT:
        case ERR_VOLTAGE_NO_NOMINAL:
            return handle_normal_error(error_type, error_data);
            
        case ERR_FATAL:
                enter_fatal_state(error_type, error_data, false);
   
            result.system_locked = true;
            result.error_handled = true;
            return result;

        default:
            return result;
    }
}

bool error_handler_timeout_expired(void) {
    if (!error_ctx.error_state_active) {
        return true;
    }
    
    uint32_t elapsed = time_ms() - error_ctx.error_start_time;
    return (elapsed >= ERROR_TIMEOUT_MS);
}

void error_handler_resume(void) {
    
    if (error_ctx.error_state_active &&
        !error_ctx.fatal_state_active &&
        error_handler_timeout_expired())
    {
        error_ctx.processing_locked = false;
        error_ctx.outputs_locked = false;
        error_ctx.error_state_active = false;
        error_ctx.error_resets++;
    }
}

bool error_handler_check_processing_timeout(uint32_t last_processing_time) {
    if (error_ctx.fatal_state_active || error_ctx.error_state_active) {
        return false;
    }
    
    uint32_t elapsed = time_ms() - last_processing_time;
    return (elapsed >= PROCESSING_TIMEOUT_MS);
}

void error_handler_enter_fatal(error_type_t fatal_error_type) {
    (void)error_handler_process(fatal_error_type, 0);
}

const error_state_t* error_handler_get_state(void) {
    return &error_ctx;
}

const char* error_type_to_string(error_type_t error_type) {
    switch (error_type) {
        case ERR_NONE:
            return "NONE";
        case ERR_INSUFFICIENT_VALID:
            return "INSUFFICIENT_VALID";
        case ERR_INTEGRITY:
            return "INTEGRITY";
        case ERR_SAFETY_TIMEOUT:
            return "SAFETY_TIMEOUT";
        case ERR_PROCESSING_TIMEOUT:
            return "PROCESSING_TIMEOUT";
        case ERR_SAFETY_SIGNAL_FAIL:
            return "SAFETY_SIGNAL_FAIL";
        case ERR_VOLTAGE_NO_NOMINAL:
            return "VOLTAGE_NO_NOMINAL";
        case ERR_FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

uint8_t error_type_to_code(error_type_t error_type) {
    switch (error_type) {
        case ERR_INSUFFICIENT_VALID:
            return 1;
        case ERR_INTEGRITY:
            return 2;
        case ERR_SAFETY_TIMEOUT:
            return 3;
        case ERR_PROCESSING_TIMEOUT:
            return 4;
        case ERR_SAFETY_SIGNAL_FAIL:
            return 5;
        case ERR_FATAL:
            return 6;
        case ERR_VOLTAGE_NO_NOMINAL:
            return 7;
        default:
            return 0;
    }
}


