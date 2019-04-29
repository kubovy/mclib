/* 
 * File:   state_machine.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * Implementation of a state machine.
 * 
 * - It starts with a status byte: SM_STATUS_ENABLED, SM_STATUS_DISABLED
 * - 2nd byte: Number of states
 * - then a sequence of 2byte addresses for each state
 * - after that the address where actions start
 * - then the states start, each state:
 *   - first a count of evaluations
 *   - then evaluations of that state start:
 *     - each evaluation contains 2 bytes * SM_PORT_SIZE
 *       - 1st byte: condition value
 *       - 2nd byte: condition mask (which bits of condition value are relevant)
 *     - next number of actions of this evaluation
 *     - next are the 2 byte addresses of each action
 * - next the 2 byte total number of actions
 * - continuing with list of 2 byte addresses of all the actions
 * - then the actions start:
 *   - 1st byte is action type
 *   - 2nd byte is action value length
 *   - then the action value
 *    
 * // Legend
 * STA   - status
 * STC   - state count low
 * i     - SCH << 8 | SCL - state count
 * SSH   - state start high
 * SSL   - state start low
 * ASH   - action start high
 * ASL   - action start low
 * ECi   - evaluation count for state i (j)
 * Cijk  - condition k in evaluation j for state i
 * Mijk  - mask k in evaluation j for state i
 * ACij  - evaluation action count
 * AHijl - action address high in evaluation j
 * ALijl - action address low in evaluation j
 * ACH   - action count high
 * ACL   - action count low
 * x     - ACH << 8 | ACL - action count
 * AAHx  - action address high
 * AALx  - action address low
 * ATx   - action type
 * ALx   - action data length
 * ADxy  - action data byte
 * 
 * // Header
 * STA  |STC  |
 * SSH0 |SSL0 |SSH1 |SSL1 | ... |SSHi |SSLi |
 * ASH  |ASL  |
 * 
 * // State 0
 * EC0  |
 * C000 |M000 |C001 |M001 |C002 |M002 |C003 |M003 |C004 |M004 |
 * AC00 |
 * AH000|AL000|AH001|AL001| ... |AH00l|AL00l|
 * C010 |M010 |C011 |M011 |C012 |M012 |C013 |M013 |C014 |M014 |
 * AC01 |
 * AH010|AL010|AH011|AL011| ... |AH01l|AL01l|
 * ...
 * C0j0 |M0j0 |C0j1 |M0j1 |C0j2 |M0j2 |C0j3 |M0j3 |C0j4 |M0j4 |
 * AC0j |
 * AH0j0|AL0j0|AH0j1|AL0j1| ... |AH0jl|AL0jl|
 * 
 * ...
 * 
 * // State i
 * ECi  |
 * Ci00 |Mi00 |Ci01 |Mi01 |Ci02 |Mi02 |Ci03 |Mi03 |Ci04 |M004 |
 * ACi0 |
 * AHi00|ALi00|AHi01|ALi01| ... |AHi0l|ALi0l|
 * ...
 * Cij0 |Mij0 |Cij1 |Mij1 |Cij2 |Mij2 |C0ij3 |Mij3 |Cij4 |Mij4 |
 * ACij |
 * AHij0|ALij0|AHij1|ALij1| ... |AHijl|ALijl|
 * 
 * // Actions
 * ACH  |ACL  |
 * AAH0 |AAL0 |AAH1 |AAL1 | ... |AAHx |AALx |
 * // Action 0
 * AT0  |AL0  |AD00 |AD01 | ... |AD0y |
 * // Action 1
 * AT1  |AL1  |AD10 |AD11 | ... |AD1y |
 * ...
 * // Action x
 * ATx  |ALx  |ADx0 |ADx1 | ... |ADxy |
 */
#ifndef STATE_MACHINE_H
#define	STATE_MACHINE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../../config.h"

// Configuration
#ifndef SM_MAX_SIZE
#ifdef MEM_SIZE
#define SM_MAX_SIZE MEM_SIZE
#endif
#endif

#ifndef SM_CHECK_DELAY
#ifdef TIMER_PERIOD
#define SM_CHECK_DELAY 100 / TIMER_PERIOD // ms
#endif
#endif

#ifdef SM_MEM_ADDRESS
#ifdef SM_MAX_SIZE
#ifdef SM_CHECK_DELAY
    
#define SM_VALUE_MAX_SIZE 0x60
#define SM_PORT_SIZE 5

// Action value type
#define SM_ACTION_TYPE_BOOL 0
#define SM_ACTION_TYPE_UINT8 1
#define SM_ACTION_TYPE_UINT8_ARR 2
#define SM_ACTION_TYPE_STRING 3

// State machine status
#define SM_STATUS_ENABLED 0x00
#define SM_STATUS_DISABLED 0xFF

// State machine error codes
#define SM_ERROR_LOOP 0x01

// MCP23017        (  39 -    0 + 1 =   40)
//                 (0x27 - 0x00 + 1 = 0x28)
#define SM_DEVICE_MCP23017_START 0x00
#define SM_DEVICE_MCP23017_END 0x27
// MCP23017 input  (  31 -    0 + 1 =   32)
//                 (0x1F - 0x00 + 1 = 0x20)
#define SM_DEVICE_MCP23017_IN_START 0x00
#define SM_DEVICE_MCP23017_IN_END 0x1F
// MCP23017 output (  39 -   32 + 1 =    8)
//                 (0x27 - 0x20 + 1 = 0x08)
#define SM_DEVICE_MCP23017_OUT_START 0x20
#define SM_DEVICE_MCP23017_OUT_END 0x27

// WS281x output (  71 - 41   + 1 = 32)
//               (0x47 - 0x28 + 1 = 20)
#define SM_DEVICE_WS281x_START 0x28
#define SM_DEVICE_WS281x_END 0x47

#define SM_DEVICE_LCD_MESSAGE 0x50
#define SM_DEVICE_LCD_BACKLIGHT 0x51
#define SM_DEVICE_LCD_RESET 0x52
#define SM_DEVICE_LCD_CLEAR 0x53

#define SM_DEVICE_BT_CONNECTED 0x60
#define SM_DEVICE_BT_TRIGGER 0x61

#define SM_DEVICE_GOTO 0x70
#define SM_DEVICE_ENTER 0x71

/** Whether the state machine is enabled or not */
uint8_t SM_status = SM_STATUS_ENABLED;

#ifndef BM78_MESSAGE_KIND_SM_CONFIGURATION
#define BM78_MESSAGE_KIND_SM_CONFIGURATION 0x80
#endif
#ifndef BM78_MESSAGE_KIND_SM_PULL
#define BM78_MESSAGE_KIND_SM_PULL 0x81
#endif
#ifndef BM78_MESSAGE_KIND_SM_PUSH
#define BM78_MESSAGE_KIND_SM_PUSH 0x82
#endif
#ifndef BM78_MESSAGE_KIND_SM_GET_STATE
#define BM78_MESSAGE_KIND_SM_GET_STATE 0x83
#endif
#ifndef BM78_MESSAGE_KIND_SM_SET_STATE
#define BM78_MESSAGE_KIND_SM_SET_STATE 0x84
#endif
#ifndef BM78_MESSAGE_KIND_SM_ACTION
#define BM78_MESSAGE_KIND_SM_ACTION 0x84
#endif

/**
 * Retrieves current state and passes it to the output parameter.
 * 
 * @param state Obtained stated output parameter.
 */
void (*SM_getStateTo)(uint8_t* state);
void (*SM_executeAction)(uint8_t, uint8_t, uint8_t*);

/**
 * Initialization, needs to be called before state machine can be used.
 * 
 * This also sets the "SM_status" depending on the memory content. If some data
 * are wrong, e.g. no valid state machine was uploaded, the "SM_status" will be
 * set to "SM_STATUS_DISABLED".
 */
void SM_init(void);

/**
 * State machine's periodical check should be called in a loop with timer
 * using the TIMER_PERIOD period. It checks the current state, evaluates the
 * state machine's conditions and executes corresponding actions.
 * 
 * The periodical check uses SM_CHECK_DELAY to delay consecutive checks.
 */
void SM_periodicalCheck(void);

/**
 * Enter a state.
 * 
 * @param stateId State's ID.
 * @return Returns true if state was changed.
 */
bool SM_enter(uint8_t stateId);

/**
 * Defines state getter.
 * 
 * @param StateGetter State getter function.
 */
void SM_setStateGetter(void (* StateGetter)(uint8_t*));

/**
 * Defines action handler.
 * 
 * @param ActionHandler Action handler.
 */
void SM_setActionHandler(void (* ActionHandler)(uint8_t, uint8_t, uint8_t*));

/**
 * Defines evaluated handler which is called when evaluation is complete.
 * 
 * @param EvaluatedHandler
 */
void SM_setEvaluatedHandler(void (* EvaluatedHandler)());

/**
 * Defines error handler.
 * 
 * @param ErrorHandler Error handler.
 */
void SM_setErrorHandler(void (* ErrorHandler)(uint8_t));

/**
 * Calculates State Machine's data length.
 * 
 * @return Length in bytes.
 */
uint16_t SM_dataLength(void);

/**
 * Calculates State Machine's checksum.
 * 
 * @return Checksum.
 */
uint8_t SM_checksum(void);

#endif
#endif
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* STATE_MACHINE_H */

