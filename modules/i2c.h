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
#include "../../config.h"

//#if defined I2C || defined I2C_MSSP || defined I2C_MSSP_FOUNDATION

#if defined I2C_MSSP
#define I2C_MAX_RETRIES 100
#endif
    
inline void I2C_init(void);

inline uint8_t I2C_readRegister(uint8_t address, uint8_t reg);

inline uint8_t I2C_readRegister2(uint8_t address, uint8_t regHigh, uint8_t regLow);

inline void I2C_writeByte(uint8_t address, uint8_t byte);

inline void I2C_writeRegister(uint8_t address, uint8_t reg, uint8_t byte);

inline void I2C_writeRegister2(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t byte);

inline void I2C_writeData(uint8_t address, uint8_t *data, uint8_t len);

//#endif

#ifdef	__cplusplus
}
#endif

#endif	/* I2C_H */

