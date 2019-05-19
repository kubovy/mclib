/* 
 * File:   state_machine_transfer.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * A component must include configuration and can additionally include modules,
 * libs, but no components.
 */
#ifndef STATE_MACHINE_TRANSFER_H
#define	STATE_MACHINE_TRANSFER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "../lib/requirements.h"

#if defined BM78_ENABLED && defined SM_MEM_ADDRESS

#include "../modules/bm78.h"
#include "../modules/i2c.h"
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif
#include "../modules/state_machine.h"
#include "bm78_communication.h"

// Configuration with default values
#ifndef SMT_BLOCK_SIZE
#ifdef BM78_DATA_PACKET_MAX_SIZE
#warning "SMT: Block size defaults to BM78_DATA_PACKET_MAX_SIZE"
#define SMT_BLOCK_SIZE BM78_DATA_PACKET_MAX_SIZE
#else
#error "SMT: One of SMT_BLOCK_SIZE or BM78_DATA_PACKET_MAX_SIZE must be defined!"
#endif
#endif
    
/**
 * State machine's BM78 application-mode response handler implementation.
 * 
 * @param response BM78 response.
 * @param data Additional response data.
 */
void SMT_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data);

/**
 * State machine's transparent data handler implementation.
 * 
 * @param length Transparent data length.
 * @param data Transparent data.
 */
void SMT_bm78TransparentDataHandler(uint8_t length, uint8_t *data);

/**
 * State machine's BM78 communication next message handler implementation.
 * 
 * @param what Type of message to send next.
 * @return Whether the message type was sent, or not (e.g. something else was 
 *         being sent at the time).
 */
bool SMT_bmcNextMessageHandler(uint8_t what, uint8_t param);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* STATE_MACHINE_TRANSFER_H */

