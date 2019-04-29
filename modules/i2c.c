/* 
 * File:   i2c.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "i2c.h"

void (*I2C_Reader)(uint8_t, uint8_t *, uint8_t);
void (*I2C_Writer)(uint8_t, uint8_t*, uint8_t);

void I2C_setup(void (* Reader)(uint8_t, uint8_t *, uint8_t), void (* Writer)(uint8_t, uint8_t*, uint8_t)) {
    I2C_Reader = Reader;
    I2C_Writer = Writer;
}

uint8_t I2C_read_register(uint8_t address, uint8_t reg) {
//    uint8_t data;
//    I2C_Writer(address, &reg, 1);
//    I2C_Reader(address, &data, 1);
//    return data;
    return i2c1_read1ByteRegister(address, reg);
}


uint8_t I2C_read_2register(uint8_t address, uint8_t regHigh, uint8_t regLow) {
//    uint8_t reg[2], data;
//    reg[0] = regHigh;
//    reg[1] = regLow;
//    
//    I2C_Writer(address, reg, 2);
//    I2C_Reader(address, &data, 1);
//    return data;
    return i2c1_read1ByteRegister2(address, regHigh, regLow);
}

void I2C_read_data_register(uint8_t address, uint8_t reg, uint8_t *data, uint8_t len) {
//    I2C_Writer(address, &reg, 1);
//    I2C_Reader(address, data, len);
    i2c1_readDataBlock(address, reg, data, len);
}

void I2C_read_data_2register(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t *data, uint8_t len) {
    uint8_t reg[2];
    reg[0] = regHigh;
    reg[1] = regLow;
    
    I2C_Writer(address, reg, 2);
    I2C_Reader(address, data, len);
}

void I2C_write_byte(uint8_t address, uint8_t byte) {
    i2c1_writeNBytes(address, &byte, 1);
}

void I2C_write_register(uint8_t address, uint8_t reg, uint8_t byte) {
//    uint8_t i, data[2];
//    data[0] = reg;
//    data[1] = byte;
//    I2C_write(address, data, 2);
    i2c1_write1ByteRegister(address, reg, byte);
}


void I2C_write_2register(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t byte) {
//    uint8_t data[3];
//    data[0] = regHigh;
//    data[1] = regLow;
//    data[2] = byte;
//    I2C_write(address, data, 3);
    i2c1_write1ByteRegister2(address, regHigh, regLow, byte);
}

void I2C_write_data(uint8_t address, uint8_t *data, uint8_t len) {
//    I2C_Writer(address, data, len);
    i2c1_writeNBytes(address, data, len);
}
