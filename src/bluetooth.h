// bluetooth.h

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "core_fsm.h"
#include "error_handler.h"
#include "hardware.h"

// ============================================================================
// Константы из ТЗ 6.1
// ============================================================================

#define BLUETOOTH_LINE_DELAY_MS    100    // Задержка между строками при выводе Q
#define BLUETOOTH_BUFFER_SIZE      64     // Размер буфера приёма

// ============================================================================
// Структуры данных
// ============================================================================

typedef struct {
    uint8_t rx_buffer[BLUETOOTH_BUFFER_SIZE];
    uint16_t rx_index;
    bool command_ready;
    
    uint32_t bytes_received;
    uint32_t bytes_sent;
    uint32_t commands_processed;
    
} bluetooth_state_t;

// ============================================================================
// Публичные функции
// ============================================================================

void bluetooth_rx_callback(UART_HandleTypeDef* huart);
void bluetooth_tx_callback(UART_HandleTypeDef* huart);
void bluetooth_init(void);
void bluetooth_process(void);
void send_line(const char* str);
void bluetooth_send_auto_message(int8_t required, int8_t limit, int8_t actual);
void bluetooth_process_command(const char* command);
void bluetooth_cmd_help(void);
void bluetooth_cmd_buffer(void);
void bluetooth_cmd_diagnostic(void);
void bluetooth_cmd_generator(uint16_t value);
const bluetooth_state_t* bluetooth_get_state(void);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_H


