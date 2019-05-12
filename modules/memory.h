/* 
 * File:   memory.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * Memory over I2C interface implementation.
 */
#ifndef MEMORY_H
#define	MEMORY_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../../config.h"

/* Configuration */
//#ifndef MEM_PAGE_SIZE
//#define MEM_PAGE_SIZE 64
//#endif

/**
 * Read byte from memory.
 * 
 * @param address I2C memory device address.
 * @param regHigh Memory registry high byte.
 * @param regLow Memory registry low byte.
 * @return Byte at register.
 */
inline uint8_t MEM_read(uint8_t address, uint8_t regHigh, uint8_t regLow);
inline uint8_t MEM_read16(uint8_t address, uint16_t reg);

/**
 * Read page from memory.
 * 
 * @param address I2C memory device address.
 * @param regHigh Memory registry start high byte.
 * @param regLow Memory registry start low byte.
 * @param length Number of bytes to read.
 * @param page Read page of bytes.
 */
//void MEM_read_page(uint8_t address, uint8_t regHigh, uint8_t regLow,
//                   uint8_t length, uint8_t *page);

/**
 * Write byte to memory.
 * 
 * @param address I2C memory device address.
 * @param regHigh Memory registry high byte.
 * @param regLow Memory registry low byte.
 * @param byte Byte to write.
 */
inline void MEM_write(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t byte);
inline void MEM_write16(uint8_t address, uint16_t reg, uint8_t byte);

/**
 * Write page to memory.
 * 
 * @param address I2C memory device address.
 * @param regHigh Memory registry start high byte.
 * @param regLow Memory registry start low byte.
 * @param length Number of bytes to write.
 * @param ... Data to write.
 */
//void MEM_write_page(uint8_t address, uint8_t regHigh, uint8_t regLow,
//                    uint8_t length, ...);

#ifdef	__cplusplus
}
#endif

#endif	/* MEMORY_H */

