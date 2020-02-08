/* 
 * File:   state_machine_transfer.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "state_machine_transfer.h"
#include "../lib/common.h"
#include "../modules/i2c.h"
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif
#include "../modules/state_machine.h"
#include "state_machine_interaction.h"

#if defined SCOM_ENABLED && defined SM_MEM_ADDRESS

Procedure_t uploadStartCallback = NULL;
Procedure_t uploadFinishedCallback = NULL;

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
    if (SCOM_canSend(channel)) {
        if (SCOM_dataTransfer.end > 0) {
            // 32 = CRC(1) + reserve(1)  + MSGTYPE(1) + LEN(2) + ADR(2) + DATA(25)
            SCOM_dataTransfer.start = SCOM_dataTransfer.start + (SMT_BLOCK_SIZE - 7);
        } else {
            SCOM_dataTransfer.end = SM_dataLength();
        }

        if (SCOM_dataTransfer.end > 0
                && SCOM_dataTransfer.start < SCOM_dataTransfer.end) {
            SCOM_addDataByte(channel, 0, MESSAGE_KIND_DATA);
            SCOM_addDataByte(channel, 1, SM_DATA_PART);
            SCOM_addDataByte2(channel, 2, SCOM_dataTransfer.start);
            SCOM_addDataByte2(channel, 4, SCOM_dataTransfer.end);

            for (uint8_t i = 0; i < SMT_BLOCK_SIZE - 7; i++) { // 32 = 25 + 5 + 2
                if ((SCOM_dataTransfer.start + i) < SCOM_dataTransfer.end) {
                    SCOM_addDataByte(channel, i + 5,
                            I2C_readRegister16(SM_MEM_ADDRESS,
                            SCOM_dataTransfer.start + i));
                }
            }
            
            // MSGTYPE(1) + LEN(2) + ADR(2)
            SCOM_commitData(channel, 6 + min16(SMT_BLOCK_SIZE - 7,
                    SCOM_dataTransfer.end - SCOM_dataTransfer.start),
                    BM78_MAX_SEND_RETRIES);
#ifdef LCD_ADDRESS
            printProgress("    Downloading     ", SCOM_dataTransfer.start + SMT_BLOCK_SIZE - 7, SCOM_dataTransfer.end);
#endif
        } else if (SCOM_dataTransfer.stage != 0x00) {
            SCOM_dataTransfer.stage = 0x00; // Finish state machine transfer
#ifdef LCD_ADDRESS
            printProgress("    Downloading     ", 0, 0);
#endif
        }
    } else if (!SCOM_canEnqueue(channel)) {
        SCOM_dataTransfer.stage = 0x00; // Cancel state machine transfer
    }
}

void SMT_bm78AppModeResponseHandler(BM78_Response_t *response) {
    switch (response->opCode) {
        case BM78_EVENT_LE_CONNECTION_COMPLETE:
        case BM78_EVENT_DISCONNECTION_COMPLETE:
        case BM78_EVENT_SPP_CONNECTION_COMPLETE:
            SCOM_dataTransfer.stage = 0x00;
            SCOM_dataTransfer.end = 0; // Cancel state machine transfer
            break;
        default:
            break;
    }
}

void SMT_scomDataHandler(SCOM_Channel_t channel, uint8_t length, uint8_t *data) {
    switch(*(data + 1)) {
        case MESSAGE_KIND_CONSISTENCY_CHECK: // SM checksum requested
            if (length == 3) switch (*(data + 2)) {
                case SM_DATA_PART:
                    SCOM_enqueue(channel, MESSAGE_KIND_CONSISTENCY_CHECK,
                            SM_DATA_PART, 0x00);
                    break;
            }
            break;
        case MESSAGE_KIND_DATA:
            if (length >= 3) switch (*(data + 2)) {
                case SM_DATA_PART:
                    if (length == 3) { // Pull
                        SCOM_sendData(channel, SM_DATA_PART, 0x0000, 0x0000);
                    } else if (length == 6) { // Pull part
                        // TODO JK: Not implemented
                    } else if (length > 6) { // Push
                        uint8_t byte, read;
                        uint16_t startReg = (*(data + 3) << 8) | (*(data + 4) & 0xFF);
                        uint16_t size = (*(data + 5) << 8) | (*(data + 6) & 0xFF);

                        if (startReg == 0 && uploadStartCallback) uploadStartCallback();

                        for(uint8_t i = 7; i < length; i++) {
                            // Make sure 1st 2 bytes are 0xFF -> disable state machine
                            if ((startReg + i - 7) == SM_MEM_START) {
                                byte = SM_STATUS_DISABLED;
                            } else {
                                byte = *(data + i);
                            }

                            read = I2C_readRegister16(SM_MEM_ADDRESS, (startReg + i - 7));
                            bool repeated = false;
                            while(read != byte) {
                                if (repeated) __delay_ms(100);
                                I2C_writeRegister16(SM_MEM_ADDRESS, (startReg + i - 7), byte);
                                read = I2C_readRegister16(SM_MEM_ADDRESS, (startReg + i - 7));
                                repeated = true;
                            }
                        }

        #ifdef LCD_ADDRESS
                        printProgress("     Uploading      ", startReg + length - 7, size);
        #endif

                        // Finished
                        if (startReg + length - 7 >= size) {
                            // Set 1st 2 bytes on end of transmission -> enable state machine
                            read = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START);
                            bool repeated = false;
                            while (read != SM_STATUS_ENABLED) {
                                if (repeated) __delay_ms(100);
                                I2C_writeRegister16(SM_MEM_ADDRESS, SM_MEM_START, SM_STATUS_ENABLED);
                                read = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START);
                                repeated = true;
                            }

                            if (uploadFinishedCallback) uploadFinishedCallback();
                            SMI_start();
                        }
                    }
                    break;
            }
            break;
        case MESSAGE_KIND_SM_STATE_ACTION:
            if (length == 2) { // Get state
                SCOM_enqueue(channel, MESSAGE_KIND_REGISTRY,
                        SM_IN1_ADDRESS, 0xFF);
                SCOM_enqueue(channel, MESSAGE_KIND_REGISTRY,
                        SM_IN2_ADDRESS, 0xFF);
                SCOM_enqueue(channel, MESSAGE_KIND_REGISTRY,
                        SM_OUT_ADDRESS, 0xFF);
#ifdef WS281x_INDICATORS
                SCOM_enqueue(channel, MESSAGE_KIND_INDICATORS,
                        0x00, SCOM_PARAM_ALL);
#endif
#ifdef LCD_ADDRESS
                SCOM_enqueue(channel, MESSAGE_KIND_LCD, 0x00, SCOM_PARAM_ALL);
#endif
            } else if (SM_executeAction && length > 4) { // Execute action
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

bool SMT_scomNextMessageHandler(SCOM_Channel_t channel, uint8_t what,
        uint8_t param1, uint8_t param2) {
    if (SCOM_dataTransfer.stage != 0x00 && what == MESSAGE_KIND_DATA) {
        // Transferring
        transmitNextBlock(channel);
        return SCOM_dataTransfer.stage == 0x00; // Consume if nothing to send
    } else switch (what) {
        case MESSAGE_KIND_CONSISTENCY_CHECK:
            switch (param1) {
                case SM_DATA_PART:
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_CONSISTENCY_CHECK);
                    SCOM_addDataByte(channel, 1, SM_DATA_PART);
                    SCOM_addDataByte(channel, 2, SM_checksum());
                    return SCOM_commitData(channel, 3, BM78_MAX_SEND_RETRIES);
                default:
                    return true;  // I have nothing to contribute, consume IMHO
            }
        case MESSAGE_KIND_DATA:
            switch (param1) {
                case SM_DATA_PART:
                    SCOM_resetChecksum(channel);
                    transmitNextBlock(channel);
                    // Consume if nothing to send
                    return SCOM_dataTransfer.stage == 0x00;
                default:
                    return true; // I have nothing to contribute, consume IMHO
            }
        default:
            return true; // I have nothing to contribute, consume IMHO
    }
}

void SMT_setUploadStartCallback(Procedure_t callback) {
    uploadStartCallback = callback;
}

void SMT_setUploadFinishedCallback(Procedure_t callback) {
    uploadFinishedCallback = callback;
}

#endif
