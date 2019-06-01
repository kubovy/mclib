/* 
 * File:   mcp22xx.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */

#include "mcp22xx.h"

#if defined MCP2200_ENABLED || defined MCP2221_ENABLED

struct {
    uint8_t index;
    uint16_t length;
    uint8_t data[SCOM_MAX_PACKET_SIZE];
    uint8_t checksum;
} MCP22xx_rx;

UART_Connection_t MCP22xx_uart;
MCP22xx_EventState_t MCP22xx_state = MCP22xx_STATE_IDLE;

DataHandler_t MCP22xx_dataHandler;


void MCP22xx_initialize(UART_Connection_t uart, DataHandler_t dataHandler) {
   MCP22xx_uart = uart; 
   MCP22xx_dataHandler = dataHandler;
}

void MCP22xx_execute(uint8_t length, ...) {
    va_list argp;
    va_start(argp, length);
    
    for (uint8_t i = 0; i < length; i++) { // Send the command bits, along with the parameters
        while (!UART_isTXReady(MCP22xx_uart)); // Wait till we can start sending.
        UART_write(MCP22xx_uart, va_arg(argp, uint8_t)); // Store each byte in the storePacket into the UART write buffer
        while (!UART_isTXDone(MCP22xx_uart)); // Wait until UART TX is done.
    }
    va_end(argp);
}

void MCP22xx_send(uint8_t length, uint8_t *data) {
    for (uint8_t i = 0; i < length; i++) {     // Send the command bits, along with the parameters
        while (!UART_isTXReady(MCP22xx_uart)); // Wait till we can start sending.
        UART_write(MCP22xx_uart, *(data + i)); // Store each byte in the storePacket into the UART write buffer
        while (!UART_isTXDone(MCP22xx_uart));  // Wait until UART TX is done.
    }
}

void MCP22xx_processByte(uint8_t byte) {
    switch (MCP22xx_state) {
        case MCP22xx_STATE_IDLE:
            if (byte == 0xAA) { // SYNC WORD
                MCP22xx_state = MCP22xx_EVENT_STATE_LENGTH_HIGH;
                MCP22xx_rx.length = 0;
                MCP22xx_rx.checksum = 0x00;
            }
            break;
        case MCP22xx_EVENT_STATE_LENGTH_HIGH:
            if (byte > 0) MCP22xx_state = MCP22xx_STATE_IDLE;
            MCP22xx_rx.length = (byte << 8);
            MCP22xx_rx.checksum += byte;
            MCP22xx_state = MCP22xx_EVENT_STATE_LENGTH_LOW;
            break;
        case MCP22xx_EVENT_STATE_LENGTH_LOW:
            MCP22xx_rx.index = 0;
            MCP22xx_rx.length |= (byte & 0x00FF);
            if (MCP22xx_rx.length > (uint16_t) SCOM_MAX_PACKET_SIZE) MCP22xx_state = MCP22xx_STATE_IDLE;
            MCP22xx_rx.checksum += byte;
            MCP22xx_state = MCP22xx_EVENT_STATE_ADDITIONAL;
            break;
        case MCP22xx_EVENT_STATE_ADDITIONAL:
            if (MCP22xx_rx.length > (uint16_t) SCOM_MAX_PACKET_SIZE) {
                MCP22xx_state = MCP22xx_STATE_IDLE;
            } else if (MCP22xx_rx.index < MCP22xx_rx.length) {
                MCP22xx_rx.data[MCP22xx_rx.index++] = byte;
                MCP22xx_rx.checksum += byte;
            } else { // Checksum
                MCP22xx_rx.checksum = 0xFF - MCP22xx_rx.checksum + 1;
                if (MCP22xx_rx.checksum == byte) {
                    if (MCP22xx_dataHandler) {
                        MCP22xx_dataHandler(MCP22xx_rx.length, MCP22xx_rx.data);
                    }
                } else {
                    // Wrong checksum
                }
                MCP22xx_state = MCP22xx_STATE_IDLE;
            }
            break;
        default:
            MCP22xx_state = MCP22xx_STATE_IDLE;
            break;
    }
}

void MCP22xx_checkNewDataAsync(void) {
    if (UART_isRXReady(MCP22xx_uart)) {
        uint8_t byte = UART_read(MCP22xx_uart);
        MCP22xx_processByte(byte);
    }
}

#endif

