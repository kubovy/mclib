/* 
 * File:   uart.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "uart.h"

#if defined UART_ENABLED || defined EUSART_ENABLED

inline bool UART_isRXReady(UART_Connection_t connection) {
    switch(connection) {
#ifdef UART1_ENABLED
        case UART_1:
            return UART1_is_rx_ready();
#endif
#ifdef UART2_ENABLED
        case UART_2:
            return UART2_is_rx_ready();
#endif
#ifdef EUSART_ENABLED
        case UART_EUSART:
            return EUSART_is_rx_ready();
#endif
    }
}

inline bool UART_isTXReady(UART_Connection_t connection) {
    switch(connection) {
#ifdef UART1_ENABLED
        case UART_1:
            return UART1_is_tx_ready();
#endif
#ifdef UART2_ENABLED
        case UART_2:
            return UART2_is_tx_ready();
#endif
#ifdef EUSART_ENABLED
        case UART_EUSART:
            return EUSART_is_tx_ready();
#endif
    }
}


inline bool UART_isTXDone(UART_Connection_t connection) {
    switch(connection) {
#ifdef UART1_ENABLED
        case UART_1:
            return UART1_is_tx_done();
#endif
#ifdef UART2_ENABLED
        case UART_2:
            return UART2_is_tx_done();
#endif
#ifdef EUSART_ENABLED
        case UART_EUSART:
            return EUSART_is_tx_done();
#endif
    }
}


inline uint8_t UART_read(UART_Connection_t connection) {
    switch(connection) {
#ifdef UART1_ENABLED
        case UART_1:
            return UART1_Read();
#endif
#ifdef UART2_ENABLED
        case UART_2:
            return UART2_Read();
#endif
#ifdef EUSART_ENABLED
        case UART_EUSART:
            return EUSART_Read();
#endif
    }
}


inline void UART_write(UART_Connection_t connection, uint8_t byte) {
    switch(connection) {
#ifdef UART1_ENABLED
        case UART_1:
            UART1_Write(byte);
            break;
#endif
#ifdef UART2_ENABLED
        case UART_2:
            UART2_Write(byte);
            break;
#endif
#ifdef EUSART_ENABLED
        case UART_EUSART:
            EUSART_Write(byte);
            break;
#endif
    }
}

#endif