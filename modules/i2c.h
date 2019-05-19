/* 
 * File:   i2c.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#ifndef I2C_H
#define	I2C_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "../lib/requirements.h"

//#ifdef I2C_ENABLED    

#if defined I2C_MSSP
#include "../../mcc_generated_files/i2c1.h"
uint8_t writeBuffer[3];
#elif defined I2C_MSSP_FOUNDATION
#include "../../mcc_generated_files/i2c1.h"
#else
#include "../../mcc_generated_files/i2c1.h"
#endif

#if defined I2C_MSSP
#define I2C_MAX_RETRIES 100
#endif

/**
 * Read one byte from a one byte register.
 * 
 * @param address I2C device's address.
 * @param reg Register.
 * @return Read byte.
 */
inline uint8_t I2C_readRegister(uint8_t address, uint8_t reg);

/**
 * Read one byte from a 2 byte register.
 * 
 * @param address I2C device's address.
 * @param regHigh Register's high byte.
 * @param regLow Register's low byte.
 * @return Read byte.
 */
inline uint8_t I2C_readRegister2(uint8_t address, uint8_t regHigh, uint8_t regLow);

/**
 * Read one from a word register.
 * 
 * @param address I2C device's address.
 * @param reg Register.
 * @return Read byte.
 */
inline uint8_t I2C_readRegister16(uint8_t address, uint16_t reg);

/**
 * Write one byte to an I2C device.
 * 
 * @param address I2C device's address.
 * @param byte Byte to write.
 */
inline void I2C_writeByte(uint8_t address, uint8_t byte);

/**
 * Write one byte to a one byte register.
 * 
 * @param address I2C device's address.
 * @param reg Register.
 * @param byte Byte to write.
 */
inline void I2C_writeRegister(uint8_t address, uint8_t reg, uint8_t byte);

/**
 * Write one byte to a 2 byte register.
 * 
 * @param address I2C device's address.
 * @param regHigh Register's high byte.
 * @param regLow Register's low byte.
 * @param byte Byte to write.
 */
inline void I2C_writeRegister2(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t byte);

/**
 * Write one byte to a one word register.
 * @param address I2C device's address.
 * @param reg Register.
 * @param byte Byte to write.
 */
inline void I2C_writeRegister16(uint8_t address, uint16_t reg, uint8_t byte);

/**
 * Writes data to an I2C device.
 * 
 * @param address I2C device's address.
 * @param len Length of the data.
 * @param data Data to write.
 */
inline void I2C_writeData(uint8_t address, uint8_t len, uint8_t *data);

//#endif

#ifdef	__cplusplus
}
#endif

#endif	/* I2C_H */

