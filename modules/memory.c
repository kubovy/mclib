/* 
 * File:   memory.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include <stdarg.h>
#include "memory.h"
#include "i2c.h"

inline uint8_t MEM_read(uint8_t address, uint8_t regHigh, uint8_t regLow) {
    return I2C_readRegister2(address, regHigh, regLow);
}

inline uint8_t MEM_read16(uint8_t address, uint16_t reg) {
    return MEM_read(address, reg >> 8, reg & 0xFF);
}

//void MEM_read_page(uint8_t address, uint8_t regHigh, uint8_t regLow,
//                   uint8_t length, uint8_t *page) {
//    I2C_read_data_2register(address, regHigh, regLow, page, length);
//}

inline void MEM_write(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t byte) {
    I2C_writeRegister2(address, regHigh, regLow, byte);
}

inline void MEM_write16(uint8_t address, uint16_t reg, uint8_t byte) {
    MEM_write(address, reg >> 8, reg & 0xFF, byte);
}

//void MEM_write_page(uint8_t address, uint8_t regHigh, uint8_t regLow,
//                    uint8_t length, ...) {
//    uint8_t i, data[0xFF];
//
//    va_list argp;
//    va_start(argp, n);
//    
//    data[0] = regHigh;
//    data[1] = regLow;
//    for (i = 0; i < length; i++) {
//        data[i + 2] = va_arg(argp, uint8_t);
//    }
//
//    I2C_write_data(address, data, length + 2);
//
//    va_end(argp);
//}
