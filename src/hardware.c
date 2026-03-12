// hardware.c

#include "hardware.h"
#include "stm32f1xx_hal.h"
#include <string.h>

// ============================================================================
// Глобальные переменные
// ============================================================================

// Структуры HAL
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;
IWDG_HandleTypeDef hiwdg;

// ============================================================================
// Вспомогательные функции
// ============================================================================

volatile uint32_t system_tick_ms = 0;

uint32_t time_ms(void) {
    return system_tick_ms;
}

uint32_t time_us(void) {
    return __HAL_TIM_GET_COUNTER(&htim2);
}

void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

void delay_us(uint32_t us) {
    uint32_t delay_count = us * (SystemCoreClock / 1000000) / 4;
    for(uint32_t i = 0; i < delay_count; i++) {
        __NOP();
    }
}

void iwdg_refresh(void) {
    HAL_IWDG_Refresh(&hiwdg);
}

uint32_t critical_enter(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void critical_exit(uint32_t state) {
    if ((state & 0x1u) == 0u) {
        __enable_irq();
    }
}

// ============================================================================
// Функции управления GPIO
// ============================================================================

void gpio_set(GPIO_TypeDef* port, uint16_t pin) {
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

void gpio_clear(GPIO_TypeDef* port, uint16_t pin) {
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

GPIO_PinState gpio_read(GPIO_TypeDef* port, uint16_t pin) {
    return HAL_GPIO_ReadPin(port, pin);
}

void gpio_toggle(GPIO_TypeDef* port, uint16_t pin) {
    HAL_GPIO_TogglePin(port, pin);
}

// ============================================================================
// Инициализация системы
// ============================================================================

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // Настройка HSI
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    // Настройка тактирования
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Включение тактирования портов
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // ==================== ВХОДЫ ====================
    
    // PA0: Вход ШИМ вентилятора (TIM2_CH1)
    GPIO_InitStruct.Pin = PIN_PWM_INPUT;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA3: Bluetooth RX (USART2_RX)
    GPIO_InitStruct.Pin = PIN_BT_RX;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // ==================== ВЫХОДЫ ====================
    
    // PA2: Bluetooth TX (USART2_TX)
    GPIO_InitStruct.Pin = PIN_BT_TX;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA8: ШИМ-генератор (TIM1_CH1)
    GPIO_InitStruct.Pin = PIN_PWM_GEN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // РЕЛЕЙНЫЕ ВЫХОДЫ:
    
    // PB15: Сигнал «НОРМА» - ВЫХОД с прямой логикой
    GPIO_InitStruct.Pin = PIN_SAFETY_SIGNAL;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_SAFETY_SIGNAL, &GPIO_InitStruct);
    
    // PB14: ТЭН 1 - ВЫХОД (инверсная логика)
    GPIO_InitStruct.Pin = PIN_HEATER_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_HEATER_1, &GPIO_InitStruct);
    
    // PB13: ТЭН 2 - ВЫХОД (инверсная логика)
    GPIO_InitStruct.Pin = PIN_HEATER_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_HEATER_2, &GPIO_InitStruct);
    
    // PB12: ТЭН 3 - ВЫХОД (инверсная логика)
    GPIO_InitStruct.Pin = PIN_HEATER_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORT_HEATER_3, &GPIO_InitStruct);
    
    // PC13: Светодиод (активен LOW на BluePill)
    GPIO_InitStruct.Pin = PIN_LED;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PORT_LED, &GPIO_InitStruct);
    
    // ==================== НАЧАЛЬНЫЕ СОСТОЯНИЯ ====================
    
    // Начальное состояние: НОРМА отсутствует, все ТЭНы выключены
    SAFETY_SIGNAL_OFF;          // PB15 = 0 (норма отсутствует)
    HEATER1_OFF;                // PB14 = 1 (ТЭН 1 выключен)
    HEATER2_OFF;                // PB13 = 1 (ТЭН 2 выключен)
    HEATER3_OFF;                // PB12 = 1 (ТЭН 3 выключен)
    LED_OFF;                    // Светодиод выключен
}

static void MX_TIM1_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 64 - 1;  // 64 МГц / 64 = 1 МГц
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 1992 - 1;   // Период 1992 мкс = 502 Гц
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;  // Начальное значение - выключено
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_TIM2_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_IC_InitTypeDef sConfigIC = {0};
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 64 - 1;  // 64 МГц / 64 = 1 МГц (1 мкс разрешение)
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFF;     // Максимальный период
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    
    if (HAL_TIM_IC_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    
    // Настройка канала 1 для измерения FALLING фронтов
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 8;  // Фильтр 8 тактов (8 мкс)
    if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    
    // Настройка канала 2 для измерения RISING фронтов
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_TIM3_Init(void) {
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 64000 - 1;   // 64 MHz / 64000 = 1000 Hz
    htim3.Init.Period = 1 - 1;          // 1 ms tick
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&htim3);
}

static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_usart2_tx.Instance = DMA1_Channel7;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;

    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);

    HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

/*static void MX_IWDG_Init(void) {
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = IWDG_TIMEOUT_MS;  // 750 мс таймаут
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}*/

void Error_Handler(void) {
    // Бесконечный цикл с миганием светодиода при ошибке
    while (1) {
        LED_ON;
        delay_ms(100);
        LED_OFF;
        delay_ms(100);
    }
}

void hardware_init(void) {
    // Инициализация HAL
    HAL_Init();
    
    // Настройка системного тактирования
    SystemClock_Config();
    
    // Инициализация периферии
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    MX_USART2_UART_Init();
    // MX_IWDG_Init(); преждевременный запуск IWDG перенесен в основной цикл
     
    // Запуск таймеров
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
    
    // Включение прерываний
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}


