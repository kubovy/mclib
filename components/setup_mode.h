/* 
 * File:   setup_mode.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * The setup mode using LCD, keypad and/or IOs.
 */
#ifndef SETUP_MODE_H
#define	SETUP_MODE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../lib/requirements.h"
#include "../modules/bm78.h"

#ifdef LCD_ADDRESS

#ifdef BM78_ENABLED
#include "../modules/bm78.h"
#endif

typedef enum {
    //SUM_MENU_INTRO 0,
    SUM_MENU_MAIN = 0x01,

#ifdef BM78_ENABLED
    SUM_MENU_BT_PAGE_1 = 0x10,
    SUM_MENU_BT_PAGE_2 = 0x11,
    SUM_MENU_BT_PAGE_3 = 0x12,
    SUM_MENU_BT_PAGE_4 = 0x13,
    SUM_MENU_BT_STATE = 0x15,
    SUM_MENU_BT_PAIRED_DEVICES = 0x16,
    SUM_MENU_BT_PAIRED_DEVICE = 0x17,
    SUM_MENU_BT_PAIRING_MODE = 0x18,
    SUM_MENU_BT_SHOW_DEVICE_NAME = 0x19,
    SUM_MENU_BT_PIN_SETUP = 0x1A,
    SUM_MENU_BT_SHOW_REMOTE_DEVICE = 0x1B,
    SUM_MENU_BT_INITIALIZE = 0x1C,
    SUM_MENU_BT_READ_CONFIG = 0x1D,
    SUM_MENU_BT_CONFIG_VIEWER = 0x1E,
    SUM_MENU_BT_SHOW_MAC_ADDRESS = 0x1F,
#endif

#if defined MCP2200_ENABLED || defined MCP2221_ENABLED || defined MCP23017_ENABLED
    SUM_MENU_MCP_MAIN = 0x20,
#ifdef MCP23017_ENABLED
    SUM_MENU_MCP_23017_MAIN = 0x25,
    SUM_MENU_MCP_23017_IN = 0x26,
    SUM_MENU_MCP_23017_OUT = 0x27,
#endif
#endif

#if defined MEM_ADDRESS || defined MEM_INTERNAL_SIZE
    SUM_MENU_MEM_MAIN = 0x30,
    SUM_MENU_MEM_VIEWER_INTRO = 0x31,
    SUM_MENU_MEM_VIEWER = 0x32,
#endif

    SUM_MENU_TEST_PAGE_1 = 0x40,
    SUM_MENU_TEST_PAGE_2 = 0x41,
    SUM_MENU_TEST_PAGE_3 = 0x42,
    SUM_MENU_TEST_PAGE_4 = 0x43,
    SUM_MENU_TEST_PAGE_5 = 0x44,

#ifdef RGB_ENABLED
    SUM_MENU_TEST_RGB_PAGE_1 = 0x50,
    SUM_MENU_TEST_RGB_PAGE_2 = 0x51,
#endif
            
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
    SUM_MENU_TEST_WS281x_PAGE_1 = 0x60,
    SUM_MENU_TEST_WS281x_PAGE_2 = 0x61,
    SUM_MENU_TEST_WS281x_PAGE_3 = 0x62,
    SUM_MENU_TEST_WS281x_PAGE_4 = 0x63,
#endif

#ifdef DHT11_PORT
    SUM_MENU_TEST_DHT11 = 0x70,
#endif

    SUM_MENU_UNKNOWN = 0xFF
} SUM_MenuPage_t;

/** SetUp Mode on/off. */
bool SUM_mode = false;

#ifdef MCP23017_ENABLED
/**
 * Propaged change of MCP23017.
 * 
 * @param address Address of MCP23017.
 */
void SUM_mcpChanged(uint8_t address);
#endif

/** 
 * Process pressed keypad key. 
 * 
 * @param key Key byte.
 */
bool SUM_processKey(uint8_t key);

#ifdef BM78_ENABLED

/**
 * BM78 application mode response handler
 * 
 * @param response BM78 response.
 */
void SUM_bm78EventHandler(BM78_Response_t *response);

#endif
#endif

#ifdef SUM_BTN_PORT
uint8_t SUM_processBtn(uint8_t maxModes);
#endif

#ifdef BM78_ENABLED

/**
 * BM78 test mode response handler.
 * 
 * @param response BM78 response.
 */
void SUM_bm78TestModeResponseHandler(BM78_Response_t *response);

/**
 * BM78 error handler.
 * 
 * @param response BM78 response.
 */
void SUM_bm78ErrorHandler(BM78_Response_t *response);

/**
 * BM78 EEPROM Initialized handler.
 */
void SUM_bm78EEPROMInitializationSuccessHandler(void);

/**
 * BM78 EEPROM Initialization failed handler.
 */
void SUM_bm78EEPROMInitializationFailedHandler(void);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SETUP_MODE_H */

