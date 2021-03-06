/* 
 * File:   state_machine_interaction.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * State machine interactions.
 */
#ifndef STATE_MACHINE_INTERACTION_H
#define	STATE_MACHINE_INTERACTION_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../lib/requirements.h"

#ifdef SM_MEM_ADDRESS

#include <stdint.h>
#include "../lib/types.h"

/** Start state machine interaction. */
void SMI_start(void);

#if defined BM78_ENABLED && defined MCP23017_ENABLED && defined SM_IN1_ADDRESS && defined SM_IN2_ADDRESS
/** 
 * State machine state getter implementation. 
 * 
 * @param Pointer the state to be set to.
 */
void SMI_stateGetter(uint8_t *state);
#endif

/**
 * State machine action handler implementation. 
 * 
 * @param device Device.
 * @param length Value length.
 * @param value Value
 */
void SMI_actionHandler(uint8_t device, uint8_t length, uint8_t *value);

/**
 * State machine error handler implementation.
 * 
 * @param error Error code.
 */
void SMI_errorHandler(uint8_t error);

/** State machine evaluation handler implementation. */
void SMI_evaluatedHandler(void);

/** Sets state machine's bluetooth trigger. */
void SMI_setBluetoothTrigger(Procedure_t bluetoothTrigger);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* STATE_MACHINE_INTERACTION_H */

