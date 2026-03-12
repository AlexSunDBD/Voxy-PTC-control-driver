// hardware.h

#ifndef HARDWARE_H
#define HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

// ============================================================================
// Константы из ТЗ 6.1
// ============================================================================

// Временные константы
#define IWDG_TIMEOUT_MS             750  //750
#define WINDOW_PERIOD_MS            500
#define TIMER_FREQ_HZ           1000000  // 1 МГц для таймеров

// Пины согласно ТЗ 6.1 (НОВЫЕ НОМЕРА)
typedef enum {
    // Входы
    PIN_PWM_INPUT    = GPIO_PIN_0,   // PA0 - вход ШИМ
    PIN_BT_RX        = GPIO_PIN_3,   // PA3 - Bluetooth RX
    
    // Выходы
    PIN_BT_TX        = GPIO_PIN_2,   // PA2 - Bluetooth TX
    PIN_PWM_GEN      = GPIO_PIN_8,   // PA8 - генератор ШИМ
    
    // НОВЫЕ ПИНЫ согласно ТЗ 6.1:
    PIN_SAFETY_SIGNAL = GPIO_PIN_15, // PB15 - сигнал "НОРМА"
    PIN_HEATER_1      = GPIO_PIN_14, // PB14 - ТЭН 1
    PIN_HEATER_2      = GPIO_PIN_13, // PB13 - ТЭН 2
    PIN_HEATER_3      = GPIO_PIN_12, // PB12 - ТЭН 3
    
    PIN_LED          = GPIO_PIN_13,  // PC13 - светодиод
} Pin_t;

// Портовая адресация
#define PORT_PWM_INPUT      GPIOA
#define PORT_BT_RX          GPIOA
#define PORT_BT_TX          GPIOA
#define PORT_PWM_GEN        GPIOA

// НОВЫЕ ПОРТЫ согласно ТЗ 6.1:
#define PORT_SAFETY_SIGNAL  GPIOB      // PB15
#define PORT_HEATER_1       GPIOB      // PB14
#define PORT_HEATER_2       GPIOB      // PB13
#define PORT_HEATER_3       GPIOB      // PB12

#define PORT_LED            GPIOC

// Синонимы для обратной совместимости (если старый код использует PB0, PB1 и т.д.)
#define PIN_NORMAL          PIN_SAFETY_SIGNAL
#define PORT_NORMAL         PORT_SAFETY_SIGNAL
#define PIN_HEATER1         PIN_HEATER_1
#define PORT_HEATER1        PORT_HEATER_1
#define PIN_HEATER2         PIN_HEATER_2
#define PORT_HEATER2        PORT_HEATER_2
#define PIN_HEATER3         PIN_HEATER_3
#define PORT_HEATER3        PORT_HEATER_3

// Константа логики выходов из ТЗ 6.1 (дублируется в output_manager)
#define HEATER_OUTPUT_INVERTED_LOGIC    1   // 0 - Прямая логика:    1 → ТЭН ВКЛ
                                            // 1 - Инверсная логика: 0 → ТЭН ВКЛ

// ============================================================================
// Абстракции GPIO
// ============================================================================

void gpio_set(GPIO_TypeDef* port, uint16_t pin);
void gpio_clear(GPIO_TypeDef* port, uint16_t pin);
GPIO_PinState gpio_read(GPIO_TypeDef* port, uint16_t pin);
void gpio_toggle(GPIO_TypeDef* port, uint16_t pin);

// ============================================================================
// Абстракции времени
// ============================================================================

extern volatile uint32_t system_tick_ms;

uint32_t time_us(void);
uint32_t time_ms(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

// ============================================================================
// Ккритические секции
// ============================================================================

uint32_t critical_enter(void);
void critical_exit(uint32_t state);

// ============================================================================
// Инициализация аппаратуры
// ============================================================================

void hardware_init(void);
void SystemClock_Config(void);
void iwdg_refresh(void);

// ============================================================================
// Глобальные объекты HAL
// ============================================================================

extern TIM_HandleTypeDef htim1;     // Таймер 1 - генератор ШИМ
extern TIM_HandleTypeDef htim2;     // Таймер 2 - измерение входного ШИМ
extern TIM_HandleTypeDef htim3;     // Таймер 3 - системный таймер 1 МГц
extern UART_HandleTypeDef huart2;   // UART2 - Bluetooth
extern DMA_HandleTypeDef hdma_usart2_tx;
extern IWDG_HandleTypeDef hiwdg;    // Сторожевой таймер

// ============================================================================
// Макросы для удобства (с учетом инверсной логики нагревателей)
// ============================================================================

// Для светодиода (активен LOW на BluePill)
#define LED_ON      gpio_clear(PORT_LED, PIN_LED)
#define LED_OFF     gpio_set(PORT_LED, PIN_LED)
#define LED_TOGGLE  gpio_toggle(PORT_LED, PIN_LED)

// Для нагревателей (Тип логики задается HEATER_OUTPUT_INVERTED_LOGIC)
// Прямая       GPIO = 1 → ТЭН включен, GPIO = 0 → ТЭН выключен
// Инверсная    GPIO = 0 → ТЭН включен, GPIO = 1 → ТЭН выключен
#if HEATER_OUTPUT_INVERTED_LOGIC
    #define HEATER_ACTIVE_LEVEL    GPIO_PIN_RESET
    #define HEATER_INACTIVE_LEVEL  GPIO_PIN_SET
#else
    #define HEATER_ACTIVE_LEVEL    GPIO_PIN_SET
    #define HEATER_INACTIVE_LEVEL  GPIO_PIN_RESET
#endif

#define HEATER1_ON     HAL_GPIO_WritePin(PORT_HEATER_1, PIN_HEATER_1, HEATER_ACTIVE_LEVEL)
#define HEATER1_OFF    HAL_GPIO_WritePin(PORT_HEATER_1, PIN_HEATER_1, HEATER_INACTIVE_LEVEL)

#define HEATER2_ON     HAL_GPIO_WritePin(PORT_HEATER_2, PIN_HEATER_2, HEATER_ACTIVE_LEVEL)
#define HEATER2_OFF    HAL_GPIO_WritePin(PORT_HEATER_2, PIN_HEATER_2, HEATER_INACTIVE_LEVEL)

#define HEATER3_ON     HAL_GPIO_WritePin(PORT_HEATER_3, PIN_HEATER_3, HEATER_ACTIVE_LEVEL)
#define HEATER3_OFF    HAL_GPIO_WritePin(PORT_HEATER_3, PIN_HEATER_3, HEATER_INACTIVE_LEVEL)


// Для сигнала НОРМА (PB15) - прямая логика
// PB15 = 1 → НОРМА присутствует, PB15 = 0 → НОРМА отсутствует
#define SAFETY_SIGNAL_ON    gpio_set(PORT_SAFETY_SIGNAL, PIN_SAFETY_SIGNAL)     // PB15 = 1
#define SAFETY_SIGNAL_OFF   gpio_clear(PORT_SAFETY_SIGNAL, PIN_SAFETY_SIGNAL)   // PB15 = 0
#define NORMAL_ON           SAFETY_SIGNAL_ON    // Синоним для обратной совместимости
#define NORMAL_OFF          SAFETY_SIGNAL_OFF   // Синоним для обратной совместимости

// Для тестового генератора ШИМ (PA8)
#define PWM_GEN_ON     gpio_set(PORT_PWM_GEN, PIN_PWM_GEN)
#define PWM_GEN_OFF    gpio_clear(PORT_PWM_GEN, PIN_PWM_GEN)

// Объявление Error_Handler для избежания warning
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_H


