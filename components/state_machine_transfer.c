/* 
 * File:   state_machine_transfer.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "state_machine_transfer.h"

#ifdef SM_MEM_ADDRESS

#include "../lib/common.h"
#include "../modules/bm78.h"
#include "../modules/lcd.h"
#include "../modules/mcp23017.h"
#include "../modules/memory.h"
#include "../modules/state_machine.h"
#include "../modules/ws281x.h"

/** State machine transmission state. */
struct {
    uint16_t address;    // Start address to sent in next block.
    uint16_t length;     // Total data length of the state machine. If set to 0
                         // than transmission is aborted. Will be also set to 0
                         // when transmission finishes.
} smTX = { 0x0000, 0x0000 };

void SMT_startTrasmission(void) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation()) {
        smTX.address = 0;
        smTX.length = SM_dataLength();
        BM78_resetChecksum();
        SMT_transmitNextBlock();
    }
}

bool SMT_hasNextBlock(void) {
    return smTX.length > 0;
}

void SMT_cancelTransmission(void) {
    smTX.length = 0; // Cancel state machine transfer
}

void SMT_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        case BM78_OPC_LE_CONNECTION_COMPLETE:
        case BM78_OPC_DISCONNECTION_COMPLETE:
        case BM78_OPC_SPP_CONNECTION_COMPLETE:
            SMT_cancelTransmission();
            break;
        default:
            break;
    }
}

bool SMT_bmcNextMessageSentHandler(uint8_t what) {
    if (SMT_hasNextBlock()) { // Transferring state machine
        SMT_transmitNextBlock();
        return false;
    } else {
        switch (what) {
            case BMC_SMT_TRANSMIT_STATE_MACHINE_CHECKSUM:
                BM78_send(2, BM78_MESSAGE_KIND_SM_CONFIGURATION, SM_checksum());
                break;
            case BMC_SMT_TRANSMIT_STATE_MACHINE:
                SMT_startTrasmission();
                break;
            case BMC_SMT_TRANSMIT_STATE_MCP23017:
                SMT_transmitIOs();
                break;
#ifdef WS281x_BUFFER                
            case BMC_SMT_TRANSMIT_STATE_WS281x:
                SMT_transmitRGBs();
                break;
#endif
#ifdef LCD_ADDRESS
            case BMC_SMT_TRANSMIT_STATE_LCD:
                SMT_transmitLCD();
                break;
#endif
        }
        return true;
    }
}

void SMT_transmitNextBlock(void) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation()) {
        if (smTX.length > 0 && BM78_isChecksumCorrect()) {
            smTX.address = smTX.address + (SM_BLOCK_SIZE - 7); // 64 = 57 + 5 + 2
        }

        if (smTX.length > 0 && smTX.address < smTX.length) {
            uint8_t block[SM_BLOCK_SIZE - 2], size = 5; // 64 = 62 + 2
            block[0] = BM78_MESSAGE_KIND_SM_PUSH;
            block[1] = smTX.length >> 8;
            block[2] = smTX.length & 0xFF;
            block[3] = smTX.address >> 8;
            block[4] = smTX.address & 0xFF;

            uint16_t addr;
            for (uint8_t i = 0; i < SM_BLOCK_SIZE - 7; i++) { // 64 = 57 + 5 + 2
                addr = smTX.address + i;
                if (addr < smTX.length) {
                    block[i + 5] = MEM_read(SM_MEM_ADDRESS, addr >> 8, addr & 0xFF);
                    size++;
                }
            }
            //block[2] = size;
            BM78_transmit(size, block);
            printProgress("|c|Downloading", addr, smTX.length);
        } else if (smTX.length > 0) {
            SMT_cancelTransmission();
            printProgress("|c|Downloading", 0, 0);
        }
    } else if (BM78.status != BM78_STATUS_SPP_CONNECTED_MODE) {
        SMT_cancelTransmission();
    }
}

void SMT_transmitIOs(void) {
    uint8_t data[0xFF];
    uint8_t index, byte, i, j;
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation() && SM_getStateTo) {
        BM78_resetChecksum();

        // MCP23017 (5 * 8 * 3 = 120)
        data[0] = BM78_MESSAGE_KIND_SM_SET_STATE; // Message kind
        data[1] = SM_PORT_SIZE * 8;               // No. of elements
        index = 2;

        // Inputs (first 4 ports are inputs)
        uint8_t state[SM_PORT_SIZE];
        SM_getStateTo(state);
        for (i = 0; i < SM_PORT_SIZE - 1; i++) {
            byte = state[i];
            for (j = 0; j < 8; j++) {
                data[index++] = i;                // Device
                data[index++] = 0x01;             // Length
                data[index++] = byte >> j & 0x01; // Value
            }
        }

        // Outputs (last port is output)
        byte = MCP_read(SM_OUT_ADDRESS, SM_OUT_PORT);
        for (i = 0; 0 < 8; i++) {
            data[index++] = 0x20 + i;         // Device
            data[index++] = 0x01;             // Length
            data[index++] = byte >> i & 0x01; // Value
        }

        BM78_transmit(index, data);
    }
}

#ifdef WS281x_BUFFER
void SMT_transmitRGBs(void) {
    uint8_t data[0xFF];
    uint8_t index, byte, i;
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation()) {
        BM78_resetChecksum();

        // WS281x (18 * (4 + 2) = 108 bytes)
        data[0] = BM78_MESSAGE_KIND_SM_SET_STATE;    // Message kind
        data[1] = WS281x_LED_COUNT;               // No. of elements
        index = 2;

        for (i = 0; i < WS281x_LED_COUNT; i++) {
            data[index++] = i + 0x28;             // Device
            data[index++] = 0x04;                 // Length
            data[index++] = WS281x_getPattern(i); // Pattern
            data[index++] = WS281x_getRed(i);     // Red component
            data[index++] = WS281x_getGreen(i);   // Green component
            data[index++] = WS281x_getBlue(i);    // Blue component
        }

        BM78_transmit(index, data);
    }
}
#endif

#ifdef LCD_ADDRESS
void SMT_transmitLCD(void) {
    uint8_t data[0xFF];
    uint8_t index, byte, i, j;
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation()) {
        BM78_resetChecksum();

        // LCD (84 + 4 = 88 bytes)
        data[0] = BM78_MESSAGE_KIND_SM_SET_STATE;// Message kind
        data[1] = 1;                          // No. of elements
        data[2] = 0x50;                       // Device
        data[3] = LCD_ROWS * (LCD_COLS + 1);  // Length: 4 * (20 + 1) = 84
        index = 4;

        for (i = 0; i < LCD_ROWS; i++) {           // Row
            for (j = 0; j < LCD_COLS; j++) {       // Column
                data[index++] = LCD_content[i][j]; // Character
            }
            data[index++] = '\n';                  // New line
        }

        BM78_transmit(index, data);
    }
}
#endif

#endif