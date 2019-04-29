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
#include "../../config.h"

#ifdef SM_MEM_ADDRESS

#include "../modules/bm78.h"
#include "../modules/state_machine.h"

// Configuration with default values
#ifndef SM_BLOCK_SIZE
#define SM_BLOCK_SIZE 64
#endif

#ifndef BMC_SMT_TRANSMIT_STATE_MACHINE_CHECKSUM
#define BMC_SMT_TRANSMIT_STATE_MACHINE_CHECKSUM 0x10
#endif
#ifndef BMC_SMT_TRANSMIT_STATE_MACHINE
#define BMC_SMT_TRANSMIT_STATE_MACHINE 0x11
#endif
#ifndef BMC_SMT_TRANSMIT_STATE_MCP23017
#define BMC_SMT_TRANSMIT_STATE_MCP23017 0x12
#endif
#ifndef BMC_SMT_TRANSMIT_STATE_WS281x
#define BMC_SMT_TRANSMIT_STATE_WS281x 0x13
#endif
#ifndef BMC_SMT_TRANSMIT_STATE_LCD
#define BMC_SMT_TRANSMIT_STATE_LCD 0x14
#endif

/**
 * Starts State Machine Transmission.
 * 
 * This will only happen if the bluetooth is in connected state and not waiting
 * for confirmation (checksum) of previous transmission block.
 * 
 * In case a state machine is already being transmitted the previous 
 * transmission will be canceled and new started.
 */
void SMT_startTrasmission(void);

/** Whether state machine transmission is ongoing and not completed yet. */
bool SMT_hasNextBlock(void);

/** Cancels state machine transmission. */
void SMT_cancelTransmission(void);

/**
 * State machine's BM78 application-mode response handler implementation.
 * 
 * @param response BM78 response.
 * @param data Additional response data.
 */
void SMT_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data);

/**
 * State machine's BM78 communiction next message sent handler implementation.
 * 
 * @param what Type of message to send next.
 * @return Whether the message type was sent, or not (e.g. something else was 
 *         sent instead)
 */
bool SMT_bmcNextMessageSentHandler(uint8_t what);

/**
 * Transfers next block of a state machine.
 * 
 * This will only happen if the bluetooth is in connected state and not waiting
 * for confirmation (checksum) of previous transmission block.
 * 
 * If last state machine block was confirmed the next state machine block will
 * be transmitted. Otherwise he last state machine block will be transmitted
 * again.
 * 
 * If the bluetooth is not in connected mode the state machine transmission will
 * be aborted.
 */
void SMT_transmitNextBlock(void);

/** 
 * Transmit State Machine I/O states.
 * 
 * This will only happen if the bluetooth is in connected state and not waiting
 * for confirmation (checksum) of previous transmission block.
 */
void SMT_transmitIOs(void);

#ifdef WS281x_BUFFER
/** 
 * Transmit State Machine RGB light's states.
 * 
 * This will only happen if the bluetooth is in connected state and not waiting
 * for confirmation (checksum) of previous transmission block.
 */
void SMT_transmitRGBs(void);
#endif

#ifdef LCD_ADDRESS
/** 
 * Transmit LCD state.
 * 
 * This will only happen if the bluetooth is in connected state and not waiting
 * for confirmation (checksum) of previous transmission block.
 */
void SMT_transmitLCD(void);
#endif

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* STATE_MACHINE_TRANSFER_H */

