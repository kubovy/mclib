/* 
 * File:   global.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * Global, cross-project constants.
 */
#ifndef GLOBAL_H
#define	GLOBAL_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>

// Message kinds:
#define BM78_MESSAGE_KIND_CRC 0x00
#define BM78_MESSAGE_KIND_IDD 0x01
#define BM78_MESSAGE_KIND_PLAIN 0x02
#define BM78_MESSAGE_KIND_SM_CONFIGURATION 0x80
#define BM78_MESSAGE_KIND_SM_PULL 0x81
#define BM78_MESSAGE_KIND_SM_PUSH 0x82
#define BM78_MESSAGE_KIND_SM_GET_STATE 0x83
#define BM78_MESSAGE_KIND_SM_SET_STATE 0x84
#define BM78_MESSAGE_KIND_SM_ACTION 0x85
#define BM78_MESSAGE_KIND_UNKNOWN 0xFF    

// Transmission kinds:
#define BMC_SMT_TRANSMIT_STATE_MACHINE_CHECKSUM 0x10
#define BMC_SMT_TRANSMIT_STATE_MACHINE 0x11
#define BMC_SMT_TRANSMIT_STATE_MCP23017 0x12
#define BMC_SMT_TRANSMIT_STATE_WS281x 0x13
#define BMC_SMT_TRANSMIT_STATE_LCD 0x14
#define BMC_NOTHING_TO_TRANSMIT 0xFF

typedef struct {
	uint16_t address; // Address
    uint8_t length;   // Packet length
	uint8_t data[32]; // Data (max. 32 bytes per packet)
} Flash32_t;

#ifdef	__cplusplus
}
#endif

#endif	/* GLOBAL_H */

