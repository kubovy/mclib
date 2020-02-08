/* 
 * File:   state_machine_transfer.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * State machine transfer component.
 */
#ifndef STATE_MACHINE_TRANSFER_H
#define	STATE_MACHINE_TRANSFER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../lib/requirements.h"

#if defined SCOM_ENABLED && defined SM_MEM_ADDRESS

#include <stdbool.h>
#include "../lib/types.h"
#include "../modules/bm78.h"
#include "serial_communication.h"

// Configuration with default values
#ifndef SMT_BLOCK_SIZE
#ifdef SCOM_MAX_PACKET_SIZE
#warning "SMT: Block size defaults to SCOM_MAX_PACKET_SIZE"
#define SMT_BLOCK_SIZE SCOM_MAX_PACKET_SIZE
#else
#error "SMT: One of SMT_BLOCK_SIZE or SCOM_MAX_PACKET_SIZE must be defined!"
#endif
#endif
    
/**
 * State machine's BM78 application-mode response handler implementation.
 * 
 * @param response BM78 response.
 */
void SMT_bm78AppModeResponseHandler(BM78_Response_t *response);

/**
 * State machine's transparent data handler implementation.
 * 
 * @param length Transparent data length.
 * @param data Transparent data.
 */
void SMT_scomDataHandler(SCOM_Channel_t channel, uint8_t length, uint8_t *data);

/**
 * State machine's BM78 communication next message handler implementation.
 * 
 * @param channel Channel.
 * @param what Type of message to send next.
 * @param param1 Parameter 1.
 * @param param2 Parameter 2.
 * @return Whether all messages of this type were sent (e.g. some more will
 *         be still sent). True will consume the item in the queue and proceed
 *         with next one.
 */
bool SMT_scomNextMessageHandler(SCOM_Channel_t channel, uint8_t what,
        uint8_t param1, uint8_t param2);

void SMT_setUploadStartCallback(Procedure_t callback);

void SMT_setUploadFinishedCallback(Procedure_t callback);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* STATE_MACHINE_TRANSFER_H */

