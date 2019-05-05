/* 
 * File:   state_machine_interaction.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "state_machine_interaction.h"

#ifdef SM_MEM_ADDRESS

#include "../modules/bm78.h"
#include "../modules/lcd.h"
#include "../modules/mcp23017.h"
#include "../modules/state_machine.h"
#include "../modules/ws281x.h"

struct {
    bool available;
    char content[SM_VALUE_MAX_SIZE + 1];
} SMI_lcd = { false };

bool SMI_shouldTrigger = false;

void (* SMI_BluetoothTrigger)(void);

void SMI_enterState(uint8_t stateId) {
    if (!SM_enter(stateId)) {
#ifdef LCD_ADDRESS
        LCD_clear();
        LCD_setString("|c|No State Machine!", 1, true);
        LCD_setString("|c|Please upload one.", 2, true);
#endif
    }
}

void SMI_start(void) {
    SM_init();
    SMI_enterState(0);
}

void SMI_stateGetter(uint8_t *state) {
    *(state + 0) = MCP_read(SM_IN1_ADDRESS, MCP_GPIOA);
    *(state + 1) = MCP_read(SM_IN1_ADDRESS, MCP_GPIOB);
    *(state + 2) = MCP_read(SM_IN2_ADDRESS, MCP_GPIOA);
    *(state + 3) = MCP_read(SM_IN2_ADDRESS, MCP_GPIOB);
    *(state + 4) = 0;
    switch(BM78.status) {
        case BM78_STATUS_STANDBY_MODE:
        case BM78_STATUS_LINK_BACK_MODE:
            break;
        case BM78_STATUS_SPP_CONNECTED_MODE:
        case BM78_STATUS_LE_CONNECTED_MODE:
            *(state + 4) |= 0b00000001;
            break;
        default:
            //*(state + 4) &= 0b00000001;
            break;
    }
    
}

void SMI_actionHandler(uint8_t device, uint8_t length, uint8_t *value) {
    if (device >= SM_DEVICE_MCP23017_OUT_START && device <= SM_DEVICE_MCP23017_OUT_END) {
        if (length >= 1) {
            uint8_t byte = MCP_read(U3_ADDRESS, MCP_GPIOA);
            if (*value & 0x01) {
                byte |= 0x01 << (device - SM_DEVICE_MCP23017_OUT_START);
            } else {
                byte &= 0x00 << (device - SM_DEVICE_MCP23017_OUT_START);
            }
            MCP_write(U3_ADDRESS, MCP_OLATA, byte);
        }
    } else if (device >= SM_DEVICE_WS281x_START && device <= SM_DEVICE_WS281x_END) {
        if (length >= 4) {
            WS281x_set(device - SM_DEVICE_WS281x_START, *value, *(value + 1), *(value + 2), *(value + 3), 10, 0, 50);
        }
    } else if (device == SM_DEVICE_LCD_MESSAGE) {
        for (uint8_t i = 0; i < length; i++) {
            if (i < SM_VALUE_MAX_SIZE) {
                uint8_t ch = *(value + i);
                if (ch == '\\' && (i + 1) < length && *(value + i + 1) == 'n') {
                    ch = '\n';
                    i++;
                }
                // Visible chars:   LF  or       SPACE till  "~" otherwise SPACE
                SMI_lcd.content[i] = (ch == '\n' || (ch >= 0x20 && ch <= 0x7E)) ? ch : ' ';
            }
        }
        SMI_lcd.content[length] = '\0';
        SMI_lcd.available = true;
    } else if (device == SM_DEVICE_LCD_BACKLIGHT) {
        LCD_setBacklight(*value & 0x01);
    } else if (device == SM_DEVICE_LCD_RESET) {
        LCD_reset();
    } else if (device == SM_DEVICE_LCD_CLEAR) {
        LCD_clear();
    } else if (device == SM_DEVICE_BT_TRIGGER) {
        SMI_shouldTrigger = true;
    } else if (device == SM_DEVICE_GOTO) {
        // Nothing to do, handled internally
    } else if (device == SM_DEVICE_ENTER) {
        
    }
}

void SMI_evaluatedHandler(void) {
    if (SMI_lcd.available) {
        SMI_lcd.available = false;
        LCD_clear();
        LCD_setBacklight(true);
        LCD_setString(SMI_lcd.content, 0, true);
    }
    if (SMI_shouldTrigger) {
        SMI_shouldTrigger = false;
        if (SMI_BluetoothTrigger) SMI_BluetoothTrigger();
    }
}

void SMI_errorHandler(uint8_t error) {
    switch (error) {
        case SM_ERROR_LOOP:
            LCD_clear();
            LCD_setString("|c|ERROR", 0, true);
            LCD_setString("|c|Loop Detected!", 1, true);
            LCD_setString("|c|Execution Halted", 3, true);
            break;
        default:
            break;
    }
}

void SMI_setBluetoothTrigger(void (* BluetoothTrigger)(void*)) {
    SMI_BluetoothTrigger = BluetoothTrigger;
}

#endif