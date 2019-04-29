/* 
 * File:   bm78_transmission.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * BM78 communication component.
 * 
 * A component must include configuration and can additionally include modules,
 * libs, but no components.
 */
#ifndef BM78_COMMUNICATION_H
#define	BM78_COMMUNICATION_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../config.h"
#include "../modules/bm78.h"

#ifdef BM78_ENABLED // Needs BM78 module

#ifndef BMC_NOTHING_TO_TRANSMIT
#define BMC_NOTHING_TO_TRANSMIT 0xFF
#endif

/**
 * Next message handler setter. This handler should implement sending a 
 * particular message depending on the parameter.
 * 
 * @param NextMessageHandler The handler.
 */
void BMC_setNextMessageHandler(bool (* NextMessageHandler)(uint8_t));

/**
 * Add definition to transmission queue.
 * 
 * @param what Definition what to transmit.
 */
void BMC_transmit(uint8_t what);

/**
 * BM78 Application mode response handler sample.
 * 
 * @param response BM78 response.
 * @param data BM78 additional response data.
 */
void BMC_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data);

/** Called when a BT message was sent. */
void BMC_bm78MessageSentHandler(void);

/**
 * BM78's transparent data handler implementation.
 * 
 * @param length Transparent data lenght.
 * @param data Transparent data.
 */
void BMC_bm78TransparentDataHandler(uint8_t length, uint8_t *data);

/**
 * BM78's error handler.
 * 
 * @param response BM78 response.
 * @param data BM78 additional response data.
 */
void BMC_bm78ErrorHandler(BM78_Response_t response, uint8_t *data);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* BM78_TRANSMISSION_H */

