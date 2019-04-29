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

void I2C_setup(void (* Reader)(uint8_t, uint8_t *, uint8_t), void (* Writer)(uint8_t, uint8_t*, uint8_t));

uint8_t I2C_read_register(uint8_t address, uint8_t reg);

uint8_t I2C_read_2register(uint8_t address, uint8_t regHigh, uint8_t regLow);

void I2C_read_data_register(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len);

void I2C_read_data_2register(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t *data, uint8_t len) ;

void I2C_write_byte(uint8_t address, uint8_t byte);

void I2C_write_register(uint8_t address, uint8_t reg, uint8_t byte);

void I2C_write_2register(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t byte);

void I2C_write_data(uint8_t address, uint8_t *data, uint8_t len);

#ifdef	__cplusplus
}
#endif

#endif	/* I2C_H */

