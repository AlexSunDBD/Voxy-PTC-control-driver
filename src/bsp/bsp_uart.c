#include "bsp_uart.h"
#include <string.h>
#include "hardware.h"

extern UART_HandleTypeDef huart2;

void bsp_uart2_init(void) {
    // Инициализация UART2
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void uart_send_string(const char* str) {
    HAL_UART_Transmit(&huart2, (uint8_t*)str, strlen(str), 100);
}

void uart_send_bytes(const uint8_t* data, uint16_t len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)data, len, 100);
}