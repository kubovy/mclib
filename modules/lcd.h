/* 
 * File:   lcd.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * LCD display over I2C interface implementation.
 * 
 * Needs LCD_ADDRESS to be defined and optionally LCD_COLS and LCD_ROWS to be
 * overwritten.
 */
#ifndef LCD_H
#define	LCD_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../../config.h"
#include "i2c.h"

#ifdef LCD_ADDRESS

// Configuration with default values (change this according to your setup)
//#define LCD_ADDRESS 0x27 // Needs to be defined in order to enable LCD
#ifndef LCD_COLS
#define LCD_COLS 20
#endif
#ifndef LCD_ROWS
#define LCD_ROWS 4
#endif

// Commands
#define LCD_CLEARDISPLAY 0x01        // 0b00000001
#define LCD_RETURNHOME 0x02          // 0b00000010
#define LCD_ENTRYMODESET 0x04        // 0b00000100
#define LCD_DISPLAYCONTROL 0x08      // 0b00001000
#define LCD_CURSORSHIFT 0x10         // 0b00010000
#define LCD_FUNCTIONSET 0x20         // 0b00100000
#define LCD_SETCGRAMADDR 0x40        // 0b01000000
#define LCD_SETDDRAMADDR 0x80        // 0b10000000

// Flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02           // 0b00000010
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// Flags for display on/off control
#define LCD_DISPLAYON 0x04           // 0b00000100
#define LCD_DISPLAYOFF 0x00          // 0b00000000
#define LCD_CURSORON 0x02            // 0b00000010
#define LCD_CURSOROFF 0x00           // 0b00000000
#define LCD_BLINKON 0x01             // 0b00000001
#define LCD_BLINKOFF 0x00            // 0b00000000

// Flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08         // 0b00001000
#define LCD_CURSORMOVE 0x00          // 0b00000000
#define LCD_MOVERIGHT 0x04           // 0b00000100
#define LCD_MOVELEFT 0x00            // 0b00000000

// Flags for function set
#define LCD_8BITMODE 0x10            // 0b00010000
#define LCD_4BITMODE 0x00            // 0b00000000
#define LCD_2LINE 0x08               // 0b00001000
#define LCD_1LINE 0x00               // 0b00000000
#define LCD_5x10DOTS 0x04            // 0b00000100
#define LCD_5x8DOTS 0x00             // 0b00000000

// Flags for backlight
#define LCD_BACKLIGHT 0x08           // 0b00001000
#define LCD_NOBACKLIGHT 0x00         // 0b00000000

// Lines
#define LCD_LINE1 0x80               // 0b10000000
#define LCD_LINE2 0xC0               // 0b11000000
#define LCD_LINE3 0x94               // 0b10010100
#define LCD_LINE4 0xD4               // 0b11010100
    
#define En 0b00000100                // Enable bit
#define Rw 0b00000010                // Read/Write bit
#define Rs 0b00000001                // Register select bit

/** LCD content cache. */
char LCD_content[LCD_ROWS][LCD_COLS + 1];

/** LCD backlight. */
uint8_t LCD_backlight = LCD_BACKLIGHT;

/**
 * LCD module intialization.
 * 
 * @param address I2C 7bit address of the LCD.
 * @param cols Number of columns.
 * @param rows Number of rows.
 * @param content Content buffer.
 */
void LCD_init(void);

/** Clear the content of the LCD. */
void LCD_clear(void);

/** Resets the LCD preserving its content. */
void LCD_reset(void);

/**
 * Turn LCD's backlight on or off.
 * 
 * @param on Whether to turn the backlight on or off.
 */
void LCD_setBacklight(bool on);

/**
 * Send a command to the LCD.
 * 
 * @param command The command.
 */
void LCD_sendCommand(uint8_t command);

/**
 * Send data to the LCD.
 * 
 * @param data The data.
 */
void LCD_sendData(uint8_t data); 

/**
 * Send string to the LCD.
 * 
 * The string length will be trimmed to the number of the LCD's columns. Also
 * multiple lines may be send at once by separating the line with a new line 
 * (\n). All lines bigger than the number of the LCD's rows will be ignore.
 * 
 * There are some modifiers which can be added on the beginning of each line:
 * <li><code>|c|</code>: will center the text</li>
 * <li><code>|l|</code>: will left-align the text (the default)</li>
 * <li><code>|r|</code>: will right-align the text</li>
 * <li><code>|d|MS|</code>: type animation with <code>MS</code> millisecond delay between each character</li>
 * 
 * @param str String to be sent.
 * @param line Line the string should be display on or begin at in case of multi-line string.
 */
void LCD_displayString(char*, uint8_t);

/**
 * Replace character on position of a line.
 * 
 * @param c Character to use.
 * @param position Position on a line.
 * @param line Line (0-based)
 */
void LCD_replaceChar(char c, uint8_t position, uint8_t line);

/**
 * Replace string on position of a line.
 * @param str String to use.
 * @param position Position to start.
 * @param line Line (0-based)
 */
void LCD_replaceString(char *str, uint8_t position, uint8_t line);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* LCD_H */

