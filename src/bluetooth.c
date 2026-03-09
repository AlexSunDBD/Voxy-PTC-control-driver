// bluetooth.c

#include "bluetooth.h"
#include "hardware.h"
#include "core_fsm.h"
#include "error_handler.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


// ============================================================================
// Внутренние переменные
// ============================================================================

static bluetooth_state_t bt_ctx;
static char bt_tx_buffer[128];
static char dma_tx_buffer[128];
static volatile bool bt_tx_busy = false;
static volatile bool bt_overflow_error = false;

static int8_t last_target = -1;
static int8_t last_limit  = -1;
static int8_t last_final  = -1;

// ============================================================================
// Вспомогательные функции
// ============================================================================

void send_line(const char* str)
{
    if (bt_tx_busy) return;

    uint16_t len = strlen(str);
    if (len + 2 >= sizeof(dma_tx_buffer)) return;

    memcpy(dma_tx_buffer, str, len);
    dma_tx_buffer[len] = '\r';
    dma_tx_buffer[len + 1] = '\n';

    bt_tx_busy = true;

    if (HAL_UART_Transmit_DMA(&huart2,
                              (uint8_t*)dma_tx_buffer,
                              len + 2) == HAL_OK)
    {
        bt_ctx.bytes_sent += (len + 2);
    }
    else
    {
        bt_tx_busy = false;
        HAL_UART_Transmit(&huart2,
                        (uint8_t*)dma_tx_buffer,
                        len + 2,
                        10);
    }
}

static bool is_digit(char c) {
    return (c >= '0' && c <= '9');
}

static uint16_t string_to_uint16(const char* str, uint16_t max_value) {
    uint16_t result = 0;
    
    while (*str && is_digit(*str)) {
        result = result * 10 + (*str - '0');
        if (result > max_value) {
            return max_value;
        }
        str++;
    }
    
    return result;
}

void bluetooth_cmd_diagnostic(void)   //заглушка
{
    send_line("DIAG:NA");
}

/**
 * @brief Проверить, является ли команда диагностической
 * Диагностические команды разрешены в FATAL режиме
 */
static bool is_diagnostic_command(const char* command) {
    if (strcmp(command, "?") == 0) return true;      // Справка
    if (strcmp(command, "D") == 0) return true;      // Диагностика
    if (strcmp(command, "Q") == 0) return true;      // Буфер измерений
    return false;
}

// ============================================================================
// Публичные функции
// ============================================================================

void bluetooth_init(void) {
    memset(&bt_ctx, 0, sizeof(bt_ctx));
    
    bt_ctx.rx_index = 0;
    bt_ctx.command_ready = false;
    bt_tx_busy = false;
    
    // Очищаем буферы
    memset(bt_tx_buffer, 0, sizeof(bt_tx_buffer));
    memset(dma_tx_buffer, 0, sizeof(dma_tx_buffer));
    
    // Отправляем приветственное сообщение (синхронно, без DMA)
    const char* welcome = "BLU READY\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)welcome, strlen(welcome), 100);
    
    // ТОЛЬКО после отправки включаем прерывания
    HAL_UART_Receive_IT(&huart2, bt_ctx.rx_buffer, 1);
}

void bluetooth_process(void) {
    if (bt_overflow_error) {
        bt_overflow_error = false;
        send_line("ERR:OVF");
    }

    if (bt_ctx.command_ready) {
        bt_ctx.rx_buffer[bt_ctx.rx_index] = '\0';
        
        bluetooth_process_command((char*)bt_ctx.rx_buffer);
        bt_ctx.commands_processed++;
        
        bt_ctx.rx_index = 0;
        bt_ctx.command_ready = false;
        
        HAL_UART_Receive_IT(&huart2, bt_ctx.rx_buffer, 1);
    }
}

void bluetooth_send_auto_message(int8_t required,
                                 int8_t limit,
                                 int8_t actual)
{
    if (required == last_target &&
        limit    == last_limit &&
        actual   == last_final)
        return;

    last_target = required;
    last_limit  = limit;
    last_final  = actual;

    snprintf(bt_tx_buffer,
             sizeof(bt_tx_buffer),
             "%d,%d,%d",
             required,
             limit,
             actual);

    send_line(bt_tx_buffer);
}

void bluetooth_process_command(const char* command) {
    while (*command == ' ' || *command == '\t') {
        command++;
    }
    
    if (*command == '\0' || *command == '\r' || *command == '\n') {
        return;
    }
    
    // Проверяем, находимся ли мы в FATAL режиме
    
    const error_state_t* error_state = error_handler_get_state();
    
    // В FATAL режиме разрешены ТОЛЬКО диагностические команды
    if (error_state->fatal_state_active && !is_diagnostic_command(command))
    {
        send_line("ERR:CMD");
        return;
    }
    
    if (strcmp(command, "?") == 0) {
        bluetooth_cmd_help();
        return;
    }
    
    if (strcmp(command, "Q") == 0) {
        bluetooth_cmd_buffer();
        return;
    }
    
    if (strcmp(command, "D") == 0) {
        bluetooth_cmd_diagnostic();
        return;
    }
    
    if (command[0] == 'F' || command[0] == 'f') {
                
        const char* value_str = command + 1;
        uint16_t value = string_to_uint16(value_str, 2000);
        bluetooth_cmd_generator(value);
        return;
    }
    send_line("ERR:CMD");
}

void bluetooth_cmd_help(void)
{
    send_line("Cmd:D,Q,Fxxx");
}

void bluetooth_cmd_buffer(void) {
    // В FATAL режиме измерения остановлены
    
}

void bluetooth_cmd_generator(uint16_t value) {
    if (value > 2000) {
        send_line("ERR:VAL");
        return;
    }
    
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, value);
    
    snprintf(bt_tx_buffer, sizeof(bt_tx_buffer), 
            "F:%u", value);
    send_line(bt_tx_buffer);
}

const bluetooth_state_t* bluetooth_get_state(void) {
    return &bt_ctx;
}

// ============================================================================
// Обработчики прерываний UART
// ============================================================================

void bluetooth_rx_callback(UART_HandleTypeDef* huart) {
    if (huart->Instance == USART2) {
        uint8_t received_char = bt_ctx.rx_buffer[0];
        bt_ctx.bytes_received++;
        
        if (received_char == '\r' || received_char == '\n') {
            if (bt_ctx.rx_index > 0) {
                bt_ctx.command_ready = true;
            }
        } else if (received_char == 0x08 || received_char == 0x7F) {
            if (bt_ctx.rx_index > 0) {
                bt_ctx.rx_index--;
            }
        } else if (bt_ctx.rx_index < (BLUETOOTH_BUFFER_SIZE - 1)) {
            bt_ctx.rx_buffer[bt_ctx.rx_index] = received_char;
            bt_ctx.rx_index++;
        } else {
            bt_ctx.rx_index = 0;
            bt_overflow_error = true;
            HAL_UART_Receive_IT(&huart2, bt_ctx.rx_buffer, 1);
        }
        
        if (!bt_ctx.command_ready) {
            HAL_UART_Receive_IT(&huart2, bt_ctx.rx_buffer, 1);
        }
    }
}

void bluetooth_tx_callback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART2)
    {
        bt_tx_busy = false;
    }
}


