/* 
 * File:   poc.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "poc.h"

#ifdef LCD_ADDRESS

#ifdef DHT11_PORT
void POC_testDHT11(void) {
    DHT11_Result dht11 = DHT11_measure();

    LCD_init();
    LCD_setBacklight(true);
    LCD_setString("DHT11 (X retries)", 0, false);
    LCD_replaceChar(48 + dht11.retries, 7, 0, true);

    if (dht11.status == DHT11_OK) {
        LCD_setString("Temp:      ?C\0", 1, false);
        LCD_replaceChar(48 + (dht11.temp / 10 % 10), 10, 1, false);
        LCD_replaceChar(48 + (dht11.temp % 10), 11, 1, true);

        LCD_setString("Humidity:  ?%\0", 2, false);
        LCD_replaceChar(48 + (dht11.rh / 10 % 10), 10, 2, false);
        LCD_replaceChar(48 + (dht11.rh % 10), 11, 2, true);
    } else switch(dht11.status) {
        case DHT11_ERR_CHKSUM:
            LCD_setString(" Checksum mismatch! ", 2, true);
            break;
        case DHT11_ERR_NO_RESPONSE:
            LCD_setString("    No response!    ", 2, true);
            break;
        default:
            LCD_setString("   Unknown error!   ", 2, true);
            break;
    }
}
#endif

void POC_bm78InitializationHandler(char *deviceName, char *pin) {
    LCD_setString("Device name:", 0, true);
    LCD_setString(deviceName, 1, true);
    LCD_setString("PIN:", 2, true);
    LCD_setString(pin, 3, true);
}

#ifdef BM78_ENABLED
void POC_bm78EventHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        case BM78_EVENT_DISCONNECTION_COMPLETE:
            LCD_setString("  BT Disconnected   ", 1, true);
            break;
        case BM78_EVENT_SPP_CONNECTION_COMPLETE:
            if (response.SPPConnectionComplete_0x74.status == BM78_COMMAND_SUCCEEDED) {
                LCD_setString("    BT Connected    ", 1, true);
                LCD_setString(" ##:##:##:##:##:##  ", 2, false);
                for (uint8_t i = 0; i < 6; i++) {
                    LCD_replaceChar(dec2hex(response.SPPConnectionComplete_0x74.peer_address[i] / 16 % 16), i * 3 + 1, 2, false);
                    LCD_replaceChar(dec2hex(response.SPPConnectionComplete_0x74.peer_address[i] % 16), i * 3 + 2, 2, false);
                }
                LCD_displayLine(2);
            } else {
                LCD_setString("BT Connection Failed", 1, true);
            }
            break;
        case BM78_EVENT_COMMAND_COMPLETE:
            switch(response.CommandComplete_0x80.command) {
                case BM78_CMD_READ_LOCAL_INFORMATION:
                    LCD_setString("ST:##,##,##         ", 0, false);
                    LCD_replaceChar(dec2hex(response.op_code / 16 % 16), 3, 0, false);
                    LCD_replaceChar(dec2hex(response.op_code % 16), 4, 0, false);
                    LCD_replaceChar(dec2hex(response.CommandComplete_0x80.command / 16 % 16), 6, 0, false);
                    LCD_replaceChar(dec2hex(response.CommandComplete_0x80.command % 16), 7, 0, false);
                    LCD_replaceChar(dec2hex(response.CommandComplete_0x80.status / 16 % 16), 9, 0, false);
                    LCD_replaceChar(dec2hex(response.CommandComplete_0x80.status % 16), 10, 0, false);
                    LCD_displayLine(0);

                    LCD_setString("V                   ", 1, false);
                    for (uint8_t i = 0; i < 5; i++) { // BT Address
                        LCD_replaceChar(dec2hex(data[i] / 16 % 16), 3 + i * 2, 1, false);
                        LCD_replaceChar(dec2hex(data[i] % 16), 4 + i * 2, 1, false);
                    }
                    LCD_displayLine(1);

                    LCD_setString("BT:##:##:##:##:##:##", 2, false);
                    for (uint8_t i = 0; i < 6; i++) { // BT Address
                        LCD_replaceChar(dec2hex(data[5 + i] / 16 % 16), 3 + (5 - i) * 3, 2, false);
                        LCD_replaceChar(dec2hex(data[5 + i] % 16), 4 + (5 - i) * 3, 2, false);
                    }
                    LCD_displayLine(2);
                    break;
                case BM78_CMD_READ_DEVICE_NAME:
                    LCD_setString("Device name:", 0, true);
                    LCD_setString((char *)data, 1, true);
                    break;
                case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                    LCD_clear();
                    for (uint8_t i = 0; i < BM78.pairedDevicesCount; i++) {
                        if (i < 4) {
                            LCD_setString("XX:##:##:##:##:##:##", i, false);
                            LCD_replaceChar(BM78.pairedDevices[i].index / 10 % 10 + 48, 0, i, false);
                            LCD_replaceChar(BM78.pairedDevices[i].index % 10 + 48, 1, i, false);
                            
                            for (uint8_t j = 0; j < 6; j++) {
                                LCD_replaceChar(dec2hex(BM78.pairedDevices[i].address[j] / 16 % 16), j * 3 + 3, i, false);
                                LCD_replaceChar(dec2hex(BM78.pairedDevices[i].address[j] % 16), j * 3 + 4, i, false);
                            }
                            LCD_displayLine(i);
                        }
                    }
                    break;
                case BM78_CMD_READ_PIN_CODE:
                    LCD_setString("PIN:", 0, true);
                    LCD_setString((char *) data, 1, true);
                    break;
                default:
                    break;
            }
            break;
        case BM78_EVENT_BM77_STATUS_REPORT:
            switch(response.StatusReport_0x81.status) {
                case BM78_STATUS_POWER_ON:
                    LCD_setString("    BT Power On     ", 3, true);
                    break;
                case BM78_STATUS_PAGE_MODE:
                    LCD_setString("    BT Page Mode    ", 3, true);
                    break;
                case BM78_STATUS_STANDBY_MODE:
                    LCD_setString("    BT Stand-By     ", 3, true);
                    break;
                case BM78_STATUS_LINK_BACK_MODE:
                    LCD_setString("    BT Link Back    ", 3, true);
                    break;
                case BM78_STATUS_SPP_CONNECTED_MODE:
                    LCD_setString("  BT SPP Connected  ", 3, true);
                    break;
                case BM78_STATUS_LE_CONNECTED_MODE:
                    LCD_setString("  BT LE Connected   ", 3, true);
                    break;
                case BM78_STATUS_IDLE_MODE:
                    LCD_setString("      BT Idle       ", 3, true);
                    break;
                case BM78_STATUS_SHUTDOWN_MODE:
                    LCD_setString("    BT Shut down    ", 3, true);
                    break;
                default:
                    LCD_setString("  BT Unknown Mode   ", 3, true);
                    break;
            }
            break;
        case BM78_EVENT_RECEIVED_TRANSPARENT_DATA:
        case BM78_EVENT_RECEIVED_SPP_DATA:
            break;
        default:
            LCD_setString("   BT Unknown MSG   ", 0, true);
            break;
    }
}

void POC_bm78TransparentDataHandler(uint8_t length, uint8_t *data) {
    switch(*(data)) {
        case BM78_MESSAGE_KIND_PLAIN:
            LCD_clear();
            LCD_setString("                    ", 2, false);
            for(uint8_t i = 0; i < length; i++) {
                if (i < 20) {
                    LCD_replaceChar(*(data + i), i, 2, false);
                }
            }
            LCD_setString("    BT Message:     ", 0, true);
            LCD_displayLine(2);
            break;
    }
}

void POC_bm78ErrorHandler(BM78_Response_t response, uint8_t *data) {
    uint8_t i;
    //                      11 11 11 12
    //                45 78 01 34 67 90
    LCD_setString("ER:  ,  ,           ", 3, false);
    LCD_replaceChar(dec2hex(response.op_code / 16 % 16), 3, 3, false);
    LCD_replaceChar(dec2hex(response.op_code % 16), 4, 3, false);
    LCD_replaceChar(dec2hex(response.CommandComplete_0x80.command / 16 % 16), 6, 3, false);
    LCD_replaceChar(dec2hex(response.CommandComplete_0x80.command % 16), 7, 3, false);
    LCD_replaceChar(dec2hex(response.CommandComplete_0x80.status / 16 % 16), 9, 3, false);
    LCD_replaceChar(dec2hex(response.CommandComplete_0x80.status % 16), 10, 3, false);
    LCD_displayLine(3);
}
#endif

void POC_displayData(uint16_t address, uint8_t length, uint8_t *data) {
    LCD_clear();

    for (uint8_t i = 0; i < 4; i++) {
        LCD_setString("XXXX:            ", i % 4, false);

        uint8_t byte = (address + (i * 4)) >> 8;
        LCD_replaceChar(dec2hex(byte / 16 % 16), 0, i % 4, false);
        LCD_replaceChar(dec2hex(byte % 16), 1, i % 4, false);

        byte = (address + (i * 4)) & 0xFF;
        LCD_replaceChar(dec2hex(byte / 16 % 16), 2, i % 4, false);
        LCD_replaceChar(dec2hex(byte % 16), 3, i % 4, false);

        for (uint8_t j = 0; j < 4; j++) {
            if (length >= (i * 4) + j) {
                byte = *(data + (i * 4) + j);
                LCD_replaceChar(dec2hex(byte / 16 % 16), 5 + (3 * j) + 1, i % 4, false);
                LCD_replaceChar(dec2hex(byte % 16), 5 + (3 * j) + 2, i % 4, false);
            }
        }
        LCD_displayLine(i % 4);
    }
}

#if defined I2C_ENABLED || defined MEM_INTERNAL_SIZE

void POC_testMem(uint8_t address, uint16_t reg) {
    LCD_clear();

    for (uint8_t i = 0; i < 4; i++) {
        LCD_setString("XXXX:            ", i % 4, false);

        uint8_t byte = (reg + (i * 4)) >> 8;
        LCD_replaceChar(dec2hex(byte / 16 % 16), 0, i % 4, false);
        LCD_replaceChar(dec2hex(byte % 16), 1, i % 4, false);

        byte = (reg + (i * 4)) & 0xFF;
        LCD_replaceChar(dec2hex(byte / 16 % 16), 2, i % 4, false);
        LCD_replaceChar(dec2hex(byte % 16), 3, i % 4, false);

        for (uint8_t j = 0; j < 4; j++) {
            if (address > 0) {
                byte = I2C_readRegister16(address, (reg + (i * 4) + j));
            } else {
#ifdef MEM_INTERNAL_SIZE
                byte = DATAEE_ReadByte(reg + (i * 4) + j);
#else
                byte = 0xFF;
#endif
            }
            LCD_replaceChar(dec2hex(byte / 16 % 16), 5 + (3 * j) + 1, i % 4, false);
            LCD_replaceChar(dec2hex(byte % 16), 5 + (3 * j) + 2, i % 4, false);
        }
        LCD_displayLine(i % 4);
    }
}

//void POC_testMemPage(void) {
//    uint8_t byte;
//    uint8_t data[MEM_PAGE_SIZE];
//    char message[20] = "XXY: ###\0";
//
//    LCD_clear();
//
//    for (i = 0; i < 2; i++) {
//        if (i > 0) __delay_ms(1000);
//        MEM_write_page(MEM_ADDRESS, 0x00, 0x00, 4, 1 * (i + 2), 2 * (i + 2), 3 * (i + 2), 4 * (i + 2));
//
//        MEM_read_page(MEM_ADDRESS, 0x00, 0x00, MEM_PAGE_SIZE, data);
//        for (l = 0; l < 4; l++) {
//            byte = l + 1;
//            message[0] = 48 + (byte / 10 % 10);
//            message[1] = 48 + (byte % 10);
//            message[2] = 65 + i;
//
//            byte = *(data + l);
//            message[5] = 48 + (byte / 100 % 10);
//            message[6] = 48 + (byte / 10 % 10);
//            message[7] = 48 + (byte / 1 % 10);
//            LCD_displayString(message, (l % 4));
//        }
//    }
//}

#endif

void POC_testDisplay(void) {
    LCD_init();
    LCD_setBacklight(true);
    LCD_setString("|d|100|....................\0", 0, true);
    
    LCD_setString("|c|hello world\0", 2, true);
    
    __delay_ms(1000);

    LCD_setBacklight(false);

    __delay_ms(1000);
    
    LCD_setString("|r|how are you?\0", 3, true);

    __delay_ms(500);
    LCD_setBacklight(true);
    __delay_ms(1000);

    LCD_clear();
    LCD_setString("|c|01234567890123456789012\n|l|abcdefghijklmnopqrstuvwxyz\n|r|abcdefghijklmnopqrstuvwxyz\n|r|01234567890123456789012\0", 0, true);

    __delay_ms(1000);
    
    LCD_clear();
    LCD_setString("|c|center\n|l|left\n|r|right\nEND\0", 0, true);
}

#ifdef MCP_ENABLED

void POC_showKeypad(uint8_t address, uint8_t port) {
    uint8_t byte = MCP_read_keypad_char(address, port);
    if (byte > 0x00) {
        LCD_setString("       KEY: ?       ", 1, false);
        LCD_replaceChar(byte, 12, 1, true);
    }
}

void POC_testMCP23017Input(uint8_t address) {
    LCD_setString("MCP23017: 0xXX", 0, true);
    LCD_replaceChar(dec2hex(address / 16), 12, 0, true);
    LCD_replaceChar(dec2hex(address % 16), 13, 0, true);

    for (uint8_t i = 0; i < 2; i++) {
        uint8_t byte = MCP_read(address, MCP_GPIOA + i);
        
        LCD_setString("GPIOx: 0b           ", i + 1, false);
        LCD_replaceChar('A' + 1, 4, i + 1, false);
        for (uint8_t j = 0; j < 8; j++) {
            LCD_replaceChar((byte & 0b10000000) ? '1' : '0', j + 9, i + 1, false);
            byte <<= 1;
        }
        LCD_displayLine(i + 1);
    }
}

void POC_testMCP23017Output(uint8_t address, uint8_t port) {
    uint8_t original = MCP_read(address, MCP_GPIOA + port);

    LCD_setString("MCP23017: 0xXX", 0, true);
    LCD_replaceChar(dec2hex(address / 16), 12, 0, true);
    LCD_replaceChar(dec2hex(address % 16), 13, 0, true);

    LCD_setString("GPIOx: 0b           ", 1, false);
    LCD_replaceChar('A' + 1, 4, 1, false);

    
    MCP_write(address, MCP_OLATA + port, 0b10101010);
    uint8_t byte = MCP_read(address, MCP_GPIOA + port);
    for (uint8_t i = 0; i < 8; i++) {
        LCD_replaceChar((byte & 0b10000000) ? '1' : '0', i + 9, 1, false);
        byte <<= 1;
    }
    LCD_displayLine(1);

    __delay_ms(1000);

    MCP_write(address, MCP_OLATA + port, 0b01010101);
    byte = MCP_read(address, MCP_GPIOA + port);
    for (uint8_t i = 0; i < 8; i++) {
        LCD_replaceChar((byte & 0b10000000) ? '1' : '0', i + 9, 1, false);
        byte <<= 1;
    }
    LCD_displayLine(1);
    
    __delay_ms(1000);
    
    MCP_write(address, MCP_OLATA + port, original);
}

#endif

#endif

#ifdef RGB_ENABLED
void POC_testRGB(void) {
#ifdef LCD_ADDRESS
    LCD_clear();
    LCD_setString("    White light     ", 1, true);
#endif
    RGB_set(RGB_PATTERN_LIGHT, 255, 255, 255, 500, 128, 255, RGB_INDEFINED);
    __delay_ms(1000);
#ifdef LCD_ADDRESS
    LCD_setString("    Blinking Red    ", 1, true);
#endif
    RGB_set(RGB_PATTERN_BLINK, 255, 0, 0, 500, 128, 255, RGB_INDEFINED);
    __delay_ms(2000);
#ifdef LCD_ADDRESS
    LCD_setString("   Fade-In Green    ", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_IN, 0, 255, 0, 10, 0, 255, RGB_INDEFINED);
    __delay_ms(3000);
#ifdef LCD_ADDRESS
    LCD_setString("   Fade-Out Blue    ", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_OUT, 0, 0, 255, 10, 0, 255, RGB_INDEFINED);
    __delay_ms(3000);
#ifdef LCD_ADDRESS
    LCD_setString(" Fade-Toggle Yellow ", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_TOGGLE, 255, 255, 0, 10, 0, 255, RGB_INDEFINED);
    __delay_ms(3000);
#ifdef LCD_ADDRESS
    LCD_setString(" Fade-In Magenta 1x ", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_IN, 255, 0, 255, 10, 0, 255, 1);
    __delay_ms(2000);
#ifdef LCD_ADDRESS
    LCD_setString(" Fade-In Magenta 2x ", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_IN, 128, 0, 128, 10, 0, 255, 2);
    __delay_ms(2000);
#ifdef LCD_ADDRESS
    LCD_setString("  Fade-Out Cyan 1x  ", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_OUT, 0, 255, 255, 10, 0, 255, 1);
    __delay_ms(2000);
#ifdef LCD_ADDRESS
    LCD_setString("  Fade-Out Cyan 2x  ", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_OUT, 0, 128, 128, 10, 0, 255, 2);
    __delay_ms(2000);
#ifdef LCD_ADDRESS
    LCD_setString("Fade-Toggle White 1x", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_TOGGLE, 128, 128, 128, 10, 0, 255, 1);
    __delay_ms(3000);
#ifdef LCD_ADDRESS
    LCD_setString("Fade-Toggle White 2x", 1, true);
#endif
    RGB_set(RGB_PATTERN_FADE_TOGGLE, 64, 64, 64, 10, 0, 255, 2);
    __delay_ms(2500);
    __delay_ms(2500);
}
#endif


#ifdef WS281x_BUFFER
void POC_demoWS281x(void) {
    WS281x_off();
    WS281x_set(2, WS281x_PATTERN_LIGHT,       0x80, 0x00, 0x80,   0, 0,   0);
    WS281x_set(3, WS281x_PATTERN_BLINK,       0xFF, 0x00, 0x00, 250, 0, 255);
    WS281x_set(4, WS281x_PATTERN_FADE_IN,     0x00, 0xFF, 0x00,  10, 0,  50);
    WS281x_set(5, WS281x_PATTERN_FADE_OUT,    0x00, 0x00, 0xFF,  10, 0,  50);
    WS281x_set(6, WS281x_PATTERN_FADE_TOGGLE, 0xFF, 0xFF, 0x00,  10, 0,  50);
}

void POC_testWS281x(void) {
#ifdef LCD_ADDRESS
    LCD_clear();
    LCD_setString("        RED         ", 1, true);
#endif
    WS281x_all(0xFF, 0x00, 0x00);
    WS281x_show();
    __delay_ms(500);

#ifdef LCD_ADDRESS
    LCD_setString("       GREEN        ", 1, true);
#endif
    WS281x_all(0x00, 0xFF, 0x00);
    WS281x_show();
    __delay_ms(500);

#ifdef LCD_ADDRESS
    LCD_setString("        BLUE        ", 1, true);
#endif
    WS281x_all(0x00, 0x00, 0xFF);
    WS281x_show();
    __delay_ms(500);

#ifdef LCD_ADDRESS
    LCD_clear();
#endif
    WS281x_all(0x00, 0x00, 0x00);
    
    //POC_demoWS281x();
}
#endif
