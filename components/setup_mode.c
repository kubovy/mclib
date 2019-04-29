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

uint8_t setupModePush = 5;
uint8_t setupPage = 0;

struct {
    bool ws281xDemo   :1;
    bool lcdBacklight :1;
    bool showKeypad   :1;
    bool showBtErrors :1;
} sw = {false, true, false, false};

struct {
    uint8_t id;
    char name[10]; // = "         \0";
    
} bm78CurrentState = { 0x00 };

uint16_t bm78ConfigAddr = 0x0000;
uint16_t memAddr = 0x0000;
char lcdCache[LCD_ROWS][LCD_COLS + 1];

struct {
    uint8_t addr;
    uint8_t port;
} mcpTest = { MCP_START_ADDRESS, MCP_PORTA };


struct {
    uint8_t timeout;
    uint8_t stage;
    uint8_t retries;
    uint8_t index;
} bm78ConfigCommand = { 0xFF, 0, 0, 0 };

struct {
    uint8_t position;
    uint16_t value;
} manualAddress = {0xFF, 0x0000};

struct {
    uint8_t position;
    char value[6];    
} manualString = {0xFF};

struct {
    uint8_t selected; // Selected device index.
    uint8_t page;
} pairedDevices = {0, 0};

#ifdef BM78_ENABLED

void SUM_setBtStatus(uint8_t status) {
    switch(status) {
        case BM78_STATUS_POWER_ON:
            strcpy(bm78CurrentState.name, "POWER ON ", 9);
            break;
        case BM78_STATUS_PAGE_MODE:
            strcpy(bm78CurrentState.name, "PAGE MODE", 9);
            break;
        case BM78_STATUS_STANDBY_MODE:
            strcpy(bm78CurrentState.name, "STAND BY ", 9);
            break;
        case BM78_STATUS_LINK_BACK_MODE:
            strcpy(bm78CurrentState.name, "LINK BACK", 9);
            break;
        case BM78_STATUS_SPP_CONNECTED_MODE:
            strcpy(bm78CurrentState.name, "SPP CONN ", 9);
            break;
        case BM78_STATUS_LE_CONNECTED_MODE:
            strcpy(bm78CurrentState.name, "LE CONN  ", 9);
            break;
        case BM78_STATUS_IDLE_MODE:
            strcpy(bm78CurrentState.name, "IDLE     ", 9);
            break;
        case BM78_STATUS_SHUTDOWN_MODE:
            strcpy(bm78CurrentState.name, "SHUT DOWN", 9);
            break;
        default:
            strcpy(bm78CurrentState.name, "UNKNOWN  ", 9);
            break;
    }
}

void SUM_getDeviceHWAddress(uint8_t index, uint8_t start, char *message) {
    if (index < BM78.pairedDevicesCount) {
        for (uint8_t j = 0; j < 6; j++) {
            if (j > 0) message[j * 3 + 2] = ':';
            message[j * 3 + start] = dec2hex(BM78.pairedDevices[index].address[5 - j] / 16 % 16);
            message[j * 3 + start + 1] = dec2hex(BM78.pairedDevices[index].address[5 - j] % 16);
        }
    }
}

#endif

void SUM_mcpChanged(uint8_t address) {
    if (SUM_mode && setupPage == SUM_MENU_TEST_MCP_IN) {
        mcpTest.addr = address;
        POC_testMCP23017Input(mcpTest.addr);
    }
}

void SUM_showMenu(uint8_t page) {
    setupPage = page;
    LCD_clear();
    switch (setupPage) {
        case SUM_MENU_INTRO: // Introduction
            LCD_displayString("|c|SetUp Mode", 0);
            LCD_displayString("|c|Activated", 1);
            LCD_displayString("|r|C) Confirm", 3);
            break;
        case SUM_MENU_MAIN: // Main Menu
#ifdef BM78_ENABLED
            LCD_displayString("1) Bluetooth", 0);
#else
            LCD_displayString("-) Bluetooth", 0);
#endif
#ifdef MEM_ADDRESS
            LCD_displayString("2) Memory", 1);
#else
            LCD_displayString("-) Memory", 1);
#endif
            LCD_displayString("3) Tests", 2);
            LCD_displayString("|r|Exit (D", 3);
            break;
#ifdef BM78_ENABLED
        case SUM_MENU_BT_PAGE_1: // Bluetooth Menu (Page 1)
            SUM_setBtStatus(bm78CurrentState.id);
            LCD_displayString("1) State [         ]", 0);
            LCD_replaceString(bm78CurrentState.name, 10, 0);
            LCD_displayString("2) Paired Devices", 1);
            LCD_displayString("3) Pairing Mode", 2);
            LCD_displayString("B) Back      Next (C", 3);
            break;
        case SUM_MENU_BT_PAGE_2: // Bluetooth Menu (Page 2)
            LCD_displayString("1) Device Name", 0);
            LCD_displayString("2) PIN   [      ]", 1);
            LCD_replaceString(BM78.pin, 10, 1);
            if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE 
                    || BM78.status == BM78_STATUS_LE_CONNECTED_MODE) {
                LCD_displayString("3) Remote Device", 2);
            }
            LCD_displayString("B) Back      Next (C", 3);
            break;
        case SUM_MENU_BT_PAGE_3: // Bluetooth Menu (Page 3)
            LCD_displayString("1) Show Errors   [ ]", 0);
            LCD_displayString("2) Read Config", 1);
            if (sw.showBtErrors) LCD_replaceChar('X', 18, 0);
            LCD_displayString("B) Back      Next (C", 3);
            break;
        case SUM_MENU_BT_PAGE_4: // Bluetooth Menu (Page 4)
            LCD_displayString("1) Init Dongle", 0);
            LCD_displayString("2) Init Application", 1);
            LCD_displayString("B) Back", 3);
            break;
        case SUM_MENU_BT_STATE: // Bluetooth: State
            LCD_displayString("State:          ", 0);
            LCD_replaceString(bm78CurrentState.name, 7, 0);
            if (BM78.status== BM78_STATUS_IDLE_MODE) {
                LCD_displayString("1) Enter Stand-By", 1);
            } else if (BM78.status == BM78_STATUS_STANDBY_MODE) {
                LCD_displayString("1) Leave Stand-By", 1);
            } else if (BM78.status == BM78_STATUS_SPP_CONNECTED_MODE) {
                LCD_displayString("1) Disconnect SPP", 1);
            } else if (BM78.status == BM78_STATUS_LE_CONNECTED_MODE) {
                LCD_displayString("1) Disconnect LE", 1);
            }
            LCD_displayString("B) Back", 3);
            break;
        case SUM_MENU_BT_PAIRED_DEVICES: // Bluetooth: Paired Devices
            if (BM78.pairedDevicesCount == 0) { // No paired devices yet
                LCD_displayString("*: Get devices", 0);
                LCD_displayString("#: Clear all paired devices", 1);
                LCD_displayString("B) Back", 3);
            } else { // Paired devices retrieved
                for (uint8_t i = 0; i < 3; i++) {
                    uint8_t index = pairedDevices.page * 3 + i;
                    if (index < BM78.pairedDevicesCount) {
                        char message[LCD_COLS + 1] = "-) ##:##:##:##:##:##\0";
                        message[0] = i + 49;
                        SUM_getDeviceHWAddress(index, 3, message);
                        LCD_displayString(message, i);
                    }
                }
                if ((pairedDevices.page + 1) * 3 < BM78.pairedDevicesCount) {
                    LCD_displayString("B) Back      Next (C", 3);
                } else {
                    LCD_displayString("B) Back", 3);
                }
            }
            break;
        case SUM_MENU_BT_PAIRED_DEVICE: {
                char message[LCD_COLS + 1] = "|c|##:##:##:##:##:##\0";
                SUM_getDeviceHWAddress(pairedDevices.selected, 3, message);
                LCD_displayString(message, 0);
                LCD_displayString("1) Remove Device", 1);
                LCD_displayString("B) Back", 3);
            }
            break;
        case SUM_MENU_BT_PAIRING_MODE:
            LCD_displayString("1) PIN", 0);
            if (BM78.pairingMode == BM78_PAIRING_PIN) LCD_replaceChar('X', 0, 0);
            LCD_displayString("2) Just Work ", 1);
            if (BM78.pairingMode == BM78_PAIRING_JUST_WORK) LCD_replaceChar('X', 0, 1);
            LCD_displayString("3) Passkey ", 2);
            if (BM78.pairingMode == BM78_PAIRING_PASSKEY) LCD_replaceChar('X', 0, 2);
            LCD_displayString("4) Confirm   Back (B", 3);
            if (BM78.pairingMode == BM78_PAIRING_USER_CONFIRM) LCD_replaceChar('X', 0, 3);
            break;
        case SUM_MENU_BT_SHOW_DEVICE_NAME:
            LCD_displayString(BM78.deviceName, 0);
            LCD_displayString("1) Refresh", 2);
            LCD_displayString("B) Back", 3);
            break;
        case SUM_MENU_BT_PIN_SETUP:
            LCD_displayString("|c|PIN", 0);
            LCD_replaceString(BM78.pin, 7, 1);
            LCD_displayString("1) Change", 2);
            LCD_displayString("2) Refresh   Back (B", 3);
            break;
        case SUM_MENU_BT_SHOW_REMOTE_DEVICE:
            LCD_displayString("1) Refresh", 2);
            LCD_displayString("B) Back", 3);
            break;
        case SUM_MENU_BT_INITIALIZE_DONGLE:
            LCD_displayString("|c|Initializing BT", 1);
            LCD_displayString("|c|(please wait...)", 2);
            break;
        case SUM_MENU_BT_INITIALIZE_APP:
            LCD_displayString("|c|Initializing App", 1);
            LCD_displayString("|c|(please wait...)", 2);
            break;
        case SUM_MENU_BT_READ_CONFIG:
            LCD_displayString("Memory controls:", 0);
            LCD_displayString("B: Previous, C: Next", 1);
            LCD_displayString("A: Abort, D: Reset", 2);
            LCD_displayString("#: Address  Con't (C", 3);
            break;
#endif
#ifdef MEM_ADDRESS
        case SUM_MENU_MEM_MAIN: // Memory Menu
            LCD_displayString("1) Memory Viewer", 0);
#ifdef SM_MEM_ADDRESS
            if (MEM_read(SM_MEM_ADDRESS, 0x00, 0x00) == 0x00) {
                LCD_displayString("2) Lock SM       [ ]", 1);
            } else {
                LCD_displayString("2) Unlock SM     [X]", 1);
            }
#endif
            LCD_displayString("B) Back", 3);
            break;
        case SUM_MENU_MEM_VIEWER_INTRO: // Memory Viewer Introduction
            LCD_displayString("Memory controls:", 0);
            LCD_displayString("B: Previous, C: Next", 1);
            LCD_displayString("A: Abort, D: Reset", 2);
            LCD_displayString("#: Address  Con't (C", 3);
            break;
#endif
        case SUM_MENU_TEST_PAGE_1: // Tests Menu (Page 1)
            LCD_displayString("1) LCD Test", 0);
            LCD_displayString(sw.lcdBacklight ? "2) LCD Backlight [X]" : "2) LCD Backlight [ ]", 1);
            LCD_displayString("B) Back      Next (C", 3);
            break;
        case SUM_MENU_TEST_PAGE_2: // Tests Menu (Page 2)
            LCD_displayString("1) MCP23017 IN Test", 0);
            LCD_displayString("2) MCP23017 OUT Test", 1);
            LCD_displayString("B) Back      Next (C", 3);
            break;
        case SUM_MENU_TEST_PAGE_3: // Tests Menu (Page 3)
#ifdef WS281x_BUFFER
            LCD_displayString("1) WS281x Test", 0);
            LCD_displayString(sw.ws281xDemo ? "2) WS281x Demo   [X]" : "2) WS281x Demo   [ ]", 1);
#else
            LCD_displayString("-) WS281x Test", 0);
            LCD_displayString("-) WS281x Demo", 1);
#endif
            LCD_displayString(sw.showKeypad ? "3) Show Keypad   [X]" : "3) Show Keypad   [ ]", 2);
            LCD_displayString("B) Back", 3);
            break;
        case SUM_MENU_TEST_MCP_IN:
            POC_testMCP23017Input(mcpTest.addr);
            LCD_displayString("B) Back      Next (C", 3);
            break;
        case SUM_MENU_TEST_MCP_OUT:
            if (mcpTest.addr == 0) {
                LCD_displayString("0) 0x20      0x24 (4", 0);
                LCD_displayString("1) 0x21      0x25 (5", 1);
                LCD_displayString("2) 0x22      0x26 (6", 2);
                LCD_displayString("3) 0x23      0x27 (7", 3);
            } else if (mcpTest.port == 0xFF) {
                LCD_displayString("A) Port A", 0);
                LCD_displayString("B) Port B", 1);
            } else {
                POC_testMCP23017Output(mcpTest.addr, mcpTest.port);
                LCD_displayString("B) Back    Repeat (C", 3);
            }
            break;
    }
}

void SUM_defaultFunction(uint8_t key) {
    switch(key) {
        case '*':
            LCD_init();
            LCD_setBacklight(true);
            SUM_showMenu(setupPage);
            break;
        case '#':
            SUM_showMenu(setupPage);
            break;
        case '0':
            SUM_showMenu(SUM_MENU_MAIN);
            break;
    }
}

void SUM_executeMenu(uint8_t key) {
    uint8_t byte;
    switch (setupPage) {
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
                    for (uint8_t r = 0; r < LCD_ROWS; r++) {
                        LCD_displayString(lcdCache[r], r);
                    }
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
                    bm78ConfigCommand.index = 0;
                    bm78ConfigCommand.stage = 0;
                    bm78ConfigCommand.timeout = 200;
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
                    pairedDevices.selected = pairedDevices.page * 3 + (key - 49);
                    SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICE);
                    break;
                case '*': // Refresh
                    BM78_execute(BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO, 0);
                    break;
                case '#': // Erase all paired devices
                    BM78_execute(BM78_CMD_ERASE_ALL_PAIRED_DEVICE_INFO, 0);
                    pairedDevices.page = 0;
                    BM78.pairedDevicesCount = 0;
                    SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICES);
                    break;
                case 'B': // Back
                    if (pairedDevices.page == 0 || BM78.pairedDevicesCount == 0) {
                        SUM_showMenu(SUM_MENU_BT_PAGE_1);
                    } else {
                        pairedDevices.page--;
                        SUM_showMenu(SUM_MENU_BT_PAIRED_DEVICES);
                    }
                    break;
                case 'C': // Next
                    if ((pairedDevices.page + 1) * 3 < BM78.pairedDevicesCount) {
                        pairedDevices.page++;
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
                    if (pairedDevices.selected < BM78.pairedDevicesCount) {
                        uint8_t index = BM78.pairedDevices[pairedDevices.selected].index;
                        BM78_execute(BM78_CMD_DELETE_PAIRED_DEVICE, 1, index);
                        pairedDevices.page = 0;
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
            if (manualString.position < 0xFF) {
                if (manualString.position < 6 && key >= '0' && key <= '9') {
                    manualString.value[manualString.position] = key;
                    LCD_replaceChar(key, manualString.position + 7, 1);
                    manualString.position++;
                    if (manualString.position >= 4) LCD_displayString("A) Abort  Confirm (C", 3);
                } else switch(key) {
                    case 'C': // Confirm
                        if (manualString.position >= 4) {
                            manualString.value[manualString.position] = '\0';
                            BM78_write(BM78_CMD_WRITE_PIN_CODE, BM78_EEPROM_STORE, (uint8_t *) manualString.value);
                        } else break;
                    case 'A': // Abort
                        manualString.position = 0xFF;
                        SUM_showMenu(SUM_MENU_BT_PIN_SETUP);
                        break;
                }
            } else switch(key) {
                case '1': // Change
                    LCD_displayString("       ......       ", 1);
                    LCD_displayString(" ", 2);
                    LCD_displayString("A) Abort", 3);
                    manualString.position = 0;
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
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;   
            }
            break;
        case SUM_MENU_BT_CONFIG_VIEWER:
            if (manualAddress.position < 0xFF) { // Enter manual address
                char hex;
                uint8_t value;
                
                if (key == 35) hex = 69; // # -> E
                else if (key == 42) hex = 70; // * -> F
                else hex = key;
                
                LCD_replaceChar(hex, 6 + (3 - manualAddress.position), 2);

                value = hex2dec(hex);

                manualAddress.value = manualAddress.value + (value << (manualAddress.position * 4));
                if (manualAddress.position > 0) {
                    manualAddress.position--;
                } else {
                    bm78ConfigAddr = manualAddress.value / 16 * 16;
                    manualAddress.value = 0x0000;
                    manualAddress.position = 0xFF;
                    BM78_readEEPROM(bm78ConfigAddr, 16);
                }
            } else switch(key) { // Browse Memory
                case 'B': // Previous
                    if (bm78ConfigAddr > 0) {
                        BM78_readEEPROM(bm78ConfigAddr - 16, 16);
                    }
                    break;
                case 'C': // Next
                    if (bm78ConfigAddr < (BM78_EEPROM_SIZE - 16)) {
                        BM78_readEEPROM(bm78ConfigAddr + 16, 16);
                    }
                    break;
                case 'A': // Abort
                    //BM78_closeEEPROM();
                    BM78_resetToAppMode();
                    SUM_showMenu(SUM_MENU_BT_PAGE_3);
                    break;
                case 'D': // Reset
                    bm78ConfigAddr = 0x0000;
                    BM78_readEEPROM(bm78ConfigAddr, 16);
                    break;
                case '*':
                    LCD_init();
                    LCD_setBacklight(true);
                    BM78_readEEPROM(bm78ConfigAddr, 16);
                    break;
                case '#':
                    LCD_clear();
                    LCD_displayString("GOTO: ....", 2);
                    manualAddress.position = 3;
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
                    setupPage = SUM_MENU_MEM_VIEWER;
                    POC_testMem(memAddr);
                    break;
                default:
                    SUM_defaultFunction(key);
                    break;
            }
            break;
        case SUM_MENU_MEM_VIEWER: // Memory Viewer
            if (manualAddress.position < 0xFF) { // Enter manual address
                char hex;
                uint8_t value;
                
                if (key == 35) hex = 69; // # -> E
                else if (key == 42) hex = 70; // * -> F
                else hex = key;
                
                LCD_replaceChar(hex, 6 + (3 - manualAddress.position), 2);

                value = hex2dec(hex);

                manualAddress.value = manualAddress.value + (value << (manualAddress.position * 4));
                if (manualAddress.position > 0) {
                    manualAddress.position--;
                } else {
                    memAddr = manualAddress.value / 16 * 16;
                    manualAddress.value = 0x0000;
                    manualAddress.position = 0xFF;
                    POC_testMem(memAddr);
                }
            } else switch(key) { // Browse Memory
                case 'B': // Previous
                    if (memAddr > 0) memAddr -= 16;
                    POC_testMem(memAddr);
                    break;
                case 'C': // Next
                    if (memAddr < (MEM_SIZE - 16)) memAddr += 16;
                    POC_testMem(memAddr);
                    break;
                case 'A': // Abort
                    SUM_showMenu(SUM_MENU_MEM_MAIN);
                    break;
                case 'D': // Reset
                    memAddr = 0x0000;
                    POC_testMem(memAddr);
                    break;
                case '*':
                    LCD_init();
                    LCD_setBacklight(true);
                    POC_testMem(memAddr);
                    break;
                case '#':
                    LCD_clear();
                    LCD_displayString("GOTO: ....", 2);
                    manualAddress.position = 3;
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
                    mcpTest.addr = MCP_START_ADDRESS;
                    SUM_showMenu(SUM_MENU_TEST_MCP_IN);
                    break;
                case '2': // MCP23017 Output Test
                    mcpTest.addr = 0x00;
                    mcpTest.port = 0xFF;
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
                    if (mcpTest.addr == 0) {
                        SUM_showMenu(SUM_MENU_TEST_PAGE_2);
                    } else {
                        mcpTest.addr--;
                        SUM_showMenu(SUM_MENU_TEST_MCP_IN);
                    }
                    break;
                case 'C': // Next
                    mcpTest.addr = mcpTest.addr == (MCP_END_ADDRESS - MCP_START_ADDRESS) 
                            ? 0 : mcpTest.addr + 1;
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
                    if (mcpTest.addr == 0) mcpTest.addr = MCP_START_ADDRESS + (key - '0');
                    SUM_showMenu(SUM_MENU_TEST_MCP_OUT);
                    break;
                case 'A':
                case 'B':
                    if (mcpTest.addr >= MCP_START_ADDRESS && mcpTest.port == 0xFF) {
                        mcpTest.port = MCP_PORTA + (key - 'A');
                        SUM_showMenu(SUM_MENU_TEST_MCP_OUT);
                    } else {
                        SUM_showMenu(SUM_MENU_TEST_PAGE_2);
                    }
                    break;
                case 'C':
                    if (mcpTest.addr >= MCP_START_ADDRESS && mcpTest.port != 0xFF) {
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
            if (sw.showKeypad) LCD_replaceChar(key, 10, 3);
            SUM_executeMenu(key);
        } else if (key == 'D') {
            setupModePush--;
            if (setupModePush == 0) {
                SUM_mode = true;
                for (uint8_t r = 0; r < LCD_ROWS; r++) {
                    for (uint8_t c = 0; c < LCD_COLS; c++) {
                        lcdCache[r][c] = LCD_content[r][c];
                    }
                    lcdCache[r][LCD_COLS] = '\0';
                }
                SUM_showMenu(0);
                setupModePush = 5;
            }
        } else {
            setupModePush = 5;
        }
    }
}

#ifdef BM78_ENABLED

void SUM_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data) {
    if (SUM_mode) switch (response.op_code) {
        case BM78_OPC_COMMAND_COMPLETE:
            switch(response.CommandComplete_0x80.command) {
                case BM78_CMD_READ_DEVICE_NAME:
                    if (setupPage == SUM_MENU_BT_SHOW_DEVICE_NAME) {
                        SUM_showMenu(setupPage);
                    } else if (setupPage == SUM_MENU_BT_INITIALIZE_APP) {
                        uint8_t advData[22];
                        if (strcmp(BT_INITIAL_DEVICE_NAME, (char *) data)) {
                            BM78_GetAdvData(advData);
                            BM78_write(BM78_CMD_WRITE_ADV_DATA, BM78_EEPROM_STORE, advData);
                        } else {
                            BM78_write(BM78_CMD_WRITE_DEVICE_NAME, BM78_EEPROM_STORE, (uint8_t *) BT_INITIAL_DEVICE_NAME);
                        }
                    }
                    break;
                case BM78_CMD_WRITE_DEVICE_NAME:
                    if (setupPage == SUM_MENU_BT_INITIALIZE_APP) {
                        BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                    }
                    break;
                case BM78_CMD_WRITE_ADV_DATA:
                    if (setupPage == SUM_MENU_BT_INITIALIZE_APP) {
                        SUM_showMenu(SUM_MENU_BT_PAGE_4);
                    }
                    break;
                case BM78_CMD_READ_PAIRING_MODE_SETTING:
                    if (setupPage == SUM_MENU_BT_PAIRING_MODE) {
                        SUM_showMenu(setupPage);
                    }
                    break;
                case BM78_CMD_WRITE_PAIRING_MODE_SETTING:
                    BM78_execute(BM78_CMD_READ_PAIRING_MODE_SETTING, 0);
                    break;
                case BM78_CMD_READ_ALL_PAIRED_DEVICE_INFO:
                    if (setupPage == SUM_MENU_BT_PAIRED_DEVICES) {
                        pairedDevices.page = 0;
                        SUM_showMenu(setupPage);
                    }
                    break;
                case BM78_CMD_READ_REMOTE_DEVICE_NAME:
                    if (setupPage == SUM_MENU_BT_SHOW_REMOTE_DEVICE) {
                        char remoteDeviceName[LCD_COLS + 1];
                        uint8_t i;
                        for (i = 0; i < response.CommandComplete_0x80.length - 1; i++) {
                            if (i < LCD_COLS) remoteDeviceName[i] = data[i];
                        }
                        remoteDeviceName[i] = '\0';
                        LCD_displayString(remoteDeviceName, 0);
                    }
                    break;
                case BM78_CMD_READ_PIN_CODE:
                    if (setupPage == SUM_MENU_BT_PIN_SETUP) {
                        SUM_showMenu(setupPage);
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
            if (bm78CurrentState.id != response.StatusReport_0x81.status) {
                bm78CurrentState.id = response.StatusReport_0x81.status;
                SUM_setBtStatus(bm78CurrentState.id);
                if (setupPage == SUM_MENU_BT_PAGE_1
                        || setupPage == SUM_MENU_BT_STATE) {
                    SUM_showMenu(setupPage);
                }
            }
            break;
        default:
            break;
    }
}

void SUM_bm78TestModeCheck(void) {
    if (SUM_mode && bm78ConfigCommand.timeout < 0xFF) {
        if (bm78ConfigCommand.timeout == 0) {
            bm78ConfigCommand.timeout = 0xFF;
            Flash32_t config = BM78_configuration[bm78ConfigCommand.index];

            __delay_ms(500);
            if (bm78ConfigCommand.stage == 0 || bm78ConfigCommand.retries >= 3) {
                BM78_resetToAppMode();
                __delay_ms(1000);
                BM78_resetToTestMode();
                __delay_ms(1000);
                BM78_openEEPROM();
                bm78ConfigCommand.stage = 0;
                bm78ConfigCommand.retries = 0;
            } else if (bm78ConfigCommand.stage == 1) {
                BM78_clearEEPROM();
            } else if (bm78ConfigCommand.retries >= 0x80) {
                BM78_writeEEPROM(config.address, config.length, config.data);
            } else {
                BM78_readEEPROM(config.address, config.length);
            }
            
            bm78ConfigCommand.timeout = 200;
            bm78ConfigCommand.retries++;
        } else {
            bm78ConfigCommand.timeout--;
        }
    }
}

void SUM_bm78TestModeResponseHandler(uint8_t length, uint8_t *data) {
    Flash32_t config;

    switch (setupPage) {
        case SUM_MENU_BT_INITIALIZE_DONGLE:
            // EEPROM was opened
            if (length == 4 && data[0] == 0x01 && data[1] == 0x03 && data[2] == 0x0C && data[3] == 0x00) {
                printProgress("|c|Initializing BT", 1, BM78_CONFIGURATION_SIZE + 3);
                __delay_ms(500);
                BM78_clearEEPROM();
                bm78ConfigCommand.stage = 1;
                bm78ConfigCommand.timeout = 200;
                bm78ConfigCommand.retries = 0;
            }
            // EEPROM was cleared
            if (length == 4 && data[0] == 0x01 && data[1] == 0x2d && data[2] == 0xFC && data[3] == 0x00) {
                printProgress("|c|Initializing BT", 2, BM78_CONFIGURATION_SIZE + 3);
                config = BM78_configuration[bm78ConfigCommand.index];
                __delay_ms(500);
                BM78_readEEPROM(config.address, config.length);
                //BM78_writeEEPROM(config.address, config.length, config.data);
                bm78ConfigCommand.stage = 2;
                bm78ConfigCommand.timeout = 200;
                bm78ConfigCommand.retries = 0;
            }
            // Write response
            if (length == 4 && data[0] == 0x01 && data[1] == 0x27 && data[2] == 0xFC && data[3] == 0x00) {
                Flash32_t config = BM78_configuration[bm78ConfigCommand.index];
                __delay_ms(500);
                BM78_readEEPROM(config.address, config.length);
                //BM78_writeEEPROM(config.address, config.length, config.data);
                bm78ConfigCommand.stage = 2;
                bm78ConfigCommand.timeout = 200;
                bm78ConfigCommand.retries = 0;
            }
            // Read response
            if (length > 7 && data[0] == 0x01 && data[1] == 0x29 && data[2] == 0xFC && data[3] == 0x00) {
                config = BM78_configuration[bm78ConfigCommand.index];
                uint8_t size = data[6];
                bool equals;
                if (length >= size + 7) {
                    uint16_t address = (data[4] << 8) | (data[5] & 0xFF);
                    equals = address == config.address && size == config.length;
                    if (equals) for (uint8_t i = 0; i < size; i++) {
                        equals = equals && data[i + 7] == config.data[i];
                    }
                } else {
                    equals = false;
                }
                if (equals) {
                    bm78ConfigCommand.index++;
                    printProgress("|c|Initializing BT", 3 + bm78ConfigCommand.index, BM78_CONFIGURATION_SIZE + 3);
                    if (bm78ConfigCommand.index < BM78_CONFIGURATION_SIZE) {
                        config = BM78_configuration[bm78ConfigCommand.index];
                        __delay_ms(500);
                        BM78_readEEPROM(config.address, config.length);
                        //BM78_writeEEPROM(config.address, config.length, config.data);
                        bm78ConfigCommand.stage = 2;
                        bm78ConfigCommand.timeout = 200;
                        bm78ConfigCommand.retries = 0;
                    } else {
                        bm78ConfigCommand.stage = 3;
                        bm78ConfigCommand.timeout = 0xFF;
                        bm78ConfigCommand.retries = 0;
                        BM78_resetToAppMode();
                        SUM_showMenu(SUM_MENU_BT_PAGE_4);
                    }
                } else {
                    __delay_ms(500);
                    BM78_writeEEPROM(config.address, config.length, config.data);
                    bm78ConfigCommand.stage = 2;
                    bm78ConfigCommand.timeout = 200;
                    bm78ConfigCommand.retries = 0x80;
                }
            }
            break;
        case SUM_MENU_BT_READ_CONFIG:
            setupPage = SUM_MENU_BT_CONFIG_VIEWER;
            BM78_readEEPROM(bm78ConfigAddr, 16);
            break;
        case SUM_MENU_BT_CONFIG_VIEWER:
            // Read response
            if (length > 7 && data[0] == 0x01 && data[1] == 0x29 && data[2] == 0xFC && data[3] == 0x00) {
                uint8_t size = data[6];
                if (length >= size + 7) {
                    uint16_t address = (data[4] << 8) | (data[5] & 0xFF);
                    uint8_t display[16];
                    for (uint8_t i = 0; i < size; i++) {
                        if (i < 16) display[i] = data[i + 7];
                    }
                    bm78ConfigAddr = address;
                    POC_displayData(address, size, display);
                }
            } 
            break;
    }
}

void SUM_bm78ErrorHandler(BM78_Response_t response, uint8_t *data) {
    if (sw.showBtErrors) POC_bm78ErrorHandler(response, data);
    if (setupPage == SUM_MENU_BT_INITIALIZE_APP) {
        uint8_t advData[22];
        switch(response.op_code) {
            case BM78_OPC_COMMAND_COMPLETE:
                switch(response.CommandComplete_0x80.command) {
                    case BM78_CMD_READ_DEVICE_NAME:
                        BM78_execute(BM78_CMD_READ_DEVICE_NAME, 0);
                        break;
                    case BM78_CMD_WRITE_DEVICE_NAME:
                        BM78_write(BM78_CMD_WRITE_DEVICE_NAME, BM78_EEPROM_STORE, (uint8_t *) BT_INITIAL_DEVICE_NAME);
                        break;
                    case BM78_CMD_WRITE_ADV_DATA:
                        BM78_GetAdvData(advData);
                        BM78_write(BM78_CMD_WRITE_ADV_DATA, BM78_EEPROM_STORE, advData);
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