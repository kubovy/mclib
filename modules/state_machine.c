/* 
 * File:   state_machine.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "state_machine.h"

#ifdef SM_MEM_ADDRESS
#ifdef SM_MAX_SIZE
#ifdef SM_CHECK_DELAY


#include "memory.h"

uint8_t SM_checkCounter = SM_CHECK_DELAY;

struct {
    uint8_t count;
} SM_states = {0};

struct {
    uint8_t id;               // Current state (0xFF state machine did not start yet)
    uint16_t start;           // Starting address of current state.
    uint8_t io[SM_PORT_SIZE]; // Stable state's data.
} SM_currentState = {0xFF, 0x0000};

struct {
    uint16_t start; // Action's starting address.
    uint16_t count; // Total number of actions.
} SM_actions = {0x0000, 0};

struct {
    uint8_t target;    // Target state (0xFF no target state)
    uint8_t index;     // Index on the path.
    uint8_t path[0xFF];// Goto path for loop detection.
} SM_goto = {0xFF, 0};

void (*SM_EvaluatedHandler)(void);
void (*SM_ErrorHandler)(uint8_t);

void SM_reset(void) {
    SM_states.count = 0;
    SM_currentState.id = 0xFF;
    SM_goto.target = 0xFF;
    SM_currentState.start = 0x0000;
    SM_actions.start = 0;
    SM_actions.count = 0;
}

void SM_init(void) {
    SM_currentState.start = 0x0000;

    // Reset current state.
    for (uint8_t i = 0; i < SM_PORT_SIZE; i++) {
        SM_currentState.io[i] = 0;
    }

    // 1st byte: Status (0x00 - Enabled, 0xFF - Disabled)
    SM_status = MEM_read(SM_MEM_ADDRESS, 0x00, 0x00);
    if (SM_status != SM_STATUS_ENABLED) {
        SM_reset();
        return;
    }

    // 2nd byte: Count of states (0-255)
    SM_states.count = MEM_read(SM_MEM_ADDRESS, 0x00, 0x01);
    if (((uint16_t) SM_states.count) >= (SM_MAX_SIZE - 2) / 2) {
        SM_reset();
        return;
    }

    // Start of actions start address.
    uint16_t actionsStartAddr = ((uint16_t) SM_states.count) * 2 + 2;
    if (actionsStartAddr >= SM_MAX_SIZE) {
        SM_reset();
        return;
    }

    // Actions start address (2 byte).
    uint8_t regHigh = MEM_read(SM_MEM_ADDRESS, actionsStartAddr >> 8, actionsStartAddr & 0xFF);
    uint8_t regLow = MEM_read(SM_MEM_ADDRESS, (actionsStartAddr + 1) >> 8, (actionsStartAddr + 1) & 0xFF);
    SM_actions.start = ((regHigh << 8) | regLow);
    if (SM_actions.start >= SM_MAX_SIZE) {
        SM_reset();
        return;
    }

    // Number of actions (2 byte).
    regHigh = MEM_read(SM_MEM_ADDRESS, SM_actions.start >> 8, SM_actions.start & 0xFF);
    regLow = MEM_read(SM_MEM_ADDRESS, (SM_actions.start + 1) >> 8, (SM_actions.start + 1) & 0xFF);
    SM_actions.count = ((regHigh << 8) | regLow);
    if (SM_actions.count >= SM_MAX_SIZE) {
        SM_reset();
        return;
    }
}

bool SM_changed(uint8_t *newState) {
    for (uint8_t i = 0; i < SM_PORT_SIZE; i++) {
        if (SM_currentState.io[i] != newState[i]) {
            SM_goto.index = 0; // Reset GOTO path index = reset loop detection
            return true;
        }
    }
    return false;
}

bool SM_enter(uint8_t stateId) {
    if (SM_status == SM_STATUS_ENABLED && stateId != SM_currentState.id) {
        SM_goto.index = 0; // Reset GOTO path index = reset loop detection
        SM_goto.target = stateId;
    }
    return SM_status == SM_STATUS_ENABLED;
}

void SM_evaluate(bool enteringState, uint8_t *newState) {
    if (SM_status != SM_STATUS_ENABLED) return;
    
    uint8_t regHigh = SM_currentState.start >> 8;
    uint8_t regLow = SM_currentState.start & 0xFF;
    uint8_t evaluationCount = MEM_read(SM_MEM_ADDRESS, regHigh, regLow);
    uint16_t evaluationStart = SM_currentState.start + 1;
    uint8_t gotoState = 0xFF;
    
    for (uint8_t e = 0; e < evaluationCount; e++) {
        bool hasConditions = false;
        bool wasChanged = false;
        bool result = true;

        for (uint8_t c = 0; c < SM_PORT_SIZE; c++) {
            uint16_t conditionStart = evaluationStart + c * 2;
            regHigh = conditionStart >> 8;
            regLow = conditionStart & 0xFF;
            uint8_t cond = MEM_read(SM_MEM_ADDRESS, regHigh, regLow);

            regHigh = (conditionStart + 1) >> 8;
            regLow = (conditionStart + 1) & 0xFF;
            uint8_t mask = MEM_read(SM_MEM_ADDRESS, regHigh, regLow);

            if (mask > 0) hasConditions = true;
            uint8_t changedMask = SM_currentState.io[c] ^ *(newState + c);
            if (mask & changedMask) wasChanged = true;
            result = result && ((cond & mask) == (*(newState + c) & mask));
        }
        
        uint16_t actionListStart = evaluationStart + SM_PORT_SIZE * 2;

        regHigh = actionListStart >> 8;
        regLow = actionListStart & 0xFF;
        uint8_t actionCount = MEM_read(SM_MEM_ADDRESS, regHigh, regLow);

        if (((hasConditions && wasChanged) || enteringState) && result) {
            for (uint8_t a = 0; a < actionCount; a++) {
                regHigh = (actionListStart + a * 2 + 1) >> 8;
                regLow = (actionListStart + a * 2 + 1) & 0xFF;
                uint16_t actionId = MEM_read(SM_MEM_ADDRESS, regHigh, regLow) << 8;

                regHigh = (actionListStart + a * 2 + 2) >> 8;
                regLow = (actionListStart + a * 2 + 2) & 0xFF;
                actionId = actionId | (MEM_read(SM_MEM_ADDRESS, regHigh, regLow) & 0xFF);

                uint16_t actionAddrStart = SM_actions.start + actionId * 2 + 2;
                regHigh = actionAddrStart >> 8;
                regLow = actionAddrStart & 0xFF;
                uint16_t actionAddr = MEM_read(SM_MEM_ADDRESS, regHigh, regLow) << 8;
                
                regHigh = (actionAddrStart + 1) >> 8;
                regLow = (actionAddrStart + 1) & 0xFF;
                actionAddr = actionAddr | (MEM_read(SM_MEM_ADDRESS, regHigh, regLow) & 0xFF);
                
                
                uint8_t device = MEM_read(SM_MEM_ADDRESS, actionAddr >> 8, actionAddr & 0xFF);
                bool includingEnteringState = (device & 0b10000000) == 0b10000000; // 0x80
                
                if (!enteringState || !hasConditions || includingEnteringState) {
                    uint8_t actionDevice = device & 0x7F;

                    uint8_t actionLength = MEM_read(SM_MEM_ADDRESS, (actionAddr + 1) >> 8, (actionAddr + 1) & 0xFF);
                    uint8_t actionValue[SM_VALUE_MAX_SIZE];

                    for (uint8_t v = 0; v < actionLength; v++) {
                        if (v < SM_VALUE_MAX_SIZE) {
                            actionValue[v] = MEM_read(SM_MEM_ADDRESS, (actionAddr + v + 2) >> 8, (actionAddr + v + 2) & 0xFF);
                        }
                    }

                    if (actionDevice == SM_DEVICE_GOTO) {
                        gotoState = actionValue[0];
                    } else if (SM_executeAction) {
                        SM_executeAction(actionDevice, actionLength, actionValue);
                    }
                }
            }
        }

        evaluationStart = actionListStart + actionCount * 2 + 1;
    }

    // Update new state as stable state
    for (uint8_t c = 0; c < SM_PORT_SIZE; c++) {
        SM_currentState.io[c] = *(newState + c);
    }

    if (gotoState < 0xFF) {
        bool loopDetected = gotoState == SM_currentState.id;
        for (uint8_t i = 0; i < SM_goto.index; i++) {
            loopDetected = loopDetected || SM_goto.path[i] == gotoState;
            if (loopDetected) break;
        }
        if (loopDetected) {
            MEM_write(SM_MEM_ADDRESS, 0x00, 0x00, 0xFF);
            SM_reset();
            if (SM_ErrorHandler) {
                SM_ErrorHandler(SM_ERROR_LOOP);
            }
        } else {
            SM_goto.path[SM_goto.index++] = gotoState;
            SM_goto.target = gotoState;
            if (SM_executeAction) {
                SM_executeAction(SM_DEVICE_GOTO, 1, &gotoState);
            }
        }
    } else {
        if (SM_EvaluatedHandler) {
            SM_EvaluatedHandler();
        }
    }
}

void SM_periodicalCheck(void) {
    if (SM_checkCounter > 0) {
        SM_checkCounter--;
    } else {
        SM_checkCounter = SM_CHECK_DELAY;
        if (SM_status == SM_STATUS_ENABLED && SM_getStateTo && (SM_goto.target < 0xFF || SM_currentState.id < 0xFF)) {
            uint8_t newState[SM_PORT_SIZE];
            SM_getStateTo(newState);

            if (SM_goto.target != SM_currentState.id) {
                SM_currentState.id = SM_goto.target;
                uint8_t regHigh = (((uint16_t) SM_goto.target) * 2 + 2) >> 8;
                uint8_t regLow = (((uint16_t) SM_goto.target) * 2 + 2) & 0xFF;
                SM_currentState.start = MEM_read(SM_MEM_ADDRESS, regHigh, regLow) << 8;

                regHigh = (((uint16_t) SM_goto.target) * 2 + 3) >> 8;
                regLow = (((uint16_t) SM_goto.target) * 2 + 3) & 0xFF;
                SM_currentState.start |= MEM_read(SM_MEM_ADDRESS, regHigh, regLow) & 0xFF;

                SM_evaluate(true, newState);
            } else if (SM_changed(newState)) {
                SM_evaluate(false, newState);
            }
        }
    }
}

void SM_setStateGetter(void (* StateGetter)(uint8_t*)) {
    SM_getStateTo = StateGetter;
}

void SM_setActionHandler(void (* ActionHandler)(uint8_t, uint8_t, uint8_t*)) {
    SM_executeAction = ActionHandler;
}

void SM_setEvaluatedHandler(void (* EvaluatedHandler)()) {
    SM_EvaluatedHandler = EvaluatedHandler;
}

void SM_setErrorHandler(void (* ErrorHandler)(uint8_t)) {
    SM_ErrorHandler = ErrorHandler;
}

uint16_t SM_dataLength(void) {
    uint8_t regHigh = MEM_read(SM_MEM_ADDRESS, 0x00, 0x00);
    uint8_t regLow = MEM_read(SM_MEM_ADDRESS, 0x00, 0x01);
    if (regHigh != SM_STATUS_ENABLED) return 0;
    if (((uint16_t) regLow) >= (SM_MAX_SIZE - 2) / 2) return 0;
    uint16_t actionsStartAddr = ((uint16_t) regLow) * 2 + 2;
    if (actionsStartAddr >= SM_MAX_SIZE) return SM_MAX_SIZE;

    regHigh = MEM_read(SM_MEM_ADDRESS, actionsStartAddr >> 8, actionsStartAddr & 0xFF);
    regLow = MEM_read(SM_MEM_ADDRESS, (actionsStartAddr + 1) >> 8, (actionsStartAddr + 1) & 0xFF);
    uint16_t actionsAddress = ((regHigh << 8) | regLow);
    if (actionsAddress >= SM_MAX_SIZE) return 0;

    regHigh = MEM_read(SM_MEM_ADDRESS, actionsAddress >> 8, actionsAddress & 0xFF);
    regLow = MEM_read(SM_MEM_ADDRESS, (actionsAddress + 1) >> 8, (actionsAddress + 1) & 0xFF);
    uint16_t actionCount = ((regHigh << 8) | regLow);
    if (actionCount >= SM_MAX_SIZE) return 0;
    if (actionsAddress >= SM_MAX_SIZE - (actionCount * 2)) return 0;

    uint16_t lastActionAddr = actionsAddress + (actionCount * 2);
    if (lastActionAddr >= SM_MAX_SIZE) return 0;

    regHigh = MEM_read(SM_MEM_ADDRESS, lastActionAddr >> 8, lastActionAddr & 0xFF);
    regLow = MEM_read(SM_MEM_ADDRESS, (lastActionAddr + 1) >> 8, (lastActionAddr + 1) & 0xFF);
    if (((regHigh << 8) | regLow) >= SM_MAX_SIZE - 1) return 0;
    uint16_t lastActionLengthAddr = ((regHigh << 8) | regLow) + 1;

    regHigh = MEM_read(SM_MEM_ADDRESS, lastActionLengthAddr >> 8, lastActionLengthAddr & 0xFF);
    if (lastActionLengthAddr >= SM_MAX_SIZE - regHigh - 1) return 0;
    return lastActionLengthAddr + regHigh + 1;
}

uint8_t SM_checksum(void) {
    uint16_t address, length = SM_dataLength();
    uint8_t regHigh, regLow, checksum = 0x00;
    for (address = 0; address < length; address++) {
        regHigh = address >> 8;
        regLow = address & 0xFF;
        checksum = checksum + MEM_read(SM_MEM_ADDRESS, regHigh, regLow);
    }
    return checksum;
}

#endif
#endif
#endif