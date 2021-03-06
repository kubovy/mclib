/* 
 * File:   mcp23017.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * MCP23017 I/O expander interface implementation.
 */
#ifndef MCP23017_H
#define	MCP23017_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../lib/requirements.h"

#ifdef MCP23017_ENABLED

#include <stdint.h>

#ifndef MCP23017_START_ADDRESS
#warning "MCP23017: Start address set to 0x20"
#define MCP23017_START_ADDRESS 0x20
#endif
#ifndef MCP23017_END_ADDRESS
#warning "MCP23017: End address set to 0x27"
#define MCP23017_END_ADDRESS 0x27
#endif

typedef enum {
    MCP23017_PORTA = 0x00,
    MCP23017_PORTB = 0x01
} MCP23017_Port_t;
    
typedef enum {
    // PIN registers for direction 
    // IO<7:0> <R/W-1> (default: 0b11111111)
    //   1 = Pin is configured as an input
    //   0 = Pin is configured as an output
    MCP23017_IODIRA = 0x00,
    MCP23017_IODIRB = 0x01,


    // Input polarity registers
    // IP<7:0> <R/W-0> (default: 0b00000000)
    //   1 = GPIO register bit reflects the opposite logic state of the input pin
    //   0 = GPIO register bit reflects the same logic state of the input pin
    MCP23017_IPOLA = 0x02,
    MCP23017_IPOLB = 0x03,

    // Interrupt-on-change control registers
    // GPINT<7:0> <R/W-0> (default: 0b00000000)
    //   1 = Enables GPIO input pin for interrupt-on-change event
    //   0 = Disables GPIO input pin for interrupt-on-change event
    MCP23017_GPINTENA = 0x04,
    MCP23017_GPINTENB = 0x05,

    // Default compare registers for interrupt-on-change
    // DEF<7:0> <R/W-0> (default: 0b00000000)
    MCP23017_DEFVALA = 0x06,
    MCP23017_DEFVALB = 0x07,

    // Interrupt control register
    // IOC<7:0> <R/W-0> (default: 0b00000000)
    //   1 = Pin value is compared against the associated bit in the DEFVAL register.
    //   0 = Pin value is compared against the previous pin value.
    MCP23017_INTCONA = 0x08,
    MCP23017_INTCONB = 0x09,

    // I/O expander configuration register
    // bit7<R/W-0> BANK: Controls how the registers are addressed
    //     1 = The registers associated with each port are separated into different
    //         banks
    //     0 = The registers are in the same bank (addresses are sequential)
    // bit6<R/W-0> MIRROR: INT Pins Mirror bit
    //     1 = The INT pins are internally connected
    //     0 = The INT pins are not connected. INTA is associated with  PORTA and
    //         INTB is associated with PORTB
    // bit5<R/W-0> SEQOP: Sequential Operation mode bit
    //     1 = Sequential operation disabled, address pointer does not  increment
    //     0 = Sequential operation enabled, address pointer increments
    // bit4<R/W-0> DISSLW: Slew Rate control bit for SDA output
    //     1 = Slew rate disabled
    //     0 = Slew rate enabled
    // bit3<R/W-0> HAEN: Hardware Address Enable bit (MCP23S17 only) (Note 1)
    //     1 = Enables the MCP23S17 address pins.
    //     0 = Disables the MCP23S17 address pins.
    // bit2<R/W-0> ODR: Configures the INT pin as an open-drain output
    //     1 = Open-drain output (overrides the INTPOL bit.)
    //     0 = Active driver output (INTPOL bit sets the polarity.)
    // bit1<R/W-0> INTPOL: This bit sets the polarity of the INT output pin
    //     1 = Active-high
    //     0 = Active-low
    // bit0<U-0> Unimplemented: Read as '0'
    MCP23017_IOCON  = 0x0A,
    MCP23017_IOCON2 = 0x0B,

    // Pull-up resistor configuration registers
    // PU<7:0> <R/W-0> (default: 0b00000000)
    //   1 = Pull-up enabled
    //   0 = Pull-up disabled
    MCP23017_GPPUA = 0x0C,
    MCP23017_GPPUB = 0x0D,

    // Interrupt flag registers
    // INT<7:0> <R-0> (default: 0b00000000)
    //   1 = Pin caused interrupt.
    //   0 = Interrupt not pending
    MCP23017_INTFA = 0x0E,
    MCP23017_INTFB = 0x0F,

    // Interrupt captured registers
    // ICP<7:0> <R-x>
    //   1 = Logic-high
    //   0 = Logic-low
    MCP23017_INTCAPA = 0x10,
    MCP23017_INTCAPB = 0x11,
    
    // Port registers
    // GP<7:0> <R/W-0> (default: 0b00000000)
    //   1 = Logic-high
    //   0 = Logic-low
    MCP23017_GPIOA = 0x12,
    MCP23017_GPIOB = 0x13,

    // Output latch registers
    // OL<7:0> <R/W-0> (default: 0b00000000)
    //   1 = Logic-high
    //   0 = Logic-low
    MCP23017_OLATA = 0x14,
    MCP23017_OLATB = 0x15
} MCP23017_Register_t;

/**
 *  Reads 8bits from defined address.
 * 
 *  This is a shorthand to start communication with <code>address</code> in
 *  <code>I2C_MODE_WRITE</code> mode and write <code>reg</code> to tell the
 *  other device what data it should send back then switching to
 *  <code>I2C_MODE_READ</code> and read the response.
 * 
 * @param address 7bit address, the 8th bit is ignored.
 * @param reg Information for the other device what data should be send back.
 * @return 8bits response.
 */
uint8_t MCP23017_read(uint8_t address, MCP23017_Register_t reg);

/**
 * Writes 8bits to defined address.
 * 
 *  This is a shorthand to start communication with <code>address</code> in
 *  <code>I2C_MODE_WRITE</code> mode and send <code>data</code>.
 * 
 * @param address 7bit address, the 8th bit is ignored.
 * @param reg Registry to write to.
 * @param data Data to be written.
 */
void MCP23017_write(uint8_t address, MCP23017_Register_t reg, uint8_t data);

/**
 * Initialize for keypad.
 * 
 * @param address I2C address of the chip.
 * @param port Port: 0 for A, 1 for B
 */
void MCP23017_init_keypad(uint8_t address, uint8_t port);

/**
 * Reads keypad.
 * 
 * @param address I2C address of the chip.
 * @param port Port: MCP23017_PORTA or MCP23017_PORTB
 * @return one byte containing selected column in upper 4 bits 
 *         and row in lower 4 bits
 */
uint8_t MCP23017_read_keypad(uint8_t address, uint8_t port);

/**
 * Reads keypad.
 * 
 * @param address I2C address of the chip.
 * @param port Port: MCP23017_PORTA or MCP23017_PORTB
 * @return ASCII code of read key.
 */
char MCP23017_read_keypad_char(uint8_t address, uint8_t port);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* MCP23017_H */
