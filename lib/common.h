/* 
 * File:   common.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * Common helper functions.
 */
#ifndef COMMON_H
#define	COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../lib/requirements.h"
#ifdef LCD_ADDRESS
#include "../modules/lcd.h"
#endif
    
/**
 * Converts an integer to HEX ASCII character.
 * 
 * @param dec Decimal number between 0 (inclusive) and 16 (exclusive)
 * @return Hexadecimal ASCII character.
 */
inline unsigned char dec2hex(uint8_t dec);

/**
 * Converts a hexadecimal ASCII character to integer.
 * 
 * @param hex Hexadecimal ASCII character (0-9A-F).
 * @return Integer representing the HEX value.
 */
inline uint8_t hex2dec(unsigned char hex);

/**
 * Returns the minimum of two 8bit numbers.
 * 
 * @param a Number a
 * @param b Number b
 * @return The minimum of the two numbers.
 */
inline uint8_t min(uint8_t a, uint8_t b);

/**
 * Returns the minimum of two 16bit numbers.
 * 
 * @param a Number a
 * @param b Number b
 * @return The minimum of the two numbers.
 */
inline uint16_t min16(uint16_t a, uint16_t b);

/**
 * Returns the maximum of two 8bit numbers.
 * 
 * @param a Number a
 * @param b Number b
 * @return The minimum of the two numbers.
 */
inline uint8_t max(uint8_t a, uint8_t b);

/**
 * Returns the maximum of two 16bit numbers.
 * 
 * @param a Number a
 * @param b Number b
 * @return The minimum of the two numbers.
 */
inline uint16_t max16(uint16_t a, uint16_t b);

/**
 * Calculates length of the string. A string must be terminated by a '\0' (NUL).
 * character.
 * 
 * @param str String to calculate the length for.
 * @param max Maximum acceptable length.
 * @return Length of the string w/o terminating '\0' (NUL) character.
 */
uint8_t strlen(char *str, uint8_t max);

/**
 * Compare 2 strings.  A string must be terminated by a '\0' (NUL).
 * @param str1 String 1.
 * @param str2 String 2.
 * @return Returns true if both strings are equal till first terminating char.
 */
bool strcmp(char *str1, char *str2, uint8_t len);

/**
 * Copy all character from "src" to "dst" until first terminating character or
 * "len" is reached.  A string must be terminated by a '\0' (NUL).
 * @param dest Copy destination.
 * @param src Copy source.
 * @param len Maximum number of characters to copy. May be less if '\0' (NUL) 
 *            character appears earlier in the "src" string.
 */
void strcpy(char *dest, char* src, uint8_t len);

/**
 * Watchdog implementation. Should blink a LED or something.
 * 
 * Additionally to the Trigger function, "WATCH_DOG_LED_LAT", if it is defined,
 * is toggled in case the trigger fires (e.g. a LED).
 * 
 * @param counter Pointer to counter, needs to be 0 in order the trigger to fire.
 * @param period In case the trigger fires, counter will be set to this value.
 * @param Trigger Trigger to call if fired.
 */
void watchDogTrigger(uint8_t *counter, uint8_t period, void (* Trigger)(void));

/**
 * Prints status information.
 * 
 * @param line Information to print.
 */
inline void printStatus(char *line);

#ifdef LCD_ADDRESS // Needs LCD

/**
 * Prints progress on the LCD.
 * 
 * @param title Title of the progress (1 line)
 * @param value Current progress value.
 * @param max Maximum value to be reached.
 */
void printProgress(char* title, uint16_t value, uint16_t RGB_max);

/** Waiting loader with dots animation. */
void waitingLoaderDots(void);

/** Waiting loader rotating fan animation. */
void waitingLoaderFan(void);

/** Waiting loader with wave animation. */
void waitingLoaderWave(void);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* COMMON_H */

