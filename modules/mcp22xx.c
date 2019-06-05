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

inline void MCP22xx_sendByte(uint8_t byte) {
    while (!UART_isTXReady(MCP22xx_uart)); // Wait till we can start sending.
    UART_write(MCP22xx_uart, byte); // Store each byte in the storePacket into the UART write buffer
    while (!UART_isTXDone(MCP22xx_uart));  // Wait until UART TX is done.
}

void MCP22xx_send(uint8_t length, uint8_t *data) {
    uint8_t chksum = 0x00 + length;
    MCP22xx_sendByte(0xAA);
    MCP22xx_sendByte(0x00); // Length High
    MCP22xx_sendByte(length); // Length Low
    
    for (uint8_t i = 0; i < length; i++) {     // Send the command bits, along with the parameters
        MCP22xx_sendByte(*(data + i));
        chksum += *(data + i);
    }
    MCP22xx_sendByte(0xFF - chksum + 1);
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

