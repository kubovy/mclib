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

#include <stdint.h>
#include "../lib/requirements.h"

#ifdef SM_MEM_ADDRESS

#include "../lib/types.h"
#include "../modules/bm78.h"
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif
#ifdef MCP23017_ENABLED
#include "../modules/mcp23017.h"
#if !defined SM_IN1_ADDRESS || !defined SM_IN2_ADDRESS
#warning "SMI: No MCP23017 used for inputs"
#endif
#ifndef SM_OUT_ADDRESS
#warning "SMI: No MCP23017 used for outputs"
#endif
#endif
#include "../modules/state_machine.h"
#ifdef WS281x_BUFFER
#include "../modules/ws281x.h"
#endif

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

