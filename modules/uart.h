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

#if defined UART || defined EUSART

#if defined UART
#include "../../mcc_generated_files/uart1.h"
#elif defined EUSART
#include "../../mcc_generated_files/eusart.h"
#endif

inline bool UART_isRXReady(void);
inline bool UART_isTXReady(void);
inline bool UART_isTXDone(void);
inline uint8_t UART_read(void);
inline void UART_write(uint8_t byte);

#endif    

#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

