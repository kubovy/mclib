/* 
 * File:   uart.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * UART wrapper.
 */
#ifndef UART_H
#define	UART_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../lib/requirements.h"

#if defined UART_ENABLED || defined EUSART_ENABLED

#include <stdbool.h>
#include <stdint.h>

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

/**
 * Checks if UART receiver is empty.
 * 
 * @param connection UART to use.
 * @return If any bytes on the UART are available for reading.
 */
inline bool UART_isRXReady(UART_Connection_t connection);

/**
 * Checks if the UART1 transmitter is ready.
 * 
 * @param connection UART to use.
 * @return If any bytes are remaining in the UART's transmit buffer.
 */
inline bool UART_isTXReady(UART_Connection_t connection);

/**
 * Checks if UART1 data is transmitted.
 * 
 * @param connection UART to use.
 * @return Status of UART1 transmit shift register.
 *         - TRUE: Data completely shifted out if the UART shift register.
 *         - FALSE: Data is not completely shifted out of the shift register.
 */
inline bool UART_isTXDone(UART_Connection_t connection);

/**
 * Read a byte of data from the UART1.
 * 
 * @param connection UART to use.
 * @return A data byte received by the driver.
 */
inline uint8_t UART_read(UART_Connection_t connection);

/**
 * Writes a byte of data to the UART1.
 * 
 * @param connection UART to use.
 * @param byte Data byte to write to the UART.
 */
inline void UART_write(UART_Connection_t connection, uint8_t byte);

#endif    

#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

