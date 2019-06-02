/* 
 * File:   bm78_eeprom.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * BM78 EEPROM component responsible for flashing the BM78 module.
 */
#ifndef BM78_EEPROM_H
#define	BM78_EEPROM_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../lib/requirements.h"
#include "../modules/bm78.h"
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif

#ifdef BM78_ENABLED

bool BM78EEPROM_initializing = false;
    
    
/**
 * Initializes BM78's EEPROM
 */
void BM78EEPROM_initialize(void);

/**
 * Sets initialization started handler.
 * 
 * @param handler The handler.
 */
void BM78EEPROM_setInitializationStartedHandler(Procedure_t handler);

/**
 * Sets initialization success handler.
 * 
 * @param initializedHandler The handler.
 */
void BM78EEPROM_setInitializationSuccessHandler(Procedure_t initializedHandler);

/**
 * Sets initialization failed handler.
 * 
 * @param failedHandler The handler.
 */
void BM78EEPROM_setInitializationFailedHandler(Procedure_t failedHandler);

/**
 * BM78 test mode response handler.
 * 
 * @param response BM78 response.
 * @param data Additional data.
 */
void BM78EEPROM_testModeResponseHandler(BM78_Response_t response, uint8_t* data);

/**
 * BM78 error handler.
 * 
 * @param response BM78 response.
 * @param data Response data.
 */
void BM78EEPROM_testModeErrorHandler(BM78_Response_t response, uint8_t* data);

/**
 * BM78 test mode periodical check.
 */
void BM78EEPROM_bm78TestModeCheck(void);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* BM78_EEPROM_H */

