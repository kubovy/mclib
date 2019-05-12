/* 
 * File:   bm78_transmission.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78_communication.h"

#ifdef BM78_ENABLED

#include "../lib/common.h"
#include "../modules/memory.h"
#include "../modules/rgb.h"
#include "../modules/state_machine.h"
#include "../modules/ws281x.h"

/** 
 * Queue of message types to be send out over BT. Should be cleared when 
 * connection ends for whatever reason. 
 */
struct {
    uint8_t index;       // Sent index.
    uint8_t tail;        // Tail index.
    uint8_t queue[BMC_QUEUE_SIZE]; // Transmission message type queue.
} tx = {0, 0, 0};

BMC_NextMessageHandler_t BMC_nextMessageHandler;

uint8_t BMC_i, BMC_j;

void BMC_setNextMessageHandler(BMC_NextMessageHandler_t nextMessageHandler) {
    BMC_nextMessageHandler = nextMessageHandler;
}

void BMC_transmit(uint8_t what) {
    uint8_t index = tx.index;
    while (index != tx.tail) {
        // Will be transmitted in the future, don't add again.
        if (tx.queue[index++] == what) return;
    }
    if ((tx.tail + 1) != tx.index) { // Do nothing if queue is full.
        tx.queue[tx.tail++] = what;
    }
}

void BMC_bm78EventHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        // Pairing Events
        case BM78_EVENT_PASSKEY_ENTRY_REQ:
        case BM78_EVENT_PAIRING_COMPLETE:
        case BM78_EVENT_PASSKEY_DISPLAY_YES_NO_REQ:
            break;
        // GAP Events
        case BM78_EVENT_LE_CONNECTION_COMPLETE:
        case BM78_EVENT_DISCONNECTION_COMPLETE:
        case BM78_EVENT_SPP_CONNECTION_COMPLETE:
            break;
        // Common Events
        case BM78_EVENT_COMMAND_COMPLETE:
            switch(response.CommandComplete_0x80.command) {
                case BM78_CMD_READ_LOCAL_INFORMATION:
                case BM78_CMD_READ_DEVICE_NAME:
                case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                case BM78_CMD_READ_PIN_CODE:
                default:
                    break;
            }
            break;
        case BM78_EVENT_BM77_STATUS_REPORT:
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
        case BM78_EVENT_CONFIGURE_MODE_STATUS:
            break;
        // SPP/GATT Transparent Event
        case BM78_EVENT_RECEIVED_TRANSPARENT_DATA:
        case BM78_EVENT_RECEIVED_SPP_DATA:
        // Unknown
        default:
            break;
    }
}

void BMC_bm78MessageSentHandler(void) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation()) {
        if (BMC_nextMessageHandler) {
            uint8_t what = tx.index != tx.tail ? tx.queue[tx.index] : BMC_NOTHING_TO_TRANSMIT;
            if (BMC_nextMessageHandler(what) && what != BMC_NOTHING_TO_TRANSMIT) tx.index++;
        }
    } else if (BM78.status != BM78_STATUS_SPP_CONNECTED_MODE) {
        // Reset transmit queue if connection lost.
        tx.index = 0;
        tx.tail = 0;
    }
}

void BMC_bm78TransparentDataHandler(uint8_t length, uint8_t *data) {
    bool repeated;
    //POC_bm78TransparentDataHandler(start, length, data);
    switch(*(data)) {
        case BM78_MESSAGE_KIND_SETTINGS:
            if (length == 24) {
                BM78.pairingMode = *(data + 1);
                for (BMC_i = 0; BMC_i < 6; BMC_i++) {
                    BM78.pin[BMC_i] = *(data + BMC_i + 2);
                }
                for (BMC_i = 0; BMC_i < 16; BMC_i++) {
                    BM78.deviceName[BMC_i] = *(data + BMC_i + 8);
                }
                BM78_setup(false, BM78.deviceName, BM78.pin, BM78.pairingMode);
            } else if (length == 1) {
                BM78_addDataByte(0, BM78_MESSAGE_KIND_SETTINGS);
                BM78_addDataByte(1, BM78.pairingMode);
                BM78_addDataBytes(2, 6, BM78.pin);
                BM78_addDataBytes(8, 16, BM78.deviceName);
                BM78_commitData(24);
            }
            break;
#ifdef RGB_ENABLED
        case BM78_MESSAGE_KIND_RGB:
            if (length == 10) { // set RGB
                // pattern, red, green, blue, delayH, delayL, min, max, count
                RGB_set(*(data + 1),                      // Pattern
                        *(data + 2),                      // Red
                        *(data + 3),                      // Green
                        *(data + 4),                      // Blue
                        (*(data + 5) << 8) | *(data + 6), // Delay (High | Low)
                        *(data + 7),                      // Min
                        *(data + 8),                      // Max
                        *(data + 9));                     // Count
            } else if (length == 1) { // get RGB
                BM78_addDataByte(0, BM78_MESSAGE_KIND_RGB);
                BM78_addDataByte(1, RGB.pattern);         // Pattern
                BM78_addDataByte(2, RGB.red);             // Red
                BM78_addDataByte(3, RGB.green);           // Green
                BM78_addDataByte(4, RGB.blue);            // Blue
                BM78_addDataByte2(5, RGB.delay * RGB_TIMER_PERIOD);
                BM78_addDataByte(7, RGB.min);             // Min
                BM78_addDataByte(8, RGB.max);             // Max
                BM78_addDataByte(9, RGB.count);           // Count
                BM78_commitData(10);
            }
            break;
#endif
#ifdef WS281x_BUFFER
        case BM78_MESSAGE_KIND_WS281x:
            if (length == 10) { // set WS281x
                // led, pattern, red, green, blue, delayH, delayL, min, max
                WS281x_set(*(data + 1),                   // LED
                        *(data + 2),                      // Pattern
                        *(data + 3),                      // Red
                        *(data + 4),                      // Green
                        *(data + 5),                      // Blue
                        (*(data + 6) << 8) | *(data + 7), // Delay
                        *(data + 8),                      // Min
                        *(data + 9));                     // Max
            } else if (length == 4) { // set all WS281x LEDs
                WS281x_all(*(data + 1), *(data + 2), *(data + 3));
            } else if (length == 2 && *(data + 1) < WS281x_LED_COUNT) { // get
                BM78_addDataByte(0, BM78_MESSAGE_KIND_WS281x);
                BM78_addDataByte(1, *(data + 1));                   // LED
                BM78_addDataByte(2, WS281x_ledPattern[*(data + 1)]);// Pattern
                BM78_addDataByte(3, WS281x_ledRed[*(data + 1)]);    // Red
                BM78_addDataByte(4, WS281x_ledGreen[*(data + 1)]);  // Green
                BM78_addDataByte(5, WS281x_ledBlue[*(data + 1)]);   // Blue
                BM78_addDataByte2(6, WS281x_ledDelay[*(data + 1)] * WS281x_TIMER_PERIOD);
                BM78_addDataByte(8, WS281x_ledMin[*(data + 1)]);    // Min
                BM78_addDataByte(9, WS281x_ledMax[*(data + 1)]);    // Max
                BM78_commitData(10);
            } else if (length == 1) { // get all
                // TODO
            }
            break;
#endif
#ifdef SM_MEM_ADDRESS
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

                for(BMC_i = 5; BMC_i < length; BMC_i++) {
                    regHigh = (startReg + BMC_i - 5) >> 8;
                    regLow = (startReg + BMC_i - 5) & 0xFF;

                    // Make sure 1st 2 bytes are 0xFF -> disable state machine
                    if (regHigh == 0x00 && regLow == 0x00) {
                        byte = SM_STATUS_DISABLED;
                    } else {
                        byte = *(data + BMC_i);
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

                printProgress("     Uploading      ", startReg + length - 5, size);

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
                for (BMC_i = 0; BMC_i < count; BMC_i++) {
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
                for (BMC_i = 0; BMC_i < count; BMC_i++) {
                    uint8_t device = *(data + index);
                    uint8_t size = *(data + index + 1);
                    uint8_t value[SM_VALUE_MAX_SIZE];
                    for (BMC_j = 0; BMC_j < size; BMC_j++) if (BMC_j < SM_VALUE_MAX_SIZE) {
                        value[BMC_j] = *(data + index + BMC_j + 2);
                    }
                    SM_executeAction(device, size, value);
                    index += size + 2;
                }
            }
            break;
#endif
#ifdef LCD_ADDRESS
        case BM78_MESSAGE_KIND_PLAIN:
            LCD_clear();
            LCD_setString("                    ", 2, false);
            for(BMC_i = 0; BMC_i < length; BMC_i++) {
                if (BMC_i < 20) {
                    LCD_replaceChar(*(data + BMC_i), BMC_i, 2, false);
                }
            }
            LCD_setString("    BT Message:     ", 0, true);
            LCD_displayLine(2);
            break;
#endif
    }
}

void BMC_bm78ErrorHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        case BM78_EVENT_COMMAND_COMPLETE:
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