/* 
 * File:   mcp22xx.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * MCP2200/MCP2221 USB bridge module.
 */
#ifndef MCP22XX_H
#define	MCP22XX_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../lib/requirements.h"
    
#if defined MCP2200_ENABLED || defined MCP2221_ENABLED

#include "uart.h"
    
// Single event states
typedef enum {
    MCP22xx_STATE_IDLE = 0x00,
    MCP22xx_EVENT_STATE_LENGTH_HIGH = 0x01,
    MCP22xx_EVENT_STATE_LENGTH_LOW = 0x02,
    MCP22xx_EVENT_STATE_ADDITIONAL = 0x03,
} MCP22xx_EventState_t;
    
void MCP22xx_initialize(UART_Connection_t uart, DataHandler_t dataHandler);

void MCP22xx_send(uint8_t length, uint8_t *data);

void MCP22xx_checkNewDataAsync(void);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* MCP22XX_H */

