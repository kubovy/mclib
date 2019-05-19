/* 
 * File:   types.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */

#ifndef TYPES_H
#define	TYPES_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint16_t address; // Address
    uint8_t length;   // Packet length
	uint8_t data[32]; // Data (max. 32 bytes per packet)
} Flash32_t;

typedef struct {
    uint16_t address; // Address
    uint8_t length;   // Packet length
    uint8_t data[11]; // Data (max. 11 bytes per packet)
} FlashPacket_t;

typedef void (*Procedure_t)(void);
typedef uint8_t (*Provider_t)();
typedef void (*Consumer_t)(uint8_t);
typedef uint8_t (*Function_t)(uint8_t);
typedef bool (*Condition_t)(void);

#ifdef	__cplusplus
}
#endif

#endif	/* TYPES_H */

