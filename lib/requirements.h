/* 
 * File:   requirements.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */

#ifndef REQUIREMENTS_H
#define	REQUIREMENTS_H

#ifdef	__cplusplus
extern "C" {
#endif

//--msgdisable=520,2053
#pragma warning disable 520
#pragma warning disable 2053

#include "../../mcc_generated_files/mcc.h"

#ifndef BM78_DISABLE
#if defined BM78_SW_BTN_PORT && defined BM78_RST_N_PORT && defined BM78_P2_0_PORT && defined BM78_P2_4_PORT && defined BM78_EAN_PORT
#define BM78_ENABLED
#endif
#endif

#include "../../config.h"

#if defined BM78_ENABLED || defined USB_ENABLED
#define SCOM_ENABLED
#endif
    
#if defined BM78_ENABLED && !defined BM78_DATA_PART
#error "BM78_DATA_PART must to be defined!"
#endif
    
#if defined SM_MEM_ADDRESS && !defined SM_DATA_PART
#error "SM_DATA_PART must to be defined!"
#endif

#if defined PIR_PORT && !defined IO_PIR
#error "IO_PIR needs to be define the PIR's IO port number"
#endif
    
#if defined MCP2200_ENABLED || defined MCP2221_ENABLED
#define USB_ENABLED
#endif
    
#if (defined MCP23017_ENABLED || defined LCD_ENABLED) && !defined I2C_ENABLED
#define I2C_ENABLED
#endif

#if defined TESTER_ADDRESS && !defined TESTER_PORT
#define TESTER_PORT MCP23017_PORTA
#endif

#ifndef RGB_DISABLE
#if defined RGB_R_DUTY_CYCLE || defined RGB_G_DUTY_CYCLE || defined RGB_B_DUTY_CYCLE
#define RGB_ENABLED
#endif
#endif
    
#if !defined UART_ENABLED && (defined UART1_ENABLED || defined UART2_ENABLED)
#define UART_ENABLED
#endif

#ifndef SCOM_MAX_PACKET_SIZE
#if defined WS281x_BUFFER && defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
#warning "REQ: SCOM_MAX_PACKET_SIZE = 33 (WS281x Light)"
#define SCOM_MAX_PACKET_SIZE 33
#elif defined BM78_ENABLED
#warning "REQ: SCOM_MAX_PACKET_SIZE = 25 (BM78)"
#define SCOM_MAX_PACKET_SIZE 25 // Bluetooth settings
#elif defined LCD_ADDRESS
#warning "REQ: SCOM_MAX_PACKET_SIZE = LCD_COLS + 5 (LCD)"
#define SCOM_MAX_PACKET_SIZE LCD_COLS + 4 + 1
#elif defined RGB_ENABLED
#warning "REQ: SCOM_MAX_PACKET_SIZE = 13 (RGB)"
#define SCOM_MAX_PACKET_SIZE 13
#elif defined WS281x_BUFFER
#warning "REQ: SCOM_MAX_PACKET_SIZE = 12 (WS281x)"
#define SCOM_MAX_PACKET_SIZE 12
#elif defined MCP23017_ENABLED
#warning "REQ: SCOM_MAX_PACKET_SIZE = 5 (MCP23017)"
#define SCOM_MAX_PACKET_SIZE 5
#elif defined DHT11_PORT
#warning "REQ: SCOM_MAX_PACKET_SIZE = 4 (DHT11)"
#define SCOM_MAX_PACKET_SIZE 4
#elif defined PIR_PORT
#warning "REQ: SCOM_MAX_PACKET_SIZE = 3 (PIR)"
#define SCOM_MAX_PACKET_SIZE 3
#endif
#endif
    
// Message kinds:
typedef enum {
    MESSAGE_KIND_CRC = 0x00,
    MESSAGE_KIND_IDD = 0x01,
    MESSAGE_KIND_CONSISTENCY_CHECK = 0x02,
    MESSAGE_KIND_DATA = 0x03,
    MESSAGE_KIND_PLAIN = 0x0F,
    MESSAGE_KIND_IO = 0x10,
#ifdef DHT11_PORT
    MESSAGE_KIND_TEMP = 0x11,
#endif
#ifdef LCD_ADDRESS
    MESSAGE_KIND_LCD = 0x12,
#endif
    MESSAGE_KIND_REGISTRY = 0x13,
#ifdef RGB_ENABLED
    MESSAGE_KIND_RGB = 0x15,
#endif
#ifdef WS281x_BUFFER
#ifdef WS281x_INDICATORS
    MESSAGE_KIND_INDICATORS = 0x16,
#endif
#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
    MESSAGE_KIND_LIGHT = 0x17,
#endif
#endif
#ifdef BM78_ENABLED
    MESSAGE_KIND_BLUETOOTH = 0x20,
#endif
#ifdef SM_MEM_ADDRESS
    MESSAGE_KIND_SM_STATE_ACTION = 0x80,
    MESSAGE_KIND_SM_INPUT = 0x81,
#endif
    MESSAGE_KIND_DEBUG = 0xFE,
    MESSAGE_KIND_NONE = 0xFF    
} MessageKind_t;

#ifdef	__cplusplus
}
#endif

#endif	/* REQUIREMENTS_H */

