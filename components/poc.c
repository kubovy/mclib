/* 
 * File:   poc.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "poc.h"
#include "../../config.h"
#include "../lib/common.h"
#include "../modules/memory.h"
#include "../modules/mcp23017.h"

#ifdef DHT11_PORT
#include "modules/dht11.h"
#endif

#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif

#ifdef WS281x_BUFFER
#include "../modules/ws281x.h"
#endif

#ifdef DHT11_PORT
void POC_testDHT11(void) {
    DHT11_Result dht11 = DHT11_measure();
    char message[LCD_COLS + 1] = "DHT11 (X retries)";
    message[7] = 48 + dht11.retries;

    LCD_init();
    LCD_setBacklight(true);

    LCD_displayString(message, 0);
    LCD_displayString("--------------------", 2);
    

    if (dht11.status == DHT11_OK) {
        char message1[LCD_COLS + 1] = "Temp:      ?C\0";
        message1[10] = 48 + ((dht11.temp / 10) % 10);
        message1[11] = 48 + (dht11.temp % 10);
        LCD_displayString(message1, 2);

        char message2[LCD_COLS + 1] = "Humidity:  ?%\0";
        message2[10] = 48 + ((dht11.rh / 10) % 10);
        message2[11] = 48 + (dht11.rh % 10);
        LCD_displayString(message2, 3);
    } else switch(dht11.status) {
        case DHT11_ERR_CHKSUM:
            LCD_displayString("|c|Checksum mismatch!", 2);
            break;
        case DHT11_ERR_NO_RESPONSE:
            LCD_displayString("|c|No response!", 2);
            break;
        default:
            LCD_displayString("|c|Unknown error!", 2);
            break;
    }
}
#endif

#ifdef LCD_ADDRESS
void POC_bm78InitializationHandler(char *deviceName, char *pin) {
    LCD_displayString("Device name:", 0);
    LCD_displayString(deviceName, 1);
    LCD_displayString("PIN:", 2);
    LCD_displayString(pin, 3);
}

void POC_bm78ResponseHandler(BM78_Response_t response, uint8_t *data) {
    uint16_t i;
    //                                     11 11 11 12
    //                               45 78 01 34 67 90
    char message[LCD_COLS + 1] = "                    \0";

    switch (response.op_code) {
        case BM78_OPC_DISCONNECTION_COMPLETE:
            LCD_displayString("|c|BT Disconnected", 1);
            break;
        case BM78_OPC_SPP_CONNECTION_COMPLETE:
            if (response.SPPConnectionComplete_0x74.status == BM78_COMMAND_SUCCEEDED) {
                LCD_displayString("|c|BT Connected", 1);
                message[0] = '|';
                message[1] = 'c';
                message[2] = '|';
                for (i = 0; i < 6; i++) {
                    message[i + 3] = dec2hex(response.SPPConnectionComplete_0x74.peer_address[i]);
                }
            } else {
                LCD_displayString("|c|BT Conn Failed", 1);
            }
            break;
        case BM78_OPC_COMMAND_COMPLETE:
            switch(response.CommandComplete_0x80.command) {
                case BM78_CMD_READ_LOCAL_INFORMATION:
                    message[0] = 'S';
                    message[1] = 'T';
                    message[2] = ':';
                    message[3] = dec2hex(response.op_code / 16 % 16);
                    message[4] = dec2hex(response.op_code % 16);
                    message[5] = ',';
                    message[6] = dec2hex(response.CommandComplete_0x80.command / 16 % 16);
                    message[7] = dec2hex(response.CommandComplete_0x80.command % 16);
                    message[8] = ',';
                    message[9] = dec2hex(response.CommandComplete_0x80.status / 16 % 16);
                    message[10] = dec2hex(response.CommandComplete_0x80.status % 16);
                    for (i = 11; i < 20; i++) message[i] = ' ';
                    LCD_displayString(message, 0);

                    message[0] = 'V';
                    message[1] = ' ';
                    for (i = 0; i < 5; i++) { // BT Address
                        message[3 + i * 2] = dec2hex(data[i] / 16 % 16);
                        message[4 + i * 2] = dec2hex(data[i] % 16);
                    }
                    for (i = 12; i < 20; i++) message[i] = ' ';
                    LCD_displayString(message, 1);

                    message[0] = 'B';
                    message[1] = 'T';
                    for (i = 0; i < 6; i++) { // BT Address
                        message[2 + (5-i) * 3] = ':';
                        message[3 + (5-i) * 3] = dec2hex(data[5 + i] / 16 % 16);
                        message[4 + (5-i) * 3] = dec2hex(data[5 + i] % 16);
                    }
                    LCD_displayString(message, 2);
                    __delay_ms(1000);
                    break;
                case BM78_CMD_READ_DEVICE_NAME:
                    LCD_displayString("Device name:", 0);
                    LCD_displayString((char *)data, 1);
                    break;
                case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                    LCD_clear();
                    for (i = 0; i < BM78.pairedDevicesCount; i++) {
                        if (i < 4) {
                            message[0] = BM78.pairedDevices[i].index / 10 % 10 + 48;
                            message[1] = BM78.pairedDevices[i].index % 10 + 48;
                            
                            for (uint8_t j = 0; j < 6; j++) {
                                if (j > 0) message[j * 3 + 2] = ':';
                                message[j * 3 + 3] = dec2hex(BM78.pairedDevices[i].address[j] / 16 % 16);
                                message[j * 3 + 4] = dec2hex(BM78.pairedDevices[i].address[j] % 16);
                            }
                            LCD_displayString(message, i);
                        }
                    }
                    break;
                case BM78_CMD_READ_PIN_CODE:
                    LCD_displayString("PIN:", 0);
                    LCD_displayString((char *) data, 1);
                    break;
                default:
                    break;
            }
            break;
        case BM78_OPC_BM77_STATUS_REPORT:
            switch(response.StatusReport_0x81.status) {
                case BM78_STATUS_POWER_ON:
                    LCD_displayString("|c|BT Power On", 3);
                    break;
                case BM78_STATUS_PAGE_MODE:
                    LCD_displayString("|c|BT Page Mode", 3);
                    break;
                case BM78_STATUS_STANDBY_MODE:
                    LCD_displayString("|c|BT Stand-By", 3);
                    break;
                case BM78_STATUS_LINK_BACK_MODE:
                    LCD_displayString("|c|BT Link Back", 3);
                    break;
                case BM78_STATUS_SPP_CONNECTED_MODE:
                    LCD_displayString("|c|BT SPP Connected", 3);
                    break;
                case BM78_STATUS_LE_CONNECTED_MODE:
                    LCD_displayString("|c|BT LE Connected", 3);
                    break;
                case BM78_STATUS_IDLE_MODE:
                    LCD_displayString("|c|BT Idle", 3);
                    break;
                case BM78_STATUS_SHUTDOWN_MODE:
                    LCD_displayString("|c|BT Shut down", 3);
                    break;
                default:
                    LCD_displayString("|c|BT Unknown Mode", 3);
                    break;
            }
            break;
        case BM78_OPC_RECEIVED_TRANSPARENT_DATA:
        case BM78_OPC_RECEIVED_SPP_DATA:
            break;
        default:
            LCD_displayString("|c|BT Unknown MSG", 0);
            break;
    }
}

void POC_bm78TransparentDataHandler(uint8_t start, uint8_t length, uint8_t *data) {
    uint8_t i;
    char message[LCD_COLS + 1] = "                    \0";
    uint8_t feedback[23];

    switch(*(data + start)) {
        case BM78_MESSAGE_KIND_PLAIN:
            feedback[0] = 0x00;
            LCD_clear();
            for(i = 0; i < length; i++) {
                if (i < 20) {
                    message[i] = *(data + start + i);
                    feedback[i + 1] = *(data + start + i);
                    feedback[i + 2] = '\n';
                }
            }
            LCD_displayString("|c|BT Message:", 0);
            LCD_displayString(message, 2);
            BM78_data(BM78_CMD_SEND_TRANSPARENT_DATA, i + 2, feedback);
            break;
    }
}

void POC_bm78ErrorHandler(BM78_Response_t response, uint8_t *data) {
    uint8_t i;
    //                                     11 11 11 12
    //                               45 78 01 34 67 90
    char message[LCD_COLS + 1] = "  :                 \0";

    message[0] = 'E';
    message[1] = 'R';
    message[3] = dec2hex(response.op_code / 16 % 16);
    message[4] = dec2hex(response.op_code % 16);
    message[5] = ',';
    message[6] = dec2hex(response.CommandComplete_0x80.command / 16 % 16);
    message[7] = dec2hex(response.CommandComplete_0x80.command % 16);
    message[8] = ',';
    message[9] = dec2hex(response.CommandComplete_0x80.status / 16 % 16);
    message[10] = dec2hex(response.CommandComplete_0x80.status % 16);
    for (i = 11; i < 20; i++) message[i] = ' ';
    LCD_displayString(message, 3);
}

void POC_displayData(uint16_t address, uint8_t length, uint8_t *data) {
    uint8_t i, j, regHigh, regLow, byte;
    char message[20] = "XXXX: ## ## ## ##\0";
    
    LCD_clear();

    for (i = 0; i < 4; i++) {
        regHigh = (address + (i * 4)) >> 8;
        message[0] = dec2hex(regHigh / 16 % 16);
        message[1] = dec2hex(regHigh % 16);

        regLow = (address + (i * 4)) & 0xFF;
        message[2] = dec2hex(regLow / 16 % 16);
        message[3] = dec2hex(regLow % 16);

        for (j = 0; j < 4; j++) {
            if (length >= (i * 4) + j) {
                byte = *(data + (i * 4) + j);
                message[5 + (3 * j) + 1] = dec2hex(byte / 16 % 16);
                message[5 + (3 * j) + 2] = dec2hex(byte % 16);
            } else {
                message[5 + (3 * j) + 1] = ' ';
                message[5 + (3 * j) + 2] = ' ';
            }
        }

        LCD_displayString(message, (i % 4));
    }
}

#ifdef MEM_ADDRESS
void POC_testMem(uint16_t address) {
    uint8_t data[16];
    uint8_t i, j, regHigh, regLow;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            regHigh = (address + (i * 4) + j) >> 8;
            regLow = (address + (i * 4) + j) & 0xFF;
            data[(i * 4) + j] = MEM_read(MEM_ADDRESS, regHigh, regLow);
        }
    }
    POC_displayData(address, 16, data);
}

void POC_testMemPage(void) {
    uint8_t i, l, byte;
    uint8_t data[MEM_PAGE_SIZE];
    char message[20] = "XXY: ###\0";

    LCD_clear();

    for (i = 0; i < 2; i++) {
        if (i > 0) __delay_ms(1000);
        MEM_write_page(MEM_ADDRESS, 0x00, 0x00, 4, 1 * (i + 2), 2 * (i + 2), 3 * (i + 2), 4 * (i + 2));

        MEM_read_page(MEM_ADDRESS, 0x00, 0x00, MEM_PAGE_SIZE, data);
        for (l = 0; l < 4; l++) {
            byte = l + 1;
            message[0] = 48 + (byte / 10 % 10);
            message[1] = 48 + (byte % 10);
            message[2] = 65 + i;

            byte = *(data + l);
            message[5] = 48 + (byte / 100 % 10);
            message[6] = 48 + (byte / 10 % 10);
            message[7] = 48 + (byte / 1 % 10);
            LCD_displayString(message, (l % 4));
        }
    }
}
#endif

void POC_testDisplay(void) {
    LED1_LAT = 0;
    LED2_LAT = 0;
    LED3_LAT = 0;
    LED4_LAT = 0;

    LCD_init();
    LCD_setBacklight(true);
    LCD_displayString("|d|100|....................\0", 0);
    
    LCD_displayString("|c|hello world\0", 2);
    
    __delay_ms(1000);

    LCD_setBacklight(false);

    __delay_ms(1000);
    
    LCD_displayString("|r|how are you?\0", 3);

    LED4_LAT = 1;

    __delay_ms(500);
    LCD_setBacklight(true);
    __delay_ms(1000);

    LCD_clear();
    LCD_displayString("|c|01234567890123456789012\n|l|abcdefghijklmnopqrstuvwxyz\n|r|abcdefghijklmnopqrstuvwxyz\n|r|01234567890123456789012\0", 0);

    __delay_ms(1000);
    
    LCD_clear();
    LCD_displayString("|c|center\n|l|left\n|r|right\nEND\0", 0);
}

void POC_showKeypad(uint8_t address, uint8_t port) {
    char message[LCD_COLS + 1] = "|c|KEY: ?\0";
    message[8] = MCP_read_keypad_char(address, port);
    if (message[8] > 0x00) {
        LCD_displayString(message, 1);
    }
}

void POC_testMCP23017Input(uint8_t address) {
    LCD_displayString("MCP23017: 0xXX", 0);
    LCD_replaceChar(dec2hex(address / 16), 12, 0);
    LCD_replaceChar(dec2hex(address % 16), 13, 0);

    for (uint8_t i = 0; i < sizeof(MCP_GPIOS); i++) {
        uint8_t byte = MCP_read(address, MCP_GPIOS[i]);
        
        char message[LCD_COLS + 1] = "GPIOx: 0b";
        message[4] = 'A' + i;
        for (uint8_t i = 0; i < 8; i++) {
            message[i + 9] = (byte & 0b10000000) ? '1' : '0';
            byte <<= 1;
        }
        message[9 + 8] = '\0';
        LCD_displayString(message, i + 1);
    }
}

void POC_testMCP23017Output(uint8_t address, uint8_t port) {
    uint8_t original = MCP_read(address, MCP_GPIOA + port);

    LCD_displayString("MCP23017: 0xXX", 0);
    LCD_replaceChar(dec2hex(address / 16), 12, 0);
    LCD_replaceChar(dec2hex(address % 16), 13, 0);

    char message[LCD_COLS + 1] = "GPIOx: ";
    message[4] = 'A' + port;

    
    MCP_write(address, MCP_OLATA + port, 0b10101010);
    uint8_t byte = MCP_read(address, MCP_GPIOA + port);
    for (uint8_t i = 0; i < 8; i++) {
        message[i + 7] = (byte & 0b10000000) ? '1' : '0';
        byte <<= 1;
    }
    message[7 + 8] = '\0';
    LCD_displayString(message, 1);

    __delay_ms(1000);

    MCP_write(address, MCP_OLATA + port, 0b01010101);
    byte = MCP_read(address, MCP_GPIOA + port);
    for (uint8_t i = 0; i < 8; i++) {
        message[i + 7] = (byte & 0b10000000) ? '1' : '0';
        byte <<= 1;
    }
    message[7 + 8] = '\0';
    LCD_displayString(message, 1);
    
    __delay_ms(1000);
    
    MCP_write(address, MCP_OLATA + port, original);
}

#endif

#ifdef WS281x_BUFFER
void POC_demoWS281x(void) {
    WS281x_off();
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

    LED4_LAT = 1;
#ifdef LCD_ADDRESS
    LCD_displayString("|c|RED", 1);
#endif
    WS281x_all(0xFF, 0x00, 0x00);
    WS281x_show();
    __delay_ms(500);

    LED3_LAT = 1;
#ifdef LCD_ADDRESS
    LCD_displayString("|c|GREEN", 1);
#endif
    WS281x_all(0x00, 0xFF, 0x00);
    WS281x_show();
    __delay_ms(500);

    LED2_LAT = 1;
#ifdef LCD_ADDRESS
    LCD_displayString("|c|BLUE", 1);
#endif
    WS281x_all(0x00, 0x00, 0xFF);
    WS281x_show();
    __delay_ms(500);

    LED1_LAT = 1;
#ifdef LCD_ADDRESS
    LCD_clear();
#endif
    WS281x_all(0x00, 0x00, 0x00);
    WS281x_show();
    
    //POC_demoWS281x();
}
#endif
