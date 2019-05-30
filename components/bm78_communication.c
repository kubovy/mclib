/* 
 * File:   bm78_transmission.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "bm78_communication.h"

#ifdef BM78_ENABLED

/** 
 * Queue of message types to be send out over BT. Should be cleared when 
 * connection ends for whatever reason. 
 */
struct {
    uint8_t index;       // Sent index.
    uint8_t tail;        // Tail index.
    uint8_t queue[BMC_QUEUE_SIZE]; // Transmission message type queue.
    uint8_t param[BMC_QUEUE_SIZE]; // Possible parameters to the queue items.
} BMC_tx = {0, 0};

BMC_NextMessageHandler_t BMC_nextMessageHandler;

inline void BMC_sendSettings(void) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_SETTINGS, 0);
    }
}

#ifdef DHT11_PORT
inline void BMC_sendDHT11(void) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_DHT11, 0);
    }
}
#endif

#ifdef LCD_ADDRESS
inline void BMC_sendLCD(uint8_t line) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_LCD, line);
    }
}

inline void BMC_sendLCDBacklight(bool on) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_LCD, on ? BMC_PARAM_LCD_BACKLIGH : BMC_PARAM_LCD_NO_BACKLIGH);
    }
}
#endif

#ifdef MCP_ENABLED
inline void BMC_sendMCP23017(uint8_t address) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_MCP23017, address);
    }
}
#endif

#ifdef PIR_PORT
inline void BMC_sendPIR(void) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_PIR, 0);
    }
}
#endif

#ifdef RGB_ENABLED
inline void BMC_sendRGB(uint8_t index) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_RGB, index);
    }
}
#endif

#ifdef WS281x_BUFFER
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
inline void BMC_sendWS281xLight(uint8_t index) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_WS281x_LIGHT, index);
    }
}
#else
inline void BMC_sendWS281xLED(uint8_t led) {
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
        BMC_enqueue(BM78_MESSAGE_KIND_WS281x, led);
    }
}
#endif
#endif

void BMC_setNextMessageHandler(BMC_NextMessageHandler_t nextMessageHandler) {
    BMC_nextMessageHandler = nextMessageHandler;
}

void BMC_enqueue(uint8_t what, uint8_t param) {
    uint8_t index = BMC_tx.index;
    while (index != BMC_tx.tail) {
        // Will be transmitted in the future, don't add again.
        if (BMC_tx.queue[index++] == what) return;
    }
    if ((BMC_tx.tail + 1) != BMC_tx.index) { // Do nothing if queue is full.
        BMC_tx.queue[BMC_tx.tail] = what;
        BMC_tx.param[BMC_tx.tail] = param;
        BMC_tx.tail++;
    }
    BMC_bm78MessageSentHandler();
}

void BMC_bm78MessageSentHandler(void) {
    uint8_t param;
#ifdef DHT11_PORT
    DHT11_Result result;
#endif
    if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE && !BM78_awatingConfirmation()) {
        switch (BMC_tx.index != BMC_tx.tail ? BMC_tx.queue[BMC_tx.index] : BM78_MESSAGE_KIND_UNKNOWN) {
            case BM78_MESSAGE_KIND_SETTINGS:
                BM78_addDataByte(0, BM78_MESSAGE_KIND_SETTINGS);
                BM78_addDataByte(1, BM78.pairingMode);
                BM78_addDataBytes(2, 6, (uint8_t *) BM78.pin);
                BM78_addDataBytes(8, 16, (uint8_t *) BM78.deviceName);
                if (BM78_commitData(24, BM78_MAX_SEND_RETRIES)) BMC_tx.index++;
                break;
#ifdef DHT11_PORT
            case BM78_MESSAGE_KIND_DHT11: 
                result = DHT11_measure();
                if (result.status == DHT11_OK) {
                    BM78_addDataByte(0, BM78_MESSAGE_KIND_DHT11);
                    BM78_addDataByte(1, result.temp);
                    BM78_addDataByte(2, result.rh);
                    if (BM78_commitData(3, BM78_MAX_SEND_RETRIES)) BMC_tx.index++;
                }
                break;
#endif
#ifdef LCD_ADDRESS
            case BM78_MESSAGE_KIND_LCD:
                param = BMC_tx.param[BMC_tx.index]; 
                if (param == BMC_PARAM_LCD_BACKLIGH || param == BMC_PARAM_LCD_NO_BACKLIGH) {
                    BM78_addDataByte(0, BM78_MESSAGE_KIND_LCD);  // Kind
                    BM78_addDataByte(1, LCD_backlight == LCD_BACKLIGHT);
                    if (BM78_commitData(2, BM78_MAX_SEND_RETRIES)) BMC_tx.index++;
                } else if ((param & BMC_PARAM_MASK) < LCD_ROWS) { // Max. 127 lines
                    BM78_addDataByte(0, BM78_MESSAGE_KIND_LCD);  // Kind
                    BM78_addDataByte(1, LCD_backlight == LCD_BACKLIGHT);
                    BM78_addDataByte(2, param & BMC_PARAM_MASK); // Line
                    BM78_addDataByte(3, LCD_COLS);               // Characters

                    for (uint8_t line = 0; line < LCD_COLS; line++) {
                        BM78_addDataByte(line + 4, 
                                LCD_getCache(param & BMC_PARAM_MASK, line));
                    }

                    if (BM78_commitData(LCD_COLS + 4, BM78_MAX_SEND_RETRIES)) {
                        if (((param & BMC_PARAM_MASK) + 1) < LCD_ROWS
                                && (param & BMC_PARAM_ALL)) {
                            BMC_tx.param[BMC_tx.index]++;
                        } else BMC_tx.index++;
                    }
                } else BMC_tx.index++; // Line overflow - consuming
                break;
#endif
#ifdef MCP_ENABLED
            case BM78_MESSAGE_KIND_MCP23017:
                param = BMC_tx.param[BMC_tx.index];
                BM78_addDataByte(0, BM78_MESSAGE_KIND_MCP23017); // Kind
                BM78_addDataByte(1, param);                      // Address
                BM78_addDataByte(2, MCP_read(param, MCP_GPIOA)); // GPIO A
                BM78_addDataByte(3, MCP_read(param, MCP_GPIOB)); // GPIO B
                if (BM78_commitData(4, BM78_MAX_SEND_RETRIES)) BMC_tx.index++;
                break;
#endif
#ifdef PIR_PORT
            case BM78_MESSAGE_KIND_PIR:
                BM78_addDataByte(0, BM78_MESSAGE_KIND_PIR);
                BM78_addDataByte(1, PIR_PORT);
                if (BM78_commitData(2, BM78_MAX_SEND_RETRIES)) BMC_tx.index++;
                break;
#endif
#ifdef RGB_ENABLED
            case BM78_MESSAGE_KIND_RGB:
                param = BMC_tx.param[BMC_tx.index] & BMC_PARAM_MASK; // index (max 128)
                if (param < RGB_list.size && param  < RGB_LIST_SIZE) {
                    BM78_addDataByte(0, BM78_MESSAGE_KIND_RGB);
                    BM78_addDataByte(1, RGB_list.size);            // Size
                    BM78_addDataByte(2, param & BMC_PARAM_MASK);   // Index
                    BM78_addDataByte(3, RGB_items[param].pattern); // Pattern
                    BM78_addDataByte(4, RGB_items[param].red);     // Red
                    BM78_addDataByte(5, RGB_items[param].green);   // Green
                    BM78_addDataByte(6, RGB_items[param].blue);    // Blue
                    BM78_addDataByte2(7, RGB_items[param].delay * RGB_TIMER_PERIOD);
                    BM78_addDataByte(9, RGB_items[param].min);     // Min
                    BM78_addDataByte(10, RGB_items[param].max);     // Max
                    BM78_addDataByte(11, RGB_items[param].timeout); // Count
                    if (BM78_commitData(12, BM78_MAX_SEND_RETRIES)) {
                        if ((param + 1) < RGB_list.size && (param + 1) < RGB_LIST_SIZE
                                && (BMC_tx.param[BMC_tx.index] & BMC_PARAM_ALL)) {
                            BMC_tx.param[BMC_tx.index]++;
                        } else BMC_tx.index++;
                    }
                } else BMC_tx.index++;
                break;
#endif
#ifdef WS281x_BUFFER
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
            case BM78_MESSAGE_KIND_WS281x_LIGHT:
                param = BMC_tx.param[BMC_tx.index] & BMC_PARAM_MASK; // index (max 128)
                if (param < WS281xLight_list.size && param < WS281x_LIGHT_LIST_SIZE) {
                    BM78_addDataByte(0, BM78_MESSAGE_KIND_WS281x_LIGHT);
                    BM78_addDataByte(1, WS281xLight_list.size);  // Size
                    BM78_addDataByte(2, param & BMC_PARAM_MASK); // Index
                    BM78_addDataByte(3, WS281xLight_items[param].pattern);
                    BM78_addDataByte(4, WS281xLight_items[param].color[0].r);
                    BM78_addDataByte(5, WS281xLight_items[param].color[0].g);
                    BM78_addDataByte(6, WS281xLight_items[param].color[0].b);
                    BM78_addDataByte(7, WS281xLight_items[param].color[1].r);
                    BM78_addDataByte(8, WS281xLight_items[param].color[1].g);
                    BM78_addDataByte(9, WS281xLight_items[param].color[1].b);
                    BM78_addDataByte(10, WS281xLight_items[param].color[2].r);
                    BM78_addDataByte(11, WS281xLight_items[param].color[2].g);
                    BM78_addDataByte(12, WS281xLight_items[param].color[2].b);
                    BM78_addDataByte(13, WS281xLight_items[param].color[3].r);
                    BM78_addDataByte(14, WS281xLight_items[param].color[3].g);
                    BM78_addDataByte(15, WS281xLight_items[param].color[3].b);
                    BM78_addDataByte(16, WS281xLight_items[param].color[4].r);
                    BM78_addDataByte(17, WS281xLight_items[param].color[4].g);
                    BM78_addDataByte(18, WS281xLight_items[param].color[4].b);
                    BM78_addDataByte(19, WS281xLight_items[param].color[5].r);
                    BM78_addDataByte(20, WS281xLight_items[param].color[5].g);
                    BM78_addDataByte(21, WS281xLight_items[param].color[5].b);
                    BM78_addDataByte(22, WS281xLight_items[param].color[6].r);
                    BM78_addDataByte(23, WS281xLight_items[param].color[6].g);
                    BM78_addDataByte(24, WS281xLight_items[param].color[6].b);
                    BM78_addDataByte2(25, WS281xLight_items[param].delay * WS281x_TIMER_PERIOD);
                    BM78_addDataByte(27, WS281xLight_items[param].width);
                    BM78_addDataByte(28, WS281xLight_items[param].fading);
                    BM78_addDataByte(29, WS281xLight_items[param].min);
                    BM78_addDataByte(30, WS281xLight_items[param].max);
                    BM78_addDataByte(31, WS281xLight_items[param].timeout);
                    if (BM78_commitData(32, BM78_MAX_SEND_RETRIES)) {
                        if ((param + 1) < WS281xLight_list.size && (param + 1) < WS281x_LIGHT_LIST_SIZE
                                && (BMC_tx.param[BMC_tx.index] & BMC_PARAM_ALL)) {
                            BMC_tx.param[BMC_tx.index]++;
                        } else BMC_tx.index++;
                    }
                } else BMC_tx.index++;
                break;
#else
            case BM78_MESSAGE_KIND_WS281x:
                param = BMC_tx.param[BMC_tx.index] & BMC_PARAM_MASK; // LED (max 128)
                if ((param) < WS281x_LED_COUNT) {
                    BM78_addDataByte(0, BM78_MESSAGE_KIND_WS281x);
                    BM78_addDataByte(1, WS281x_LED_COUNT);        // LED Count
                    BM78_addDataByte(2, param); // LED
                    BM78_addDataByte(3, WS281x_ledPattern[param]);// Pattern
                    BM78_addDataByte(4, WS281x_ledRed[param]);    // Red
                    BM78_addDataByte(5, WS281x_ledGreen[param]);  // Green
                    BM78_addDataByte(6, WS281x_ledBlue[param]);   // Blue
                    BM78_addDataByte2(7, WS281x_ledDelay[param] * WS281x_TIMER_PERIOD);
                    BM78_addDataByte(8, WS281x_ledMin[param]);    // Min
                    BM78_addDataByte(10, WS281x_ledMax[param]);   // Max
                    if (BM78_commitData(11, BM78_MAX_SEND_RETRIES)) {
                        if ((param + 1) < WS281x_LED_COUNT
                                && (BMC_tx.param[BMC_tx.index] & BMC_PARAM_ALL)) {
                            BMC_tx.param[BMC_tx.index]++;
                        } else BMC_tx.index++;
                    }
                } else BMC_tx.index++;
                break;
#endif
#endif
            case BM78_MESSAGE_KIND_UNKNOWN:
                // do nothing
                break;
            default:
                if (BMC_nextMessageHandler) { // An external handler exist
                    // if it consumed the message successfully
                    if (BMC_nextMessageHandler(BMC_tx.queue[BMC_tx.index], BMC_tx.param[BMC_tx.index])) {
                        BMC_tx.index++;
                    }
                } else {
                    BMC_tx.index++;
                }
                break;
        }
        
    } else if (BM78.status != BM78_STATUS_SPP_CONNECTED_MODE) {
        // Reset transmit queue if connection lost.
        BMC_tx.index = 0;
        BMC_tx.tail = 0;
    }
}

void BMC_bm78TransparentDataHandler(uint8_t length, uint8_t *data) {
    switch(*(data)) {
        case BM78_MESSAGE_KIND_SETTINGS:
            if (length == 1) BMC_sendSettings();
            else if (length == 24) {
                uint8_t i;
                BM78.pairingMode = *(data + 1);
                for (i = 0; i < 6; i++) {
                    BM78.pin[i] = *(data + i + 2);
                }
                for (i = 0; i < 16; i++) {
                    BM78.deviceName[i] = *(data + i + 8);
                }
                BM78_setup(false);
            } 
            break;
#ifdef DHT11_PORT
        case BM78_MESSAGE_KIND_DHT11:
            if (length == 1) BMC_sendDHT11();
            break;
#endif
#ifdef PIR_PORT
        case BM78_MESSAGE_KIND_PIR:
            if (length == 1) BMC_sendPIR();
            break;
#endif
#ifdef LCD_ADDRESS
        case BM78_MESSAGE_KIND_LCD:
            if (length == 1) BMC_sendLCD(BMC_PARAM_ALL);
            else if (length == 2) switch (*(data + 1)) {
                case BMC_PARAM_LCD_CLEAR:
                    LCD_clear();
                    break;
                case BMC_PARAM_LCD_RESET:
                    LCD_reset();
                    break;
                case BMC_PARAM_LCD_BACKLIGH:
                case BMC_PARAM_LCD_NO_BACKLIGH:
                    LCD_setBacklight(*(data + 1) == BMC_PARAM_LCD_BACKLIGH);
                    break;
                default:
                    BMC_sendLCD(*(data + 1));
                    break;
            } else if (length > 3) {
                LCD_setString("                    ", *(data + 1), true);
                for (uint8_t i = 0; i < *(data + 2); i++) {
                    if (i < length && i < LCD_COLS) {
                        LCD_replaceChar(*(data + i + 3), i, *(data + 1), false);
                    }
                }
                LCD_displayLine(*(data + 1));
            }
            break;
#endif
#ifdef MCP_ENABLED
        case BM78_MESSAGE_KIND_MCP23017:
            if (length == 2) BMC_sendMCP23017(*(data + 1));
            else if (length == 4 || length == 5) switch (*(data + 2)) {
                case 0b00000001: // Only port A
                    MCP_write(*(data + 1), MCP_OLATA, *(data + 3));
                    break;
                case 0b00000010: // Only port B
                    MCP_write(*(data + 1), MCP_OLATB, *(data + 3));
                    break;
                case 0b00000011: // Both, port A and port B
                    if (length == 5) {
                        MCP_write(*(data + 1), MCP_OLATA, *(data + 3));
                        MCP_write(*(data + 1), MCP_OLATB, *(data + 4));
                    }
                    break;
            }
            break;
#endif
#ifdef RGB_ENABLED
        case BM78_MESSAGE_KIND_RGB:
            if (length == 1) BMC_sendRGB(BMC_PARAM_ALL);
            else if (length == 2) BMC_sendRGB(*(data + 1));
            else if (length == 10) { // set RGB
                if (*(data + 1) == RGB_PATTERN_OFF) WS281xLight_off();
                else if (*(data + 1) & BMC_PARAM_ALL) RGB_set(
                        *(data + 1) & BMC_PARAM_MASK,          // Pattern
                        *(data + 2), *(data + 3), *(data + 4), // Color
                        (*(data + 5) << 8) | *(data + 6),      // Delay
                        *(data + 7), *(data + 8),              // Min - Max
                        *(data + 9));                          // Timeout
                else RGB_add(
                        *(data + 1) & BMC_PARAM_MASK,          // Pattern
                        *(data + 2), *(data + 3), *(data + 4), // Color
                        (*(data + 5) << 8) | *(data + 6),      // Delay
                        *(data + 7), *(data + 8),              // Min - Max
                        *(data + 9));                          // Timeout
            }
            break;
#endif
#ifdef WS281x_BUFFER
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
        case BM78_MESSAGE_KIND_WS281x_LIGHT:
            if (length == 1) BMC_sendWS281xLight(BMC_PARAM_ALL);
            else if (length == 2) BMC_sendWS281xLight(*(data + 1));
            else if (length == 30) {
                if (*(data + 1) == WS281x_LIGHT_OFF) WS281xLight_off();
                else if (*(data + 1) & BMC_PARAM_ALL) WS281xLight_set(
                        *(data + 1) & BMC_PARAM_MASK,             // Pattern
                        *(data + 2), *(data + 3), *(data + 4),    // Color 1
                        *(data + 5), *(data + 6), *(data + 7),    // Color 2
                        *(data + 8), *(data + 9), *(data + 10),   // Color 3
                        *(data + 11), *(data + 12), *(data + 13), // Color 4
                        *(data + 14), *(data + 15), *(data + 16), // Color 5
                        *(data + 17), *(data + 18), *(data + 19), // Color 6
                        *(data + 20), *(data + 21), *(data + 22), // Color 7
                        (*(data + 23) << 8) | *(data + 24),       // Delay
                        *(data + 25),                             // Width
                        *(data + 26),                             // Fading
                        *(data + 27), *(data + 28),               // Min - Max
                        *(data + 29));                            // Timeout
                else WS281xLight_add(
                        *(data + 1) & BMC_PARAM_MASK,             // Pattern
                        *(data + 2), *(data + 3), *(data + 4),    // Color 1
                        *(data + 5), *(data + 6), *(data + 7),    // Color 2
                        *(data + 8), *(data + 9), *(data + 10),   // Color 3
                        *(data + 11), *(data + 12), *(data + 13), // Color 4
                        *(data + 14), *(data + 15), *(data + 16), // Color 5
                        *(data + 17), *(data + 18), *(data + 19), // Color 6
                        *(data + 20), *(data + 21), *(data + 22), // Color 7
                        (*(data + 23) << 8) | *(data + 24),       // Delay
                        *(data + 25),                             // Width
                        *(data + 26),                             // Fading
                        *(data + 27), *(data + 28),               // Min - Max
                        *(data + 29));                            // Timeout
            }
            break;
#else
        case BM78_MESSAGE_KIND_WS281x:
            if (length == 1) BMC_sendWS281xLED(BMC_PARAM_ALL);
            else if (length == 2) BMC_sendWS281xLED(*(data + 1));
            else if (length == 4) { // set all WS281x LEDs
                WS281x_all(*(data + 1), *(data + 2), *(data + 3));
            } else if (length == 10) { // set WS281x
                // led, pattern, red, green, blue, delayH, delayL, min, max
                WS281x_set(*(data + 1),                        // LED
                        *(data + 2),                           // Pattern
                        *(data + 3), *(data + 4), *(data + 5), // Color
                        (*(data + 6) << 8) | *(data + 7),      // Delay
                        *(data + 8),                           // Min
                        *(data + 9));                          // Max
            }
            break;
#endif
#endif
#ifdef LCD_ADDRESS
        case BM78_MESSAGE_KIND_PLAIN:
            LCD_clear();
            LCD_setString("                    ", 2, false);
            for(uint8_t i = 0; i < length; i++) {
                if (i < LCD_COLS) LCD_replaceChar(*(data + i), i, 2, false);
            }
            LCD_setString("    BT Message:     ", 0, true);
            LCD_displayLine(2);
            break;
#endif
    }
}

#endif