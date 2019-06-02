/* 
 * File:   poc.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * Proof-of-Concept functions.
 */
#ifndef POC_H
#define	POC_H

#ifdef	__cplusplus
extern "C" {
#endif

//#include <stdint.h>
#ifdef MEM_INTERNAL_SIZE
#include "../../mcc_generated_files/memory.h"
#endif
#include "../lib/common.h"
#include "../lib/requirements.h"
#ifdef BM78_ENABLED
#include "../modules/bm78.h"
#endif
#ifdef DHT11_PORT
#include "../modules/dht11.h"
#endif
#ifdef I2C_ENABLED
#include "../modules/i2c.h"
#endif
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif
#ifdef MCP23017_ENABLED
#include "../modules/mcp23017.h"
#endif
#ifdef RGB_ENABLED
#include "../modules/rgb.h"
#endif
#ifdef WS281x_BUFFER
#include "../modules/ws281x.h"
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
#include "../modules/ws281x_light.h"
#endif
#endif
#ifdef SCOM_ENABLED
#include "serial_communication.h"
#endif


#ifdef DHT11_PORT
void POC_testDHT11(void);
#endif
#ifdef LCD_ADDRESS
#ifdef BM78_ENABLED
void POC_bm78InitializationHandler(char *deviceName, char *pin);
void POC_bm78EventHandler(BM78_Response_t response, uint8_t *data);
void POC_bm78ErrorHandler(BM78_Response_t response, uint8_t *data);
#endif
#ifdef SCOM_ENABLED
void POC_scomDataHandler(SCOM_Channel_t channel, uint8_t length, uint8_t *data);
#endif
void POC_displayData(uint16_t address, uint8_t length, uint8_t *data);
#if defined I2C_ENABLED || defined MEM_INTERNAL_SIZE
void POC_testMem(uint8_t address, uint16_t reg);
//void POC_testMemPage(void);
#endif
void POC_testDisplay(void);
#ifdef MCP23017_ENABLED
void POC_showKeypad(uint8_t address, uint8_t port);
void POC_testMCP23017Input(uint8_t address);
void POC_testMCP23017Output(uint8_t address, uint8_t port);
#endif
#endif
#ifdef RGB_ENABLED
void POC_testRGB(RGB_Pattern_t pattern);
#endif
#ifdef WS281x_BUFFER
void POC_demoWS281x(void);
void POC_testWS281x(void);
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
void POC_testWS281xLight(WS281xLight_Pattern_t pattern);
#endif
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* POC_H */

