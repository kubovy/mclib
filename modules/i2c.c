/* 
 * File:   i2c.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "i2c.h"

//#if defined I2C || defined I2C_MSSP || defined I2C_MSSP_FOUNDATION

#if defined I2C
#include "../../mcc_generated_files/i2c1.h"
#elif defined I2C_MSSP
#include "../../mcc_generated_files/i2c1.h"
uint8_t writeBuffer[3];
#elif defined I2C_MSSP_FOUNDATION
#include "../../mcc_generated_files/i2c1.h"
#endif

inline void I2C_init(void) {
#if defined I2C_MSSP
    //I2C1_Initialize();  
#elif defined I2C_MSSP_FOUNDATION
    i2c1_driver_open();
#else
#endif
}

inline uint8_t I2C_readRegister(uint8_t address, uint8_t reg) {
#if defined I2C
    return i2c1_read1ByteRegister(address, reg);
#elif defined I2C_MSSP
    uint8_t byte;
    
    I2C1_Initialize();
    while(I2C1_MasterQueueIsFull());
    
    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;
    I2C1_TRANSACTION_REQUEST_BLOCK trb[2];
    writeBuffer[0] = reg;
    I2C1_MasterWriteTRBBuild(&trb[0], writeBuffer, 1, address);
    I2C1_MasterReadTRBBuild(&trb[1], &byte, 1, address);
    uint8_t timeout = 0;
    while(status != I2C1_MESSAGE_FAIL) {
        I2C1_MasterTRBInsert(2, trb, &status);
        while(status == I2C1_MESSAGE_PENDING);
        if (status == I2C1_MESSAGE_COMPLETE || timeout == I2C_MAX_RETRIES) {
            break;
        } else {
            timeout++;
        }
    }
#elif defined I2C_MSSP_FOUNDATION
    return i2c_read1ByteRegister(address, reg);
#endif
}

inline uint8_t I2C_readRegister2(uint8_t address, uint8_t regHigh, uint8_t regLow) {
#if defined I2C
    return i2c1_read1ByteRegister2(address, regHigh, regLow);
#elif defined I2C_MSSP
    uint8_t byte;
    I2C_read_data_2register(address, regHigh, regLow, &byte, 1);
    return byte;
#elif defined I2C_MSSP_FOUNDATION
    return i2c_read1ByteRegister2(address, regHigh, regLow);
#endif
}

inline void I2C_writeByte(uint8_t address, uint8_t byte) {
#if defined I2C
    i2c1_writeNBytes(address, &byte, 1);
#elif defined I2C_MSSP
    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;
    uint8_t timeout = 0;
    while(status != I2C1_MESSAGE_FAIL) {
        I2C1_MasterWrite(&byte, 1, address, &status);
        while(status == I2C1_MESSAGE_PENDING);
        if (status == I2C1_MESSAGE_COMPLETE || timeout == I2C_MAX_RETRIES) {
            break;
        }
        timeout++;
    }
#elif defined I2C_MSSP_FOUNDATION
    i2c_writeNBytes(address, &byte, 1);
#endif
}

inline void I2C_writeRegister(uint8_t address, uint8_t reg, uint8_t byte) {
#if defined I2C
    i2c1_write1ByteRegister(address, reg, byte);
#elif defined I2C_MSSP
    I2C1_Initialize();
    while(I2C1_MasterQueueIsFull());

    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;
    writeBuffer[0] = reg;
    writeBuffer[1] = byte;
    uint8_t timeout = 0;
    while(status != I2C1_MESSAGE_FAIL) {
        I2C1_MasterWrite(writeBuffer, 2, address, &status);
        while(status == I2C1_MESSAGE_PENDING);
        if (status == I2C1_MESSAGE_COMPLETE || timeout == I2C_MAX_RETRIES) {
            break;
        }
        timeout++;
    }
#elif defined I2C_MSSP_FOUNDATION
    i2c_write1ByteRegister(address, reg, byte);
#endif
}

inline void I2C_writeRegister2(uint8_t address, uint8_t regHigh, uint8_t regLow, uint8_t byte) {
#if defined I2C
    i2c1_write1ByteRegister2(address, regHigh, regLow, byte);
#elif defined I2C_MSSP
    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;

    // build the write buffer first
    // starting address of the EEPROM memory
    writeBuffer[0] = regHigh; // high registry
    writeBuffer[1] = regLow;  // low registry

    // data to be written
    writeBuffer[2] = byte;

    // Now it is possible that the slave device will be slow.
    // As a work around on these slaves, the application can
    // retry sending the transaction
    uint8_t timeout = 0;
    while(status != I2C1_MESSAGE_FAIL) {
        // write one byte to EEPROM (3 is the number of bytes to write)
        I2C1_MasterWrite(writeBuffer, 3, address, &status);

        // wait for the message to be sent or status has changed.
        while(status == I2C1_MESSAGE_PENDING);

        // if status is  I2C1_MESSAGE_ADDRESS_NO_ACK, or I2C1_DATA_NO_ACK,
        // The device may be busy and needs more time for the last write so
        // we can retry writing the data, this is why we use a while loop here

        // check for max retry and skip this byte
        if (status == I2C1_MESSAGE_COMPLETE || timeout == I2C_MAX_RETRIES) {
            break;
        }
        timeout++;
    }
#elif defined I2C_MSSP_FOUNDATION
    i2c_write1ByteRegister2(address, regHigh, regLow, byte);
#endif
}

inline void I2C_writeData(uint8_t address, uint8_t *data, uint8_t len) {
#if defined I2C
    i2c1_writeNBytes(address, data, len);
#elif defined I2C_MSSP
    I2C1_Initialize();
    while(I2C1_MasterQueueIsFull());
    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;
    uint8_t timeout = 0;
    while(status != I2C1_MESSAGE_FAIL) {
        // write one byte to EEPROM (3 is the number of bytes to write)
        I2C1_MasterWrite(data, len, address, &status);

        // wait for the message to be sent or status has changed.
        while(status == I2C1_MESSAGE_PENDING);

        // if status is  I2C1_MESSAGE_ADDRESS_NO_ACK, or I2C1_DATA_NO_ACK,
        // The device may be busy and needs more time for the last write so
        // we can retry writing the data, this is why we use a while loop here

        // check for max retry and skip this byte
        if (status == I2C1_MESSAGE_COMPLETE || timeout == I2C_MAX_RETRIES) {
            break;
        }
        timeout++;
    }
#elif defined I2C_MSSP_FOUNDATION
    i2c_writeNBytes(address, data, len);
#endif
}

//#endif