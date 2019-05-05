/* 
 * File:   poc.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "poc.h"
#include "../../config.h"
#include "../lib/common.h"
#include "../modules/bm78.h"
#include "../modules/memory.h"
#include "../modules/mcp23017.h"

#ifdef DHT11_PORT
#include "../modules/dht11.h"
#endif

#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif

#ifdef RGB_R_DUTY_CYCLE
#include "../modules/rgb.h"
#endif

#ifdef WS281x_BUFFER
#include "../modules/ws281x.h"
#endif

uint8_t POC_i, POC_j, POC_byte;

#ifdef LCD_ADDRESS

#ifdef DHT11_PORT
void POC_testDHT11(void) {
    DHT11_Result dht11 = DHT11_measure();

    LCD_init();
    LCD_setBacklight(true);
    LCD_setString("DHT11 (X retries)", 0, false);
    LCD_replaceChar(48 + dht11.retries, 7, 0, true);
    LCD_setString("--------------------", 1, true);
    

    if (dht11.status == DHT11_OK) {
        LCD_setString("Temp:      ?C\0", 2, false);
        LCD_replaceChar(48 + (dht11.temp / 10 % 10), 10, 2, false);
        LCD_replaceChar(48 + (dht11.temp % 10), 11, 2, true);

        LCD_setString("Humidity:  ?%\0", 2, false);
        LCD_replaceChar(48 + (dht11.rh / 10 % 10), 10, 2, false);
        LCD_replaceChar(48 + (dht11.rh % 10), 11, 2, true);
    } else switch(dht11.status) {
        case DHT11_ERR_CHKSUM:
            LCD_setString("|c|Checksum mismatch!", 2, true);
            break;
        case DHT11_ERR_NO_RESPONSE:
            LCD_setString("|c|No response!", 2, true);
            break;
        default:
            LCD_setString("|c|Unknown error!", 2, true);
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
void POC_bm78ResponseHandler(BM78_Response_t response, uint8_t *data) {
    switch (response.op_code) {
        case BM78_OPC_DISCONNECTION_COMPLETE:
            LCD_setString("|c|BT Disconnected", 1, true);
            break;
        case BM78_OPC_SPP_CONNECTION_COMPLETE:
            if (response.SPPConnectionComplete_0x74.status == BM78_COMMAND_SUCCEEDED) {
                LCD_setString("|c|BT Connected", 1, true);
                LCD_setString(" ##:##:##:##:##:##  ", 2, false);
                for (POC_i = 0; POC_i < 6; POC_i++) {
                    LCD_replaceChar(dec2hex(response.SPPConnectionComplete_0x74.peer_address[POC_i] / 16 % 16), POC_i * 3 + 1, 2, false);
                    LCD_replaceChar(dec2hex(response.SPPConnectionComplete_0x74.peer_address[POC_i] % 16), POC_i * 3 + 2, 2, false);
                }
                LCD_displayLine(2);
            } else {
                LCD_setString("|c|BT Conn Failed", 1, true);
            }
            break;
        case BM78_OPC_COMMAND_COMPLETE:
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
                    for (POC_i = 0; POC_i < 5; POC_i++) { // BT Address
                        LCD_replaceChar(dec2hex(data[POC_i] / 16 % 16), 3 + POC_i * 2, 1, false);
                        LCD_replaceChar(dec2hex(data[POC_i] % 16), 4 + POC_i * 2, 1, false);
                    }
                    LCD_displayLine(1);

                    LCD_setString("BT:##:##:##:##:##:##", 2, false);
                    for (POC_i = 0; POC_i < 6; POC_i++) { // BT Address
                        LCD_replaceChar(dec2hex(data[5 + POC_i] / 16 % 16), 3 + (5 - POC_i) * 3, 2, false);
                        LCD_replaceChar(dec2hex(data[5 + POC_i] % 16), 4 + (5 - POC_i) * 3, 2, false);
                    }
                    LCD_displayLine(2);
                    break;
                case BM78_CMD_READ_DEVICE_NAME:
                    LCD_setString("Device name:", 0, true);
                    LCD_setString((char *)data, 1, true);
                    break;
                case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                    LCD_clear();
                    for (POC_i = 0; POC_i < BM78.pairedDevicesCount; POC_i++) {
                        if (POC_i < 4) {
                            LCD_setString("XX:##:##:##:##:##:##", POC_i, false);
                            LCD_replaceChar(BM78.pairedDevices[POC_i].index / 10 % 10 + 48, 0, POC_i, false);
                            LCD_replaceChar(BM78.pairedDevices[POC_i].index % 10 + 48, 1, POC_i, false);
                            
                            for (POC_j = 0; POC_j < 6; POC_j++) {
                                LCD_replaceChar(dec2hex(BM78.pairedDevices[POC_i].address[POC_j] / 16 % 16), POC_j * 3 + 3, POC_i, false);
                                LCD_replaceChar(dec2hex(BM78.pairedDevices[POC_i].address[POC_j] % 16), POC_j * 3 + 4, POC_i, false);
                            }
                            LCD_displayLine(POC_i);
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
        case BM78_OPC_BM77_STATUS_REPORT:
            switch(response.StatusReport_0x81.status) {
                case BM78_STATUS_POWER_ON:
                    LCD_setString("|c|BT Power On", 3, true);
                    break;
                case BM78_STATUS_PAGE_MODE:
                    LCD_setString("|c|BT Page Mode", 3, true);
                    break;
                case BM78_STATUS_STANDBY_MODE:
                    LCD_setString("|c|BT Stand-By", 3, true);
                    break;
                case BM78_STATUS_LINK_BACK_MODE:
                    LCD_setString("|c|BT Link Back", 3, true);
                    break;
                case BM78_STATUS_SPP_CONNECTED_MODE:
                    LCD_setString("|c|BT SPP Connected", 3, true);
                    break;
                case BM78_STATUS_LE_CONNECTED_MODE:
                    LCD_setString("|c|BT LE Connected", 3, true);
                    break;
                case BM78_STATUS_IDLE_MODE:
                    LCD_setString("|c|BT Idle", 3, true);
                    break;
                case BM78_STATUS_SHUTDOWN_MODE:
                    LCD_setString("|c|BT Shut down", 3, true);
                    break;
                default:
                    LCD_setString("|c|BT Unknown Mode", 3, true);
                    break;
            }
            break;
        case BM78_OPC_RECEIVED_TRANSPARENT_DATA:
        case BM78_OPC_RECEIVED_SPP_DATA:
            break;
        default:
            LCD_setString("|c|BT Unknown MSG", 0, true);
            break;
    }
}

void POC_bm78TransparentDataHandler(uint8_t start, uint8_t length, uint8_t *data) {
    //uint8_t feedback[23];

    switch(*(data + start)) {
        case BM78_MESSAGE_KIND_PLAIN:
            //feedback[0] = 0x00;
            LCD_clear();
            LCD_setString("                    ", 2, false);
            for(POC_i = 0; POC_i < length; POC_i++) {
                if (POC_i < 20) {
                    LCD_replaceChar(*(data + start + POC_i), POC_i, 2, false);
                    //feedback[i + 1] = *(data + start + i);
                    //feedback[i + 2] = '\n';
                }
            }
            LCD_setString("|c|BT Message:", 0, true);
            LCD_displayLine(2);
            //BM78_data(BM78_CMD_SEND_TRANSPARENT_DATA, i + 2, feedback);
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

    for (POC_i = 0; POC_i < 4; POC_i++) {
        LCD_setString("XXXX:            ", POC_i % 4, false);

        POC_byte = (address + (POC_i * 4)) >> 8;
        LCD_replaceChar(dec2hex(POC_byte / 16 % 16), 0, POC_i % 4, false);
        LCD_replaceChar(dec2hex(POC_byte % 16), 1, POC_i % 4, false);

        POC_byte = (address + (POC_i * 4)) & 0xFF;
        LCD_replaceChar(dec2hex(POC_byte / 16 % 16), 2, POC_i % 4, false);
        LCD_replaceChar(dec2hex(POC_byte % 16), 3, POC_i % 4, false);

        for (POC_j = 0; POC_j < 4; POC_j++) {
            if (length >= (POC_i * 4) + POC_j) {
                POC_byte = *(data + (POC_i * 4) + POC_j);
                LCD_replaceChar(dec2hex(POC_byte / 16 % 16), 5 + (3 * POC_j) + 1, POC_i % 4, false);
                LCD_replaceChar(dec2hex(POC_byte % 16), 5 + (3 * POC_j) + 2, POC_i % 4, false);
            }
        }
        LCD_displayLine(POC_i % 4);
    }
}

#ifdef MEM_ADDRESS
void POC_testMem(uint16_t address) {
    LCD_clear();

    for (POC_i = 0; POC_i < 4; POC_i++) {
        LCD_setString("XXXX:            ", POC_i % 4, false);

        POC_byte = (address + (POC_i * 4)) >> 8;
        LCD_replaceChar(dec2hex(POC_byte / 16 % 16), 0, POC_i % 4, false);
        LCD_replaceChar(dec2hex(POC_byte % 16), 1, POC_i % 4, false);

        POC_byte = (address + (POC_i * 4)) & 0xFF;
        LCD_replaceChar(dec2hex(POC_byte / 16 % 16), 2, POC_i % 4, false);
        LCD_replaceChar(dec2hex(POC_byte % 16), 3, POC_i % 4, false);

        for (POC_j = 0; POC_j < 4; POC_j++) {
            POC_byte = MEM_read(MEM_ADDRESS,
                    (address + (POC_i * 4) + POC_j) >> 8,
                    (address + (POC_i * 4) + POC_j) & 0xFF);
            LCD_replaceChar(dec2hex(POC_byte / 16 % 16), 5 + (3 * POC_j) + 1, POC_i % 4, false);
            LCD_replaceChar(dec2hex(POC_byte % 16), 5 + (3 * POC_j) + 2, POC_i % 4, false);
        }
        LCD_displayLine(POC_i % 4);
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

void POC_showKeypad(uint8_t address, uint8_t port) {
    POC_byte = MCP_read_keypad_char(address, port);
    //if (POC_byte > 0x00) {
        //LCD_setString("       KEY: ?       ", 1, false);
        //LCD_replaceChar(POC_byte, 12, 1, true);
    //}
}

void POC_testMCP23017Input(uint8_t address) {
    LCD_setString("MCP23017: 0xXX", 0, true);
    LCD_replaceChar(dec2hex(address / 16), 12, 0, true);
    LCD_replaceChar(dec2hex(address % 16), 13, 0, true);

    for (POC_i = 0; POC_i < sizeof(MCP_GPIOS); POC_i++) {
        POC_byte = MCP_read(address, MCP_GPIOS[POC_i]);
        
        LCD_setString("GPIOx: 0b           ", POC_i + 1, false);
        LCD_replaceChar('A' + 1, 4, POC_i + 1, false);
        for (POC_j = 0; POC_j < 8; POC_j++) {
            LCD_replaceChar((POC_byte & 0b10000000) ? '1' : '0', POC_j + 9, POC_i + 1, false);
            POC_byte <<= 1;
        }
        LCD_displayLine(POC_i + 1);
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
    POC_byte = MCP_read(address, MCP_GPIOA + port);
    for (POC_i = 0; POC_i < 8; POC_i++) {
        LCD_replaceChar((POC_byte & 0b10000000) ? '1' : '0', POC_i + 9, 1, false);
        POC_byte <<= 1;
    }
    LCD_displayLine(1);

    __delay_ms(1000);

    MCP_write(address, MCP_OLATA + port, 0b01010101);
    POC_byte = MCP_read(address, MCP_GPIOA + port);
    for (POC_i = 0; POC_i < 8; POC_i++) {
        LCD_replaceChar((POC_byte & 0b10000000) ? '1' : '0', POC_i + 9, 1, false);
        POC_byte <<= 1;
    }
    LCD_displayLine(1);
    
    __delay_ms(1000);
    
    MCP_write(address, MCP_OLATA + port, original);
}

#endif

#ifdef RGB_ENABLED
void POC_testRGB(void) {
    RGB_set(RGB_PATTERN_LIGHT, 255, 255, 255, 500, 128, 255, RGB_INDEFINED);
    __delay_ms(1000);
    RGB_set(RGB_PATTERN_BLINK, 255, 0, 0, 500, 128, 255, RGB_INDEFINED);
    __delay_ms(2000);
    RGB_set(RGB_PATTERN_FADE_IN, 0, 255, 0, 10, 0, 255, RGB_INDEFINED);
    __delay_ms(3000);
    RGB_set(RGB_PATTERN_FADE_OUT, 0, 0, 255, 10, 0, 255, RGB_INDEFINED);
    __delay_ms(3000);
    RGB_set(RGB_PATTERN_FADE_TOGGLE, 255, 255, 0, 10, 0, 255, RGB_INDEFINED);
    __delay_ms(3000);
    RGB_set(RGB_PATTERN_FADE_IN, 255, 0, 255, 10, 0, 255, 1);
    __delay_ms(2000);
    RGB_set(RGB_PATTERN_FADE_IN, 128, 0, 128, 10, 0, 255, 2);
    __delay_ms(2000);
    RGB_set(RGB_PATTERN_FADE_OUT, 0, 255, 255, 10, 0, 255, 1);
    __delay_ms(2000);
    RGB_set(RGB_PATTERN_FADE_OUT, 0, 128, 128, 10, 0, 255, 2);
    __delay_ms(2000);
    RGB_set(RGB_PATTERN_FADE_TOGGLE, 128, 128, 128, 10, 0, 255, 1);
    __delay_ms(3000);
    RGB_set(RGB_PATTERN_FADE_TOGGLE, 64, 64, 64, 10, 0, 255, 2);
    __delay_ms(5000);
}
#endif


#ifdef WS281x_BUFFER
void POC_demoWS281x(void) {
    WS281x_off();
    WS281x_RGB(2, 128, 0, 128);
    WS281x_blink(3, 255, 0, 0, 250);
    WS281x_fadeIn(4, 0, 255, 0, 10, 0, 50);
    WS281x_fadeOut(5, 0, 0, 255, 10, 0, 50);
    WS281x_fadeToggle(6, 255, 255, 0, 10, 0, 50);
}

void POC_testWS281x(void) {
#ifdef LCD_ADDRESS
    LCD_init();
    LCD_setBacklight(true);
    LCD_clear();
#endif

#ifdef LCD_ADDRESS
    LCD_setString("|c|RED", 1, true);
#endif
    WS281x_all(0xFF, 0x00, 0x00);
    __delay_ms(500);

#ifdef LCD_ADDRESS
    LCD_setString("|c|GREEN", 1, true);
#endif
    WS281x_all(0x00, 0xFF, 0x00);
    __delay_ms(500);

#ifdef LCD_ADDRESS
    LCD_setString("|c|BLUE", 1, true);
#endif
    WS281x_all(0x00, 0x00, 0xFF);
    __delay_ms(500);

#ifdef LCD_ADDRESS
    LCD_clear();
#endif
    WS281x_all(0x00, 0x00, 0x00);
    
    //POC_demoWS281x();
}
#endif
