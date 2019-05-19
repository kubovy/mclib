/* 
 * File:   state_machine_transfer.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "state_machine_transfer.h"

#if defined BM78_ENABLED && defined SM_MEM_ADDRESS

/** State machine transmission state. */
struct {
    uint16_t address;    // Start address to sent in next block.
    uint16_t length;     // Total data length of the state machine. If set to 0
                         // than transmission is aborted. Will be also set to 0
                         // when transmission finishes.
} smTX = { 0x0000, 0x0000 };

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
void SMT_transmitNextBlock(void) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation()) {
        if (smTX.length > 0 && BM78_isChecksumCorrect()) {
            // 32 = CRC(1) + reserve(1)  + MSGTYPE(1) + LEN(2) + ADR(2) + DATA(25)
            smTX.address = smTX.address + (SMT_BLOCK_SIZE - 7);
        }

        if (smTX.length > 0 && smTX.address < smTX.length) {
            BM78_addDataByte(0, BM78_MESSAGE_KIND_SM_PUSH);
            BM78_addDataByte2(1, smTX.length);
            BM78_addDataByte2(3, smTX.address);

            for (uint8_t i = 0; i < SMT_BLOCK_SIZE - 7; i++) { // 32 = 25 + 5 + 2
                if ((smTX.address + i) < smTX.length) {
                    BM78_addDataByte(i + 5,
                            I2C_readRegister16(SM_MEM_ADDRESS, smTX.address + i));
                }
            }
            
            // MSGTYPE(1) + LEN(2) + ADR(2)
            BM78_commitData(5 + min(smTX.address + SMT_BLOCK_SIZE - 7, smTX.length), BM78_MAX_SEND_RETRIES);
#ifdef LCD_ADDRESS
            printProgress("    Downloading     ", smTX.address + SMT_BLOCK_SIZE - 7, smTX.length);
#endif
        } else if (smTX.length > 0) {
            smTX.length = 0; // Cancel state machine transfer
#ifdef LCD_ADDRESS
            printProgress("    Downloading     ", 0, 0);
#endif
        }
    } else if (BM78.status != BM78_STATUS_SPP_CONNECTED_MODE) {
        smTX.length = 0; // Cancel state machine transfer
    }
}

void SMT_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        case BM78_EVENT_LE_CONNECTION_COMPLETE:
        case BM78_EVENT_DISCONNECTION_COMPLETE:
        case BM78_EVENT_SPP_CONNECTION_COMPLETE:
            smTX.length = 0; // Cancel state machine transfer
            break;
        default:
            break;
    }
}

void SMT_bm78TransparentDataHandler(uint8_t length, uint8_t *data) {
    switch(*(data)) {
        case BM78_MESSAGE_KIND_SM_CONFIGURATION: // SM checksum requested
            BMC_enqueue(BM78_MESSAGE_KIND_SM_CONFIGURATION, 0);
            break;
        case BM78_MESSAGE_KIND_SM_PULL: // SM pull requested
            BMC_enqueue(BM78_MESSAGE_KIND_SM_PUSH, 0);
            break;
        case BM78_MESSAGE_KIND_SM_PUSH: // SM block pushed
            if (length > 5) {
                uint8_t byte, read;
                uint16_t size = (*(data + 1) << 8) | (*(data + 2) & 0xFF);
                uint16_t startReg = (*(data + 3) << 8) | (*(data + 4) & 0xFF);

                for(uint8_t i = 5; i < length; i++) {
                    // Make sure 1st 2 bytes are 0xFF -> disable state machine
                    if ((startReg + i - 5) == SM_MEM_START) {
                        byte = SM_STATUS_DISABLED;
                    } else {
                        byte = *(data + i);
                    }

                    read = I2C_readRegister16(SM_MEM_ADDRESS, (startReg + i - 5));
                    bool repeated = false;
                    while(read != byte) {
                        if (repeated) __delay_ms(100);
                        I2C_writeRegister16(SM_MEM_ADDRESS, (startReg + i - 5), byte);
                        read = I2C_readRegister16(SM_MEM_ADDRESS, (startReg + i - 5));
                        repeated = true;
                    }
                }

#ifdef LCD_ADDRESS
                printProgress("     Uploading      ", startReg + length - 5, size);
#endif

                // Finished
                if (startReg + length - 5 >= size) {
                    // Set 1st 2 bytes on end of transmission -> enable state machine
                    read = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START);
                    bool repeated = false;
                    while (read != SM_STATUS_ENABLED) {
                        if (repeated) __delay_ms(100);
                        I2C_writeRegister16(SM_MEM_ADDRESS, SM_MEM_START, SM_STATUS_ENABLED);
                        read = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START);
                        repeated = true;
                    }
                    
                    // TODO: smStart();
                }
            }
            break;
        case BM78_MESSAGE_KIND_SM_GET_STATE:
            BMC_enqueue(BM78_MESSAGE_KIND_MCP23017, SM_IN1_ADDRESS);
            BMC_enqueue(BM78_MESSAGE_KIND_MCP23017, SM_IN2_ADDRESS);
            BMC_enqueue(BM78_MESSAGE_KIND_MCP23017, SM_OUT_ADDRESS);
            BMC_enqueue(BM78_MESSAGE_KIND_SM_SET_STATE, 0);
#ifdef WS281x_BUFFER
            BMC_enqueue(BM78_MESSAGE_KIND_WS281x, BMC_PARAM_ALL);
#endif
#ifdef LCD_ADDRESS
            BMC_enqueue(BM78_MESSAGE_KIND_LCD, BMC_PARAM_ALL);
#endif
            break;
        case BM78_MESSAGE_KIND_SM_ACTION:
            if (SM_executeAction && length > 1) {
                uint8_t count = *(data + 1);
                uint8_t index = 2;
                while (count > 0) {
                    uint8_t device = *(data + index);
                    uint8_t size = *(data + index + 1);
                    uint8_t value[SM_VALUE_MAX_SIZE];
                    for (uint8_t i = 0; i < size; i++) if (i < SM_VALUE_MAX_SIZE) {
                        value[i] = *(data + index + i + 2);
                    }
                    SM_executeAction(device, size, value);
                    index += size + 2;
                    count--;
                }
            }
            break;
    }
}

bool SMT_bmcNextMessageHandler(uint8_t what, uint8_t param) {
    if (smTX.length > 0) { // Transferring state machine
        SMT_transmitNextBlock();
        return false;
    } else {
        switch (what) {
            case BM78_MESSAGE_KIND_SM_CONFIGURATION:
                BM78_addDataByte(0, BM78_MESSAGE_KIND_SM_CONFIGURATION);
                BM78_addDataByte(1, SM_checksum());
                return BM78_commitData(2, BM78_MAX_SEND_RETRIES);
                break;
            case BM78_MESSAGE_KIND_SM_PUSH:
                smTX.address = 0;
                smTX.length = SM_dataLength();
                BM78_resetChecksum();
                SMT_transmitNextBlock();
                return smTX.length == 0; // Consume if nothing to send.
                break;
            case BM78_MESSAGE_KIND_SM_SET_STATE:
                BM78_addDataByte(0, BM78_MESSAGE_KIND_SM_SET_STATE);
                //BM78_addDataByte(1, )
                break;
        }
        return true;
    }
}
#endif