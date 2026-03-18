//stm32f1xx_it.c

/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 Your Company
  * All rights reserved.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_it.h"
#include "hardware.h"
#include "pwm_input.h"
#include "bluetooth.h"
#include "core_fsm.h"
#include <string.h>


/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_tx;

/******************************************************************************/
/*            Cortex-M3 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

void pwm_input_capture_callback(TIM_HandleTypeDef* htim);

/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  * ВНИМАНИЕ: SysTick_Handler НЕ должен быть закомментирован!
  * Он необходим для работы HAL_Delay()
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/**
  * @brief This function handles TIM1 update interrupt.
  */
void TIM1_UP_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}

/**
  * @brief This function handles TIM1 trigger and commutation interrupts.
  */
void TIM1_TRG_COM_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}

/**
  * @brief This function handles TIM1 capture compare interrupt.
  */
void TIM1_CC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}

/**
  * @brief This function handles TIM2 global interrupt.
  * TIM2 используется для измерения входного ШИМ-сигнала
  */
void TIM2_IRQHandler(void)
{
  /* Вызываем HAL обработчик */
  HAL_TIM_IRQHandler(&htim2);
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
void TIM3_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim3);
}

/**
  * @brief This function handles USART2 global interrupt.
  * USART2 используется для Bluetooth-интерфейса
  */
void USART2_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart2);
}

void DMA1_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

/******************************************************************************/
/* HAL Callback Functions - вызываются из HAL_*_IRQHandler                    */
/******************************************************************************/

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    system_tick_ms++;      // Временная база time_ms() для логики проекта

    static uint16_t ms_counter = 0;

    /* 0 мс: открыть окно накопления */
    if (ms_counter == 0) {
        pwm_open_measurement_window();
    }

    ms_counter++;

    /* 64 мс: закрыть окно накопления */
    if (ms_counter == PROCESSING_WINDOW_MS) {
        pwm_close_measurement_window();
    }

    /* 500 мс: начать новый цикл */
    if (ms_counter >= PROCESSING_INTERVAL_MS) {
        ms_counter = 0;
    }
  }
}

/**
  * @brief  Input Capture callback in non blocking mode 
  * @param  htim TIM handle
  * @retval None
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    // Захват фронтов ШИМ-сигнала
    pwm_input_capture_callback(htim);
  }
}

/**
  * @brief  Rx Transfer completed callback (UART)
  * @param  huart UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    // Приём данных по Bluetooth
    bluetooth_rx_callback(huart);
  }
}

/**
  * @brief  UART error callback
  * @param  huart UART handle
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    // Обработка ошибок Bluetooth UART
    // Можно добавить логирование ошибок
  }
}

/**
  * @brief  Tx Transfer completed callback (UART)
  * @param  huart UART handle
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    bluetooth_tx_callback(huart);
}


