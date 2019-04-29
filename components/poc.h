/* 
 * File:   poc.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#ifndef POC_H
#define	POC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../config.h"
#include "../modules/bm78.h"
#ifdef DHT11_PORT
#include "../modules/dht11.h"
#endif
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif

#ifdef DHT11_PORT
void POC_testDHT11(void);
#endif
#ifdef LCD_ADDRESS
void POC_bm78InitializationHandler(char *deviceName, char *pin);
void POC_bm78ResponseHandler(BM78_Response_t response, uint8_t *data);
void POC_bm78TransparentDataHandler(uint8_t start, uint8_t length, uint8_t *data);
void POC_bm78ErrorHandler(BM78_Response_t response, uint8_t *data);
void POC_displayData(uint16_t address, uint8_t length, uint8_t *data);
#ifdef MEM_ADDRESS
void POC_testMem(uint16_t address);
void POC_testMemPage(void);
#endif
void POC_testDisplay(void);
void POC_showKeypad(uint8_t address, uint8_t port);
void POC_testMCP23017Input(uint8_t address);
void POC_testMCP23017Output(uint8_t address, uint8_t port);
#endif

#ifdef WS281x_BUFFER
void POC_demoWS281x(void);
void POC_testWS281x(void);
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* POC_H */

