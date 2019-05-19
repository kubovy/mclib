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
#include "../lib/requirements.h"

#ifdef LCD_ADDRESS
#include "i2c.h"

// Configuration with default values (change this according to your setup)
//#define LCD_ADDRESS 0x27 // Needs to be defined in order to enable LCD
#ifndef LCD_COLS
#warning "LCD: Numer of columns defaults to 20"
#define LCD_COLS 20
#endif
#ifndef LCD_ROWS
#warning "LCD: Numer of rows defaults to 20"
#define LCD_ROWS 4
#endif

// Commands
#define LCD_CLEARDISPLAY   0b00000001 // 0   0   0   0   0   0   0   1
                                      // Clears entire display and sets DDRAM 
                                      // address 0 in address counter.

#define LCD_RETURNHOME     0b00000010 // 0   0   0   0   0   0   1   -
                                      // Sets DDRAM address 0 in address counter.
                                      // Also returns display from being shifted
                                      // to original position. DDRAM contents
                                      // remain unchanged.

#define LCD_ENTRYMODESET   0b00000100 // 0   0   0   0   0   1 I/D   S
                                      // Sets cursor move direction and 
                                      // specifies display shift. These 
                                      // operations are performed during data 
                                      // write and read.
#define LCD_ENTRY_LEFT     0b00000010 // I/D=1: Increment
#define LCD_ENTRY_RIGHT    0b00000000 // I/D=0: Decrement
#define LCD_ENTRY_SHIFT    0b00000001 // S  =1: Accompanies display shift
#define LCD_ENTRY_NOSHIFT  0b00000000 // S  =0: Withoud display shift

#define LCD_DISPLAYCONTROL 0b00001000 // 0   0   0   0   1   D   C   B
                                      // Sets entire display (D) on/off, cursor 
                                      // on/off (C), and blinking of cursor
                                      // position character (B).
#define LCD_DISPLAYON      0b00000100 // D=1: Entire display on
#define LCD_DISPLAYOFF     0b00000000 // D=0: Entire display off
#define LCD_CURSORON       0b00000010 // C=1: Cursor on
#define LCD_CURSOROFF      0b00000000 // C=0: Cursor off
#define LCD_BLINKON        0b00000001 // B=1: Blinking of cursor position character on
#define LCD_BLINKOFF       0b00000000 // B=0: Blinking of cursor position character off

#define LCD_CURSORSHIFT    0b00010000 // 0   0   0   1 S/C R/L   -   -
                                      // Moves cursor and shifts display without
                                      // changing DDRAM contents.
#define LCD_DISPLAYSHIFT   0b00001000 // S/C=1: Display shift
#define LCD_CURSORMOVE     0b00000000 // S/C=0: Cursor move
#define LCD_MOVERIGHT      0b00000100 // R/L=1: Shift to the right
#define LCD_MOVELEFT       0b00000000 // R/L=0: Shift to the left

#define LCD_FUNCTIONSET    0b00100000 // 0   0   1  DL   N   F   -   -
                                      // Sets interface data length (DL), number
                                      // of display lines (N), and character
                                      // font (F).
#define LCD_8BITMODE       0b00010000 // DL=1: 8bits
#define LCD_4BITMODE       0b00000000 // DL=0: 4bits
#define LCD_2LINE          0b00001000 // N =1: 2lines
#define LCD_1LINE          0b00000000 // N =0: 1line
#define LCD_5x10DOTS       0b00000100 // F =1: 5×10dots
#define LCD_5x8DOTS        0b00000000 // F =0: 5×8dots

#define LCD_SETCGRAMADDR   0b01000000 // 0   1 ACG ACG ACG ACG ACG ACG
                                      // Sets CGRAM address. CGRAM data is sent 
                                      // and received after this setting.

#define LCD_SETDDRAMADDR   0b10000000 // 1 ADD ADD ADD ADD ADD ADD ADD
                                      // Sets DDRAM address. DDRAM data is sent 
                                      // and received after this setting.

// Flags for backlight
#define LCD_BACKLIGHT      0b00001000 // Backlight on
#define LCD_NOBACKLIGHT    0b00000000 // Backlight off

// Line selection commands
#define LCD_LINE1          0b10000000 // Line 1, start at 0x00 (0b00000000)
#define LCD_LINE2          0b11000000 // Line 2, start at 0x40 (0b01000000)
#define LCD_LINE3          0b10010100 // Line 1, start at 0x14 (0b00010100)
#define LCD_LINE4          0b11010100 // line 2, start at 0x54 (0b01010100)
    
#define En                 0b00000100 // Enable bit
#define Rw                 0b00000010 // Read/Write bit
#define Rs                 0b00000001 // Register select bit

/** LCD backlight. */
uint8_t LCD_backlight = LCD_BACKLIGHT;

const uint8_t LCD_CHAR_BACKSLASH[8] = {
  0b00000000,
  0b00010000,
  0b00001000,
  0b00000100,
  0b00000010,
  0b00000001,
  0b00000000,
  0b00000000
};

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
inline void LCD_clear(void);

/**
 * Get a character displayed on certain coordinates.
 * 
 * @param row Row.
 * @param column Column.
 */
inline char LCD_getCache(uint8_t row, uint8_t column);

/**
 * Set a character to the cache.
 * 
 * @param row Row.
 * @param column Column.
 * @param ch Character.
 */
inline void LCD_setCache(uint8_t row, uint8_t column, char ch);

/** Display the content of the cache to the LCD. */
void LCD_displayCache(void);

/** Resets the LCD preserving its content. */
inline void LCD_reset(void);

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

inline void LCD_displayLine(uint8_t line);

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
 * @param display Whether display right away or not.
 */
void LCD_setString(char* str, uint8_t line, bool display);

/**
 * Replace character on position of a line.
 * 
 * @param c Character to use.
 * @param position Position on a line.
 * @param line Line (0-based).
 * @param display Whether display right away or not.
 */
void LCD_replaceChar(char c, uint8_t position, uint8_t line, bool display);

/**
 * Replace string on position of a line.
 * @param str String to use.
 * @param position Position to start.
 * @param line Line (0-based)
 * @param display Whether display right away or not.
 */
void LCD_replaceString(char *str, uint8_t position, uint8_t line, bool display);

/**
 * Create a custom character.
 * @param location Location in the EEPROM (ASCII code) [0-8].
 * @param charmap The character map.
 */
void LCD_createChar(uint8_t location, uint8_t charmap[]);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* LCD_H */

