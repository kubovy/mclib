/* 
 * File:   state_machine_interaction.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "state_machine_interaction.h"
#include "serial_communication.h"
#include "../lib/types.h"
#include "../modules/bm78.h"
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif
#ifdef MCP23017_ENABLED
#include "../modules/mcp23017.h"
#if !defined SM_IN1_ADDRESS || !defined SM_IN2_ADDRESS
#warning "SMI: No MCP23017 used for inputs"
#endif
#ifndef SM_OUT_ADDRESS
#warning "SMI: No MCP23017 used for outputs"
#endif
#endif
#include "../modules/state_machine.h"
#ifdef WS281x_BUFFER
#include "../modules/ws281x.h"
#endif

#ifdef SM_MEM_ADDRESS

struct {
    bool available;
    char content[SM_VALUE_MAX_SIZE + 1];
} SMI_lcd = { false };

bool sendingUpdate = false;
bool shouldTrigger = false;

Procedure_t SMI_BluetoothTrigger;

void SMI_enterState(uint8_t stateId) {
    if (!SM_enter(stateId)) {
#ifdef LCD_ADDRESS
        LCD_clearCache();
        LCD_setString(" No State Machine!  ", 1, false);
        LCD_setString(" Please upload one. ", 2, false);
        LCD_displayCache();
#endif
    }
}

void SMI_start(void) {
    SM_reset();
    SM_init();
    SMI_enterState(0);
}

#if defined MCP23017_ENABLED && defined SM_IN1_ADDRESS && defined SM_IN2_ADDRESS
void SMI_stateGetter(uint8_t *state) {
    *(state + 0) = MCP23017_read(SM_IN1_ADDRESS, MCP23017_GPIOA);
    *(state + 1) = MCP23017_read(SM_IN1_ADDRESS, MCP23017_GPIOB);
    *(state + 2) = MCP23017_read(SM_IN2_ADDRESS, MCP23017_GPIOA);
    *(state + 3) = MCP23017_read(SM_IN2_ADDRESS, MCP23017_GPIOB);
    *(state + 4) = 0;
#ifdef BM78_ENABLED
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
#endif
}
#endif

void SMI_actionHandler(uint8_t device, uint8_t length, uint8_t *value) {
#if defined MCP23017_ENABLED && defined SM_OUT_ADDRESS
    if (device >= SM_DEVICE_MCP23017_OUT_START && device <= SM_DEVICE_MCP23017_OUT_END) {
        if (length >= 1) {
            uint8_t byte = MCP23017_read(SM_OUT_ADDRESS, MCP23017_GPIOA + SM_OUT_PORT);
            if (*value) {
                byte |= 0x01 << (device - SM_DEVICE_MCP23017_OUT_START);
            } else {
                byte &= ~(0x01 << (device - SM_DEVICE_MCP23017_OUT_START));
            }
            MCP23017_write(SM_OUT_ADDRESS, MCP23017_OLATA + SM_OUT_PORT, byte);
            //SCOM_sendMCP23017(SCOM_CHANNEL_BT, SM_OUT_ADDRESS);
        }
    } else
#endif
#ifdef WS281x_BUFFER
    if (device >= SM_DEVICE_WS281x_START && device <= SM_DEVICE_WS281x_END) {
        if (length >= 8) {
            WS281x_set(device - SM_DEVICE_WS281x_START,
                    *value, // Pattern
                    *(value + 1), *(value + 2), *(value + 3), // RGB
                    (*(value + 4) << 8) | (*(value + 5)), // Delay
                    *(value + 6), *(value + 7)); // Min, Max
            //SCOM_sendWS281xLED(SCOM_CHANNEL_BT, device - SM_DEVICE_WS281x_START);
        }
    } else
#endif
#ifdef LCD_ADDRESS
    if (device == SM_DEVICE_LCD_MESSAGE) {
        for (uint8_t i = 0; i < length && i < SM_VALUE_MAX_SIZE; i++) {
            uint8_t ch = *(value + i);
            // Visible chars: LF or SPACE till "~" otherwise SPACE
            SMI_lcd.content[i] = (ch == '\n' || (ch >= 0x20 && ch <= 0x7E))
                    ? ch : ' ';
        }
        SMI_lcd.content[length] = '\0';
        SMI_lcd.available = true;
    } else if (device == SM_DEVICE_LCD_BACKLIGHT) {
        LCD_setBacklight(*value & 0x01);
    } else if (device == SM_DEVICE_LCD_RESET) {
        LCD_reset();
    } else if (device == SM_DEVICE_LCD_CLEAR) {
        LCD_clear();
    } else
#endif
    if (device == SM_DEVICE_BT_CONNECTED) {
        SCOM_sendMCP23017(SCOM_CHANNEL_BT, SCOM_PARAM_ALL);
        SCOM_sendWS281xLED(SCOM_CHANNEL_BT, SCOM_PARAM_ALL);
        SCOM_sendLCD(SCOM_CHANNEL_BT, SCOM_PARAM_ALL);
    } else if (device == SM_DEVICE_BT_TRIGGER) {
        shouldTrigger = true;
    } else if (device == SM_DEVICE_GOTO) {
        // Nothing to do, handled internally
    } else if (device == SM_DEVICE_ENTER) {
        
    }
    //SCOM_sendMCP23017(SCOM_CHANNEL_BT, SM_IN1_ADDRESS);
    //SCOM_sendMCP23017(SCOM_CHANNEL_BT, SM_IN2_ADDRESS);
}

void SMI_evaluatedHandler(void) {
    SCOM_sendMCP23017(SCOM_CHANNEL_BT, SCOM_PARAM_ALL);
    SCOM_sendWS281xLED(SCOM_CHANNEL_BT, SCOM_PARAM_ALL);
    if (SMI_lcd.available) {
        SMI_lcd.available = false;
#ifdef LCD_ADDRESS
        LCD_clearCache();
        LCD_setBacklight(true);
        LCD_setString(SMI_lcd.content, 0, true);
        SCOM_sendLCD(SCOM_CHANNEL_BT, SCOM_PARAM_ALL);
#endif
    }
    if (shouldTrigger) {
        shouldTrigger = false;
        if (SMI_BluetoothTrigger) SMI_BluetoothTrigger();
    }
}

void SMI_errorHandler(uint8_t error) {
    switch (error) {
        case SM_ERROR_LOOP:
#ifdef LCD_ADDRESS
            LCD_clearCache();
            LCD_setString("       ERROR        ", 0, true);
            LCD_setString("   Loop Detected!   ", 1, true);
            LCD_setString("                    ", 2, true);
            LCD_setString("  Execution Halted  ", 3, true);
#endif
            break;
        default:
            break;
    }
}

void SMI_setBluetoothTrigger(Procedure_t BluetoothTrigger) {
    SMI_BluetoothTrigger = BluetoothTrigger;
}

#endif
