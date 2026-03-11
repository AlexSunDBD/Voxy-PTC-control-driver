#include "bsp_gpio.h"
#include "hardware.h"

void bsp_gpio_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Включение тактирования портов
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // ==================== ВХОДЫ ====================
    
    // PA0 - Вход ШИМ вентилятора (TIM2_CH1)
    GPIO_InitStruct.Pin = PIN_PWM_INPUT;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_PWM_INPUT, &GPIO_InitStruct);
    
    // PA3 - Bluetooth RX (USART2_RX) - ВХОД
    GPIO_InitStruct.Pin = PIN_BT_RX;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_BT_RX, &GPIO_InitStruct);
    
    // ==================== ВЫХОДЫ ====================
    
    // PA2 - Bluetooth TX (USART2_TX) - альтернативная функция
    GPIO_InitStruct.Pin = PIN_BT_TX;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_BT_TX, &GPIO_InitStruct);
    
    // PA8 - ШИМ-генератор (TIM1_CH1) - альтернативная функция
    GPIO_InitStruct.Pin = PIN_PWM_GEN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_PWM_GEN, &GPIO_InitStruct);
    
    // НОВЫЕ ПИНЫ согласно ТЗ 6.1:
    
    // PB15 - Сигнал "НОРМА" (прямая логика)
    GPIO_InitStruct.Pin = PIN_SAFETY_SIGNAL;  // GPIO_PIN_15
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_SAFETY_SIGNAL, &GPIO_InitStruct);  // GPIOB
    
    // PB14 - ТЭН 1 (инверсная логика)
    GPIO_InitStruct.Pin = PIN_HEATER_1;  // GPIO_PIN_14
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_HEATER_1, &GPIO_InitStruct);  // GPIOB
    
    // PB13 - ТЭН 2 (инверсная логика)
    GPIO_InitStruct.Pin = PIN_HEATER_2;  // GPIO_PIN_13
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_HEATER_2, &GPIO_InitStruct);  // GPIOB
    
    // PB12 - ТЭН 3 (инверсная логика)
    GPIO_InitStruct.Pin = PIN_HEATER_3;  // GPIO_PIN_12
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_HEATER_3, &GPIO_InitStruct);  // GPIOB
    
    // PC13 - Светодиод (активен LOW)
    GPIO_InitStruct.Pin = PIN_LED;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PORT_LED, &GPIO_InitStruct);
    
    // ==================== НАЧАЛЬНЫЕ СОСТОЯНИЯ ====================
    
    // Устанавливаем начальные состояния согласно ТЗ 6.1
    
    // Сигнал НОРМА отсутствует (PB15 = 0)
    SAFETY_SIGNAL_OFF;  // gpio_clear(PORT_SAFETY_SIGNAL, PIN_SAFETY_SIGNAL)
    
    // Все ТЭНы выключены (инверсная логика: GPIO = 1 → ТЭН выключен)
    HEATER1_OFF;  // gpio_set(PORT_HEATER_1, PIN_HEATER_1)     - PB14 = 1
    HEATER2_OFF;  // gpio_set(PORT_HEATER_2, PIN_HEATER_2)     - PB13 = 1
    HEATER3_OFF;  // gpio_set(PORT_HEATER_3, PIN_HEATER_3)     - PB12 = 1
    
    // Светодиод выключен
    LED_OFF;      // gpio_set(PORT_LED, PIN_LED)               - PC13 = 1
    
    // Для отладки: кратковременно включаем LED при инициализации
    LED_ON;
    static uint32_t last_time = 0;

    if(time_ms() - last_time >= 50)
    {
        last_time = time_ms();

        do_something();
    };
    LED_OFF;
}