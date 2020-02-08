/* 
 * File:   serial_communication.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "serial_communication.h"

#if defined USB_ENABLED || defined BM78_ENABLED

#ifdef BM78_ENABLED
#include "../modules/bm78.h"
#endif
#ifdef DHT11_PORT
#include "../modules/dht11.h"
#endif
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif
#ifdef MCP23017_ENABLED
#include "../modules/mcp23017.h"
#endif
#ifdef RGB_ENABLED
#include "../modules/rgb.h"
#endif
#ifdef SM_MEM_ADDRESS
#include "../modules/state_machine.h"
#endif
#if defined MCP2200_ENABLED || defined MCP2221_ENABLED
#include "../modules/mcp22xx.h"
#endif
#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
#include "../modules/ws281x.h"
#endif
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
#include "../modules/ws281x_light.h"
#endif
#endif

typedef struct {
    uint8_t length;         // Transmission length: 0 = nothing to transmit
    uint8_t chksumExpected; // Expected checksum calculated before sending.
    uint8_t chksumReceived; // Received checksum from the peer.
    uint16_t timeout;       // Timeout countdown used by retry trigger.
    uint8_t retries;        // Number of transparent transmissions retries
    uint8_t data[SCOM_MAX_PACKET_SIZE]; // Checksum + Data
} SCOM_TX_t;

SCOM_TX_t SCOM_tx[SCOM_CHANNEL_COUNT]; // = {0, 0x00, 0xFF, 0, 0xFF};

/** 
 * Queue of message types to be send out over BT. Should be cleared when 
 * connection ends for whatever reason. 
 */
typedef struct {
    uint8_t index;                  // Sent index.
    uint8_t tail;                   // Tail index.
    uint8_t queue[SCOM_QUEUE_SIZE]; // Transmission message type queue.
    uint8_t param1[SCOM_QUEUE_SIZE]; // Possible parameter to the queue items.
    uint8_t param2[SCOM_QUEUE_SIZE]; // Possible parameter to the queue items.
} SCOM_Queue_t;

SCOM_Queue_t SCOM_queue[SCOM_CHANNEL_COUNT]; // = {0, 0};

SCOM_DataHandler_t additionalDataHandler = NULL;

SCOM_NextMessageHandler_t nextMessageHandler = NULL;

void SCOM_cancelTransmission(SCOM_Channel_t channel) {
    SCOM_tx[channel].length = 0;
    SCOM_resetChecksum(channel);
}

void SCOM_resetChecksum(SCOM_Channel_t channel) {
    SCOM_tx[channel].chksumExpected = 0x00;
    SCOM_tx[channel].chksumReceived = 0xFF;
}

bool SCOM_awatingConfirmation(SCOM_Channel_t channel) {
    return SCOM_tx[channel].length > 0;
}   

bool SCOM_isChecksumCorrect(SCOM_Channel_t channel) {
    return SCOM_tx[channel].chksumExpected == SCOM_tx[channel].chksumReceived;
}

inline bool SCOM_canEnqueue(SCOM_Channel_t channel) {
    switch (channel) {
#ifdef USB_ENABLED
        case SCOM_CHANNEL_USB:
            return true;
#endif
#ifdef BM78_ENABLED
        case SCOM_CHANNEL_BT:
            return BM78.status == BM78_STATUS_SPP_CONNECTED_MODE;
#endif
        default:
            return false;
    }
}

inline bool SCOM_canSend(SCOM_Channel_t channel) {
    switch (channel) {
#ifdef USB_ENABLED
        case SCOM_CHANNEL_USB:
            return !SCOM_awatingConfirmation(channel);
#endif
#ifdef BM78_ENABLED
        case SCOM_CHANNEL_BT:
            return BM78.status == BM78_STATUS_SPP_CONNECTED_MODE
                    && !SCOM_awatingConfirmation(channel);
#endif
        default:
            return false;
    }
}

void SCOM_retryTrigger(void) {
    for (uint8_t channel = 0; channel < SCOM_CHANNEL_COUNT; channel++) {
        if (!SCOM_canEnqueue(channel)) {
            SCOM_cancelTransmission(channel);
        }

        if (SCOM_tx[channel].length > 0 && SCOM_isChecksumCorrect(channel)) {
            SCOM_cancelTransmission(channel);
            SCOM_messageSentHandler(channel);
        } else if (SCOM_tx[channel].length > 0
                && !SCOM_isChecksumCorrect(channel)
                && SCOM_tx[channel].timeout == 0) {

            if (SCOM_tx[channel].retries == 0) {
                SCOM_tx[channel].retries = SCOM_NO_RETRY_LIMIT;
                SCOM_cancelTransmission(channel); // Cancel transmission
                SCOM_messageSentHandler(channel);
                return;
            } else if (SCOM_tx[channel].retries < SCOM_NO_RETRY_LIMIT) {
                SCOM_tx[channel].retries--;
            }

            SCOM_tx[channel].timeout = (uint16_t) BM78_RESEND_TIMEOUT / (uint16_t) BM78_TRIGGER_PERIOD;

            // (Re)calculate Transparent Checksum
            SCOM_tx[channel].chksumExpected = 0x00;
            for (uint8_t i = 1; i < SCOM_tx[channel].length; i++) {
                SCOM_tx[channel].chksumExpected = SCOM_tx[channel].chksumExpected + SCOM_tx[channel].data[i];
            }
            SCOM_tx[channel].data[0] = SCOM_tx[channel].chksumExpected;
            SCOM_tx[channel].chksumReceived = SCOM_tx[channel].chksumExpected == 0x00 ? 0xFF : 0x00;

            switch(channel) {
#ifdef USB_ENABLED
                case SCOM_CHANNEL_USB:
                    MCP22xx_send(SCOM_tx[channel].length, SCOM_tx[channel].data);
                    break;
#endif
#ifdef BM78_ENABLED
                case SCOM_CHANNEL_BT:
                    BM78_sendTransparentData(SCOM_tx[channel].length, SCOM_tx[channel].data);
                    break;
#endif  
            }
        } else if (SCOM_tx[channel].length == 0 && SCOM_tx[channel].timeout == 0) {
            // Periodically try message sent handler if someone has something to send
            SCOM_tx[channel].timeout = (uint16_t) BM78_RESEND_TIMEOUT / (uint16_t) BM78_TRIGGER_PERIOD;
            SCOM_messageSentHandler(channel);
        } else if (SCOM_tx[channel].timeout > 0) {
            SCOM_tx[channel].timeout--;
        }

        // Just to make sure, weird behavior observed here
        if (SCOM_tx[channel].timeout > BM78_RESEND_TIMEOUT / BM78_TRIGGER_PERIOD) {
            SCOM_tx[channel].timeout = (uint16_t) BM78_RESEND_TIMEOUT / (uint16_t) BM78_TRIGGER_PERIOD;
        }
    }
}

inline void SCOM_addDataByte(SCOM_Channel_t channel, uint8_t position, uint8_t byte) {
    if (SCOM_tx[channel].length == 0) SCOM_tx[channel].data[position + 1] = byte;
}

inline void SCOM_addDataByte2(SCOM_Channel_t channel, uint8_t position, uint16_t word) {
    if (SCOM_tx[channel].length == 0) {
        SCOM_tx[channel].data[position + 1] = word >> 8;
        SCOM_tx[channel].data[position + 2] = word & 0xFF;
    }
}

inline void SCOM_addDataBytes(SCOM_Channel_t channel, uint8_t position, uint8_t length, uint8_t *data) {
    if (SCOM_tx[channel].length == 0)  for (uint8_t i = 0; i < length; i++) {
        SCOM_tx[channel].data[position + i + 1] = *(data + i);
    }
}

bool SCOM_commitData(SCOM_Channel_t channel, uint8_t length, uint8_t maxRetries) {
    if (SCOM_tx[channel].length == 0) {
        SCOM_tx[channel].length = length + 1;
        SCOM_tx[channel].timeout = 0; // Send right-away
        SCOM_tx[channel].retries = maxRetries;
        SCOM_resetChecksum(channel);
        //SCOM_retryTrigger(); this will be triggered by a timer.
        return true;
    }
    return false;
}

void SCOM_transmitData(SCOM_Channel_t channel, uint8_t length, uint8_t *data, uint8_t maxRetries) {
    if (SCOM_tx[channel].length == 0 && length < SCOM_MAX_PACKET_SIZE - 1) {
        for (uint8_t i = 0; i < length; i++) {
            SCOM_addDataByte(channel, i, *(data + i));
        }
        SCOM_commitData(channel, length, maxRetries);
    }
}

#ifdef BM78_ENABLED

inline void SCOM_sendData(SCOM_Channel_t channel, uint8_t num, uint16_t start,
        uint16_t length) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_dataTransfer.stage = 0x01; // Start transfer (e.g. open EEPROM)
        SCOM_dataTransfer.channel = channel;
        SCOM_dataTransfer.start = start;
        SCOM_dataTransfer.end = start + length;
        if (SCOM_dataTransfer.start < SCOM_dataTransfer.end) {
            SCOM_enqueue(channel, MESSAGE_KIND_DATA, num, 0x00);
        }
    }
}
#endif

void SCOM_setAdditionalDataHandler(SCOM_DataHandler_t handler) {
    additionalDataHandler = handler;
}

void SCOM_setNextMessageHandler(SCOM_NextMessageHandler_t handler) {
    nextMessageHandler = handler;
}

bool SCOM_isQueueEmpty(SCOM_Channel_t channel) {
    return SCOM_queue[channel].index == SCOM_queue[channel].tail;
}

void SCOM_enqueue(SCOM_Channel_t channel, MessageKind_t what, uint8_t param1,
        uint8_t param2) {
    if (!SCOM_canEnqueue(channel)) return;

    uint8_t index = SCOM_queue[channel].index;
    while (index != SCOM_queue[channel].tail) {
        // Will be transmitted in the future, don't add again.
        if (SCOM_queue[channel].queue[index] == what
                && SCOM_queue[channel].param1[index] == param1
                && SCOM_queue[channel].param2[index] == param2) return;
        index = (index + 1) % SCOM_QUEUE_SIZE;
    }
    if ((SCOM_queue[channel].tail + 1) % SCOM_QUEUE_SIZE != SCOM_queue[channel].index) { // Do nothing if queue is full.
        SCOM_queue[channel].queue[SCOM_queue[channel].tail] = what;
        SCOM_queue[channel].param1[SCOM_queue[channel].tail] = param1;
        SCOM_queue[channel].param2[SCOM_queue[channel].tail] = param2;
        SCOM_queue[channel].tail = (SCOM_queue[channel].tail + 1) % SCOM_QUEUE_SIZE;
    }
    SCOM_messageSentHandler(channel);
}

void SCOM_messageSentHandler(SCOM_Channel_t channel) {
    uint8_t param1, param2;
#ifdef DHT11_PORT
    DHT11_Result result;
#endif
    if (SCOM_canSend(channel)) {
        uint8_t messageKind = SCOM_queue[channel].index != SCOM_queue[channel].tail
                ? SCOM_queue[channel].queue[SCOM_queue[channel].index]
                : MESSAGE_KIND_NONE;
        switch (messageKind) {
            case MESSAGE_KIND_IDD:
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index];
                switch(param1) {
                    case 0x00: // IDD - Capabilities
                        SCOM_addDataByte(channel, 0, MESSAGE_KIND_IDD);  // Kind
                        SCOM_addDataByte(channel, 1, 0x00); // Reserved
                        SCOM_addDataByte(channel, 2, param1); // IDD Message Type
                        param1 = 0x00;
#ifdef BM78_ENABLED
                        param1 |= 0b00000001;
#endif
#if defined MCP2200_ENABLED || defined MCP2221_ENABLED
                        param1 |= 0b00000010;
#endif
#ifdef DHT11_PORT
                        param1 |= 0b00000100;
#endif
#ifdef LCD_ADDRESS
                        param1 |= 0b00001000;
#endif
#ifdef MCP23017_ENABLED
                        param1 |= 0b00010000;
#endif
#ifdef PIR_PORT
                        param1 |= 0b00100000;
#endif
                        SCOM_addDataByte(channel, 3, param1);

                        param1 = 0x00;
#ifdef RGB_ENABLED
                        param1 |= 0b00000001;
#endif
#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
                        param1 |= 0b00000010;
#endif
#ifdef WS281x_LIGHT_ROWS
                        param1 |= 0b00000100;
#endif
#endif
                        SCOM_addDataByte(channel, 4, param1);

                        if (SCOM_commitData(channel, 5, BM78_MAX_SEND_RETRIES)) {
                            SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        }
                        break;
                    case 0x01: // IDD - Name
                        SCOM_addDataByte(channel, 0, MESSAGE_KIND_IDD);  // Kind
                        SCOM_addDataByte(channel, 1, 0x00); // Reserved
                        SCOM_addDataByte(channel, 2, param1); // IDD Message Type

                        param1 = 0;
#ifdef BM78_ENABLED
                        while (param1 < 16 && BM78.deviceName[param1] != 0x00) {
                            SCOM_addDataByte(channel, param1 + 3, BM78.deviceName[param1]);
                            param1++;
                        }
#endif
                        if (SCOM_commitData(channel, param1 + 3, BM78_MAX_SEND_RETRIES)) {
                            SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        }
                        break;
                }
                break;
            case MESSAGE_KIND_CONSISTENCY_CHECK:
                if (nextMessageHandler) { // An external handler exist
                    // if it consumed the message successfully
                    if (nextMessageHandler(
                            channel,
                            SCOM_queue[channel].queue[SCOM_queue[channel].index],
                            SCOM_queue[channel].param1[SCOM_queue[channel].index],
                            SCOM_queue[channel].param2[SCOM_queue[channel].index])) {
                         SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else { // Don't know what to do with the message - drop it
                     SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                }
                break;
            case MESSAGE_KIND_DATA:
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index];
                switch (param1) {
#ifdef BM78_ENABLED
                    case BM78_DATA_PART:
                        switch (SCOM_dataTransfer.stage) {
                            case 0x01: // Open EEPROM
                                BM78_resetTo(BM78_MODE_TEST);
                                BM78_openEEPROM();
                                break;
                            case 0x02: // Read EEPROM
                                BM78_readEEPROM(SCOM_dataTransfer.start, 
                                        SCOM_dataTransfer.start + SCOM_MAX_PACKET_SIZE - 6 < SCOM_dataTransfer.start
                                        || SCOM_dataTransfer.start + SCOM_MAX_PACKET_SIZE - 6 >= SCOM_dataTransfer.end
                                        ? SCOM_dataTransfer.end - SCOM_dataTransfer.start + 1
                                        : SCOM_MAX_PACKET_SIZE - 6);
                                break;
                            case 0x03: // Notify finish
                                SCOM_addDataByte(SCOM_dataTransfer.channel, 0, MESSAGE_KIND_DATA);
                                SCOM_addDataByte(SCOM_dataTransfer.channel, 1, 0xFF);
                                if (SCOM_commitData(channel, 2, SCOM_NO_RETRY_LIMIT)) {
                                    SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                                }
                                break;
                        }
                        break;
#endif
                    default:
                        if (nextMessageHandler) { // An external handler exist
                            // if it consumed the message successfully
                            if (nextMessageHandler(
                                    channel,
                                    SCOM_queue[channel].queue[SCOM_queue[channel].index],
                                    SCOM_queue[channel].param1[SCOM_queue[channel].index],
                                    SCOM_queue[channel].param2[SCOM_queue[channel].index])) {
                                 SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                            }
                        } else { // Don't know what to do with the message - drop it
                             SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        }
                        break;
                }
                break;
            case MESSAGE_KIND_IO:
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index];
                switch (param1) {
#ifdef PIR_PORT
                    case IO_PIR:
                        SCOM_addDataByte(channel, 0, MESSAGE_KIND_PIR);
                        SCOM_addDataByte(channel, 1, IO_PIR);
                        SCOM_addDataByte(channel, 2, PIR_PORT);
                        if (SCOM_commitData(channel, 3, BM78_MAX_SEND_RETRIES)) {
                            SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        }
                        break;
#endif
                    default:
                        SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        break;
                }
                break;
#ifdef DHT11_PORT
            case MESSAGE_KIND_TEMP: 
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index];
                switch (param1) {
                    case 0x00: // Request sensor 0 measurements
                        result = DHT11_measure();
                        if (result.status == DHT11_OK) {
                            SCOM_addDataByte(channel, 0, MESSAGE_KIND_TEMP);
                            SCOM_addDataByte(channel, 1, param1); // Number
                            SCOM_addDataByte2(channel, 2, result.temp);
                            SCOM_addDataByte2(channel, 4, result.rh);
                            if (SCOM_commitData(channel, 6, BM78_MAX_SEND_RETRIES)) {
                                SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                            }
                        }
                        break;
                    case 0xFF: // Request sensor count
                        SCOM_addDataByte(channel, 0, MESSAGE_KIND_TEMP);
                        SCOM_addDataByte(channel, 1, 0xFF); // Differentiating byte
                        SCOM_addDataByte(channel, 2, 1); // Sensor count
                        if (SCOM_commitData(channel, 3, BM78_MAX_SEND_RETRIES)) {
                            SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        }
                        break;
                    default: // Ignore, only one sensor implemented
                        SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        break;
                }
                break;
#endif
#ifdef LCD_ADDRESS
            case MESSAGE_KIND_LCD:
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index]; // NUM
                param2 = SCOM_queue[channel].param2[SCOM_queue[channel].index]; // Line/command
                if (param1 == 0x00) switch (param2) { // Send command
                    case SCOM_PARAM_LCD_CLEAR:
                    case SCOM_PARAM_LCD_RESET:
                    case SCOM_PARAM_LCD_NO_BACKLIGH:
                    case SCOM_PARAM_LCD_BACKLIGH:
                        SCOM_addDataByte(channel, 0, MESSAGE_KIND_LCD); // Kind
                        SCOM_addDataByte(channel, 1, param1);           // NUM: Only one implemented
                        SCOM_addDataByte(channel, 2, 0xFF);             // Differentiating byte
                        SCOM_addDataByte(channel, 3, param2);           // Command
                        if (SCOM_commitData(channel, 4, BM78_MAX_SEND_RETRIES)) {
                            SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        }
                        break;
                    case 0xFF: // LCD count response
                        SCOM_addDataByte(channel, 0, MESSAGE_KIND_LCD); // Kind
                        SCOM_addDataByte(channel, 1, 1);                // Count
                        if (SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES)) {
                            SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        }
                        break;
                    default: // LCD line response
                        if ((param2 & SCOM_PARAM_MASK) < LCD_ROWS) { 
                            SCOM_addDataByte(channel, 0, MESSAGE_KIND_LCD);        // Kind
                            SCOM_addDataByte(channel, 1, 0);                       // NUM: Only one implemented
                            SCOM_addDataByte(channel, 2, LCD_backlight);           // Command
                            SCOM_addDataByte(channel, 3, param2 & SCOM_PARAM_MASK); // Line
                            SCOM_addDataByte(channel, 4, LCD_COLS);                // Characters

                            for (uint8_t line = 0; line < LCD_COLS; line++) {
                                SCOM_addDataByte(channel, line + 5,
                                        LCD_getCache(param2 & SCOM_PARAM_MASK, line));
                            }

                            if (SCOM_commitData(channel, LCD_COLS + 5, BM78_MAX_SEND_RETRIES)) {
                                if (((param2 & SCOM_PARAM_MASK) + 1) < LCD_ROWS
                                        && (param2 & SCOM_PARAM_ALL)) {
                                    SCOM_queue[channel].param2[SCOM_queue[channel].index]++;
                                } else  SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                            }
                        } else { // Line overflow - consuming
                            SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                        }
                        break;
                } else { // Ignore - Only one LCD implemented
                    SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                }
                break;
#endif
#ifdef MCP23017_ENABLED
            case MESSAGE_KIND_REGISTRY:
                // Address
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index];
                // Registry
                param2 = SCOM_queue[channel].param2[SCOM_queue[channel].index];
                if (param1 >= MCP23017_START_ADDRESS
                        && param1 <= MCP23017_END_ADDRESS
                        && (param2 == MCP23017_GPIOA
                        || param2 == MCP23017_GPIOB
                        || param2 == 0xFF)) {
                    
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_REGISTRY); // Kind
                    SCOM_addDataByte(channel, 1, param1); // Address
                    SCOM_addDataByte(channel, 2,
                            param2 == 0xFF ? MCP23017_GPIOA : param2);
                    if (param2 == 0xFF) { // Count (only if multiple)
                        SCOM_addDataByte(channel, 3, param2 == 0xFF ? 2 : 1);
                    }
                    SCOM_addDataByte(channel, param2 == 0xFF ? 4 : 3,
                            MCP23017_read(param1, param2 == 0xFF ? MCP23017_GPIOA : param2));
                    if (param2 == 0xFF) {
                        SCOM_addDataByte(channel, 5, MCP23017_read(param1, MCP23017_GPIOB));
                    }
                    if (SCOM_commitData(channel, param2 == 0xFF ? 6 : 4, BM78_MAX_SEND_RETRIES)) {
                        SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else  SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                break;
#endif
#ifdef RGB_ENABLED
            case MESSAGE_KIND_RGB:
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index]
                param2 = SCOM_queue[channel].param2[SCOM_queue[channel].index]
                        & SCOM_PARAM_MASK; // index (max 128)
                if (param1 == 0xFF && param2 == 0xFF) { // Send count
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_RGB);
                    SCOM_addDataByte(channel, 1, 1); // Only one implemented
                    if (SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES)) {
                        SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else if (param1 != 0x00) { // Ignore - only one strip
                    SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                } else if (param2 < RGB_list.size && param2  < RGB_LIST_SIZE) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_RGB);
                    SCOM_addDataByte(channel, 1, param1);
                    SCOM_addDataByte(channel, 2, RGB_list.size);
                    SCOM_addDataByte(channel, 3, param2 & SCOM_PARAM_MASK);
                    SCOM_addDataByte(channel, 4, RGB_items[param2].pattern);
                    SCOM_addDataByte(channel, 5, RGB_items[param2].red);
                    SCOM_addDataByte(channel, 6, RGB_items[param2].green);
                    SCOM_addDataByte(channel, 7, RGB_items[param2].blue);
                    SCOM_addDataByte2(channel, 8, RGB_items[param2].delay * RGB_TIMER_PERIOD);
                    SCOM_addDataByte(channel, 10, RGB_items[param2].min);    // Min
                    SCOM_addDataByte(channel, 11, RGB_items[param2].max);    // Max
                    SCOM_addDataByte(channel, 12, RGB_items[param2].timeout);// Timeout
                    if (SCOM_commitData(channel, 13, BM78_MAX_SEND_RETRIES)) {
                        if ((param2 + 1) < RGB_list.size && (param2 + 1) < RGB_LIST_SIZE
                                && (SCOM_queue[channel].param2[SCOM_queue[channel].index] & SCOM_PARAM_ALL)) {
                            SCOM_queue[channel].param2[SCOM_queue[channel].index]++;
                        } else  SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                break;
#endif
#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
            case MESSAGE_KIND_INDICATORS:
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index];
                param2 = SCOM_queue[channel].param2[SCOM_queue[channel].index]
                        & SCOM_PARAM_MASK; // LED (max 128)
                if (param1 == 0xFF && param2 == 0xFF) { // Strip count
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_INDICATORS);
                    SCOM_addDataByte(channel, 1, 1); // Only one implemented
                    if (SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES)) {
                        SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else if (param1 == 0x00 && param2 < WS281x_LED_COUNT) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_INDICATORS);
                    SCOM_addDataByte(channel, 1, param1); // NUM
                    SCOM_addDataByte(channel, 2, WS281x_LED_COUNT);
                    SCOM_addDataByte(channel, 3, param2); // LED
                    SCOM_addDataByte(channel, 4, WS281x_ledPattern[param2]);
                    SCOM_addDataByte(channel, 5, WS281x_ledRed[param2]);
                    SCOM_addDataByte(channel, 6, WS281x_ledGreen[param2]);
                    SCOM_addDataByte(channel, 7, WS281x_ledBlue[param2]);
                    SCOM_addDataByte2(channel, 8, WS281x_ledDelay[param2] * WS281x_TIMER_PERIOD);
                    SCOM_addDataByte(channel, 10, WS281x_ledMin[param2]);
                    SCOM_addDataByte(channel, 11, WS281x_ledMax[param2]);
                    if (SCOM_commitData(channel, 12, BM78_MAX_SEND_RETRIES)) {
                        if ((param2 + 1) < WS281x_LED_COUNT
                                && (SCOM_queue[channel].param2[SCOM_queue[channel].index] & SCOM_PARAM_ALL)) {
                            SCOM_queue[channel].param2[SCOM_queue[channel].index]++;
                        } else  SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                break;
#endif
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
            case MESSAGE_KIND_LIGHT:
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index];
                param2 = SCOM_queue[channel].param2[SCOM_queue[channel].index]
                        & SCOM_PARAM_MASK; // index (max 128)
                
                if (param1 == 0xFF && param2 == 0xFF) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_LIGHT);
                    SCOM_addDataByte(channel, 1, 1); // Only one implemented
                    if (SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES)) {
                        SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else if (param1 == 0x00 // NUM: Only one light implemented
                        && param2 < WS281xLight_list.size
                        && param2 < WS281x_LIGHT_LIST_SIZE) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_LIGHT);
                    SCOM_addDataByte(channel, 1, param1);
                    SCOM_addDataByte(channel, 2, WS281xLight_list.size);
                    SCOM_addDataByte(channel, 3, param2 & SCOM_PARAM_MASK);
                    SCOM_addDataByte(channel, 4, WS281xLight_items[param2].pattern);
                    SCOM_addDataByte(channel, 5, WS281xLight_items[param2].color[0].r);
                    SCOM_addDataByte(channel, 6, WS281xLight_items[param2].color[0].g);
                    SCOM_addDataByte(channel, 7, WS281xLight_items[param2].color[0].b);
                    SCOM_addDataByte(channel, 8, WS281xLight_items[param2].color[1].r);
                    SCOM_addDataByte(channel, 9, WS281xLight_items[param2].color[1].g);
                    SCOM_addDataByte(channel, 10, WS281xLight_items[param2].color[1].b);
                    SCOM_addDataByte(channel, 11, WS281xLight_items[param2].color[2].r);
                    SCOM_addDataByte(channel, 12, WS281xLight_items[param2].color[2].g);
                    SCOM_addDataByte(channel, 13, WS281xLight_items[param2].color[2].b);
                    SCOM_addDataByte(channel, 14, WS281xLight_items[param2].color[3].r);
                    SCOM_addDataByte(channel, 15, WS281xLight_items[param2].color[3].g);
                    SCOM_addDataByte(channel, 16, WS281xLight_items[param2].color[3].b);
                    SCOM_addDataByte(channel, 17, WS281xLight_items[param2].color[4].r);
                    SCOM_addDataByte(channel, 18, WS281xLight_items[param2].color[4].g);
                    SCOM_addDataByte(channel, 19, WS281xLight_items[param2].color[4].b);
                    SCOM_addDataByte(channel, 20, WS281xLight_items[param2].color[5].r);
                    SCOM_addDataByte(channel, 21, WS281xLight_items[param2].color[5].g);
                    SCOM_addDataByte(channel, 22, WS281xLight_items[param2].color[5].b);
                    SCOM_addDataByte(channel, 23, WS281xLight_items[param2].color[6].r);
                    SCOM_addDataByte(channel, 24, WS281xLight_items[param2].color[6].g);
                    SCOM_addDataByte(channel, 25, WS281xLight_items[param2].color[6].b);
                    SCOM_addDataByte2(channel, 26, WS281xLight_items[param2].delay * WS281x_TIMER_PERIOD);
                    SCOM_addDataByte(channel, 28, WS281xLight_items[param2].width);
                    SCOM_addDataByte(channel, 29, WS281xLight_items[param2].fading);
                    SCOM_addDataByte(channel, 30, WS281xLight_items[param2].min);
                    SCOM_addDataByte(channel, 31, WS281xLight_items[param2].max);
                    SCOM_addDataByte(channel, 32, WS281xLight_items[param2].timeout);
                    if (SCOM_commitData(channel, 33, BM78_MAX_SEND_RETRIES)) {
                        if ((param2 + 1) < WS281xLight_list.size && (param2 + 1) < WS281x_LIGHT_LIST_SIZE
                                && (SCOM_queue[channel].param2[SCOM_queue[channel].index] & SCOM_PARAM_ALL)) {
                            SCOM_queue[channel].param2[SCOM_queue[channel].index]++;
                        } else  SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                break;
#endif
#endif
#ifdef BM78_ENABLED
            case MESSAGE_KIND_BLUETOOTH:
                SCOM_addDataByte(channel, 0, MESSAGE_KIND_BLUETOOTH);
                SCOM_addDataByte(channel, 1, BM78.pairingMode);
                SCOM_addDataBytes(channel, 2, 6, (uint8_t *) BM78.pin);
                SCOM_addDataBytes(channel, 8, 16, (uint8_t *) BM78.deviceName);
                if (SCOM_commitData(channel, 24, BM78_MAX_SEND_RETRIES)) {
                    SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                }
                break;
#endif
            case MESSAGE_KIND_DEBUG:
                param1 = SCOM_queue[channel].param1[SCOM_queue[channel].index];
                SCOM_addDataByte(channel, 0, MESSAGE_KIND_DEBUG);
                SCOM_addDataByte(channel, 1, param1);
                if (SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES)) {
                     SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                }
                break;
            case MESSAGE_KIND_NONE:
                // Nothing to transmit
                break;
            default:
                if (nextMessageHandler) { // An external handler exist
                    // if it consumed the message successfully
                    if (nextMessageHandler(
                            channel,
                            SCOM_queue[channel].queue[SCOM_queue[channel].index],
                            SCOM_queue[channel].param1[SCOM_queue[channel].index],
                            SCOM_queue[channel].param2[SCOM_queue[channel].index])) {
                         SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                    }
                } else { // Don't know what to do with the message - drop it
                     SCOM_queue[channel].index = (SCOM_queue[channel].index + 1) % SCOM_QUEUE_SIZE;
                }
                break;
        }
        
    } else if (!SCOM_canEnqueue(channel)) {
        // Reset transmit queue if connection lost.
        SCOM_queue[channel].index = 0;
        SCOM_queue[channel].tail = 0;
    }
}

void SCOM_dataHandler(SCOM_Channel_t channel, uint8_t length, uint8_t *data) {
    uint8_t chksum = 0; // Calculated checksum
    uint8_t buffer[4];
    for (uint8_t i = 1; i < length; i++) {
        chksum = chksum + *(data + i);
    }
    
    if (chksum == *(data) && *(data + 1) != MESSAGE_KIND_CRC) switch (channel) {
#ifdef USB_ENABLED
        case SCOM_CHANNEL_USB:
            buffer[0] = chksum;           // Checksum of the packet
            buffer[1] = MESSAGE_KIND_CRC; // Message kind
            buffer[2] = chksum;           // Payload
            MCP22xx_send(3, buffer);
            break;
#endif
#ifdef BM78_ENABLED
        case SCOM_CHANNEL_BT:
            buffer[0] = 0x00;             // Reserved
            buffer[1] = chksum;           // Checksum of the packet
            buffer[2] = MESSAGE_KIND_CRC; // Message kind
            buffer[3] = chksum;           // Payload
            BM78_data(BM78_CMD_SEND_TRANSPARENT_DATA, 4, buffer);
            break;
#endif
    }
    
    if (chksum == *(data)) switch(*(data + 1)) {
        case MESSAGE_KIND_CRC:
            if (length == 3) SCOM_tx[channel].chksumReceived = *(data + 2);
            break;
        case MESSAGE_KIND_IDD:
            // 2nd byte is a random number for checksum check
            if (length == 4) {
                SCOM_enqueue(channel, MESSAGE_KIND_IDD, *(data + 3), 0x00);
            }
            break;
        case MESSAGE_KIND_CONSISTENCY_CHECK:
            if (length == 3) switch (*(data + 2)) {
                default:
                    if (additionalDataHandler) {
                        additionalDataHandler(channel, length, data);
                    }
                    break;
            }
            break;
        case MESSAGE_KIND_DATA:
            if (length >= 3) switch (*(data + 2)) {
#ifdef BM78_ENABLED        
                case BM78_DATA_PART:
                    if (length == 3) {
                        SCOM_sendData(channel, MESSAGE_KIND_DATA, 0x0000, 0x1FFF);
                    } else if (length == 6) {
                        SCOM_sendData(channel, MESSAGE_KIND_DATA,
                                *(data + 3) << 8 | *(data + 4),
                                *(data + 5) << 8 | *(data + 6));
                    } else if (length > 6) {
                        if (((*(data + 4) << 8 | *(data + 5))) // length
                                - ((*(data + 2) << 8) | *(data + 3)) // - start
                                == length - 6) { // == length - 6
                            // TODO write EEPROM
                        }
                    }
                    break;
#endif
                default:
                    if (additionalDataHandler) {
                        additionalDataHandler(channel, length, data);
                    }
                    break;
            }
            break;
        case MESSAGE_KIND_IO:
            if (length == 3 || length == 4) switch (*(data + 2)) {
#ifdef PIR_PORT
                case IO_PIR:
                    if (length == 3) {
                        SCOM_enqueue(channel, MESSAGE_KIND_IO, IO_PIR, 0x00);
                    }
                    break;
#endif
                default:
                    break;
            }
            break;
#ifdef DHT11_PORT
        case MESSAGE_KIND_TEMP:
            // Request sensor count
            if (length == 2) {
                SCOM_enqueue(channel, MESSAGE_KIND_TEMP, 0xFF, 0x00);
            } else if (lenght == 3) {
                SCOM_enqueue(channel, MESSAGE_KIND_TEMP, *(data + 2), 0x00)
            }
            break;
#endif
#ifdef LCD_ADDRESS
        case MESSAGE_KIND_LCD:
            if (length == 2) { // Request LCD count
                SCOM_enqueue(channel, MESSAGE_KIND_LCD, 0xFF, 0x00);
            } else if (length == 4 // Request a line
                    && *(data + 2) == 0x00) { // NUM: Only one LCD implemented
                SCOM_enqueue(channel, MESSAGE_KIND_LCD, *(data + 2), *(data + 3));
            } else if (length == 5 // Command received
                    && *(data + 2) == 0x00 // NUM: Only one LCD implemented
                    && *(data + 3) == 0xFF) { // Differentiating byte
                switch (*(data + 4)) {
                    case SCOM_PARAM_LCD_CLEAR:
                        LCD_clearCache();
                        LCD_displayCache();
                        break;
                    case SCOM_PARAM_LCD_RESET:
                        LCD_reset();
                        break;
                    case SCOM_PARAM_LCD_BACKLIGH:
                    case SCOM_PARAM_LCD_NO_BACKLIGH:
                        LCD_setBacklight(*(data + 4) == SCOM_PARAM_LCD_BACKLIGH);
                        break;
                    default:
                        break;
                }
            } else if (length > 5 // Set line
                    && *(data + 2) == 0x00) { // NUM: Only one LCD implemented
                LCD_setBacklight(*(data + 3) != SCOM_PARAM_LCD_NO_BACKLIGH);
                LCD_setString("                    ", *(data + 4), true);
                for (uint8_t i = 0; i < *(data + 5); i++) {
                    if (i < length && i < LCD_COLS) {
                        LCD_replaceChar(*(data + i + 6), i, *(data + 4), false);
                    }
                }
                LCD_displayLine(*(data + 4));
            }
            break;
#endif
#ifdef MCP23017_ENABLED
        case MESSAGE_KIND_REGISTRY:
            if (length == 4) { // Request all registries
                SCOM_enqueue(channel, MESSAGE_KIND_REGISTRY,
                        *(data + 2), 0xFF);
            } else if (length == 4) { // Request registry
                SCOM_enqueue(channel, MESSAGE_KIND_REGISTRY,
                        *(data + 2), *(data + 3));
            } else if (length >= 5) { // Write registry(-ies)
                uint8_t address = *(data + 2);
                uint8_t registry = *(data + 3);
                uint8_t count = length == 5 ? 1 : *(data + 4);
                uint8_t index = length == 5 ? 4 : 5;
                if (address >= MCP23017_START_ADDRESS
                        && address <= MCP23017_END_ADDRESS) {
                    for (uint8_t i = 0; i < count; i++) {
                        if ((registry + i) == MCP23017_OLATA
                                || (registry + i) == MCP23017_OLATB) {
                            MCP23017_write(address, (registry + i),
                                    *(data + index + i));
                        }
                    }
                }
            }
            break;
#endif
#ifdef RGB_ENABLED
        case MESSAGE_KIND_RGB:
            if (length == 2) { // Request strip count.
                SCOM_enqueue(channel, MESSAGE_KIND_RGB, 0xFF, 0xFF);
            } else if (length == 4) { // Request strip configuration
                SCOM_enqueue(channel, MESSAGE_KIND_RGB,
                        *(data + 2), *(data + 3));
            } else if (length == 12  // Add/set configuration
                    && *(data + 2) == 0x00) { // NUM: Only one strip possible
                if (*(data + 3) == RGB_PATTERN_OFF) RGB_off();
                else if (*(data + 3) & SCOM_PARAM_ALL) RGB_set(
                        *(data + 3) & SCOM_PARAM_MASK,         // Pattern
                        *(data + 4), *(data + 5), *(data + 6), // Color
                        (*(data + 7) << 8) | *(data + 8),      // Delay
                        *(data + 9), *(data + 10),             // Min - Max
                        *(data + 11));                         // Timeout
                else RGB_add(
                        *(data + 3) & SCOM_PARAM_MASK,         // Pattern
                        *(data + 4), *(data + 5), *(data + 6), // Color
                        (*(data + 7) << 8) | *(data + 8),      // Delay
                        *(data + 9), *(data + 10),             // Min - Max
                        *(data + 11));                         // Timeout
            }
            break;
#endif
#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
        case MESSAGE_KIND_INDICATORS:
            if (length == 2) { // Request strip count
                SCOM_enqueue(channel, MESSAGE_KIND_INDICATORS, 0xFF, 0xFF);
            } else if (length == 4 // Request a LED
                    && *(data + 2) == 0x00) { // NUM: Only one strip implemented
                SCOM_enqueue(channel, MESSAGE_KIND_INDICATORS,
                        *(data + 2), *(data + 3));
            } else if (length == 6 // set all WS281x LEDs
                    && *(data + 2) == 0x00) { // NUM: Only one strip implemented
                WS281x_all(*(data + 3), *(data + 4), *(data + 5));
            } else if (length == 13 // set WS281x
                    && *(data + 2) == 0x00) { // NUM: Only one strip implemented
                WS281x_set(*(data + 3),                        // LED
                        *(data + 4),                           // Pattern
                        *(data + 5), *(data + 6), *(data + 7), // Color
                        (*(data + 8) << 8) | *(data + 9),      // Delay
                        *(data + 10),                          // Min
                        *(data + 11));                         // Max
            }
            break;
#endif
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
        case MESSAGE_KIND_LIGHT:
            if (length == 2) { // Request strip count
                SCOM_enqueue(channel, MESSAGE_KIND_LIGHT, 0xFF, 0xFF);
            } else if (length == 4 // Request config item
                    && *(data + 2) == 0x00) { // NUM: Only one strip implemented
                SCOM_enqueue(channel, MESSAGE_KIND_LIGHT,
                        *(data + 2), *(data + 3));
            } else if (length == 32 // Add/set configuration
                    && *(data + 2) == 0x00) { // NUM: Only one strip implemented
                if (*(data + 3) & SCOM_PARAM_ALL) WS281xLight_set(
                        *(data + 3) & SCOM_PARAM_MASK,            // Pattern
                        *(data + 4), *(data + 5), *(data + 6),    // Color 1
                        *(data + 7), *(data + 8), *(data + 9),    // Color 2
                        *(data + 10), *(data + 11), *(data + 12),  // Color 3
                        *(data + 13), *(data + 14), *(data + 15), // Color 4
                        *(data + 16), *(data + 17), *(data + 18), // Color 5
                        *(data + 19), *(data + 20), *(data + 21), // Color 6
                        *(data + 22), *(data + 23), *(data + 24), // Color 7
                        (*(data + 25) << 8) | *(data + 26),       // Delay
                        *(data + 27),                             // Width
                        *(data + 28),                             // Fading
                        *(data + 29), *(data + 30),               // Min - Max
                        *(data + 31));                            // Timeout
                else WS281xLight_add(
                        *(data + 3) & SCOM_PARAM_MASK,            // Pattern
                        *(data + 4), *(data + 5), *(data + 6),    // Color 1
                        *(data + 7), *(data + 8), *(data + 9),    // Color 2
                        *(data + 10), *(data + 11), *(data + 12), // Color 3
                        *(data + 13), *(data + 14), *(data + 15), // Color 4
                        *(data + 16), *(data + 17), *(data + 18), // Color 5
                        *(data + 19), *(data + 20), *(data + 21), // Color 6
                        *(data + 22), *(data + 23), *(data + 24), // Color 7
                        (*(data + 25) << 8) | *(data + 26),       // Delay
                        *(data + 27),                             // Width
                        *(data + 28),                             // Fading
                        *(data + 29), *(data + 30),               // Min - Max
                        *(data + 31));                            // Timeout
            }
            break;
#endif
#endif
#ifdef BM78_ENABLED
        case MESSAGE_KIND_BLUETOOTH:
            if (length == 2) { // Request Bluetooth settings
                SCOM_enqueue(channel, MESSAGE_KIND_BLUETOOTH, 0x00, 0x00);
            } else if (length == 25) { // Set Bluetooth settings
                uint8_t i;
                BM78.pairingMode = *(data + 2);
                for (i = 0; i < 6; i++) {
                    BM78.pin[i] = *(data + i + 3);
                }
                for (i = 0; i < 16; i++) {
                    BM78.deviceName[i] = *(data + i + 9);
                }
                BM78_setup(false);
            } 
            break;
#endif
#ifdef LCD_ADDRESS
        case MESSAGE_KIND_PLAIN:
        case MESSAGE_KIND_DEBUG:
            LCD_clearCache();
            switch (channel) {
#ifdef USB_ENABLED
                case SCOM_CHANNEL_USB:
                    LCD_setString("    USB Message:    ", 0, false);
                    break;
#endif
#ifdef BM78_ENABLED
                case SCOM_CHANNEL_BT:
                    LCD_setString("     BT Message:    ", 0, false);
                    break;
#endif
            }
            LCD_setString("                    ", 2, false);
            for(uint8_t i = 2; i < length; i++) {
                if (i < LCD_COLS) LCD_replaceChar(*(data + i + 2), i, 2, false);
            }
            LCD_displayCache();
            if (*(data + 1) == MESSAGE_KIND_DEBUG) {
                SCOM_enqueue(channel, MESSAGE_KIND_DEBUG, *(data + 2), 0x00);
            }
            break;
#endif
        default:
            if (additionalDataHandler) {
                additionalDataHandler(channel, length, data);
            }
    }
}

void SCOM_bm78TestModeResponseHandler(BM78_Response_t *response) {
    if (SCOM_dataTransfer.stage > 0x00) switch (response->ISSC_Event.ogf) {
        case BM78_ISSC_OGF_COMMAND:
            switch (response->ISSC_Event.ocf) {
                case BM78_ISSC_OCF_OPEN:
                    SCOM_dataTransfer.stage = 0x02; // Reading EEPROM
                    BM78_readEEPROM(SCOM_dataTransfer.start, 
                            SCOM_dataTransfer.start + SCOM_MAX_PACKET_SIZE - 6 < SCOM_dataTransfer.start
                            || SCOM_dataTransfer.start + SCOM_MAX_PACKET_SIZE - 6 >= SCOM_dataTransfer.end
                            ? SCOM_dataTransfer.end - SCOM_dataTransfer.start + 1
                            : SCOM_MAX_PACKET_SIZE - 6);
                    break;
                default:
                    break;
            }
            break;
        case BM78_ISSC_OGF_OPERATION:
            switch (response->ISSC_Event.ocf) {
                case BM78_ISSC_OCF_READ: // EEPROM read -> Send content
                    SCOM_addDataByte(SCOM_dataTransfer.channel, 0, MESSAGE_KIND_DATA);
                    SCOM_addDataByte(SCOM_dataTransfer.channel, 1, BM78_DATA_PART);
                    SCOM_addDataByte(SCOM_dataTransfer.channel, 2, response->ISSC_ReadEvent.address >> 8);
                    SCOM_addDataByte(SCOM_dataTransfer.channel, 3, response->ISSC_ReadEvent.address & 0xFF);
                    SCOM_addDataByte(SCOM_dataTransfer.channel, 4, response->ISSC_ReadEvent.dataLength >> 8);
                    SCOM_addDataByte(SCOM_dataTransfer.channel, 5, response->ISSC_ReadEvent.dataLength & 0xFF);
                    for (uint8_t i = 0; i < response->ISSC_ReadEvent.dataLength; i++) {
                        SCOM_addDataByte(SCOM_dataTransfer.channel, i + 6, response->ISSC_ReadEvent.data[i]);
                    }
                    if (SCOM_commitData(SCOM_dataTransfer.channel, response->ISSC_ReadEvent.dataLength + 6, SCOM_NO_RETRY_LIMIT)) {
                        SCOM_dataTransfer.start += SCOM_MAX_PACKET_SIZE - 7;
                        if (SCOM_dataTransfer.start >= SCOM_dataTransfer.end) {
                            SCOM_dataTransfer.start = 0x00; // Idle
                            SCOM_dataTransfer.stage = 0x03; // Notify done
                            BM78_resetTo(BM78_MODE_APP);
                        }
                    }
                    break;
                default:
                    break;
            }
    }
}
#endif