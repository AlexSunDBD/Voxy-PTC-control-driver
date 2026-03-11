//pwm_input.h

#ifndef PWM_INPUT_H
#define PWM_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Константы из ТЗ 6.1
// ============================================================================

#define ACCUMULATION_WINDOW_MS          64    // Длительность измерительного окна
#define PWM_BUFFER_SIZE                 32    // 64ms / ~2ms ≈ 32 импульса (под номинал 1992us)

// Пороги валидации
#define REAL_PERIOD_US                1992    // Номинальный период
#define MIN_PERIOD_US                 1932    // -3%
#define MAX_PERIOD_US                 2052    // +3%
#define MIN_PULSE_WIDTH               400     // Минимальная длительность LOW
#define MAX_PULSE_WIDTH              1500     // Максимальная длительность LOW

// ============================================================================
// Структуры данных
// ============================================================================

/**
 * @brief Одно измерение ШИМ
 */
typedef struct {
    uint32_t pulse_width_us;   // Длительность LOW-импульса
    uint32_t period_us;        // Период сигнала
    bool is_valid;             // Флаг валидности (для отладки)
} pwm_measurement_t;

/**
 * @brief Завершённое измерительное окно
 */
typedef struct {
    pwm_measurement_t samples[PWM_BUFFER_SIZE];     // Буфер измерений
    uint8_t count;                     // Количество измерений в окне
    uint32_t timestamp_ms;
} measurement_window_t;

/**
 * @brief Состояние измерительного модуля
 */
typedef enum {
    PWM_STATE_IDLE,           // Измерения остановлены (FATAL режим)
    PWM_STATE_MEASURING,      // Активные измерения
    PWM_STATE_WINDOW_READY,   // Окно готово для обработки
} pwm_state_t;

// ============================================================================
// Публичные функции
// ============================================================================

void pwm_open_measurement_window(void);
void pwm_close_measurement_window(void);

/**
 * @brief Инициализация модуля измерения ШИМ
 */
void pwm_input_init(void);

/**
 * @brief Остановить измерения (используется в FATAL режиме)
 */
void pwm_input_stop(void);

/**
 * @brief Возобновить измерения (после выхода из FATAL)
 */
void pwm_input_resume(void);

/**
 * @brief Проверка готовности нового измерительного окна
 * @return true - новое окно готово, false - нет
 */
bool pwm_try_get_window(measurement_window_t* out_window);
bool pwm_window_is_open(void);


/**
 * @brief Сброс состояния модуля (при переходе в ERROR)
 */
void pwm_input_reset(void);

/**
 * @brief Получить текущее количество накопленных измерений в активном окне
 * @return Количество измерений
 */
uint8_t pwm_get_current_count(void);

/**
 * @brief Получить текущее состояние модуля
 * @return Состояние модуля измерения
 */
pwm_state_t pwm_get_state(void);

#ifdef __cplusplus
}
#endif

#endif // PWM_INPUT_H


