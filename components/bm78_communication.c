/* 
 * File:   bm78_transmission.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78_communication.h"

#ifdef BM78_ENABLED

#include "../lib/common.h"
#include "../modules/memory.h"
#include "../modules/state_machine.h"

/** 
 * Queue of message types to be send out over BT. Should be cleared when 
 * connection ends for whatever reason. 
 */
struct {
    uint8_t index;       // Sent index.
    uint8_t tail;        // Tail index.
    uint8_t queue[0xFF]; // Transmission message type queue.
} tx = {0, 0, 0};

bool (*BMC_NextMessageHandler)(uint8_t);

void BMC_setNextMessageHandler(bool (* NextMessageHandler)(uint8_t)) {
    BMC_NextMessageHandler = NextMessageHandler;
}

void BMC_transmit(uint8_t what) {
    uint8_t index = tx.index;
    while (index != tx.tail) {
        // Will be transmitted in the future, don't add again.
        if (tx.queue[index++] == what) return;
    }
    // FIXME: We are relying here that the queue will never be overfilled.
    tx.queue[tx.tail++] = what;
}

void BMC_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        // Pairing Events
        case BM78_OPC_PASSKEY_ENTRY_REQ:
        case BM78_OPC_PAIRING_COMPLETE:
        case BM78_OPC_PASSKEY_DISPLAY_YES_NO_REQ:
            break;
        // GAP Events
        case BM78_OPC_LE_CONNECTION_COMPLETE:
        case BM78_OPC_DISCONNECTION_COMPLETE:
        case BM78_OPC_SPP_CONNECTION_COMPLETE:
            break;
        // Common Events
        case BM78_OPC_COMMAND_COMPLETE:
            switch(response.CommandComplete_0x80.command) {
                case BM78_CMD_READ_LOCAL_INFORMATION:
                case BM78_CMD_READ_DEVICE_NAME:
                case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                case BM78_CMD_READ_PIN_CODE:
                default:
                    break;
            }
            break;
        case BM78_OPC_BM77_STATUS_REPORT:
            switch(response.StatusReport_0x81.status) {
                case BM78_STATUS_STANDBY_MODE:
                case BM78_STATUS_SPP_CONNECTED_MODE:
                case BM78_STATUS_LE_CONNECTED_MODE:
                case BM78_STATUS_IDLE_MODE:
                case BM78_STATUS_POWER_ON:
                case BM78_STATUS_PAGE_MODE:
                case BM78_STATUS_LINK_BACK_MODE:
                case BM78_STATUS_SHUTDOWN_MODE:
                default:
                    break;
            }
            break;
        case BM78_OPC_CONFIGURE_MODE_STATUS:
            break;
        // SPP/GATT Transparent Event
        case BM78_OPC_RECEIVED_TRANSPARENT_DATA:
        case BM78_OPC_RECEIVED_SPP_DATA:
        // Unknown
        default:
            break;
    }
}

void BMC_bm78MessageSentHandler(void) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation()) {
        if (BMC_NextMessageHandler) {
            uint8_t what = tx.index != tx.tail ? tx.queue[tx.index] : BMC_NOTHING_TO_TRANSMIT;
            if (BMC_NextMessageHandler(what) && what != BMC_NOTHING_TO_TRANSMIT) tx.index++;
        }
    } else if (BM78.status != BM78_STATUS_SPP_CONNECTED_MODE) {
        // Reset transmit queue if connection lost.
        tx.index = 0;
        tx.tail = 0;
    }
}

void BMC_bm78TransparentDataHandler(uint8_t length, uint8_t *data) {
    uint8_t i, j;
    bool repeated;
    //POC_bm78TransparentDataHandler(start, length, data);
    switch(*(data)) {
        case BM78_MESSAGE_KIND_SM_CONFIGURATION: // SM checksum requested
            BMC_transmit(BMC_SMT_TRANSMIT_STATE_MACHINE_CHECKSUM);
            break;
        case BM78_MESSAGE_KIND_SM_PULL: // SM pull requested
            BMC_transmit(BMC_SMT_TRANSMIT_STATE_MACHINE);
            break;
        case BM78_MESSAGE_KIND_SM_PUSH: // SM block pushed
            if (length > 5) {
                uint8_t byte, read;
                uint8_t regHigh = *(data + 1);
                uint8_t regLow = *(data + 2);
                uint16_t size = (regHigh << 8) | (regLow & 0xFF);

                regHigh = *(data + 3);
                regLow = *(data + 4);
                uint16_t startReg = (regHigh << 8) | (regLow & 0xFF);

                for(i = 5; i < length; i++) {
                    regHigh = (startReg + i - 5) >> 8;
                    regLow = (startReg + i - 5) & 0xFF;

                    // Make sure 1st 2 bytes are 0xFF -> disable state machine
                    if (regHigh == 0x00 && regLow == 0x00) {
                        byte = SM_STATUS_DISABLED;
                    } else {
                        byte = *(data + i);
                    }

                    read = MEM_read(SM_MEM_ADDRESS, regHigh, regLow);
                    repeated = false;
                    while(read != byte) {
                        if (repeated) __delay_ms(100);
                        MEM_write(SM_MEM_ADDRESS, regHigh, regLow, byte);
                        read = MEM_read(SM_MEM_ADDRESS, regHigh, regLow);
                        repeated = true;
                    }
                }

                printProgress("|c|Uploading", startReg + length - 5, size);

                // Finished
                if (startReg + length - 5 >= size) {
                    // Set 1st 2 bytes on end of transmission -> enable state machine
                    read = MEM_read(SM_MEM_ADDRESS, 0x00, 0x00);
                    repeated = false;
                    while (read != SM_STATUS_ENABLED) {
                        if (repeated) __delay_ms(100);
                        MEM_write(SM_MEM_ADDRESS, 0x00, 0x00, SM_STATUS_ENABLED);
                        read = MEM_read(SM_MEM_ADDRESS, 0x00, 0x00);
                        repeated = true;
                    }
                    
                    // TODO: smStart();
                }
            }
            break;
        case BM78_MESSAGE_KIND_SM_GET_STATE:
            BMC_transmit(BMC_SMT_TRANSMIT_STATE_MCP23017);
            BMC_transmit(BMC_SMT_TRANSMIT_STATE_WS281x);
            BMC_transmit(BMC_SMT_TRANSMIT_STATE_LCD);
            break;
        /*case BM78_MESSAGE_KIND_SM_SET_STATE:
            if (length > 1) {
                uint8_t count = *(data + 1);
                uint8_t index = 2;
                for (i = 0; i < count; i++) {
                    uint8_t device = *(data + index);
                    uint8_t size = *(data + index + 1);
                    if (device < 32 && size == 1) {
                        uint8_t value = *(data + index + 2);
                        SM_set(device, value & 0x01);
                    }
                    index += size + 2;
                }
            }
            break;*/
        case BM78_MESSAGE_KIND_SM_ACTION:
            if (SM_executeAction && length > 1) {
                uint8_t count = *(data + 1);
                uint8_t index = 2;
                for (i = 0; i < count; i++) {
                    uint8_t device = *(data + index);
                    uint8_t size = *(data + index + 1);
                    uint8_t value[SM_VALUE_MAX_SIZE];
                    for (j = 0; j < size; j++) if (j < SM_VALUE_MAX_SIZE) {
                        value[j] = *(data + index + j + 2);
                    }
                    SM_executeAction(device, size, value);
                    index += size + 2;
                }
            }
            break;
        case BM78_MESSAGE_KIND_PLAIN:
            if (length > 1) {
                char message[SM_VALUE_MAX_SIZE];
                for (i = 1; i < length; i++) {
                    if ((i - 1) < SM_VALUE_MAX_SIZE) {
                        message[i - 1] = *(data + i);
                    }
                }
                LCD_displayString(message, 0);
            }
            break;
    }
}

void BMC_bm78ErrorHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        case BM78_OPC_COMMAND_COMPLETE:
            switch(response.CommandComplete_0x80.command) {
                case BM78_CMD_DISCONNECT:
                    switch (response.CommandComplete_0x80.status) {
                        case BM78_ERR_COMMAND_DISALLOWED:
                            BM78_reset();
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

#endif