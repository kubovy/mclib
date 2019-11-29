/* 
 * File:   state_machine_transfer.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "state_machine_transfer.h"

#if defined SCOM_ENABLED && defined SM_MEM_ADDRESS

/** State machine transmission state. */
typedef struct {
    uint16_t address;    // Start address to sent in next block.
    uint16_t length;     // Total data length of the state machine. If set to 0
                         // than transmission is aborted. Will be also set to 0
                         // when transmission finishes.
} SMT_TX_t; // = { 0x0000, 0x0000 };

SMT_TX_t smTX[SCOM_CHANNEL_COUNT];

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
void transmitNextBlock(SCOM_Channel_t channel) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !SCOM_awatingConfirmation(SCOM_CHANNEL_BT)) {
        if (smTX[channel].length > 0) {
            // 32 = CRC(1) + reserve(1)  + MSGTYPE(1) + LEN(2) + ADR(2) + DATA(25)
            smTX[channel].address = smTX[channel].address + (SMT_BLOCK_SIZE - 7);
        } else {
            smTX[channel].length = SM_dataLength();
        }

        if (smTX[channel].length > 0 && smTX[channel].address < smTX[channel].length) {
            SCOM_addDataByte(SCOM_CHANNEL_BT, 0, MESSAGE_KIND_SM_PUSH);
            SCOM_addDataByte2(SCOM_CHANNEL_BT, 1, smTX[channel].length);
            SCOM_addDataByte2(SCOM_CHANNEL_BT, 3, smTX[channel].address);

            for (uint8_t i = 0; i < SMT_BLOCK_SIZE - 7; i++) { // 32 = 25 + 5 + 2
                if ((smTX[channel].address + i) < smTX[channel].length) {
                    SCOM_addDataByte(SCOM_CHANNEL_BT, i + 5,
                            I2C_readRegister16(SM_MEM_ADDRESS, smTX[channel].address + i));
                }
            }
            
            // MSGTYPE(1) + LEN(2) + ADR(2)
            SCOM_commitData(SCOM_CHANNEL_BT, 6 + min16(SMT_BLOCK_SIZE - 7, smTX[channel].length - smTX[channel].address), BM78_MAX_SEND_RETRIES);
#ifdef LCD_ADDRESS
            printProgress("    Downloading     ", smTX[channel].address + SMT_BLOCK_SIZE - 7, smTX[channel].length);
#endif
        } else if (smTX[channel].length > 0) {
            smTX[channel].length = 0; // Cancel state machine transfer
#ifdef LCD_ADDRESS
            printProgress("    Downloading     ", 0, 0);
#endif
        }
    } else if (BM78.status != BM78_STATUS_SPP_CONNECTED_MODE) {
        smTX[channel].length = 0; // Cancel state machine transfer
    }
}

void SMT_bm78AppModeResponseHandler(BM78_Response_t *response) {
    switch (response->opCode) {
        case BM78_EVENT_LE_CONNECTION_COMPLETE:
        case BM78_EVENT_DISCONNECTION_COMPLETE:
        case BM78_EVENT_SPP_CONNECTION_COMPLETE:
            smTX[SCOM_CHANNEL_BT].length = 0; // Cancel state machine transfer
            break;
        default:
            break;
    }
}

void SMT_scomDataHandler(SCOM_Channel_t channel, uint8_t length, uint8_t *data) {
    switch(*(data + 1)) {
        case MESSAGE_KIND_SM_CONFIGURATION: // SM checksum requested
            SCOM_enqueue(channel, MESSAGE_KIND_SM_CONFIGURATION, 0);
            break;
        case MESSAGE_KIND_SM_PULL: // SM pull requested
            SCOM_enqueue(channel, MESSAGE_KIND_SM_PUSH, 0);
            break;
        case MESSAGE_KIND_SM_PUSH: // SM block pushed
            if (length > 5) {
                uint8_t byte, read;
                uint16_t size = (*(data + 2) << 8) | (*(data + 3) & 0xFF);
                uint16_t startReg = (*(data + 4) << 8) | (*(data + 5) & 0xFF);

                for(uint8_t i = 6; i < length; i++) {
                    // Make sure 1st 2 bytes are 0xFF -> disable state machine
                    if ((startReg + i - 6) == SM_MEM_START) {
                        byte = SM_STATUS_DISABLED;
                    } else {
                        byte = *(data + i);
                    }

                    read = I2C_readRegister16(SM_MEM_ADDRESS, (startReg + i - 6));
                    bool repeated = false;
                    while(read != byte) {
                        if (repeated) __delay_ms(100);
                        I2C_writeRegister16(SM_MEM_ADDRESS, (startReg + i - 6), byte);
                        read = I2C_readRegister16(SM_MEM_ADDRESS, (startReg + i - 6));
                        repeated = true;
                    }
                }

#ifdef LCD_ADDRESS
                printProgress("     Uploading      ", startReg + length - 6, size);
#endif

                // Finished
                if (startReg + length - 6 >= size) {
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
        case MESSAGE_KIND_SM_GET_STATE:
            SCOM_enqueue(channel, MESSAGE_KIND_MCP23017, SM_IN1_ADDRESS);
            SCOM_enqueue(channel, MESSAGE_KIND_MCP23017, SM_IN2_ADDRESS);
            SCOM_enqueue(channel, MESSAGE_KIND_MCP23017, SM_OUT_ADDRESS);
            SCOM_enqueue(channel, MESSAGE_KIND_SM_SET_STATE, 0);
#ifdef WS281x_INDICATORS
            SCOM_enqueue(channel, MESSAGE_KIND_WS281x, SCOM_PARAM_ALL);
#endif
#ifdef LCD_ADDRESS
            SCOM_enqueue(channel, MESSAGE_KIND_LCD, SCOM_PARAM_ALL);
#endif
            break;
        case MESSAGE_KIND_SM_ACTION:
            if (SM_executeAction && length > 1) {
                uint8_t count = *(data + 2);
                uint8_t index = 3;
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

bool SMT_scomNextMessageHandler(SCOM_Channel_t channel, uint8_t what, uint8_t param) {
    if (smTX[channel].length > 0) { // Transferring state machine
        transmitNextBlock(channel);
        return smTX[channel].length == 0;
    } else {
        switch (what) {
            case MESSAGE_KIND_SM_CONFIGURATION:
                SCOM_addDataByte(channel, 0, MESSAGE_KIND_SM_CONFIGURATION);
                SCOM_addDataByte(channel, 1, SM_checksum());
                return SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES);
                break;
            case MESSAGE_KIND_SM_PUSH:
                smTX[channel].address = 0;
                smTX[channel].length = 0;
                SCOM_resetChecksum(SCOM_CHANNEL_BT);
                transmitNextBlock(channel);
                return smTX[channel].length == 0; // Consume if nothing to send.
                break;
            case MESSAGE_KIND_SM_SET_STATE:
                SCOM_addDataByte(channel, 0, MESSAGE_KIND_SM_SET_STATE);
                //SCOM_addDataByte(1, )
                break;
        }
        return true;
    }
}
#endif