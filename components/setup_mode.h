/* 
 * File:   setup_mode.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * A component must include configuration and can additionally include modules,
 * libs, but no components.
 */
#ifndef SETUP_MODE_H
#define	SETUP_MODE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../lib/requirements.h"
#include "../lib/common.h"
#ifdef BM78_ENABLED
#include "../modules/bm78.h"
#endif
#ifdef I2C_ENABLED
#include "../modules/i2c.h"
#endif
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif
#ifdef MCP_ENABLED
#include "../modules/mcp23017.h"
#endif
#ifdef RGB_ENABLED
#include "../modules/rgb.h"
#endif
#ifdef SM_MEM_ADDRESS
#include "../modules/state_machine.h"
#include "state_machine_interaction.h"
#endif
#ifdef WS281x_BUFFER
#include "../modules/ws281x.h"
#endif
#include "poc.h"

#ifdef LCD_ADDRESS

//#define SUM_MENU_INTRO 0
#define SUM_MENU_MAIN 1
#ifdef BM78_ENABLED
#define SUM_MENU_BT_PAGE_1 10
#define SUM_MENU_BT_PAGE_2 11
#define SUM_MENU_BT_PAGE_3 12
#define SUM_MENU_BT_PAGE_4 13
#define SUM_MENU_BT_STATE 15
#define SUM_MENU_BT_PAIRED_DEVICES 20
#define SUM_MENU_BT_PAIRED_DEVICE 21
#define SUM_MENU_BT_PAIRING_MODE 22
#define SUM_MENU_BT_SHOW_DEVICE_NAME 23
#define SUM_MENU_BT_PIN_SETUP 24
#define SUM_MENU_BT_SHOW_REMOTE_DEVICE 25
#define SUM_MENU_BT_INITIALIZE 26
#define SUM_MENU_BT_READ_CONFIG 27
#define SUM_MENU_BT_CONFIG_VIEWER 28
#define SUM_MENU_BT_SHOW_MAC_ADDRESS 29
#endif
#if defined MEM_ADDRESS || defined MEM_INTERNAL_SIZE
#define SUM_MENU_MEM_MAIN 50
#define SUM_MENU_MEM_VIEWER_INTRO 51
#define SUM_MENU_MEM_VIEWER 52
#endif
#define SUM_MENU_TEST_PAGE_1 60
#define SUM_MENU_TEST_PAGE_2 61
#define SUM_MENU_TEST_PAGE_3 62
#define SUM_MENU_TEST_PAGE_4 63
#define SUM_MENU_TEST_PAGE_5 64
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
#define SUM_MENU_TEST_WS281x_PAGE_1 70
#define SUM_MENU_TEST_WS281x_PAGE_2 71
#define SUM_MENU_TEST_WS281x_PAGE_3 72
#define SUM_MENU_TEST_WS281x_PAGE_4 73
#endif
#ifdef DHT11_PORT
#define SUM_MENU_TEST_DHT11 80
#endif
#ifdef MCP_ENABLED
#define SUM_MENU_TEST_MCP_IN 81
#define SUM_MENU_TEST_MCP_OUT 82
#endif

/** SetUp Mode on/off. */
bool SUM_mode = false;

#ifdef MCP_ENABLED
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
 * @param data Response data.
 */
void SUM_bm78EventHandler(BM78_Response_t response, uint8_t *data);

#endif
#endif

#ifdef SUM_BTN_PORT
uint8_t SUM_processBtn(uint8_t maxModes);
#endif

#ifdef BM78_ENABLED

void SUM_bm78InitializeDongle(void);

bool SUM_bm78InitializationDone(void);

/**
 * BM78 test mode periodical check hook. 
 */
void SUM_bm78TestModeCheck(void);

/**
 * BM78 test mode response handler.
 * 
 * @param length Length of response.
 * @param data Response data.
 */
void SUM_bm78TestModeResponseHandler(uint8_t length, uint8_t *data);

#ifdef LCD_ADDRESS
/**
 * BM78 error handler.
 * 
 * @param response BM78 response.
 * @param data Response data.
 */
void SUM_bm78ErrorHandler(BM78_Response_t response, uint8_t *data);
#endif

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* SETUP_MODE_H */

