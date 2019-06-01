/* 
 * File:   uart.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#ifndef UART_H
#define	UART_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../lib/requirements.h"

#if defined UART_ENABLED || defined EUSART_ENABLED

#ifdef UART1_ENABLED
#include "../../mcc_generated_files/uart1.h"
#endif
#ifdef UART2_ENABLED
#include "../../mcc_generated_files/uart2.h"
#endif
#ifdef EUSART_ENABLED
#include "../../mcc_generated_files/eusart.h"
#endif

typedef enum {
#if defined UART1_ENABLED && defined UART2_ENABLED
    UART_1,
    UART_2
#elif defined UART1_ENABLED
    UART_1
#elif defined UART2_ENABLED
    UART_2
#elif defined EUSART_ENABLED
    UART_EUSART
#endif
} UART_Connection_t;

inline bool UART_isRXReady(UART_Connection_t connection);
inline bool UART_isTXReady(UART_Connection_t connection);
inline bool UART_isTXDone(UART_Connection_t connection);
inline uint8_t UART_read(UART_Connection_t connection);
inline void UART_write(UART_Connection_t connection, uint8_t byte);

#endif    

#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

