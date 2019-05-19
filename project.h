/* 
 * File:   project.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * Includes all libraries, modules, components and generated MC files.
 * 
 * This file should be included only by the project's main.c file.
 */
#ifndef PROJECT_H
#define	PROJECT_H

#ifdef	__cplusplus
extern "C" {
#endif
    
//--msgdisable=520,2053
#pragma warning disable 520
#pragma warning disable 2053

// Library
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

// Globals

// Common
#include "lib/ascii.h"
#include "lib/common.h"
#include "lib/types.h"

// Configuration
#include "lib/requirements.h"

// Modules
#ifdef BM78_ENABLED
#include "modules/bm78.h"
#endif
#ifdef DHT11_PORT
#include "modules/dht11.h"
#endif
#ifdef I2C_ENABLED
#include "modules/i2c.h"
#endif
#ifdef LCD_ADDRESS
#include "modules/lcd.h"
#endif
#ifdef MCP_ENABLED
#include "modules/mcp23017.h"
#endif
//#include "modules/mem.h"
#ifdef RGB_ENABLED
#include "modules/rgb.h"
#endif
#ifdef SM_MEM_ADDRESS
#include "modules/state_machine.h"
#endif
#if defined UART || defined EUSART
#include "modules/uart.h"
#endif
#ifdef WS281x_BUFFER
#include "modules/ws281x.h"
#endif

// Components
#ifdef BM78_ENABLED
#include "components/bm78_pairing.h"
#include "components/bm78_communication.h"
#endif
#include "components/poc.h"
#ifdef SM_MEM_ADDRESS
#include "components/state_machine_interaction.h"
#ifdef BM78_ENABLED
#include "components/state_machine_transfer.h"
#endif
#endif
#include "components/setup_mode.h"

#ifdef	__cplusplus
}
#endif

#endif	/* PROJECT_H */

