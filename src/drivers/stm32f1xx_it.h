//stm32f1xx_it.h

#ifndef STM32F1XX_IT_H
#define STM32F1XX_IT_H

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Обработчики исключений ядра Cortex-M3
// ============================================================================
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

// ============================================================================
// Обработчики прерываний периферии
// ============================================================================

// Таймеры
void TIM1_UP_IRQHandler(void);
void TIM1_TRG_COM_IRQHandler(void);
void TIM1_CC_IRQHandler(void);
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);

// UART
void USART2_IRQHandler(void);
void DMA1_Channel7_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // STM32F1XX_IT_H