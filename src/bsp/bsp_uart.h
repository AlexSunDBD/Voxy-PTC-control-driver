#ifndef BSP_UART_H
#define BSP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

void bsp_uart2_init(void);
void uart_send_string(const char* str);
void uart_send_bytes(const uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // BSP_UART_H