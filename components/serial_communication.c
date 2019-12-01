/* 
 * File:   serial_communication.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "serial_communication.h"

#if defined USB_ENABLED || defined BM78_ENABLED

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
    uint8_t param[SCOM_QUEUE_SIZE]; // Possible parameters to the queue items.
} SCOM_Queue_t;

#ifdef BM78_ENABLED
struct {
    uint8_t stage;
    SCOM_Channel_t channel;
    uint16_t start;
    uint16_t end;
} SCOM_BM78eeprom = { 0x00, 0x0000, 0x0000 };
#endif

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
            return BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !SCOM_awatingConfirmation(channel);
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

inline void SCOM_sendIDD(SCOM_Channel_t channel, uint8_t param) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_enqueue(channel, MESSAGE_KIND_IDD, param);
    }
}

#ifdef DHT11_PORT
inline void SCOM_sendDHT11(SCOM_Channel_t channel) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_enqueue(channel, MESSAGE_KIND_DHT11, 0);
    }
}
#endif

#ifdef LCD_ADDRESS
inline void SCOM_sendLCD(SCOM_Channel_t channel, uint8_t line) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_enqueue(channel, MESSAGE_KIND_LCD, line);
    }
}

inline void SCOM_sendLCDBacklight(SCOM_Channel_t channel, bool on) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_enqueue(channel, MESSAGE_KIND_LCD,
                on ? SCOM_PARAM_LCD_BACKLIGH : SCOM_PARAM_LCD_NO_BACKLIGH);
    }
}
#endif

#ifdef MCP23017_ENABLED
inline void SCOM_sendMCP23017(SCOM_Channel_t channel, uint8_t address) {
    if (SCOM_canEnqueue(channel)) {
        if (address == SCOM_PARAM_ALL) address |= MCP23017_START_ADDRESS;
        SCOM_enqueue(channel, MESSAGE_KIND_MCP23017, address);
    }
}
#endif

#ifdef PIR_PORT
inline void SCOM_sendPIR(SCOM_Channel_t channel) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_enqueue(channel, MESSAGE_KIND_PIR, 0);
    }
}
#endif

#ifdef RGB_ENABLED
inline void SCOM_sendRGB(SCOM_Channel_t channel, uint8_t index) {
    if (SCOM_canEnqueue(channel)
            && (index & SCOM_PARAM_MASK) < RGB_list.size) {
        SCOM_enqueue(channel, MESSAGE_KIND_RGB, index);
    }
}
#endif

#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
inline void SCOM_sendWS281xLED(SCOM_Channel_t channel, uint8_t led) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_enqueue(channel, MESSAGE_KIND_WS281x, led);
    }
}
#endif
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
inline void SCOM_sendWS281xLight(SCOM_Channel_t channel, uint8_t index) {
    if (SCOM_canEnqueue(channel)
            && (index & SCOM_PARAM_MASK) < WS281xLight_list.size) {
        SCOM_enqueue(channel, MESSAGE_KIND_WS281x_LIGHT, index);
    }
}
#endif
#endif

#ifdef BM78_ENABLED
inline void SCOM_sendBluetoothSettings(SCOM_Channel_t channel) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_enqueue(channel, MESSAGE_KIND_BT_SETTINGS, 0);
    }
}

inline void SCOM_sendBluetoothEEPROM(SCOM_Channel_t channel, uint16_t start, uint16_t length) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_BM78eeprom.stage = 0x01; // Open EEPROM
        SCOM_BM78eeprom.channel = channel;
        SCOM_BM78eeprom.start = start;
        SCOM_BM78eeprom.end = start + length;
        if (SCOM_BM78eeprom.start < SCOM_BM78eeprom.end) {
            SCOM_enqueue(channel, MESSAGE_KIND_BT_EEPROM, 0);
        }
    }
}
#endif

inline void SCOM_sendDebug(SCOM_Channel_t channel, uint8_t debug) {
    if (SCOM_canEnqueue(channel)) {
        SCOM_enqueue(channel, MESSAGE_KIND_DEBUG, debug);
    }
}

void SCOM_setAdditionalDataHandler(SCOM_DataHandler_t handler) {
    additionalDataHandler = handler;
}

void SCOM_setNextMessageHandler(SCOM_NextMessageHandler_t handler) {
    nextMessageHandler = handler;
}

void SCOM_enqueue(SCOM_Channel_t channel, MessageKind_t what, uint8_t param) {
    uint8_t index = SCOM_queue[channel].index;
    while (index != SCOM_queue[channel].tail) {
        // Will be transmitted in the future, don't add again.
        if (SCOM_queue[channel].queue[index] == what
                && SCOM_queue[channel].param[index] == param) return;
        index++;
    }
    if ((SCOM_queue[channel].tail + 1) != SCOM_queue[channel].index) { // Do nothing if queue is full.
        SCOM_queue[channel].queue[SCOM_queue[channel].tail] = what;
        SCOM_queue[channel].param[SCOM_queue[channel].tail] = param;
        SCOM_queue[channel].tail++;
    }
    SCOM_messageSentHandler(channel);
}

void SCOM_messageSentHandler(SCOM_Channel_t channel) {
    uint8_t param;
#ifdef DHT11_PORT
    DHT11_Result result;
#endif
    if (SCOM_canSend(channel)) {
        uint8_t messageKind = SCOM_queue[channel].index != SCOM_queue[channel].tail
                ? SCOM_queue[channel].queue[SCOM_queue[channel].index]
                : MESSAGE_KIND_NONE;
        switch (messageKind) {
            case MESSAGE_KIND_IDD:
                param = SCOM_queue[channel].param[SCOM_queue[channel].index];
                switch(param) {
                    case 0x00: // IDD - Capabilities
                        SCOM_addDataByte(channel, 0, MESSAGE_KIND_IDD);  // Kind
                        SCOM_addDataByte(channel, 1, 0x00); // Reserved
                        SCOM_addDataByte(channel, 2, param); // IDD Message Type
                        param = 0x00;
#ifdef BM78_ENABLED
                        param |= 0b00000001;
#endif
#if defined MCP2200_ENABLED || defined MCP2221_ENABLED
                        param |= 0b00000010;
#endif
#ifdef DHT11_PORT
                        param |= 0b00000100;
#endif
#ifdef LCD_ADDRESS
                        param |= 0b00001000;
#endif
#ifdef MCP23017_ENABLED
                        param |= 0b00010000;
#endif
#ifdef PIR_PORT
                        param |= 0b00100000;
#endif
                        SCOM_addDataByte(channel, 3, param);

                        param = 0x00;
#ifdef RGB_ENABLED
                        param |= 0b00000001;
#endif
#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
                        param |= 0b00000010;
#endif
#ifdef WS281x_LIGHT_ROWS
                        param |= 0b00000100;
#endif
#endif
                        SCOM_addDataByte(channel, 4, param);

                        if (SCOM_commitData(channel, 5, BM78_MAX_SEND_RETRIES)) 
                            SCOM_queue[channel].index++;
                        break;
                    case 0x01: // IDD - Name
                        SCOM_addDataByte(channel, 0, MESSAGE_KIND_IDD);  // Kind
                        SCOM_addDataByte(channel, 1, 0x00); // Reserved
                        SCOM_addDataByte(channel, 2, param); // IDD Message Type

                        param = 0;
#ifdef BM78_ENABLED
                        while (param < 16 && BM78.deviceName[param] != 0x00) {
                            SCOM_addDataByte(channel, param + 3, BM78.deviceName[param]);
                            param++;
                        }
#endif
                        if (SCOM_commitData(channel, param + 3, BM78_MAX_SEND_RETRIES)) 
                            SCOM_queue[channel].index++;
                        break;
                }
                break;
#ifdef DHT11_PORT
            case MESSAGE_KIND_DHT11: 
                result = DHT11_measure();
                if (result.status == DHT11_OK) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_DHT11);
                    SCOM_addDataByte(channel, 1, result.temp);
                    SCOM_addDataByte(channel, 2, result.rh);
                    if (SCOM_commitData(channel, 3, BM78_MAX_SEND_RETRIES)) SCOM_queue[channel].index++;
                }
                break;
#endif
#ifdef LCD_ADDRESS
            case MESSAGE_KIND_LCD:
                param = SCOM_queue[channel].param[SCOM_queue[channel].index]; 
                if (param == SCOM_PARAM_LCD_BACKLIGH || param == SCOM_PARAM_LCD_NO_BACKLIGH) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_LCD);  // Kind
                    SCOM_addDataByte(channel, 1, 0); // Num - only one implemented
                    SCOM_addDataByte(channel, 2, LCD_backlight == LCD_BACKLIGHT);
                    if (SCOM_commitData(channel, 3, BM78_MAX_SEND_RETRIES)) SCOM_queue[channel].index++;
                } else if ((param & SCOM_PARAM_MASK) < LCD_ROWS) { // Max. 127 lines
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_LCD);  // Kind
                    SCOM_addDataByte(channel, 1, 0); // Num - only one implemented
                    SCOM_addDataByte(channel, 2, LCD_backlight == LCD_BACKLIGHT);
                    SCOM_addDataByte(channel, 3, param & SCOM_PARAM_MASK); // Line
                    SCOM_addDataByte(channel, 4, LCD_COLS);               // Characters

                    for (uint8_t line = 0; line < LCD_COLS; line++) {
                        SCOM_addDataByte(channel, line + 5,
                                LCD_getCache(param & SCOM_PARAM_MASK, line));
                    }

                    if (SCOM_commitData(channel, LCD_COLS + 5, BM78_MAX_SEND_RETRIES)) {
                        if (((param & SCOM_PARAM_MASK) + 1) < LCD_ROWS
                                && (param & SCOM_PARAM_ALL)) {
                            SCOM_queue[channel].param[SCOM_queue[channel].index]++;
                        } else SCOM_queue[channel].index++;
                    }
                } else SCOM_queue[channel].index++; // Line overflow - consuming
                break;
#endif
#ifdef MCP23017_ENABLED
            case MESSAGE_KIND_MCP23017:
                param = SCOM_queue[channel].param[SCOM_queue[channel].index] & SCOM_PARAM_MASK;
                if (param >= MCP23017_START_ADDRESS && param <= MCP23017_END_ADDRESS) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_MCP23017);                // Kind
                    SCOM_addDataByte(channel, 1, param);                                // Address
                    SCOM_addDataByte(channel, 2, MCP23017_read(param, MCP23017_GPIOA)); // GPIO A
                    SCOM_addDataByte(channel, 3, MCP23017_read(param, MCP23017_GPIOB)); // GPIO B
                    if (SCOM_commitData(channel, 4, BM78_MAX_SEND_RETRIES)) {
                        if ((SCOM_queue[channel].param[SCOM_queue[channel].index] & SCOM_PARAM_ALL)
                                && param < MCP23017_END_ADDRESS) {
                            SCOM_queue[channel].param[SCOM_queue[channel].index]++;
                        } else SCOM_queue[channel].index++;
                    }
                } else SCOM_queue[channel].index++;
                break;
#endif
#ifdef PIR_PORT
            case MESSAGE_KIND_PIR:
                SCOM_addDataByte(channel, 0, MESSAGE_KIND_PIR);
                SCOM_addDataByte(channel, 1, PIR_PORT);
                if (SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES)) SCOM_queue[channel].index++;
                break;
#endif
#ifdef RGB_ENABLED
            case MESSAGE_KIND_RGB:
                param = SCOM_queue[channel].param[SCOM_queue[channel].index] & SCOM_PARAM_MASK; // index (max 128)
                if (param < RGB_list.size && param  < RGB_LIST_SIZE) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_RGB);
                    SCOM_addDataByte(channel, 1, 0);                        // Num - here just one strip possible
                    SCOM_addDataByte(channel, 2, RGB_list.size);            // Size
                    SCOM_addDataByte(channel, 3, param & SCOM_PARAM_MASK);  // Index
                    SCOM_addDataByte(channel, 4, RGB_items[param].pattern); // Pattern
                    SCOM_addDataByte(channel, 5, RGB_items[param].red);     // Red
                    SCOM_addDataByte(channel, 6, RGB_items[param].green);   // Green
                    SCOM_addDataByte(channel, 7, RGB_items[param].blue);    // Blue
                    SCOM_addDataByte2(channel, 8, RGB_items[param].delay * RGB_TIMER_PERIOD);
                    SCOM_addDataByte(channel, 10, RGB_items[param].min);     // Min
                    SCOM_addDataByte(channel, 11, RGB_items[param].max);    // Max
                    SCOM_addDataByte(channel, 12, RGB_items[param].timeout);// Count
                    if (SCOM_commitData(channel, 13, BM78_MAX_SEND_RETRIES)) {
                        if ((param + 1) < RGB_list.size && (param + 1) < RGB_LIST_SIZE
                                && (SCOM_queue[channel].param[SCOM_queue[channel].index] & SCOM_PARAM_ALL)) {
                            SCOM_queue[channel].param[SCOM_queue[channel].index]++;
                        } else SCOM_queue[channel].index++;
                    }
                } else SCOM_queue[channel].index++;
                break;
#endif
#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
            case MESSAGE_KIND_WS281x:
                param = SCOM_queue[channel].param[SCOM_queue[channel].index] & SCOM_PARAM_MASK; // LED (max 128)
                if (param < WS281x_LED_COUNT) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_WS281x);
                    SCOM_addDataByte(channel, 1, 0);                       // Number - here just one light possible
                    SCOM_addDataByte(channel, 2, WS281x_LED_COUNT);        // LED Count
                    SCOM_addDataByte(channel, 3, param); // LED
                    SCOM_addDataByte(channel, 4, WS281x_ledPattern[param]);// Pattern
                    SCOM_addDataByte(channel, 5, WS281x_ledRed[param]);    // Red
                    SCOM_addDataByte(channel, 6, WS281x_ledGreen[param]);  // Green
                    SCOM_addDataByte(channel, 7, WS281x_ledBlue[param]);   // Blue
                    SCOM_addDataByte2(channel, 8, WS281x_ledDelay[param] * WS281x_TIMER_PERIOD);
                    SCOM_addDataByte(channel, 10, WS281x_ledMin[param]);    // Min
                    SCOM_addDataByte(channel, 11, WS281x_ledMax[param]);   // Max
                    if (SCOM_commitData(channel, 12, BM78_MAX_SEND_RETRIES)) {
                        if ((param + 1) < WS281x_LED_COUNT
                                && (SCOM_queue[channel].param[SCOM_queue[channel].index] & SCOM_PARAM_ALL)) {
                            SCOM_queue[channel].param[SCOM_queue[channel].index]++;
                        } else SCOM_queue[channel].index++;
                    }
                } else SCOM_queue[channel].index++;
                break;
#endif
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
            case MESSAGE_KIND_WS281x_LIGHT:
                param = SCOM_queue[channel].param[SCOM_queue[channel].index] & SCOM_PARAM_MASK; // index (max 128)
                if (param < WS281xLight_list.size && param < WS281x_LIGHT_LIST_SIZE) {
                    SCOM_addDataByte(channel, 0, MESSAGE_KIND_WS281x_LIGHT);
                    SCOM_addDataByte(channel, 1, 0);                       // Num
                    SCOM_addDataByte(channel, 2, WS281xLight_list.size);   // Size
                    SCOM_addDataByte(channel, 3, param & SCOM_PARAM_MASK); // Index
                    SCOM_addDataByte(channel, 4, WS281xLight_items[param].pattern);
                    SCOM_addDataByte(channel, 5, WS281xLight_items[param].color[0].r);
                    SCOM_addDataByte(channel, 6, WS281xLight_items[param].color[0].g);
                    SCOM_addDataByte(channel, 7, WS281xLight_items[param].color[0].b);
                    SCOM_addDataByte(channel, 8, WS281xLight_items[param].color[1].r);
                    SCOM_addDataByte(channel, 9, WS281xLight_items[param].color[1].g);
                    SCOM_addDataByte(channel, 10, WS281xLight_items[param].color[1].b);
                    SCOM_addDataByte(channel, 11, WS281xLight_items[param].color[2].r);
                    SCOM_addDataByte(channel, 12, WS281xLight_items[param].color[2].g);
                    SCOM_addDataByte(channel, 13, WS281xLight_items[param].color[2].b);
                    SCOM_addDataByte(channel, 14, WS281xLight_items[param].color[3].r);
                    SCOM_addDataByte(channel, 15, WS281xLight_items[param].color[3].g);
                    SCOM_addDataByte(channel, 16, WS281xLight_items[param].color[3].b);
                    SCOM_addDataByte(channel, 17, WS281xLight_items[param].color[4].r);
                    SCOM_addDataByte(channel, 18, WS281xLight_items[param].color[4].g);
                    SCOM_addDataByte(channel, 19, WS281xLight_items[param].color[4].b);
                    SCOM_addDataByte(channel, 20, WS281xLight_items[param].color[5].r);
                    SCOM_addDataByte(channel, 21, WS281xLight_items[param].color[5].g);
                    SCOM_addDataByte(channel, 22, WS281xLight_items[param].color[5].b);
                    SCOM_addDataByte(channel, 23, WS281xLight_items[param].color[6].r);
                    SCOM_addDataByte(channel, 24, WS281xLight_items[param].color[6].g);
                    SCOM_addDataByte(channel, 25, WS281xLight_items[param].color[6].b);
                    SCOM_addDataByte2(channel, 26, WS281xLight_items[param].delay * WS281x_TIMER_PERIOD);
                    SCOM_addDataByte(channel, 28, WS281xLight_items[param].width);
                    SCOM_addDataByte(channel, 29, WS281xLight_items[param].fading);
                    SCOM_addDataByte(channel, 30, WS281xLight_items[param].min);
                    SCOM_addDataByte(channel, 31, WS281xLight_items[param].max);
                    SCOM_addDataByte(channel, 32, WS281xLight_items[param].timeout);
                    if (SCOM_commitData(channel, 33, BM78_MAX_SEND_RETRIES)) {
                        if ((param + 1) < WS281xLight_list.size && (param + 1) < WS281x_LIGHT_LIST_SIZE
                                && (SCOM_queue[channel].param[SCOM_queue[channel].index] & SCOM_PARAM_ALL)) {
                            SCOM_queue[channel].param[SCOM_queue[channel].index]++;
                        } else SCOM_queue[channel].index++;
                    }
                } else SCOM_queue[channel].index++;
                break;
#endif
#endif
#ifdef BM78_ENABLED
            case MESSAGE_KIND_BT_SETTINGS:
                SCOM_addDataByte(channel, 0, MESSAGE_KIND_BT_SETTINGS);
                SCOM_addDataByte(channel, 1, BM78.pairingMode);
                SCOM_addDataBytes(channel, 2, 6, (uint8_t *) BM78.pin);
                SCOM_addDataBytes(channel, 8, 16, (uint8_t *) BM78.deviceName);
                if (SCOM_commitData(channel, 24, BM78_MAX_SEND_RETRIES)) SCOM_queue[channel].index++;
                break;
            case MESSAGE_KIND_BT_EEPROM:
                switch (SCOM_BM78eeprom.stage) {
                    case 0x01: // Open EEPROM
                        BM78_resetTo(BM78_MODE_TEST);
                        BM78_openEEPROM();
                        break;
                    case 0x02: // Read EEPROM
                        BM78_readEEPROM(SCOM_BM78eeprom.start, 
                                SCOM_BM78eeprom.start + SCOM_MAX_PACKET_SIZE - 6 < SCOM_BM78eeprom.start
                                || SCOM_BM78eeprom.start + SCOM_MAX_PACKET_SIZE - 6 >= SCOM_BM78eeprom.end
                                ? SCOM_BM78eeprom.end - SCOM_BM78eeprom.start + 1
                                : SCOM_MAX_PACKET_SIZE - 6);
                        break;
                    case 0x03: // Notify finish
                        SCOM_addDataByte(SCOM_BM78eeprom.channel, 0, MESSAGE_KIND_BT_EEPROM);
                        SCOM_addDataByte(SCOM_BM78eeprom.channel, 1, 0xFF);
                        if (SCOM_commitData(channel, 2, SCOM_NO_RETRY_LIMIT)) {
                            SCOM_queue[SCOM_BM78eeprom.channel].index++;
                        }
                        break;
                }
                break;
#endif
            case MESSAGE_KIND_DEBUG:
                param = SCOM_queue[channel].param[SCOM_queue[channel].index];
                SCOM_addDataByte(channel, 0, MESSAGE_KIND_DEBUG);
                SCOM_addDataByte(channel, 1, param);
                if (SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES)) {
                    SCOM_queue[channel].index++;
                }
                break;
            case MESSAGE_KIND_NONE:
                // Nothing to transmit
                //SCOM_addDataByte(channel, 0, MESSAGE_KIND_IDD);
                //SCOM_addDataByte(channel, 1, 0xAC);
                //SCOM_commitData(channel, 2, BM78_MAX_SEND_RETRIES);
                break;
            default:
                if (nextMessageHandler) { // An external handler exist
                    // if it consumed the message successfully
                    if (nextMessageHandler(
                            channel,
                            SCOM_queue[channel].queue[SCOM_queue[channel].index],
                            SCOM_queue[channel].param[SCOM_queue[channel].index])) {
                        SCOM_queue[channel].index++;
                    }
                } else { // Don't know what to do with the message - drop it
                    SCOM_queue[channel].index++;
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
            // 3rd byte is a random number for checksum check
            if (length == 4) SCOM_sendIDD(channel, *(data + 3));
            break;
#ifdef DHT11_PORT
        case MESSAGE_KIND_DHT11:
            if (length == 2) SCOM_sendDHT11(channel);
            break;
#endif
#ifdef PIR_PORT
        case MESSAGE_KIND_PIR:
            if (length == 2) SCOM_sendPIR(channel);
            break;
#endif
#ifdef LCD_ADDRESS
        case MESSAGE_KIND_LCD:
            if (length == 3) SCOM_sendLCD(channel, SCOM_PARAM_ALL);
            else if (length == 4) switch (*(data + 3)) {
                case SCOM_PARAM_LCD_CLEAR:
                    LCD_clearCache();
                    LCD_displayCache();
                    break;
                case SCOM_PARAM_LCD_RESET:
                    LCD_reset();
                    break;
                case SCOM_PARAM_LCD_BACKLIGH:
                case SCOM_PARAM_LCD_NO_BACKLIGH:
                    LCD_setBacklight(*(data + 3) == SCOM_PARAM_LCD_BACKLIGH);
                    break;
                default:
                    SCOM_sendLCD(channel, *(data + 2));
                    break;
            } else if (length > 4) {
                LCD_setBacklight(*(data + 3) == SCOM_PARAM_LCD_BACKLIGH);
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
        case MESSAGE_KIND_MCP23017:
            if (length == 3) SCOM_sendMCP23017(channel, *(data + 2));
            else if (length == 5 || length == 6) switch (*(data + 3)) {
                case 0b00000001: // Only port A
                    MCP23017_write(*(data + 2), MCP23017_OLATA, *(data + 4));
                    break;
                case 0b00000010: // Only port B
                    MCP23017_write(*(data + 2), MCP23017_OLATB, *(data + 4));
                    break;
                case 0b00000011: // Both, port A and port B
                    if (length == 6) {
                        MCP23017_write(*(data + 2), MCP23017_OLATA, *(data + 5));
                        MCP23017_write(*(data + 2), MCP23017_OLATB, *(data + 6));
                    }
                    break;
            }
            break;
#endif
#ifdef RGB_ENABLED
        case MESSAGE_KIND_RGB:
            if (length == 4) SCOM_sendRGB(channel, *(data + 3));
            else if (length == 12) { // set RGB
                if (*(data + 2) == RGB_PATTERN_OFF) RGB_off();
                else if (*(data + 2) & SCOM_PARAM_ALL) RGB_set(
                        *(data + 2) & SCOM_PARAM_MASK,         // Pattern
                        // data + 3 = Num - here only one strip possible
                        *(data + 4), *(data + 5), *(data + 6), // Color
                        (*(data + 7) << 8) | *(data + 8),      // Delay
                        *(data + 9), *(data + 10),              // Min - Max
                        *(data + 11));                         // Timeout
                else RGB_add(
                        *(data + 2) & SCOM_PARAM_MASK,         // Pattern
                        // data + 3 = Num - here only one strip possible
                        *(data + 4), *(data + 5), *(data + 6), // Color
                        (*(data + 7) << 8) | *(data + 8),      // Delay
                        *(data + 9), *(data + 10),              // Min - Max
                        *(data + 11));                         // Timeout
            }
            break;
#endif
#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
        case MESSAGE_KIND_WS281x:
            if (length == 4) SCOM_sendWS281xLED(channel, *(data + 3));
            else if (length == 6) { // set all WS281x LEDs
                WS281x_all(*(data + 3), *(data + 4), *(data + 5));
            } else if (length == 12) { // set WS281x
                // led, pattern, red, green, blue, delayH, delayL, min, max
                // data + 2 = Num - here only one strip possible
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
        case MESSAGE_KIND_WS281x_LIGHT:
            if (length == 4) SCOM_sendWS281xLight(channel, *(data + 3));
            else if (length == 32) {
                // data + 2 = Num - here only one light possible
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
        case MESSAGE_KIND_BT_SETTINGS:
            if (length == 2) SCOM_sendBluetoothSettings(channel);
            else if (length == 25) {
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
        case MESSAGE_KIND_BT_EEPROM:
            if (length == 2) SCOM_sendBluetoothEEPROM(channel, 0x0000, 0x1FFF);
            else if (length == 6) SCOM_sendBluetoothEEPROM(channel,
                    *(data + 2) << 8 | *(data + 3),
                    *(data + 4) << 8 | *(data + 5));
            else if (length > 6) {
                //  | length                       | - | start                      | == length - 6
                if (((*(data + 4) << 8 | *(data + 5))) - ((*(data + 2) << 8) | *(data + 3)) == length - 6) {
                    // TODO write EEPROM
                }
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
                SCOM_sendDebug(channel, *(data + 2));
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
    if (SCOM_BM78eeprom.stage > 0x00) switch (response->ISSC_Event.ogf) {
        case BM78_ISSC_OGF_COMMAND:
            switch (response->ISSC_Event.ocf) {
                case BM78_ISSC_OCF_OPEN:
                    SCOM_BM78eeprom.stage = 0x02; // Reading EEPROM
                    BM78_readEEPROM(SCOM_BM78eeprom.start, 
                            SCOM_BM78eeprom.start + SCOM_MAX_PACKET_SIZE - 6 < SCOM_BM78eeprom.start
                            || SCOM_BM78eeprom.start + SCOM_MAX_PACKET_SIZE - 6 >= SCOM_BM78eeprom.end
                            ? SCOM_BM78eeprom.end - SCOM_BM78eeprom.start + 1
                            : SCOM_MAX_PACKET_SIZE - 6);
                    break;
                default:
                    break;
            }
            break;
        case BM78_ISSC_OGF_OPERATION:
            switch (response->ISSC_Event.ocf) {
                case BM78_ISSC_OCF_READ: // EEPROM read -> Send content
                    SCOM_addDataByte(SCOM_BM78eeprom.channel, 0, MESSAGE_KIND_BT_EEPROM);
                    SCOM_addDataByte(SCOM_BM78eeprom.channel, 1, response->ISSC_ReadEvent.address >> 8);
                    SCOM_addDataByte(SCOM_BM78eeprom.channel, 2, response->ISSC_ReadEvent.address & 0xFF);
                    SCOM_addDataByte(SCOM_BM78eeprom.channel, 3, response->ISSC_ReadEvent.dataLength >> 8);
                    SCOM_addDataByte(SCOM_BM78eeprom.channel, 4, response->ISSC_ReadEvent.dataLength & 0xFF);
                    for (uint8_t i = 0; i < response->ISSC_ReadEvent.dataLength; i++) {
                        SCOM_addDataByte(SCOM_BM78eeprom.channel, i + 5, response->ISSC_ReadEvent.data[i]);
                    }
                    if (SCOM_commitData(SCOM_BM78eeprom.channel, response->ISSC_ReadEvent.dataLength + 5, SCOM_NO_RETRY_LIMIT)) {
                        SCOM_BM78eeprom.start += SCOM_MAX_PACKET_SIZE - 6;
                        if (SCOM_BM78eeprom.start >= SCOM_BM78eeprom.end) {
                            SCOM_BM78eeprom.start = 0x00; // Idle
                            SCOM_BM78eeprom.stage = 0x03; // Notify done
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