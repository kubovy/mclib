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
#include "../../config.h"

#ifdef LCD_ADDRESS

#include "../modules/bm78.h"

#define SUM_MENU_INTRO 0
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
#define SUM_MENU_BT_INITIALIZE_DONGLE 26
#define SUM_MENU_BT_INITIALIZE_APP 27
#define SUM_MENU_BT_READ_CONFIG 28
#define SUM_MENU_BT_CONFIG_VIEWER 29
#endif
#ifdef MEM_ADDRESS
#define SUM_MENU_MEM_MAIN 50
#define SUM_MENU_MEM_VIEWER_INTRO 51
#define SUM_MENU_MEM_VIEWER 52
#endif
#define SUM_MENU_TEST_PAGE_1 60
#define SUM_MENU_TEST_PAGE_2 61
#define SUM_MENU_TEST_PAGE_3 62
#define SUM_MENU_TEST_MCP_IN 70
#define SUM_MENU_TEST_MCP_OUT 71

/** SetUp Mode on/off. */
bool SUM_mode = false;

/** 
 * Process pressed keypad key. 
 * 
 * @param key Key byte.
 */
void SUM_processKey(uint8_t key);

#ifdef BM78_ENABLED

/**
 * BM78 test mode periodical check hook. 
 */
void SUM_bm78TestModeCheck(void);

/**
 * Propaged change of MCP23017.
 * 
 * @param address Address of MCP23017.
 */
void SUM_mcpChanged(uint8_t address);

/**
 * BM78 application mode response handler
 * 
 * @param response BM78 response.
 * @param data Response data.
 */
void SUM_bm78AppModeResponseHandler(BM78_Response_t response, uint8_t *data);

/**
 * BM78 test mode response handler.
 * 
 * @param length Length of response.
 * @param data Response data.
 */
void SUM_bm78TestModeResponseHandler(uint8_t length, uint8_t *data);

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

