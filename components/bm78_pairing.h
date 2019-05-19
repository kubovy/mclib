/* 
 * File:   bm78_pairing.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * A component must include configuration and can additionally include modules,
 * libs, but no components.
 */
#ifndef BM78_PAIRING_H
#define	BM78_PAIRING_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../lib/requirements.h"

#ifdef BM78_ENABLED

#include "../lib/common.h"
#include "../modules/bm78.h"
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif

#ifndef BMP_CMD_TIMEOUT
#warning "BMP: Command timeout defaults to BM78_INIT_CMD_TIMEOUT"
#define BMP_CMD_TIMEOUT BM78_INIT_CMD_TIMEOUT
#endif
    
#ifdef BM78_ADVANCED_PAIRING
/** Whether or not paring request is waiting. */
bool BMP_waiting(void);

/** Cancels paring. */
void BMP_cancel(void);

/**
 * Process key for pairing. Does nothing unless pairing request is waiting.
 * 
 * @param key Entered key. Accepts: 0-9, A-D, *, #
 */
void BMP_processKey(uint8_t key);

#endif

/** 
 * Retry trigger.
 * 
 * Should be called in TIMER_PERIOD intervals from the main loop.
 */
void BMP_retryTrigger(void);

/**
 * Enter pairing mode
 */
void BMP_enterPairingMode(void);

/**
 * Initiates removal of all paired devices.
 */
void BMP_removeAllPairedDevices(void);

/**
 * Response handler catching BM78 pairing events.
 * 
 * @param response BM78 event response.
 * @param data Additional event data.
 */
void BMP_bm78EventHandler(BM78_Response_t response, uint8_t *data);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* BM78_PAIRING_H */

