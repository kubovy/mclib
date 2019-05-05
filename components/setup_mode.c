/* 
 * File:   setup_mode.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "setup_mode.h"

#ifdef LCD_ADDRESS

#include "../lib/common.h"
#include "../components/poc.h"
#include "../modules/lcd.h"
#include "../modules/mcp23017.h"

#ifdef MEM_ADDRESS
#include "../modules/memory.h"
#ifdef SM_MEM_ADDRESS
#include "../modules/state_machine.h"
#include "state_machine_interaction.h"
#endif
#endif

#ifdef WS281x_BUFFER
#include "../modules/ws281x.h"
#endif

uint8_t SUM_setupModePush = 5;
uint8_t SUM_setupPage = 0;

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

uint16_t SUM_addr = 0x0000;
uint8_t SUM_i, SUM_j;
#ifndef MEM_SUM_LCD_CACHE_START
char lcdCache[LCD_ROWS][LCD_COLS + 1];
#endif

struct {
    uint8_t addr;
    uint8_t port;
} SUM_mcpTest = { MCP_START_ADDRESS, MCP_PORTA };


struct {
    uint8_t timeout;
    uint8_t stage;
    uint8_t retries;
    uint8_t index;
} SUM_bm78ConfigCommand = { 0xFF, 0, 0, 0 };

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

uint8_t SUM_advData[22];

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
            strcpy(SUM_bm78CurrentState.name, "UNKNOWN  ", 9);
            break;
    }
}

void SUM_displayHWAddress(uint8_t index, uint8_t line, uint8_t start) {
    if (index < BM78.pairedDevicesCount) {
        for (SUM_j = 0; SUM_j < 6; SUM_j++) {
            LCD_replaceChar(dec2hex(BM78.pairedDevices[index].address[5 - SUM_j] / 16 % 16), SUM_j * 3 + start, line, false);
            LCD_replaceChar(dec2hex(BM78.pairedDevices[index].address[5 - SUM_j] % 16), SUM_j * 3 + start + 1, line, false);
            if (SUM_j > 0) LCD_replaceChar(':', SUM_j * 3 + start + 2, line, false);
        }
    }
}

#endif

void SUM_mcpChanged(uint8_t address) {
    if (SUM_mode && SUM_setupPage == SUM_MENU_TEST_MCP_IN) {
        SUM_mcpTest.addr = address;
        POC_testMCP23017Input(SUM_mcpTest.addr);
    }
}

void SUM_showMenu(uint8_t page) {
    SUM_setupPage = page;
    LCD_clear();
    switch (SUM_setupPage) {
        case SUM_MENU_INTRO: // Introduction
            LCD_setString("|c|SetUp Mode", 0, true);
            LCD_setString("|c|Activated", 1, true);
            LCD_setString("|r|C) Confirm", 3, true);
            break;
        case SUM_MENU_MAIN: // Main Menu
#ifdef BM78_ENABLED
            LCD_setString("1) Bluetooth", 0, true);
#else
            LCD_setString("-) Bluetooth", 0, true);
#endif
#ifdef MEM_ADDRESS
            LCD_setString("2) Memory", 1, true);
#else
            LCD_setString("-) Memory", 1, true);
#endif
            LCD_setString("3) Tests", 2, true);
            LCD_setString("|r|Exit (D", 3, true);
            break;
#ifdef BM78_ENABLED
        case SUM_MENU_BT_PAGE_1: // Bluetooth Menu (Page 1)
            SUM_setBtStatus(SUM_bm78CurrentState.id);
            LCD_setString("1) State [         ]", 0, false);
            LCD_replaceString(SUM_bm78CurrentState.name, 10, 0, true);
            LCD_setString("2) Paired Devices", 1, true);
            LCD_setString("3) Pairing Mode", 2, true);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_BT_PAGE_2: // Bluetooth Menu (Page 2)
            LCD_setString("1) Device Name", 0, true);
            LCD_setString("2) PIN   [      ]", 1, false);
            LCD_replaceString(BM78.pin, 10, 1, true);
            if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE 
                    || BM78.status == BM78_STATUS_LE_CONNECTED_MODE) {
                LCD_setString("3) Remote Device", 2, true);
            }
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_BT_PAGE_3: // Bluetooth Menu (Page 3)
            LCD_setString("1) Show Errors   [ ]", 0, false);
            LCD_replaceChar(sw.showBtErrors ? 'X' : ' ', 18, 0, true);
            LCD_setString("2) Read Config", 1, true);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_BT_PAGE_4: // Bluetooth Menu (Page 4)
            LCD_setString("1) Init Dongle", 0, true);
            LCD_setString("2) Init Application", 1, true);
            LCD_setString("B) Back", 3, true);
            break;
        case SUM_MENU_BT_STATE: // Bluetooth: State
            LCD_setString("State:          ", 0, false);
            LCD_replaceString(SUM_bm78CurrentState.name, 7, 0, true);
            if (BM78.status== BM78_STATUS_IDLE_MODE) {
                LCD_setString("1) Enter Stand-By", 1, true);
            } else if (BM78.status == BM78_STATUS_STANDBY_MODE) {
                LCD_setString("1) Leave Stand-By", 1, true);
            } else if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
                LCD_setString("1) Disconnect SPP", 1, true);
            } else if (BM78.status == BM78_STATUS_LE_CONNECTED_MODE) {
                LCD_setString("1) Disconnect LE", 1, true);
            }
            LCD_setString("B) Back", 3, true);
            break;
        case SUM_MENU_BT_PAIRED_DEVICES: // Bluetooth: Paired Devices
            if (BM78.pairedDevicesCount == 0) { // No paired devices yet
                LCD_setString("*: Get devices", 0, true);
                LCD_setString("#: Clear all paired devices", 1, true);
                LCD_setString("B) Back", 3, true);
            } else { // Paired devices retrieved
                for (SUM_i = 0; SUM_i < 3; SUM_i++) {
                    if ((SUM_pairedDevices.page * 3 + SUM_i) < BM78.pairedDevicesCount) {
                        LCD_setString("-) ##:##:##:##:##:##\0", SUM_i, false);
                        LCD_replaceChar(SUM_i + 49, 0, SUM_i, false);
                        SUM_displayHWAddress(SUM_pairedDevices.page * 3 + SUM_i, SUM_i, 3);
                        LCD_displayLine(SUM_i);
                    }
                }
                if ((SUM_pairedDevices.page + 1) * 3 < BM78.pairedDevicesCount) {
                    LCD_setString("B) Back      Next (C", 3, true);
                } else {
                    LCD_setString("B) Back", 3, true);
                }
            }
            break;
        case SUM_MENU_BT_PAIRED_DEVICE: {
                LCD_setString(" ##:##:##:##:##:##  \0", 0, false);
                SUM_displayHWAddress(SUM_pairedDevices.selected, 0, 1);
                LCD_displayLine(0);
                LCD_setString("1) Remove Device", 1, true);
                LCD_setString("B) Back", 3, true);
            }
            break;
        case SUM_MENU_BT_PAIRING_MODE:
            LCD_setString(" ) PIN", 0, true);
            LCD_replaceChar(BM78.pairingMode == BM78_PAIRING_PIN ? 'X' : '1', 0, 0, true);
            LCD_setString(" ) Just Work ", 1, false);
            LCD_replaceChar(BM78.pairingMode == BM78_PAIRING_JUST_WORK ? 'X' : '2', 0, 1, true);
            LCD_setString(" ) Passkey ", 2, false);
            LCD_replaceChar(BM78.pairingMode == BM78_PAIRING_PASSKEY ? 'X' : '3', 0, 2, true);
            LCD_setString(" ) Confirm   Back (B", 3, false);
            LCD_replaceChar(BM78.pairingMode == BM78_PAIRING_USER_CONFIRM ? 'X' : '4', 0, 3, true);
            break;
        case SUM_MENU_BT_SHOW_DEVICE_NAME:
            LCD_setString(BM78.deviceName, 0, true);
            LCD_setString("1) Refresh", 2, true);
            LCD_setString("B) Back", 3, true);
            break;
        case SUM_MENU_BT_PIN_SETUP:
            LCD_setString("|c|PIN", 0, true);
            LCD_setString("                    ", 1, false);
            LCD_replaceString(BM78.pin, 7, 1, true);
            LCD_setString("1) Change", 2, true);
            LCD_setString("2) Refresh   Back (B", 3, true);
            break;
        case SUM_MENU_BT_SHOW_REMOTE_DEVICE:
            LCD_setString("1) Refresh", 2, true);
            LCD_setString("B) Back", 3, true);
            break;
        case SUM_MENU_BT_INITIALIZE_DONGLE:
            LCD_setString("|c|Initializing BT", 1, true);
            LCD_setString("|c|(please wait...)", 2, true);
            break;
        case SUM_MENU_BT_INITIALIZE_APP:
            LCD_setString("|c|Initializing App", 1, true);
            LCD_setString("|c|(please wait...)", 2, true);
            break;
        case SUM_MENU_BT_READ_CONFIG:
            LCD_setString("Memory controls:", 0, true);
            LCD_setString("B: Previous, C: Next", 1, true);
            LCD_setString("A: Abort, D: Reset", 2, true);
            LCD_setString("#: Address  Con't (C", 3, true);
            break;
#endif
#ifdef MEM_ADDRESS
        case SUM_MENU_MEM_MAIN: // Memory Menu
            LCD_setString("1) Memory Viewer", 0, true);
#ifdef SM_MEM_ADDRESS
            if (MEM_read(SM_MEM_ADDRESS, 0x00, 0x00) == 0x00) {
                LCD_setString("2) Lock SM       [ ]", 1, true);
            } else {
                LCD_setString("2) Unlock SM     [X]", 1, true);
            }
#endif
            LCD_setString("B) Back", 3, true);
            break;
        case SUM_MENU_MEM_VIEWER_INTRO: // Memory Viewer Introduction
            LCD_setString("Memory controls:", 0, true);
            LCD_setString("B: Previous, C: Next", 1, true);
            LCD_setString("A: Abort, D: Reset", 2, true);
            LCD_setString("#: Address  Con't (C", 3, true);
            break;
#endif
        case SUM_MENU_TEST_PAGE_1: // Tests Menu (Page 1)
            LCD_setString("1) LCD Test", 0, true);
            LCD_setString("2) LCD Backlight [ ]", 1, false);
            LCD_replaceChar(sw.lcdBacklight ? 'X' : ' ', 18, 1, true);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_TEST_PAGE_2: // Tests Menu (Page 2)
            LCD_setString("1) MCP23017 IN Test", 0, true);
            LCD_setString("2) MCP23017 OUT Test", 1, true);
            LCD_setString("B) Back      Next (C", 3, true);
            break;
        case SUM_MENU_TEST_PAGE_3: // Tests Menu (Page 3)
#ifdef WS281x_BUFFER
            LCD_setString("1) WS281x Test", 0, true);
            LCD_setString("2) WS281x Demo   [ ]", 1, false);
            LCD_replaceChar(sw.ws281xDemo ? 'X' : ' ', 18, 1, true);
#else
            LCD_setString("-) WS281x Test", 0, true);
            LCD_setString("-) WS281x Demo", 1, true);
#endif
            LCD_setString("3) Show Keypad   [ ]", 2, false);
            LCD_replaceChar(sw.showKeypad ? 'X' : ' ', 18, 2, true);
            LCD_setString("B) Back", 3, true);
            break;
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
                LCD_setString("A) Port A", 0, true);
                LCD_setString("B) Port B", 1, true);
            } else {
                POC_testMCP23017Output(SUM_mcpTest.addr, SUM_mcpTest.port);
                LCD_setString("B) Back    Repeat (C", 3, true);
            }
            break;
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
        case SUM_MENU_INTRO: // Introduction
            switch(key) {
                case 'C': // Confirm and goto Main Menu
                    SUM_showMenu(SUM_MENU_MAIN);
                    break;
            }
            break;
        case SUM_MENU_MAIN: // Main Menu
            switch(key) {
#ifdef BM78_ENABLED
                case '1': // Goto Bluetooth Menu (Page 1)
                    SUM_showMenu(SUM_MENU_BT_PAGE_1);
                    BM78_execute(BM78_CMD_READ_STATUS, 0);
                    break;
#endif
#ifdef MEM_ADDRESS
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
                            SUM_addr = MEM_SUM_LCD_CACHE_START + (SUM_i * LCD_COLS) + SUM_j;
                            LCD_setCache(SUM_i, SUM_j, MEM_read(MEM_ADDRESS, SUM_addr >> 8, SUM_addr & 0xFF));
                        }
                    }
                    LCD_displayCache();
#else
                    for (SUM_i = 0; SUM_i < LCD_ROWS; SUM_i++) {
                        LCD_setString(lcdCache[SUM_i], SUM_i, true);
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
                    SUM_showMenu(SUM_MENU_BT_INITIALIZE_DONGLE);
                    BM78_resetToTestMode();
                    printProgress("|c|Initializing BT", 0, BM78_CONFIGURATION_SIZE + 3);
                    __delay_ms(1000);
                    SUM_bm78ConfigCommand.index = 0;
                    SUM_bm78ConfigCommand.stage = 0;
                    SUM_bm78ConfigCommand.timeout = 200;
                    BM78_openEEPROM();
                    break;
                case '2':
                    SUM_showMenu(SUM_MENU_BT_INITIALIZE_APP);
                    BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
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
                        BM78.enforceStandBy = true;
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_ENTER);
                    } else if (BM78.status == BM78_STATUS_STANDBY_MODE) {
                        BM78.enforceStandBy = false;
                        BM78_execute(BM78_CMD_INVISIBLE_SETTING, 1, BM78_STANDBY_MODE_LEAVE);
                    } else if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
                        BM78_execute(BM78_CMD_DISCONNECT, 1, 0x00);
                    } else if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
                        BM78_execute(BM78_CMD_DISCONNECT, 1, 0x00);
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
                    if (BM78.pairingMode != (key - 49)) {
                        BM78_execute(BM78_CMD_WRITE_PAIRING_MODE_SETTING, 2, BM78_EEPROM_STORE, key - 49);
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
                    LCD_setString(" ", 2, true);
                    LCD_setString("A) Abort", 3, true);
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
                    SUM_addr = 0x0000;
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;   
            }
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
                    SUM_addr = SUM_manual.address / 16 * 16;
                    SUM_manual.address = 0x0000;
                    SUM_manual.position = 0xFF;
                    BM78_readEEPROM(SUM_addr, 16);
                }
            } else switch(key) { // Browse Memory
                case 'B': // Previous
                    if (SUM_addr > 0) {
                        BM78_readEEPROM(SUM_addr - 16, 16);
                    }
                    break;
                case 'C': // Next
                    if (SUM_addr < (BM78_EEPROM_SIZE - 16)) {
                        BM78_readEEPROM(SUM_addr + 16, 16);
                    }
                    break;
                case 'A': // Abort
                    //BM78_closeEEPROM();
                    BM78_resetToAppMode();
                    SUM_showMenu(SUM_MENU_BT_PAGE_3);
                    break;
                case 'D': // Reset
                    SUM_addr = 0x0000;
                    BM78_readEEPROM(SUM_addr, 16);
                    break;
                case '*':
                    LCD_init();
                    LCD_setBacklight(true);
                    BM78_readEEPROM(SUM_addr, 16);
                    break;
                case '#':
                    LCD_clear();
                    LCD_setString("GOTO: ....", 2, true);
                    SUM_manual.position = 3;
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
#endif
#ifdef MEM_ADDRESS
        case SUM_MENU_MEM_MAIN: // Memory Menu
            switch(key) {
                case '1': // Goto Memory Viewer
                    SUM_showMenu(SUM_MENU_MEM_VIEWER_INTRO);
                    break;
#ifdef SM_MEM_ADDRESS
                case '2': // Lock/Unlock State Machine
                    byte = MEM_read(SM_MEM_ADDRESS, 0x00, 0x00);
                    MEM_write(SM_MEM_ADDRESS, 0x00, 0x00, byte == 0xFF ? 0x00 : 0xFF);
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
                    SUM_addr = 0x0000;
                    POC_testMem(SUM_addr);
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
                    SUM_addr = SUM_manual.address / 16 * 16;
                    SUM_manual.address = 0x0000;
                    SUM_manual.position = 0xFF;
                    POC_testMem(SUM_addr);
                }
            } else switch(key) { // Browse Memory
                case 'B': // Previous
                    if (SUM_addr > 0) SUM_addr -= 16;
                    POC_testMem(SUM_addr);
                    break;
                case 'C': // Next
                    if (SUM_addr < (MEM_SIZE - 16)) SUM_addr += 16;
                    POC_testMem(SUM_addr);
                    break;
                case 'A': // Abort
                    SUM_showMenu(SUM_MENU_MEM_MAIN);
                    break;
                case 'D': // Reset
                    SUM_addr = 0x0000;
                    POC_testMem(SUM_addr);
                    break;
                case '*':
                    LCD_init();
                    LCD_setBacklight(true);
                    POC_testMem(SUM_addr);
                    break;
                case '#':
                    LCD_clear();
                    LCD_setString("GOTO: ....", 2, true);
                    SUM_manual.position = 3;
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
#endif
        case SUM_MENU_TEST_PAGE_1: // Tests Menu (Page 1)
            switch(key) {
                case '1': // LCD Test
                    POC_testDisplay();
                    SUM_showMenu(SUM_MENU_TEST_PAGE_1);
                    break;
                case '2': // LCD Backlight
                    sw.lcdBacklight = !sw.lcdBacklight;
                    LCD_setBacklight(sw.lcdBacklight);
                    SUM_showMenu(SUM_MENU_TEST_PAGE_1);
                    break;
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
        case SUM_MENU_TEST_PAGE_2: // Tests Menu (Page 2)
            switch (key) {
                case '1': // MCP23017 Input Test
                    SUM_mcpTest.addr = MCP_START_ADDRESS;
                    SUM_showMenu(SUM_MENU_TEST_MCP_IN);
                    break;
                case '2': // MCP23017 Output Test
                    SUM_mcpTest.addr = 0x00;
                    SUM_mcpTest.port = 0xFF;
                    SUM_showMenu(SUM_MENU_TEST_MCP_OUT);
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
        case SUM_MENU_TEST_PAGE_3: // Tests Menu (Page 3)
            switch(key) {
#ifdef WS281x_BUFFER
                case '1': // WS281x Test
                    POC_testWS281x();
                    SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    break;
                case '2': // WS281x Demo
                    sw.ws281xDemo = !sw.ws281xDemo;
                    if (sw.ws281xDemo) POC_demoWS281x();
                    else WS281x_off();
                    SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    break;
#endif
                case '3': // Show Keypad
                    sw.showKeypad = !sw.showKeypad;
                    SUM_showMenu(SUM_MENU_TEST_PAGE_3);
                    break;
                case 'B': // Back
                    SUM_showMenu(SUM_MENU_TEST_PAGE_2);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_TEST_MCP_IN:
            switch (key) {
                case 'A': // Abort
                    SUM_showMenu(SUM_MENU_TEST_PAGE_2);
                    break;
                case 'B': // Back
                    if (SUM_mcpTest.addr == 0) {
                        SUM_showMenu(SUM_MENU_TEST_PAGE_2);
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
                        SUM_showMenu(SUM_MENU_TEST_PAGE_2);
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
        default:
            SUM_defaultFunction(key);
            break;
    }
}

void SUM_processKey(uint8_t key) {
    if (key > 0) {
        if (SUM_mode) {
            if (sw.showKeypad) LCD_replaceChar(key, 10, 3, true);
            SUM_executeMenu(key);
        } else if (key == 'D') {
            SUM_setupModePush--;
            if (SUM_setupModePush == 0) {
                SUM_mode = true;
                for (SUM_i = 0; SUM_i < LCD_ROWS; SUM_i++) {
                    for (SUM_j = 0; SUM_j < LCD_COLS; SUM_j++) {
#ifdef MEM_SUM_LCD_CACHE_START
                        SUM_addr = MEM_SUM_LCD_CACHE_START + (SUM_i * LCD_COLS) + SUM_j;
                        MEM_write(MEM_ADDRESS, SUM_addr >> 8, SUM_addr & 0xFF, LCD_getCache(SUM_i, SUM_j));
#else
                        lcdCache[SUM_i][SUM_j] = LCD_getCache(SUM_i, SUM_j);
#endif
                    }
                }
                SUM_showMenu(0);
                SUM_setupModePush = 5;
            }
        } else {
            SUM_setupModePush = 5;
        }
    }
}

#ifdef BM78_ENABLED

void SUM_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data) {
    if (SUM_mode) switch (response.op_code) {
        case BM78_OPC_COMMAND_COMPLETE:
            switch(response.CommandComplete_0x80.command) {
                case BM78_CMD_READ_DEVICE_NAME:
                    if (SUM_setupPage == SUM_MENU_BT_SHOW_DEVICE_NAME) {
                        SUM_showMenu(SUM_setupPage);
                    } else if (SUM_setupPage == SUM_MENU_BT_INITIALIZE_APP) {
                        if (strcmp(BT_INITIAL_DEVICE_NAME, (char *) data)) {
                            BM78_GetAdvData(SUM_advData);
                            BM78_write(BM78_CMD_WRITE_ADV_DATA, BM78_EEPROM_STORE, 22, SUM_advData);
                        } else {
                            BM78_write(BM78_CMD_WRITE_DEVICE_NAME, BM78_EEPROM_STORE, strlen(BT_INITIAL_DEVICE_NAME), (uint8_t *) BT_INITIAL_DEVICE_NAME);
                        }
                    }
                    break;
                case BM78_CMD_WRITE_DEVICE_NAME:
                    if (SUM_setupPage == SUM_MENU_BT_INITIALIZE_APP) {
                        BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                    }
                    break;
                case BM78_CMD_WRITE_ADV_DATA:
                    if (SUM_setupPage == SUM_MENU_BT_INITIALIZE_APP) {
                        SUM_showMenu(SUM_MENU_BT_PAGE_4);
                    }
                    break;
                case BM78_CMD_READ_PAIRING_MODE_SETTING:
                    if (SUM_setupPage == SUM_MENU_BT_PAIRING_MODE) {
                        SUM_showMenu(SUM_setupPage);
                    }
                    break;
                case BM78_CMD_WRITE_PAIRING_MODE_SETTING:
                    BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
                    break;
                case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                    if (SUM_setupPage == SUM_MENU_BT_PAIRED_DEVICES) {
                        SUM_pairedDevices.page = 0;
                        SUM_showMenu(SUM_setupPage);
                    }
                    break;
                case BM78_CMD_READ_REMOTE_DEVICE_NAME:
                    if (SUM_setupPage == SUM_MENU_BT_SHOW_REMOTE_DEVICE) {
                        for (SUM_i = 0; SUM_i < response.CommandComplete_0x80.length - 1; SUM_i++) {
                            if (SUM_i < LCD_COLS) LCD_replaceChar(data[SUM_i], SUM_i, 0, false);
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
                    BM78_execute(BM78_CMD_READ_PIN_CODE, 0);
                    break;
                default:
                    break;
            }
            break;
        case BM78_OPC_BM77_STATUS_REPORT:
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

void SUM_bm78TestModeCheck(void) {
    if (SUM_mode && SUM_bm78ConfigCommand.timeout < 0xFF) {
        if (SUM_bm78ConfigCommand.timeout == 0) {
            SUM_bm78ConfigCommand.timeout = 0xFF;
            Flash32_t config = BM78_configuration[SUM_bm78ConfigCommand.index];

            __delay_ms(500);
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
    Flash32_t config;

    switch (SUM_setupPage) {
        case SUM_MENU_BT_INITIALIZE_DONGLE:
            // EEPROM was opened
            if (length == 4 && data[0] == 0x01 && data[1] == 0x03 && data[2] == 0x0C && data[3] == 0x00) {
                printProgress("|c|Initializing BT", 1, BM78_CONFIGURATION_SIZE + 3);
                __delay_ms(500);
                BM78_clearEEPROM();
                SUM_bm78ConfigCommand.stage = 1;
                SUM_bm78ConfigCommand.timeout = 200;
                SUM_bm78ConfigCommand.retries = 0;
            }
            // EEPROM was cleared
            if (length == 4 && data[0] == 0x01 && data[1] == 0x2d && data[2] == 0xFC && data[3] == 0x00) {
                printProgress("|c|Initializing BT", 2, BM78_CONFIGURATION_SIZE + 3);
                config = BM78_configuration[SUM_bm78ConfigCommand.index];
                __delay_ms(500);
                BM78_readEEPROM(config.address, config.length);
                SUM_bm78ConfigCommand.stage = 2;
                SUM_bm78ConfigCommand.timeout = 200;
                SUM_bm78ConfigCommand.retries = 0;
            }
            // Write response
            if (length == 4 && data[0] == 0x01 && data[1] == 0x27 && data[2] == 0xFC && data[3] == 0x00) {
                Flash32_t config = BM78_configuration[SUM_bm78ConfigCommand.index];
                __delay_ms(500);
                BM78_readEEPROM(config.address, config.length);
                SUM_bm78ConfigCommand.stage = 2;
                SUM_bm78ConfigCommand.timeout = 200;
                SUM_bm78ConfigCommand.retries = 0;
            }
            // Read response
            if (length > 7 && data[0] == 0x01 && data[1] == 0x29 && data[2] == 0xFC && data[3] == 0x00) {
                config = BM78_configuration[SUM_bm78ConfigCommand.index];
                uint8_t size = data[6];
                bool equals;
                if (length >= size + 7) {
                    SUM_addr = (data[4] << 8) | (data[5] & 0xFF);
                    equals = SUM_addr == config.address && size == config.length;
                    if (equals) for (SUM_i = 0; SUM_i < size; SUM_i++) {
                        equals = equals && data[SUM_i + 7] == config.data[SUM_i];
                    }
                } else {
                    equals = false;
                }
                if (equals) {
                    SUM_bm78ConfigCommand.index++;
                    printProgress("|c|Initializing BT", 3 + SUM_bm78ConfigCommand.index, BM78_CONFIGURATION_SIZE + 3);
                    if (SUM_bm78ConfigCommand.index < BM78_CONFIGURATION_SIZE) {
                        config = BM78_configuration[SUM_bm78ConfigCommand.index];
                        __delay_ms(500);
                        BM78_readEEPROM(config.address, config.length);
                        SUM_bm78ConfigCommand.stage = 2;
                        SUM_bm78ConfigCommand.timeout = 200;
                        SUM_bm78ConfigCommand.retries = 0;
                    } else {
                        SUM_bm78ConfigCommand.stage = 3;
                        SUM_bm78ConfigCommand.timeout = 0xFF;
                        SUM_bm78ConfigCommand.retries = 0;
                        BM78_resetToAppMode();
                        SUM_showMenu(SUM_MENU_BT_PAGE_4);
                    }
                } else {
                    __delay_ms(500);
                    BM78_writeEEPROM(config.address, config.length, config.data);
                    SUM_bm78ConfigCommand.stage = 2;
                    SUM_bm78ConfigCommand.timeout = 200;
                    SUM_bm78ConfigCommand.retries = 0x80;
                }
            }
            break;
        case SUM_MENU_BT_READ_CONFIG:
            SUM_setupPage = SUM_MENU_BT_CONFIG_VIEWER;
            SUM_addr = 0x0000;
            BM78_readEEPROM(SUM_addr, 16);
            break;
        case SUM_MENU_BT_CONFIG_VIEWER:
            // Read response
            if (length > 7 && data[0] == 0x01 && data[1] == 0x29 && data[2] == 0xFC && data[3] == 0x00) {
                uint8_t size = data[6];
                if (length >= size + 7) {
                    SUM_addr = (data[4] << 8) | (data[5] & 0xFF);
                    uint8_t display[16];
                    for (SUM_i = 0; SUM_i < size; SUM_i++) {
                        if (SUM_i < 16) display[SUM_i] = data[SUM_i + 7];
                    }
                    POC_displayData(SUM_addr, size, display);
                }
            } 
            break;
    }
}

void SUM_bm78ErrorHandler(BM78_Response_t response, uint8_t *data) {
    if (sw.showBtErrors) POC_bm78ErrorHandler(response, data);
    if (SUM_setupPage == SUM_MENU_BT_INITIALIZE_APP) {
        switch(response.op_code) {
            case BM78_OPC_COMMAND_COMPLETE:
                switch(response.CommandComplete_0x80.command) {
                    case BM78_CMD_READ_DEVICE_NAME:
                        BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                        break;
                    case BM78_CMD_WRITE_DEVICE_NAME:
                        BM78_write(BM78_CMD_WRITE_DEVICE_NAME, BM78_EEPROM_STORE, strlen(BT_INITIAL_DEVICE_NAME), (uint8_t *) BT_INITIAL_DEVICE_NAME);
                        break;
                    case BM78_CMD_WRITE_ADV_DATA:
                        BM78_GetAdvData(SUM_advData);
                        BM78_write(BM78_CMD_WRITE_ADV_DATA, BM78_EEPROM_STORE, 22, SUM_advData);
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}

#endif
#endif