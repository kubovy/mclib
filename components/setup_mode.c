/* 
 * File:   setup_mode.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "setup_mode.h"

struct {
    uint8_t timeout;
    uint8_t stage;
    uint8_t retries;
    uint8_t index;
} SUM_bm78ConfigCommand = { 0xFF, 0, 0, 0 };

struct {
    uint8_t addr;
    uint16_t reg;
} SUM_mem = { 0x00, 0x0000 };

#ifdef LCD_ADDRESS

uint8_t SUM_setupModePush = 5;
uint8_t SUM_setupPage = SUM_MENU_MAIN;

union {
    struct {
        bool ws281xDemo   :1;
        bool lcdBacklight :1;
        bool showKeypad   :1;
        bool showBtErrors :1;
    };
} sw = {false, true, false, false};

struct {
    uint8_t id;
    char name[10]; // = "         \0";
    
} SUM_bm78CurrentState = { 0x00 };

#ifndef MEM_SUM_LCD_CACHE_START
char lcdCache[LCD_ROWS][LCD_COLS + 1];
#endif

#ifdef MCP_ENABLED
struct {
    uint8_t addr;
    uint8_t port;
} SUM_mcpTest = { MCP_START_ADDRESS, MCP_PORTA };
#endif

struct {
    uint8_t position;
    uint16_t address;
    char string[6];
} SUM_manual = { 0xFF, 0x0000 };

struct {
    uint8_t selected; // Selected device index.
    uint8_t page;
} SUM_pairedDevices = {0, 0};

#ifdef BM78_ENABLED

void SUM_setBtStatus(uint8_t status) {
    switch(status) {
        case BM78_STATUS_POWER_ON:
            strcpy(SUM_bm78CurrentState.name, "POWER ON ", 9);
            break;
        case BM78_STATUS_PAGE_MODE:
            strcpy(SUM_bm78CurrentState.name, "PAGE MODE", 9);
            break;
        case BM78_STATUS_STANDBY_MODE:
            strcpy(SUM_bm78CurrentState.name, "STAND BY ", 9);
            break;
        case BM78_STATUS_LINK_BACK_MODE:
            strcpy(SUM_bm78CurrentState.name, "LINK BACK", 9);
            break;
        case BM78_STATUS_SPP_CONNECTED_MODE:
            strcpy(SUM_bm78CurrentState.name, "SPP CONN ", 9);
            break;
        case BM78_STATUS_LE_CONNECTED_MODE:
            strcpy(SUM_bm78CurrentState.name, "LE CONN  ", 9);
            break;
        case BM78_STATUS_IDLE_MODE:
            strcpy(SUM_bm78CurrentState.name, "IDLE     ", 9);
            break;
        case BM78_STATUS_SHUTDOWN_MODE:
            strcpy(SUM_bm78CurrentState.name, "SHUT DOWN", 9);
            break;
        default:
            strcpy(SUM_bm78CurrentState.name, "0x##     ", 9);
            SUM_bm78CurrentState.name[2] = dec2hex(status / 16 % 16);
            SUM_bm78CurrentState.name[3] = dec2hex(status % 16);
            break;
    }
}

void SUM_displayHWAddress(uint8_t index, uint8_t line, uint8_t start) {
    if (index < BM78.pairedDevicesCount) {
        for (uint8_t i = 0; i < 6; i++) {
            LCD_replaceChar(dec2hex(BM78.pairedDevices[index].address[5 - i] / 16 % 16), i * 3 + start, line, false);
            LCD_replaceChar(dec2hex(BM78.pairedDevices[index].address[5 - i] % 16), i * 3 + start + 1, line, false);
            if (i > 0) LCD_replaceChar(':', i * 3 + start + 2, line, false);
        }
    }
}

#endif

#ifdef MCP_ENABLED
void SUM_mcpChanged(uint8_t address) {
    if (SUM_mode && SUM_setupPage == SUM_MENU_TEST_MCP_IN) {
        SUM_mcpTest.addr = address;
        POC_testMCP23017Input(SUM_mcpTest.addr);
    }
}
#endif

void SUM_showMenu(uint8_t page) {
    SUM_setupPage = page;
    LCD_clear();
    switch (SUM_setupPage) {
//        case SUM_MENU_INTRO: // Introduction
//            LCD_setString("     SetUp Mode     ", 0, true);
//            LCD_setString("     Activated      ", 1, true);
//            LCD_setString("          C) Confirm", 3, true);
//            break;
        case SUM_MENU_MAIN: // Main Menu
#ifdef BM78_ENABLED
            LCD_setString("1) Bluetooth", 0, true);
#else
            LCD_setString("-) Bluetooth", 0, true);
#endif
#if defined MEM_ADDRESS || defined MEM_INTERNAL_SIZE
            LCD_setString("2) Memory", 1, true);
#else
            LCD_setString("-) Memory", 1, true);
#endif
            LCD_setString("3) Tests", 2, true);
            LCD_setString("             Exit (D", 3, true);
            break;
#ifdef BM78_ENABLED
        case SUM_MENU_BT_PAGE_1: // Bluetooth Menu (Page 1)
            SUM_setBtStatus(SUM_bm78CurrentState.id);
            LCD_setString("1) State [         ]", 0, false);
            LCD_replaceString(SUM_bm78CurrentState.name, 10, 0, true);
            LCD_setString("2) Paired Devices   ", 1, true);
            LCD_setString("3) Pairing Mode     ", 2, true);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_BT_PAGE_2: // Bluetooth Menu (Page 2)
            LCD_setString("1) Device Name      ", 0, true);
            LCD_setString("2) PIN   [      ]   ", 1, false);
            LCD_replaceString(BM78.pin, 10, 1, true);
            if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE 
                    || BM78.status == BM78_STATUS_LE_CONNECTED_MODE) {
                LCD_setString("3) Remote Device    ", 2, true);
            }
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_BT_PAGE_3: // Bluetooth Menu (Page 3)
            LCD_setString("1) Show Errors   [ ]", 0, false);
            LCD_replaceChar(sw.showBtErrors ? 'X' : ' ', 18, 0, true);
            LCD_setString("2) Read Config      ", 1, true);
            LCD_setString("3) MAC Address      ", 2, true);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_BT_PAGE_4: // Bluetooth Menu (Page 4)
            LCD_setString("1) Initialize BM78  ", 0, true);
            LCD_setString("B) Back             ", 3, true);
            break;
        case SUM_MENU_BT_STATE: // Bluetooth: State
            LCD_setString("State:          ", 0, false);
            LCD_replaceString(SUM_bm78CurrentState.name, 7, 0, true);
            if (BM78.status== BM78_STATUS_IDLE_MODE) {
                LCD_setString("1) StandBy/ Trust (2", 1, true);
                LCD_setString("3) Beacon / Trust (4", 2, true);
            } else if (BM78.status == BM78_STATUS_STANDBY_MODE) {
                LCD_setString("1) Leave Stand-By   ", 1, true);
            } else if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
                LCD_setString("1) Disconnect SPP   ", 1, true);
            } else if (BM78.status == BM78_STATUS_LE_CONNECTED_MODE) {
                LCD_setString("1) Disconnect LE    ", 1, true);
            }
            LCD_setString("B) Back             ", 3, true);
            break;
        case SUM_MENU_BT_PAIRED_DEVICES: // Bluetooth: Paired Devices
            if (BM78.pairedDevicesCount == 0) { // No paired devices yet
                LCD_setString("*: Get devices      ", 0, true);
                LCD_setString("#: Clear all paired ", 1, true);
                LCD_setString("B) Back             ", 3, true);
            } else { // Paired devices retrieved
                for (uint8_t i = 0; i < 3; i++) {
                    if ((SUM_pairedDevices.page * 3 + i) < BM78.pairedDevicesCount) {
                        LCD_setString("-) ##:##:##:##:##:##", i, false);
                        LCD_replaceChar(i + 49, 0, i, false);
                        SUM_displayHWAddress(SUM_pairedDevices.page * 3 + i, i, 3);
                        LCD_displayLine(i);
                    }
                }
                if ((SUM_pairedDevices.page + 1) * 3 < BM78.pairedDevicesCount) {
                    LCD_setString("B) Back      Next (C", 3, true);
                } else {
                    LCD_setString("B) Back             ", 3, true);
                }
            }
            break;
        case SUM_MENU_BT_PAIRED_DEVICE: {
                LCD_setString(" ##:##:##:##:##:##  ", 0, false);
                SUM_displayHWAddress(SUM_pairedDevices.selected, 0, 1);
                LCD_displayLine(0);
                LCD_setString("1) Remove Device    ", 1, true);
                LCD_setString("B) Back             ", 3, true);
            }
            break;
        case SUM_MENU_BT_PAIRING_MODE:
            LCD_setString(" ) PIN              ", 0, true);
            LCD_replaceChar(BM78.pairingMode == BM78_PAIRING_PIN ? 'X' : '1', 0, 0, true);
            LCD_setString(" ) Just Work        ", 1, false);
            LCD_replaceChar(BM78.pairingMode == BM78_PAIRING_JUST_WORK ? 'X' : '2', 0, 1, true);
            LCD_setString(" ) Passkey          ", 2, false);
            LCD_replaceChar(BM78.pairingMode == BM78_PAIRING_PASSKEY ? 'X' : '3', 0, 2, true);
            LCD_setString(" ) Confirm   Back (B", 3, false);
            LCD_replaceChar(BM78.pairingMode == BM78_PAIRING_USER_CONFIRM ? 'X' : '4', 0, 3, true);
            break;
        case SUM_MENU_BT_SHOW_DEVICE_NAME:
            LCD_setString(BM78.deviceName, 0, true);
            LCD_setString("1) Refresh          ", 2, true);
            LCD_setString("B) Back             ", 3, true);
            break;
        case SUM_MENU_BT_PIN_SETUP:
            LCD_setString("        PIN         ", 0, true);
            LCD_setString("                    ", 1, false);
            LCD_replaceString(BM78.pin, 7, 1, true);
            LCD_setString("1) Change           ", 2, true);
            LCD_setString("2) Refresh   Back (B", 3, true);
            break;
        case SUM_MENU_BT_SHOW_REMOTE_DEVICE:
            LCD_setString("1) Refresh          ", 2, true);
            LCD_setString("B) Back             ", 3, true);
            break;
        case SUM_MENU_BT_INITIALIZE:
            LCD_setString("  Initializing BT   ", 1, true);
            LCD_setString("  (please wait...)  ", 2, true);
            break;
        case SUM_MENU_BT_READ_CONFIG:
            LCD_setString("Memory controls:    ", 0, true);
            LCD_setString("B: Previous, C: Next", 1, true);
            LCD_setString("A: Abort, D: Reset  ", 2, true);
            LCD_setString("#: Address  Con't (C", 3, true);
            break;
        case SUM_MENU_BT_SHOW_MAC_ADDRESS:
            LCD_setString("  Bluetooth's MAC   ", 0, true);
            LCD_setString("        ...         ", 1, true);
            LCD_setString("   (please wait)    ", 1, true);
            LCD_setString("B) Back   Refresh (1", 3, true);
            break;
#endif
#if defined MEM_ADDRESS || MEM_INTERNAL_SIZE
        case SUM_MENU_MEM_MAIN: // Memory Menu
#ifdef MEM_INTERNAL_SIZE
            LCD_setString("1) PIC Memory Viewer", 0, true);
#else
            LCD_setString("-) PIC Memory Viewer", 0, true);
#endif
#ifdef MEM_ADDRESS
            LCD_setString("2) Memory Viewer    ", 1, true);
#else
            LCD_setString("-) Memory Viewer    ", 1, true);
#endif
#ifdef SM_MEM_ADDRESS
            if (I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START) == SM_STATUS_ENABLED) {
                LCD_setString("3) Lock SM       [ ]", 2, true);
            } else {
                LCD_setString("3) Unlock SM     [X]", 2, true);
            }
#endif
            LCD_setString("B) Back             ", 3, true);
            break;
        case SUM_MENU_MEM_VIEWER_INTRO: // Memory Viewer Introduction
            LCD_setString("Memory controls:    ", 0, true);
            LCD_setString("B: Previous, C: Next", 1, true);
            LCD_setString("A: Abort, D: Reset  ", 2, true);
            LCD_setString("#: Address  Con't (C", 3, true);
            break;
#endif
        case SUM_MENU_TEST_PAGE_1:
#ifdef DHT11_PORT
            LCD_setString("1) DHT11 Test       ", 0, true);
#else
            LCD_setString("-) DHT11 Test       ", 0, true);
#endif
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_TEST_PAGE_2: // Tests Menu (Page 1)
            LCD_setString("1) LCD Test         ", 0, true);
            LCD_setString("2) LCD Backlight [ ]", 1, false);
            LCD_replaceChar(sw.lcdBacklight ? 'X' : ' ', 18, 1, true);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_TEST_PAGE_3: // Tests Menu (Page 2)
#ifdef MCP_ENABLED
            LCD_setString("1) MCP23017 IN Test ", 0, true);
            LCD_setString("2) MCP23017 OUT Test", 1, true);
#else
            LCD_setString("-) MCP23017 IN Test ", 0, true);
            LCD_setString("-) MCP23017 OUT Test", 1, true);
#endif
            LCD_setString("3) Show Keypad   [ ]", 2, false);
            LCD_replaceChar(sw.showKeypad ? 'X' : ' ', 18, 2, true);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_TEST_PAGE_4: // Tests Menu (Page 3)
#ifdef RGB_ENABLED
            LCD_setString("1) RGB Test         ", 0, true);
#else
            LCD_setString("-) RGB Test         ", 0, true);
#endif
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_TEST_PAGE_5: // Tests Menu (Page 4)
#ifdef WS281x_BUFFER
            LCD_setString("1) WS281x Test      ", 0, true);
            LCD_setString("2) WS281x Demo   [ ]", 1, false);
            LCD_replaceChar(sw.ws281xDemo ? 'X' : ' ', 18, 1, true);
#else
            LCD_setString("-) WS281x Test      ", 0, true);
            LCD_setString("-) WS281x Demo      ", 1, true);
#endif
            LCD_setString("B) Back             ", 3, true);
            break;
#ifdef DHT11_PORT
        case SUM_MENU_TEST_DHT11:
            POC_testDHT11();
            LCD_setString("B) Back   Refresh (C", 3, true);
            break;
#endif
#ifdef MCP_ENABLED
        case SUM_MENU_TEST_MCP_IN:
            POC_testMCP23017Input(SUM_mcpTest.addr);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_TEST_MCP_OUT:
            if (SUM_mcpTest.addr == 0) {
                LCD_setString("0) 0x20      0x24 (4", 0, true);
                LCD_setString("1) 0x21      0x25 (5", 1, true);
                LCD_setString("2) 0x22      0x26 (6", 2, true);
                LCD_setString("3) 0x23      0x27 (7", 3, true);
            } else if (SUM_mcpTest.port == 0xFF) {
                LCD_setString("A) Port A           ", 0, true);
                LCD_setString("B) Port B           ", 1, true);
            } else {
                POC_testMCP23017Output(SUM_mcpTest.addr, SUM_mcpTest.port);
                LCD_setString("B) Back    Repeat (C", 3, true);
            }
            break;
#endif
    }
}

void SUM_defaultFunction(uint8_t key) {
    switch(key) {
        case '*':
            LCD_init();
            LCD_setBacklight(true);
            SUM_showMenu(SUM_setupPage);
            break;
        case '#':
            SUM_showMenu(SUM_setupPage);
            break;
        case '0':
            SUM_showMenu(SUM_MENU_MAIN);
            break;
    }
}

void SUM_executeMenu(uint8_t key) {
#ifdef SM_MEM_ADDRESS
    uint8_t byte;
#endif
    switch (SUM_setupPage) {
//        case SUM_MENU_INTRO: // Introduction
//            switch(key) {
//                case 'C': // Confirm and goto Main Menu
//                    SUM_showMenu(SUM_MENU_MAIN);
//                    break;
//            }
//            break;
        case SUM_MENU_MAIN: // Main Menu
            switch(key) {
#ifdef BM78_ENABLED
                case '1': // Goto Bluetooth Menu (Page 1)
                    SUM_showMenu(SUM_MENU_BT_PAGE_1);
                    BM78_execute(BM78_CMD_READ_STATUS, 0);
                    break;
#endif
#if defined MEM_ADDRESS || defined MEM_INTERNAL_SIZE
                case '2': // Goto Memory Menu
                    SUM_showMenu(SUM_MENU_MEM_MAIN);
                    break;
#endif
                case '3': // Goto Tests Menu (Page 1)
                    SUM_showMenu(SUM_MENU_TEST_PAGE_1);
                    break;
                case 'D': // Exit
                    SUM_mode = false;
                    LCD_clear();
#ifdef MEM_SUM_LCD_CACHE_START
                    for (SUM_i = 0; SUM_i < LCD_ROWS; SUM_i++) {
                        for (SUM_j = 0; SUM_j < LCD_COLS; SUM_j++) {
                            SUM_mem.reg = MEM_SUM_LCD_CACHE_START + (SUM_i * LCD_COLS) + SUM_j;
                            LCD_setCache(SUM_i, SUM_j, I2C_readRegister16(MEM_ADDRESS, SUM_mem.reg));
                        }
                    }
                    LCD_displayCache();
#else
                    for (uint8_t i = 0; i < LCD_ROWS; i++) {
                        LCD_setString(lcdCache[i], i, true);
                    }
#endif
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
#ifdef BM78_ENABLED
        case SUM_MENU_BT_PAGE_1: // Bluetooth Menu (Page 1)
            switch (key) {
                case '1': // Goto Bluetooth State
                    SUM_showMenu(SUM_MENU_BT_STATE);
                    break;
                case '2': // Goto Paired Devices
                    BM78.pairedDevicesCount = 0;
                    SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICES);
                    break;
                case '3': // Goto Pairing Mode
                    BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
                    SUM_showMenu(SUM_MENU_BT_PAIRING_MODE);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_MAIN);
                    break;
                case 'C': // Next
                    SUM_showMenu(SUM_MENU_BT_PAGE_2);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_PAGE_2: // Bluetooth Menu (Page 2)
            switch (key) {
                case '1': // Show Bluetooth Device Name
                    BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                    SUM_showMenu(SUM_MENU_BT_SHOW_DEVICE_NAME);
                    break;
                case '2': // Goto Bluetooth PIN setup
                    SUM_showMenu(SUM_MENU_BT_PIN_SETUP);
                    break;
                case '3': // Read Remote Device
                    SUM_showMenu(SUM_MENU_BT_SHOW_REMOTE_DEVICE);
                    BM78_execute(BM78_CMD_READ_REMOTE_DEVICE_NAME, 0);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_BT_PAGE_1);
                    break;
                case 'C': // Next
                    SUM_showMenu(SUM_MENU_BT_PAGE_3);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_PAGE_3: // Bluetooth Menu (Page 3)
            switch(key) {
                case '1': // Show Bluetooth Errors
                    sw.showBtErrors = !sw.showBtErrors;
                    SUM_showMenu(SUM_MENU_BT_PAGE_3);
                    break;
                case '2': // Read configuration
                    SUM_showMenu(SUM_MENU_BT_READ_CONFIG);
                    break;
                case '3':
                    SUM_showMenu(SUM_MENU_BT_SHOW_MAC_ADDRESS);
                    BM78_execute(BM78_CMD_READ_LOCAL_INFORMATION, 0);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_BT_PAGE_2);
                    break;
                case 'C': // Next
                    SUM_showMenu(SUM_MENU_BT_PAGE_4);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_PAGE_4: // Bluetooth Menu (Page 4)
            switch(key) {
                case '1': // Initialize Bluetooth Dongle
                    SUM_showMenu(SUM_MENU_BT_INITIALIZE);
                    printProgress("  Initializing BT   ", 0,
                            BM78_CONFIGURATION_SIZE + 3);
                    SUM_bm78InitializeDongle();
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_BT_PAGE_3);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_STATE:
            switch(key) {
                case '1': // Enter/Leave Stand-By Mode or Disconnect
                    if (BM78.status == BM78_STATUS_IDLE_MODE) {
                        BM78.enforceState = BM78_STANDBY_MODE_ENTER;
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                    } else if (BM78.status == BM78_STATUS_STANDBY_MODE) {
                        BM78.enforceState = BM78_STANDBY_MODE_LEAVE;
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_LEAVE);
                    } else if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
                        BM78_execute(BM78_CMD_DISCONNECT, 1, 0x00);
                    } else if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
                        BM78_execute(BM78_CMD_DISCONNECT, 1, 0x00);
                    }
                    break;
                case '2':
                    if (BM78.status == BM78_STATUS_IDLE_MODE) {
                        BM78.enforceState = BM78_STANDBY_MODE_ENTER_ONLY_TRUSTED;
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER_ONLY_TRUSTED);
                    }
                    break;
                case '3':
                    if (BM78.status == BM78_STATUS_IDLE_MODE) {
                        BM78.enforceState = BM78_STANDBY_BEACON_MODE_ENTER;
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_BEACON_MODE_ENTER);
                    }
                    break;
                case '4':
                    if (BM78.status == BM78_STATUS_IDLE_MODE) {
                        BM78.enforceState = BM78_STANDBY_BEACON_MODE_ENTER_ONLY_TRUSTED;
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_BEACON_MODE_ENTER_ONLY_TRUSTED);
                    }
                    break;
                case 'B':
                    SUM_showMenu(SUM_MENU_BT_PAGE_1);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_PAIRED_DEVICES: // Bluetooth Paired Devices
            switch(key) {
                case '1': // Select Device 1
                case '2': // Select Device 2
                case '3': // Select Device 3
                    SUM_pairedDevices.selected = SUM_pairedDevices.page * 3 + (key - 49);
                    SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICE);
                    break;
                case '*': // Refresh
                    BM78_execute(BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO, 0);
                    break;
                case '#': // Erase all paired devices
                    BM78_execute(BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO, 0);
                    SUM_pairedDevices.page = 0;
                    BM78.pairedDevicesCount = 0;
                    SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICES);
                    break;
                case 'B': // Back
                    if (SUM_pairedDevices.page == 0 || BM78.pairedDevicesCount == 0) {
                        SUM_showMenu(SUM_MENU_BT_PAGE_1);
                    } else {
                        SUM_pairedDevices.page--;
                        SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICES);
                    }
                    break;
                case 'C': // Next
                    if ((SUM_pairedDevices.page + 1) * 3 < BM78.pairedDevicesCount) {
                        SUM_pairedDevices.page++;
                        SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICES);
                    }
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_PAIRED_DEVICE: // Bluetooth Selected Paired Device
            switch(key) {
                case '1': // Remove
                    if (SUM_pairedDevices.selected < BM78.pairedDevicesCount) {
                        uint8_t index = BM78.pairedDevices[SUM_pairedDevices.selected].index;
                        BM78_execute(BM78_CMD_DELETE_PAIRED_DEVICE, 1, index);
                        SUM_pairedDevices.page = 0;
                        BM78.pairedDevicesCount = 0;
                        SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICES);
                    }
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICES);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_PAIRING_MODE: // Bluetooth Pairing Mode
            switch(key) {
                case '1': // Select PIN
                case '2': // Select Just Work
                case '3': // Select Passkey
                case '4': // Select User Confirm
                    key = key - 49;
                    if (BM78.pairingMode != key) {
                        BM78_write(BM78_CMD_WRITE_PAIRING_MODE_SETTING, BM78_EEPROM_STORE, 1, &key);
                    }
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_BT_PAGE_1);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_SHOW_DEVICE_NAME: // Bluetooth Device Name
            switch(key) {
                case '1': // Refresh
                    BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_BT_PAGE_2);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_PIN_SETUP: // Bluetooth PIN Setup
            if (SUM_manual.position < 0xFF) {
                if (SUM_manual.position < 6 && key >= '0' && key <= '9') {
                    SUM_manual.string[SUM_manual.position] = key;
                    LCD_replaceChar(key, SUM_manual.position + 7, 1, true);
                    SUM_manual.position++;
                    if (SUM_manual.position >= 4) LCD_setString("A) Abort  Confirm (C", 3, true);
                } else switch(key) {
                    case 'C': // Confirm
                        if (SUM_manual.position >= 4) {
                            SUM_manual.string[SUM_manual.position] = '\0';
                            BM78_write(BM78_CMD_WRITE_PIN_CODE, BM78_EEPROM_STORE, SUM_manual.position, (uint8_t *) SUM_manual.string);
                        } else break;
                    case 'A': // Abort
                        SUM_manual.position = 0xFF;
                        SUM_showMenu(SUM_MENU_BT_PIN_SETUP);
                        break;
                }
            } else switch(key) {
                case '1': // Change
                    LCD_setString("       ......       ", 1, true);
                    LCD_setString("                    ", 2, true);
                    LCD_setString("A) Abort            ", 3, true);
                    SUM_manual.position = 0;
                    break;
                case '2':
                    BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_BT_PAGE_2);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_SHOW_REMOTE_DEVICE:
            switch(key) {
                case '1': // Refresh
                    BM78_execute(BM78_CMD_READ_REMOTE_DEVICE_NAME, 0);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_BT_PAGE_2);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_BT_READ_CONFIG: 
            switch(key) {
                case 'C':
                    BM78_resetToTestMode();
                    __delay_ms(1000);
                    BM78_openEEPROM();
                    SUM_mem.reg = 0x0000;
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;   
            }
            break;
        case SUM_MENU_BT_SHOW_MAC_ADDRESS:
            switch(key) {
                case '1':
                    SUM_showMenu(SUM_MENU_BT_SHOW_MAC_ADDRESS);
                    BM78_execute(BM78_CMD_READ_LOCAL_INFORMATION, 0);
                    break;
                case 'B':
                    SUM_showMenu(SUM_MENU_BT_PAGE_3);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;   
            }
            break;
            break;
        case SUM_MENU_BT_CONFIG_VIEWER:
            if (SUM_manual.position < 0xFF) { // Enter manual address
                char hex;
                uint8_t value;
                
                if (key == 35) hex = 69; // # -> E
                else if (key == 42) hex = 70; // * -> F
                else hex = key;
                
                LCD_replaceChar(hex, 6 + (3 - SUM_manual.position), 2, true);

                value = hex2dec(hex);

                SUM_manual.address = SUM_manual.address + (value << (SUM_manual.position * 4));
                if (SUM_manual.position > 0) {
                    SUM_manual.position--;
                } else {
                    SUM_mem.reg = SUM_manual.address / 16 * 16;
                    SUM_manual.address = 0x0000;
                    SUM_manual.position = 0xFF;
                    BM78_readEEPROM(SUM_mem.reg, 16);
                }
            } else switch(key) { // Browse Memory
                case 'B': // Previous
                    if (SUM_mem.reg > 0) {
                        BM78_readEEPROM(SUM_mem.reg - 16, 16);
                    }
                    break;
                case 'C': // Next
                    if (SUM_mem.reg < (BM78_EEPROM_SIZE - 16)) {
                        BM78_readEEPROM(SUM_mem.reg + 16, 16);
                    }
                    break;
                case 'A': // Abort
                    //BM78_closeEEPROM();
                    BM78_resetToAppMode();
                    SUM_showMenu(SUM_MENU_BT_PAGE_3);
                    break;
                case 'D': // Reset
                    SUM_mem.reg = 0x0000;
                    BM78_readEEPROM(SUM_mem.reg, 16);
                    break;
                case '*':
                    LCD_init();
                    LCD_setBacklight(true);
                    BM78_readEEPROM(SUM_mem.reg, 16);
                    break;
                case '#':
                    LCD_clear();
                    LCD_setString("GOTO: ....          ", 2, true);
                    SUM_manual.position = 3;
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
#endif
#if defined MEM_ADDRESS || defined MEM_INTERNAL_SIZE
        case SUM_MENU_MEM_MAIN: // Memory Menu
            switch(key) {
#ifdef MEM_INITIALIZED_ADDR
                case '1': // Goto Memory Viewer
                    SUM_mem.addr = 0x00;
                    SUM_showMenu(SUM_MENU_MEM_VIEWER_INTRO);
                    break;
#endif
#ifdef MEM_ADDRESS
                case '2': // Goto Memory Viewer
                    SUM_mem.addr = MEM_ADDRESS;
                    SUM_showMenu(SUM_MENU_MEM_VIEWER_INTRO);
                    break;
#endif
#ifdef SM_MEM_ADDRESS
                case '3': // Lock/Unlock State Machine
                    byte = I2C_readRegister16(SM_MEM_ADDRESS, SM_MEM_START);
                    I2C_writeRegister16(SM_MEM_ADDRESS, SM_MEM_START,
                            byte == SM_STATUS_DISABLED 
                            ? SM_STATUS_ENABLED : SM_STATUS_DISABLED);
                    SM_init();
                    if (byte == 0x00) SMI_start();
                    SUM_showMenu(SUM_MENU_MEM_MAIN);
                    break;
#endif
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_MAIN);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_MEM_VIEWER_INTRO: // Memory Viewer Introduction
            switch(key) {
                case 'C': // Continue
                    SUM_setupPage = SUM_MENU_MEM_VIEWER;
                    SUM_mem.reg = 0x0000;
                    POC_testMem(SUM_mem.addr, SUM_mem.reg);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_MEM_VIEWER: // Memory Viewer
            if (SUM_manual.position < 0xFF) { // Enter manual address
                char hex;
                uint8_t value;
                
                if (key == 35) hex = 69; // # -> E
                else if (key == 42) hex = 70; // * -> F
                else hex = key;
                
                LCD_replaceChar(hex, 6 + (3 - SUM_manual.position), 2, true);

                value = hex2dec(hex);

                SUM_manual.address = SUM_manual.address + (value << (SUM_manual.position * 4));
                if (SUM_manual.position > 0) {
                    SUM_manual.position--;
                } else {
#if defined MEM_ADDRESS && defined MEM_INTERNAL_SIZE
                    SUM_mem.reg = min16(SUM_manual.address / 16 * 16,
                            ((SUM_mem.addr == 0x00 ? MEM_INTERNAL_SIZE : MEM_SIZE) - 15));
#elif defined MEM_ADDRESS
                    SUM_mem.reg = min16(SUM_manual.address / 16 * 16,
                            (MEM_SIZE - 15));
#elif defined MEM_INTERNAL_SIZE
                    SUM_mem.reg = min16(SUM_manual.address / 16 * 16,
                            (MEM_INTERNAL_SIZE - 15));
#endif
                    SUM_manual.address = 0x0000;
                    SUM_manual.position = 0xFF;
                    POC_testMem(SUM_mem.addr, SUM_mem.reg);
                }
            } else switch(key) { // Browse Memory
                case 'B': // Previous
                    if (SUM_mem.reg > 0) SUM_mem.reg -= 16;
                    POC_testMem(SUM_mem.addr, SUM_mem.reg);
                    break;
                case 'C': // Next
#if defined MEM_ADDRESS && defined MEM_INTERNAL_SIZE
                    if (SUM_mem.reg < ((SUM_mem.addr == 0x00 ? MEM_INTERNAL_SIZE : MEM_SIZE) - 16)) SUM_mem.reg += 16;
#elif defined MEM_ADDRESS
                    if (SUM_mem.reg < (MEM_SIZE - 16)) SUM_mem.reg += 16;
#elif defined MEM_INTERNAL_SIZE
                    if (SUM_mem.reg < (MEM_INTERNAL_SIZE - 16)) SUM_mem.reg += 16;
#endif
                    POC_testMem(SUM_mem.addr, SUM_mem.reg);
                    break;
                case 'A': // Abort
                    SUM_showMenu(SUM_MENU_MEM_MAIN);
                    break;
                case 'D': // Reset
                    SUM_mem.reg = 0x0000;
                    POC_testMem(SUM_mem.addr, SUM_mem.reg);
                    break;
                case '*':
                    LCD_init();
                    LCD_setBacklight(true);
                    POC_testMem(SUM_mem.addr, SUM_mem.reg);
                    break;
                case '#':
                    LCD_clear();
                    LCD_setString("GOTO: ....          ", 2, true);
                    SUM_manual.position = 3;
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
#endif
        case SUM_MENU_TEST_PAGE_1:
            switch(key) {
#ifdef DHT11_PORT
                case '1': // DHT11 Test
                    SUM_showMenu(SUM_MENU_TEST_DHT11);
                    break;
#endif
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_MAIN);
                    break;
                case 'C': // Next
                    SUM_showMenu(SUM_MENU_TEST_PAGE_2);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_TEST_PAGE_2: // Tests Menu (Page 1)
            switch(key) {
                case '1': // LCD Test
                    POC_testDisplay();
                    SUM_showMenu(SUM_MENU_TEST_PAGE_2);
                    break;
                case '2': // LCD Backlight
                    sw.lcdBacklight = !sw.lcdBacklight;
                    LCD_setBacklight(sw.lcdBacklight);
                    SUM_showMenu(SUM_MENU_TEST_PAGE_2);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_TEST_PAGE_1);
                    break;
                case 'C': // Next
                    SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_TEST_PAGE_3: // Tests Menu (Page 2)
            switch (key) {
#ifdef MCP_ENABLED
                case '1': // MCP23017 Input Test
                    SUM_mcpTest.addr = MCP_START_ADDRESS;
                    SUM_showMenu(SUM_MENU_TEST_MCP_IN);
                    break;
                case '2': // MCP23017 Output Test
                    SUM_mcpTest.addr = 0x00;
                    SUM_mcpTest.port = 0xFF;
                    SUM_showMenu(SUM_MENU_TEST_MCP_OUT);
                    break;
#endif
                case '3': // Show Keypad
                    sw.showKeypad = !sw.showKeypad;
                    SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_TEST_PAGE_2);
                    break;
                case 'C': // Next
                    SUM_showMenu(SUM_MENU_TEST_PAGE_4);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_TEST_PAGE_4: // Tests Menu (Page 3)
            switch(key) {
#ifdef RGB_ENABLED
                case '1': // RGB Test
                    POC_testRGB();
                    SUM_showMenu(SUM_MENU_TEST_PAGE_4);
                    break;
#endif
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    break;
                case 'C': // Next
                    SUM_showMenu(SUM_MENU_TEST_PAGE_5);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_TEST_PAGE_5: // Tests Menu (Page 4)
            switch(key) {
#ifdef WS281x_BUFFER
                case '1': // WS281x Test
                    POC_testWS281x();
                    SUM_showMenu(SUM_MENU_TEST_PAGE_5);
                    break;
                case '2': // WS281x Demo
                    sw.ws281xDemo = !sw.ws281xDemo;
                    if (sw.ws281xDemo) POC_demoWS281x();
                    else WS281x_off();
                    SUM_showMenu(SUM_MENU_TEST_PAGE_5);
                    break;
#endif
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_TEST_PAGE_4);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
#ifdef DHT11_PORT
        case SUM_MENU_TEST_DHT11:
            switch (key) {
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_TEST_PAGE_1);
                    break;
                case 'C': // Refresh
                    SUM_showMenu(SUM_MENU_TEST_DHT11);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
#endif
#ifdef MCP_ENABLED
        case SUM_MENU_TEST_MCP_IN:
            switch (key) {
                case 'A': // Abort
                    SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    break;
                case 'B': // Back
                    if (SUM_mcpTest.addr == 0) {
                        SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    } else {
                        SUM_mcpTest.addr--;
                        SUM_showMenu(SUM_MENU_TEST_MCP_IN);
                    }
                    break;
                case 'C': // Next
                    SUM_mcpTest.addr = SUM_mcpTest.addr == (MCP_END_ADDRESS - MCP_START_ADDRESS) 
                            ? 0 : SUM_mcpTest.addr + 1;
                    SUM_showMenu(SUM_MENU_TEST_MCP_IN);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_TEST_MCP_OUT:
            switch (key) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    if (SUM_mcpTest.addr == 0) SUM_mcpTest.addr = MCP_START_ADDRESS + (key - '0');
                    SUM_showMenu(SUM_MENU_TEST_MCP_OUT);
                    break;
                case 'A':
                case 'B':
                    if (SUM_mcpTest.addr >= MCP_START_ADDRESS && SUM_mcpTest.port == 0xFF) {
                        SUM_mcpTest.port = MCP_PORTA + (key - 'A');
                        SUM_showMenu(SUM_MENU_TEST_MCP_OUT);
                    } else {
                        SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    }
                    break;
                case 'C':
                    if (SUM_mcpTest.addr >= MCP_START_ADDRESS && SUM_mcpTest.port != 0xFF) {
                        SUM_showMenu(SUM_MENU_TEST_MCP_OUT);
                    }
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
#endif
        default:
            SUM_defaultFunction(key);
            break;
    }
}

bool SUM_processKey(uint8_t key) {
    if (key > 0) {
        if (SUM_mode) {
            SUM_executeMenu(key);
            if (sw.showKeypad) LCD_replaceChar(key, 10, 3, true);
            return true;
        } else if (key == 'D') {
            SUM_setupModePush--;
            if (SUM_setupModePush == 0) {
                SUM_mode = true;
                for (uint8_t i = 0; i < LCD_ROWS; i++) {
                    for (uint8_t j = 0; j < LCD_COLS; j++) {
#ifdef MEM_SUM_LCD_CACHE_START
                        SUM_mem.reg = MEM_SUM_LCD_CACHE_START + (i * LCD_COLS) + j;
                        MEM_write16(MEM_ADDRESS, SUM_mem.reg, LCD_getCache(i, j));
#else
                        lcdCache[i][j] = LCD_getCache(i, j);
#endif
                    }
                }
                SUM_showMenu(SUM_MENU_MAIN);
                SUM_setupModePush = 5;
            }
            return true;
        } else {
            SUM_setupModePush = 5;
        }
    }
    return false;
}

#ifdef BM78_ENABLED

void SUM_bm78EventHandler(BM78_Response_t response, uint8_t *data) {
    if (SUM_mode) switch (response.op_code) {
        case BM78_EVENT_COMMAND_COMPLETE:
            switch(response.CommandComplete_0x80.command) {
                case BM78_CMD_READ_LOCAL_INFORMATION:
                    if (SUM_setupPage == SUM_MENU_BT_SHOW_MAC_ADDRESS) {
                        LCD_setString(" V:                 ", 1, false);
                        for (uint8_t i = 0; i < 5; i++) { // BT Version
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
                    }
                    break;
                case BM78_CMD_READ_DEVICE_NAME:
                    if (SUM_setupPage == SUM_MENU_BT_SHOW_DEVICE_NAME) {
                        SUM_showMenu(SUM_setupPage);
                    }
                    break;
                case BM78_CMD_READ_PAIRING_MODE_SETTING:
                    if (SUM_setupPage == SUM_MENU_BT_PAIRING_MODE) {
                        SUM_showMenu(SUM_setupPage);
                    }
                    break;
                case BM78_CMD_WRITE_PAIRING_MODE_SETTING:
                    if (SUM_setupPage == SUM_MENU_BT_PAIRING_MODE) {
                        BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
                    }
                    break;
                case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                    if (SUM_setupPage == SUM_MENU_BT_PAIRED_DEVICES) {
                        SUM_pairedDevices.page = 0;
                        SUM_showMenu(SUM_setupPage);
                    }
                    break;
                case BM78_CMD_READ_REMOTE_DEVICE_NAME:
                    if (SUM_setupPage == SUM_MENU_BT_SHOW_REMOTE_DEVICE) {
                        for (uint8_t i = 0; i < response.CommandComplete_0x80.length - 1; i++) {
                            if (i < LCD_COLS) LCD_replaceChar(data[i], i, 0, false);
                        }
                        LCD_displayLine(0);
                    }
                    break;
                case BM78_CMD_READ_PIN_CODE:
                    if (SUM_setupPage == SUM_MENU_BT_PIN_SETUP) {
                        SUM_showMenu(SUM_setupPage);
                    }
                    break;
                case BM78_CMD_WRITE_PIN_CODE:
                    if (SUM_setupPage == SUM_MENU_BT_PIN_SETUP) {
                        BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
                    }
                    break;
                default:
                    break;
            }
            break;
        case BM78_EVENT_BM77_STATUS_REPORT:
            if (SUM_bm78CurrentState.id != response.StatusReport_0x81.status) {
                SUM_bm78CurrentState.id = response.StatusReport_0x81.status;
                SUM_setBtStatus(SUM_bm78CurrentState.id);
                if (SUM_setupPage == SUM_MENU_BT_PAGE_1
                        || SUM_setupPage == SUM_MENU_BT_STATE) {
                    SUM_showMenu(SUM_setupPage);
                }
            }
            break;
        default:
            break;
    }
}

#endif

#endif

#ifdef SUM_BTN_PORT
uint16_t SUM_btnSetupDownCounter = 0; //0xFFFF;
uint8_t SUM_btnMode = 0;
#define SUM_BTN_DELAY 100 // x TIMER_PERIOD ms

void SUM_blink(uint8_t times) {
    for (uint8_t i = 0; i < times; i++) {
        if (i > 0) __delay_ms(100);
        SUM_LED_LAT = 1;
        __delay_ms(50);
        SUM_LED_LAT = 0;
    }
}

uint8_t SUM_processBtn(uint8_t maxModes) {
    if (SUM_btnSetupDownCounter < 0x8000) {
        if (SUM_BTN_PORT) {
            if (SUM_btnMode > maxModes) {
                SUM_btnMode = 0;
                SUM_LED_LAT = 0;
                SUM_btnSetupDownCounter = 0;
            } else if (SUM_btnMode > 0 && SUM_btnSetupDownCounter > 0) {
                SUM_btnSetupDownCounter = 0x8000;
            }
        } else if (SUM_btnSetupDownCounter < 0x7FFF) {
            if (SUM_btnSetupDownCounter == 0) {
                SUM_LED_LAT = 1;
                __delay_ms(1000);
                SUM_LED_LAT = 0;
            }
            SUM_btnSetupDownCounter++;
            if (SUM_btnSetupDownCounter % SUM_BTN_DELAY == 0) {
                SUM_btnMode = SUM_btnSetupDownCounter / SUM_BTN_DELAY;
                if (SUM_btnMode <= maxModes) {
                    SUM_blink(SUM_btnMode);
                } else {
                    SUM_LED_LAT = 1;
                }
            }
        }
    }

    if (SUM_btnSetupDownCounter == 0x8000) {
        SUM_blink(SUM_btnMode);
        __delay_ms(500);
        if (!SUM_BTN_PORT) SUM_btnSetupDownCounter++;
    } else if (SUM_btnSetupDownCounter > (0x8000 + SUM_BTN_DELAY)) { // Cancel
        SUM_LED_LAT = 1;
        __delay_ms(1500);
        SUM_btnSetupDownCounter = 0;
        SUM_btnMode = 0;
    } else if (SUM_btnSetupDownCounter > 0x8000) {
        SUM_btnSetupDownCounter++;
        if (SUM_BTN_PORT) { // Confirm
            SUM_btnSetupDownCounter = 0;
            return SUM_btnMode;
        }
    }
    return 0;
}

#endif

#ifdef BM78_ENABLED

void SUM_bm78InitializeDongle(void) {
    BM78_resetToTestMode();
    __delay_ms(1000);
    SUM_bm78ConfigCommand.index = 0;
    SUM_bm78ConfigCommand.retries = 0;
    SUM_bm78ConfigCommand.stage = 0;
    SUM_bm78ConfigCommand.timeout = 200;
    BM78_openEEPROM();
}

bool SUM_bm78InitializationDone(void) {
    return SUM_bm78ConfigCommand.stage = 3;
}

void SUM_bm78TestModeCheck(void) {
    if (SUM_bm78ConfigCommand.timeout < 0xFF) {
        if (SUM_bm78ConfigCommand.timeout == 0) {
            SUM_bm78ConfigCommand.timeout = 0xFF;
            FlashPacket_t config = BM78_configuration[SUM_bm78ConfigCommand.index];

            //__delay_ms(500);
            if (SUM_bm78ConfigCommand.stage == 0 || SUM_bm78ConfigCommand.retries >= 3) {
                BM78_resetToAppMode();
                __delay_ms(1000);
                BM78_resetToTestMode();
                __delay_ms(1000);
                BM78_openEEPROM();
                SUM_bm78ConfigCommand.stage = 0;
                SUM_bm78ConfigCommand.retries = 0;
            } else if (SUM_bm78ConfigCommand.stage == 1) {
                BM78_clearEEPROM();
            } else if (SUM_bm78ConfigCommand.retries >= 0x80) {
                BM78_writeEEPROM(config.address, config.length, config.data);
            } else {
                BM78_readEEPROM(config.address, config.length);
            }
            
            SUM_bm78ConfigCommand.timeout = 200;
            SUM_bm78ConfigCommand.retries++;
        } else {
            SUM_bm78ConfigCommand.timeout--;
        }
    }
}

void SUM_bm78TestModeResponseHandler(uint8_t length, uint8_t *data) {
    FlashPacket_t config;
    uint8_t i, size;
#ifdef LCD_ADDRESS
    if (!SUM_mode || SUM_setupPage != SUM_MENU_BT_READ_CONFIG) {
#endif
        // EEPROM was opened
        if (length == 4 && data[0] == 0x01 && data[1] == 0x03 && data[2] == 0x0C && data[3] == 0x00) {
#ifdef LCD_ADDRESS
            printProgress("  Initializing BT   ", 1, BM78_CONFIGURATION_SIZE + 3);
#endif
            //__delay_ms(500);
            SUM_bm78ConfigCommand.retries = 0;
            SUM_bm78ConfigCommand.stage = 1;
            SUM_bm78ConfigCommand.timeout = 200;
            BM78_clearEEPROM();
        } else
        // EEPROM was cleared
        if (length == 4 && data[0] == 0x01 && data[1] == 0x2d && data[2] == 0xFC && data[3] == 0x00) {
#ifdef LCD_ADDRESS
            printProgress("  Initializing BT   ", 2, BM78_CONFIGURATION_SIZE + 3);
#endif
            config = BM78_configuration[SUM_bm78ConfigCommand.index];
            //__delay_ms(500);
            SUM_bm78ConfigCommand.retries = 0;
            SUM_bm78ConfigCommand.stage = 2;
            SUM_bm78ConfigCommand.timeout = 200;
            BM78_readEEPROM(config.address, config.length);
        } else
        // Write response
        if (length == 4 && data[0] == 0x01 && data[1] == 0x27 && data[2] == 0xFC && data[3] == 0x00) {
            FlashPacket_t config = BM78_configuration[SUM_bm78ConfigCommand.index];
            //__delay_ms(500);
            SUM_bm78ConfigCommand.retries = 0;
            SUM_bm78ConfigCommand.stage = 2;
            SUM_bm78ConfigCommand.timeout = 200;
            BM78_readEEPROM(config.address, config.length);
        } else
#ifdef LCD_ADDRESS
        // Read response
        if (SUM_setupPage == SUM_MENU_BT_CONFIG_VIEWER 
                && length > 7 && data[0] == 0x01 && data[1] == 0x29 && data[2] == 0xFC && data[3] == 0x00) {
            size = data[6];
                if (length >= size + 7) {
                    SUM_mem.reg = (data[4] << 8) | (data[5] & 0xFF);
                    uint8_t display[16];
                    for (i = 0; i < size; i++) {
                        if (i < 16) display[i] = data[i + 7];
                    }
                    POC_displayData(SUM_mem.reg, size, display);
                }
        } else
#endif
        // Read response
        if (length > 7 && data[0] == 0x01 && data[1] == 0x29 && data[2] == 0xFC && data[3] == 0x00) {
            config = BM78_configuration[SUM_bm78ConfigCommand.index];
            size = data[6];
            bool equals;
            if (length >= size + 7) {
                SUM_mem.reg = (data[4] << 8) | (data[5] & 0xFF);
                equals = SUM_mem.reg == config.address && size == config.length;
                if (equals) for (i = 0; i < size; i++) {
                    equals = equals && data[i + 7] == config.data[i];
                }
            } else {
                equals = false;
            }
            if (equals) {
                SUM_bm78ConfigCommand.index++;
#ifdef LCD_ADDRESS
                printProgress("  Initializing BT   ", 3 + SUM_bm78ConfigCommand.index, BM78_CONFIGURATION_SIZE + 3);
#endif
                if (SUM_bm78ConfigCommand.index < BM78_CONFIGURATION_SIZE) {
                    config = BM78_configuration[SUM_bm78ConfigCommand.index];
                    //__delay_ms(500);
                    SUM_bm78ConfigCommand.retries = 0;
                    SUM_bm78ConfigCommand.stage = 2;
                    SUM_bm78ConfigCommand.timeout = 200;
                    BM78_readEEPROM(config.address, config.length);
                } else {
                    SUM_bm78ConfigCommand.stage = 3;
                    SUM_bm78ConfigCommand.timeout = 0xFF;
                    SUM_bm78ConfigCommand.retries = 0;
                    BM78_resetToAppMode();
#ifdef LCD_ADDRESS
                    if (SUM_mode) SUM_showMenu(SUM_MENU_BT_PAGE_4);
#endif
                }
            } else {
                //__delay_ms(500);
                SUM_bm78ConfigCommand.retries = 0x80;
                SUM_bm78ConfigCommand.stage = 2;
                SUM_bm78ConfigCommand.timeout = 200;
                BM78_writeEEPROM(config.address, config.length, config.data);
            }
        }
#ifdef LCD_ADDRESS
    } else if (SUM_setupPage == SUM_MENU_BT_READ_CONFIG) {
        SUM_setupPage = SUM_MENU_BT_CONFIG_VIEWER;
        SUM_mem.reg = 0x0000;
        BM78_readEEPROM(SUM_mem.reg, 16);
    }
#endif
}

#ifdef LCD_ADDRESS
void SUM_bm78ErrorHandler(BM78_Response_t response, uint8_t *data) {
    if (sw.showBtErrors) POC_bm78ErrorHandler(response, data);
}
#endif

#endif