#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

void bsp_gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_GPIO_H