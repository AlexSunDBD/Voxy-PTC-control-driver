//pwm_input.c

#include "pwm_input.h"
#include "hardware.h"
#include <string.h>

// ============================================================================
// Внутренние структуры и переменные
// ============================================================================

// Состояния машины захвата
typedef enum {
    CAPTURE_WAIT_RISING,    // Ждём RISING после FALLING
    CAPTURE_WAIT_FALLING,   // Ждём FALLING для завершения периода
} capture_state_t;

// Внутренний контекст модуля
static struct {
    // Измерительные окна
    measurement_window_t measurement_window;    // Активное окно (наполняется)
    bool window_ready_flag;                // Флаг готовности нового окна
    
    // Состояние захвата
    capture_state_t state;
    uint32_t last_falling_edge;           // Время последнего FALLING фронта
    uint32_t last_rising_edge;            // Время последнего RISING фронта
    
    // Тайминги
    uint32_t window_start_time;           // Время начала активного окна
    volatile bool window_collecting_flag; // Флаг открытого окна
    pwm_state_t module_state;             // Общее состояние модуля
    
    // Статистика
    uint32_t total_measurements;
    uint32_t valid_measurements;
    uint32_t missed_rising_count;
    
    // Критическая секция
    __IO uint32_t lock;
} pwm_ctx;

// ============================================================================
// Вспомогательные функции
// ============================================================================

static inline void enter_critical(void) {
    pwm_ctx.lock = 1;
    __disable_irq();
}

static inline void exit_critical(void) {
    __enable_irq();
    pwm_ctx.lock = 0;
}

static void reset_capture_state(void) {
    pwm_ctx.state = CAPTURE_WAIT_RISING;
    pwm_ctx.last_falling_edge = 0;
    pwm_ctx.last_rising_edge = 0;
}

void pwm_open_measurement_window(void)
{
    enter_critical();

    pwm_ctx.measurement_window.count = 0;
    reset_capture_state();
    pwm_ctx.window_start_time = time_ms();
    pwm_ctx.window_collecting_flag = true;
    pwm_ctx.window_ready_flag = false;

    exit_critical();
}


static void process_falling_edge(uint32_t capture_time) {
    switch (pwm_ctx.state) {
        case CAPTURE_WAIT_RISING:
            // Первый FALLING — просто запоминаем время
            pwm_ctx.last_falling_edge = capture_time;
            pwm_ctx.state = CAPTURE_WAIT_RISING;
            break;

        case CAPTURE_WAIT_FALLING:
            if (pwm_ctx.measurement_window.count < PWM_BUFFER_SIZE) {
                pwm_measurement_t* m = &pwm_ctx.measurement_window.samples[pwm_ctx.measurement_window.count];
                m->period_us = capture_time - pwm_ctx.last_falling_edge;
                
                if (pwm_ctx.last_rising_edge > pwm_ctx.last_falling_edge) {
                    m->pulse_width_us = pwm_ctx.last_rising_edge - pwm_ctx.last_falling_edge;
                } else {
                    m->pulse_width_us = m->period_us;
                    pwm_ctx.missed_rising_count++;
                }
                
                m->is_valid = false;
                pwm_ctx.measurement_window.count++;
                pwm_ctx.total_measurements++;
            }
            pwm_ctx.last_falling_edge = capture_time;
            pwm_ctx.state = CAPTURE_WAIT_RISING;
            break;
    }
}

static void process_rising_edge(uint32_t capture_time) {
    if (pwm_ctx.state == CAPTURE_WAIT_RISING) {
        pwm_ctx.last_rising_edge = capture_time;
        pwm_ctx.state = CAPTURE_WAIT_FALLING;
    }
}

// ============================================================================
// Обработчики прерываний
// ============================================================================

void pwm_input_capture_callback(TIM_HandleTypeDef* htim) {
    if (pwm_ctx.module_state == PWM_STATE_IDLE) {
        return;
    }
    
    if (__HAL_TIM_GET_FLAG(htim, TIM_FLAG_CC1) != RESET) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC1);
        
        uint32_t capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        
        if (pwm_ctx.window_collecting_flag) {
            process_falling_edge(capture);
        }
    }
    
    if (__HAL_TIM_GET_FLAG(htim, TIM_FLAG_CC2) != RESET) {
        __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_CC2);
        
        uint32_t capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        
        if (pwm_ctx.window_collecting_flag) {
            process_rising_edge(capture);
        }
    }
}

// ============================================================================
// Публичные функции
// ============================================================================

void pwm_input_init(void) {
    memset(&pwm_ctx, 0, sizeof(pwm_ctx));
    
    pwm_ctx.state = CAPTURE_WAIT_RISING;
    pwm_ctx.window_collecting_flag = false;
    pwm_ctx.window_ready_flag = false;
    pwm_ctx.module_state = PWM_STATE_MEASURING;
}

void pwm_input_stop(void) {
    enter_critical();
    
    HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_2);
    
    pwm_ctx.module_state = PWM_STATE_IDLE;
    
    exit_critical();
}

void pwm_input_resume(void) {
    enter_critical();
    
    pwm_ctx.module_state = PWM_STATE_MEASURING;
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
    
    exit_critical();
}

bool pwm_window_is_open(void)
{
    return pwm_ctx.window_collecting_flag;
}

void pwm_close_measurement_window(void)
{
    enter_critical();

    if (pwm_ctx.window_collecting_flag)
    {
        pwm_ctx.measurement_window.timestamp_ms = time_ms();
        pwm_ctx.window_collecting_flag = false;
        pwm_ctx.window_ready_flag = true;
    }

    exit_critical();
}

bool pwm_try_get_window(measurement_window_t* out_window)
{
    bool ready = false;

    enter_critical();

    if (pwm_ctx.window_ready_flag)
    {
        *out_window = pwm_ctx.measurement_window;
        pwm_ctx.window_ready_flag = false;
        ready = true;
    }

    exit_critical();

    return ready;
}

void pwm_input_reset(void) {
    enter_critical();
    
    memset(&pwm_ctx.measurement_window, 0, sizeof(measurement_window_t));
    
    reset_capture_state();
    
    pwm_ctx.window_ready_flag = false;
    pwm_ctx.window_collecting_flag = false;
    
    exit_critical();
}

uint8_t pwm_get_current_count(void) {
    uint8_t count;
    enter_critical();
    count = pwm_ctx.measurement_window.count;
    exit_critical();
    return count;
}

pwm_state_t pwm_get_state(void) {
    return pwm_ctx.module_state;
}


