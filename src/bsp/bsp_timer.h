//bsp_timer.h

#ifndef BSP_TIMER_H
#define BSP_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

void bsp_iwdg_init(void);
void bsp_timer1_pwm_init(void);
void bsp_timer2_input_capture_init(void);
void bsp_timer3_init(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_TIMER_H