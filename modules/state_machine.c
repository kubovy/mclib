/* 
 * File:   state_machine.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "state_machine.h"

#ifdef SM_MEM_ADDRESS

uint8_t SM_checkCounter = SM_CHECK_INTERVAL / TIMER_PERIOD;

struct {
    uint8_t count;
} SM_states = {0};

struct {
    uint8_t id;                // Current state (0xFF state machine did not start yet)
    uint16_t start;            // Starting address of current state.
    uint8_t io[SM_STATE_SIZE]; // Stable state's data.
} SM_currentState = {0xFF, SM_MEM_START};

struct {
    uint16_t start; // Action's starting address.
    uint16_t count; // Total number of actions.
} SM_actions = {SM_MEM_START, 0};

struct {
    uint8_t target;    // Target state (0xFF no target state)
    uint8_t index;     // Index on the path.
    uint8_t path[0xFF];// Goto path for loop detection.
} SM_goto = {0xFF, 0};

Procedure_t SM_EvaluatedHandler;
void (*SM_ErrorHandler)(uint8_t);

void SM_reset(void) {
    SM_states.count = 0;
    SM_currentState.id = 0xFF;
    SM_goto.target = 0xFF;
    SM_currentState.start = SM_MEM_START;
    SM_actions.start = 0;
    SM_actions.count = 0;
}

void SM_init(void) {
    SM_currentState.start = SM_MEM_START;

    // Reset current state.
    for (uint8_t i = 0; i < SM_STATE_SIZE; i++) {
        SM_currentState.io[i] = 0;
    }

    // 1st byte: Status (0x00 - Enabled, 0xFF - Disabled)
    SM_status = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START);
    if (SM_status != SM_STATUS_ENABLED) {
        SM_reset();
        return;
    }

    // 2nd byte: Count of states (0-255)
    SM_states.count = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START + 1);
    if (((uint16_t) SM_states.count) >= (SM_MAX_SIZE - SM_MEM_START - 2) / 2) {
        SM_reset();
        return;
    }

    // Start of actions start address.
    uint16_t actionsStartAddr = SM_MEM_START + ((uint16_t) SM_states.count) * 2 + 2;
    if (actionsStartAddr >= SM_MAX_SIZE) {
        SM_reset();
        return;
    }

    // Actions start address (2 byte).
    uint8_t regHigh = I2C_readRegister16(SM_MEM_ADDRESS, actionsStartAddr);
    uint8_t regLow = I2C_readRegister16(SM_MEM_ADDRESS, actionsStartAddr + 1);
    SM_actions.start = ((regHigh << 8) | regLow);
    if (SM_actions.start >= SM_MAX_SIZE) {
        SM_reset();
        return;
    }

    // Number of actions (2 byte).
    regHigh = I2C_readRegister16(SM_MEM_ADDRESS, SM_actions.start);
    regLow = I2C_readRegister16(SM_MEM_ADDRESS, SM_actions.start + 1);
    SM_actions.count = ((regHigh << 8) | regLow);
    if (SM_actions.count >= SM_MAX_SIZE) {
        SM_reset();
        return;
    }
}

bool SM_changed(uint8_t *newState) {
    for (uint8_t i = 0; i < SM_STATE_SIZE; i++) {
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
    
    uint8_t evaluationCount = I2C_readRegister16(SM_MEM_ADDRESS, SM_currentState.start);
    uint16_t evaluationStart = SM_currentState.start + 1;
    uint8_t gotoState = 0xFF;
    
    for (uint8_t e = 0; e < evaluationCount; e++) {
        bool hasConditions = false;
        bool wasChanged = false;
        bool result = true;

        for (uint8_t c = 0; c < SM_STATE_SIZE; c++) {
            uint16_t conditionStart = evaluationStart + c * 2;
            uint8_t cond = I2C_readRegister16(SM_MEM_ADDRESS, conditionStart);
            uint8_t mask = I2C_readRegister16(SM_MEM_ADDRESS, conditionStart + 1);

            if (mask > 0) hasConditions = true;
            uint8_t changedMask = SM_currentState.io[c] ^ *(newState + c);
            if (mask & changedMask) wasChanged = true;
            result = result && ((cond & mask) == (*(newState + c) & mask));
        }
        
        uint16_t actionListStart = evaluationStart + SM_STATE_SIZE * 2;
        uint8_t actionCount = I2C_readRegister16(SM_MEM_ADDRESS, actionListStart);

        if (((hasConditions && wasChanged) || enteringState) && result) {
            for (uint8_t a = 0; a < actionCount; a++) {
                uint16_t actionId = I2C_readRegister16(SM_MEM_ADDRESS, actionListStart + a * 2 + 1) << 8;
                actionId = actionId | (I2C_readRegister16(SM_MEM_ADDRESS, actionListStart + a * 2 + 2) & 0xFF);

                uint16_t actionAddrStart = SM_actions.start + actionId * 2 + 2;
                uint16_t actionAddr = I2C_readRegister16(SM_MEM_ADDRESS, actionAddrStart) << 8;
                actionAddr = actionAddr | (I2C_readRegister16(SM_MEM_ADDRESS, actionAddrStart + 1) & 0xFF);

                uint8_t device = I2C_readRegister16(SM_MEM_ADDRESS, actionAddr);
                bool includingEnteringState = (device & 0b10000000) == 0b10000000; // 0x80
                
                if (!enteringState || !hasConditions || includingEnteringState) {
                    uint8_t actionDevice = device & 0x7F;

                    uint8_t actionLength = I2C_readRegister16(SM_MEM_ADDRESS, actionAddr + 1);
                    uint8_t actionValue[SM_VALUE_MAX_SIZE];

                    for (uint8_t v = 0; v < actionLength; v++) {
                        if (v < SM_VALUE_MAX_SIZE) {
                            actionValue[v] = I2C_readRegister16(SM_MEM_ADDRESS, actionAddr + v + 2);
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
    for (uint8_t c = 0; c < SM_STATE_SIZE; c++) {
        SM_currentState.io[c] = *(newState + c);
    }

    if (gotoState < 0xFF) {
        bool loopDetected = gotoState == SM_currentState.id;
        for (uint8_t i = 0; i < SM_goto.index; i++) {
            loopDetected = loopDetected || SM_goto.path[i] == gotoState;
            if (loopDetected) break;
        }
        if (loopDetected) {
            I2C_writeRegister16(SM_MEM_ADDRESS, SM_MEM_START, SM_STATUS_DISABLED);
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
        SM_checkCounter = SM_CHECK_INTERVAL / TIMER_PERIOD;
        if (SM_status == SM_STATUS_ENABLED && SM_getStateTo && (SM_goto.target < 0xFF || SM_currentState.id < 0xFF)) {
            uint8_t newState[SM_STATE_SIZE];
            SM_getStateTo(newState);

            if (SM_goto.target != SM_currentState.id) {
                SM_currentState.id = SM_goto.target;
                SM_currentState.start = I2C_readRegister16(SM_MEM_ADDRESS,
                        SM_MEM_START + ((uint16_t) SM_goto.target) * 2 + 2) << 8;
                SM_currentState.start |= I2C_readRegister16(SM_MEM_ADDRESS,
                        SM_MEM_START + ((uint16_t) SM_goto.target) * 2 + 3) & 0xFF;

                SM_evaluate(true, newState);
            } else if (SM_changed(newState)) {
                SM_evaluate(false, newState);
            }
        }
    }
}

void SM_setStateGetter(SM_StateConsumer_t stateGetter) {
    SM_getStateTo = stateGetter;
}

void SM_setActionHandler(SM_executeAction_t actionHandler) {
    SM_executeAction = actionHandler;
}

void SM_setEvaluatedHandler(Procedure_t evaluatedHandler) {
    SM_EvaluatedHandler = evaluatedHandler;
}

void SM_setErrorHandler(Consumer_t errorHandler) {
    SM_ErrorHandler = errorHandler;
}

uint16_t SM_dataLength(void) {
    uint8_t regHigh = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START);
    uint8_t regLow = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START + 1);
    if (regHigh != SM_STATUS_ENABLED) return 0;
    if (((uint16_t) regLow) >= (SM_MAX_SIZE - SM_MEM_START - 2) / 2) return 0;
    uint16_t actionsStartAddr = ((uint16_t) regLow) * 2 + 2;
    if (actionsStartAddr >= SM_MAX_SIZE) return SM_MAX_SIZE;

    regHigh = I2C_readRegister16(SM_MEM_ADDRESS, actionsStartAddr);
    regLow = I2C_readRegister16(SM_MEM_ADDRESS, actionsStartAddr + 1);
    uint16_t actionsAddress = ((regHigh << 8) | regLow);
    if (actionsAddress >= SM_MAX_SIZE) return 0;

    regHigh = I2C_readRegister16(SM_MEM_ADDRESS, actionsAddress);
    regLow = I2C_readRegister16(SM_MEM_ADDRESS, actionsAddress + 1);
    uint16_t actionCount = ((regHigh << 8) | regLow);
    if (actionCount >= SM_MAX_SIZE) return 0;
    if (actionsAddress >= SM_MAX_SIZE - (actionCount * 2)) return 0;

    uint16_t lastActionAddr = actionsAddress + (actionCount * 2);
    if (lastActionAddr >= SM_MAX_SIZE) return 0;

    regHigh = I2C_readRegister16(SM_MEM_ADDRESS, lastActionAddr);
    regLow = I2C_readRegister16(SM_MEM_ADDRESS, lastActionAddr + 1);
    if (((regHigh << 8) | regLow) >= SM_MAX_SIZE - 1) return 0;
    uint16_t lastActionLengthAddr = ((regHigh << 8) | regLow) + 1;

    regHigh = I2C_readRegister16(SM_MEM_ADDRESS, lastActionLengthAddr);
    if (lastActionLengthAddr >= SM_MAX_SIZE - regHigh - 1) return 0;
    return lastActionLengthAddr + regHigh + 1;
}

uint8_t SM_checksum(void) {
    uint16_t address, length = SM_dataLength();
    uint8_t checksum = 0x00;
    for (address = 0; address < length; address++) {
        checksum = checksum + I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START + address);
    }
    return checksum;
}

#endif