/* 
 * File:   mcp23017.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "mcp23017.h"

#ifdef MCP_ENABLED

uint8_t MCP_read(uint8_t address, uint8_t reg) {
    return I2C_readRegister(address, reg);
}

void MCP_write(uint8_t address, uint8_t reg, uint8_t data) {
    I2C_writeRegister(address, reg, data);
}

void MCP_init_keypad(uint8_t address, uint8_t port) {
    MCP_write(address, MCP_IODIRA + port, 0b00001111); // GPIOx 0-3 input, 4-7 output
    MCP_write(address, MCP_IPOLA + port, 0b11111111); // Reverse polarity on GPIOx
    MCP_write(address, MCP_GPPUA + port, 0b11111111); // Enable pull-up on GPIOx
    MCP_write(address, MCP_GPINTENA + port, 0b00001111); // Enable interrupt on GPIOx
    MCP_write(address, MCP_INTCONA + port, 0b00000000); // Interrupt-on-change (default)
    MCP_write(address, MCP_OLATA + port, 0b00000000); // Ground all outputs
    MCP_read(address, MCP_INTCAPA + port); // Read to clear interrupt
}

uint8_t MCP_read_keypad(uint8_t address, uint8_t port) {
    uint8_t cols, combination;
    uint8_t rows = (MCP_read(address, MCP_INTCAPA + port) | MCP_read(address, MCP_GPIOA + port)) & 0x0F;
    
    MCP_write(address, MCP_GPINTENA + port, 0b00000000); // Disable interrupt on GPIOx
    MCP_write(address, MCP_IODIRA + port, 0b11110000); // GPIOx 0-3 output, 4-7 inputs
    MCP_write(address, MCP_OLATA + port, 0b00000000); // Ground all outputs
                
    cols = (MCP_read(address, MCP_INTCAPA + port) | MCP_read(address, MCP_GPIOA + port)) & 0xF0;
#ifdef MCP_KEYPAD_INVERSE
    cols >>= 4;
    rows <<= 4;
#endif
    combination = cols | rows;
    
    MCP_write(address, MCP_IODIRA + port, 0b00001111); // GPIOB 0-3 input, 4-7 output
    MCP_write(address, MCP_IPOLA + port, 0b11111111); // Reverse polarity on GPIOx
    MCP_write(address, MCP_GPPUA + port, 0b11111111); // Enable pull-up on GPIOx
    MCP_write(address, MCP_GPINTENA + port, 0b11111111); // Enable interrupt on GPIOx
    MCP_write(address, MCP_INTCONA + port, 0b00000000); // Interrupt-on-change (default)
    MCP_write(address, MCP_OLATA + port, 0b00000000); // Ground all outputs
    MCP_read(address, MCP_INTCAPA + port); // Read to clear interrupt
    
    return combination;
}

char MCP_read_keypad_char(uint8_t address, uint8_t port) {
    /* |----------| *
     * |X|   |0123| *
     * |-----|----| *
     * |Y|  0|123A| *
     * |Y|  1|456B| *
     * |Y|  2|789C| *
     * |Y|  3|*0#D| *
     * |----------| *
     * |0bXXXXYYYY| *
     * |----------| */
    switch (MCP_read_keypad(address, port)) {
        case 0b00010001:
            return '1';
        case 0b00100001:
            return '2';
        case 0b01000001:
            return '3';
        case 0b10000001:
            return 'A';
        case 0b00010010:
            return '4';
        case 0b00100010:
            return '5';
        case 0b01000010:
            return '6';
        case 0b10000010:
            return 'B';
        case 0b00010100:
            return '7';
        case 0b00100100:
            return '8';
        case 0b01000100:
            return '9';
        case 0b10000100:
            return 'C';
        case 0b00011000:
            return '*';
        case 0b00101000:
            return '0';
        case 0b01001000:
            return '#';
        case 0b10001000:
            return 'D';
        default:
            return 0x00;
    }
}

#endif