/* 
 * File:   ws281x.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * WS281x implementation. Needs WS281x_BUFFER to be defined to SPI transmission
 * buffer (e.g. SPI1TXB) and WS281x_LED_COUNT to be set to number of LEDs on the
 * strip.
 */
#ifndef WS2812_H
#define	WS2812_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../../config.h"

// Configuration
#ifdef WS281x_BUFFER
//#define WS281x_BUFFER SPI1TXB // Output PIN 
#ifndef WS281x_LED_COUNT
#define WS281x_LED_COUNT 32 // Number of LEDs
#endif

#ifndef WS281x_TIMER_PERIOD
#ifdef TIMER_PERIOD
#define WS281x_TIMER_PERIOD TIMER_PERIOD
#endif
#endif

#define WS281x_PATTERN_LIGHT 0x00       // Simple light
#ifdef WS281x_TIMER_PERIOD
#define WS281x_PATTERN_BLINK 0x01       // Blink 50/50
#define WS281x_PATTERN_FADE_IN 0x02     // Fade in 0>1
#define WS281x_PATTERN_FADE_OUT 0x03    // Fade out 1>0
#define WS281x_PATTERN_FADE_TOGGLE 0x04 // Fade toggle 0>1>0
#endif

/**
 * Sets switcher to turn the LED strip on or off.
 * 
 * @param Switcher Switcher function.
 */
void WS281x_setSwitcher(void (* Switcher)(bool));

/**
 * Show/populate configuration buffer on WS281x LEDs
 */
extern inline void WS281x_show(void);

#ifdef WS281x_TIMER_PERIOD
/**
 * Update configuration buffer.
 * 
 * This function should be called periodically by a timer. The timers period
 * should match WS281x_TIMER_PERIOD constant.
 */
void WS281x_update(void);
#endif

/**
 * Turns WS281x LED strip off.
 */
void WS281x_off(void);

/**
 * Gets a pattern of a LED.
 * @param led LED ID.
 * @return Pattern.
 */
uint8_t WS281x_getPattern(uint8_t led);

/**
 * Gets a red component of a LED.
 * @param led LED ID.
 * @return Red component.
 */
uint8_t WS281x_getRed(uint8_t led);

/**
 * Gets a green component of a LED.
 * @param led LED ID.
 * @return Green component.
 */
uint8_t WS281x_getGreen(uint8_t led);

/**
 * Gets a blue component of a LED.
 * @param led LED ID.
 * @return Blue component.
 */
uint8_t WS281x_getBlue(uint8_t led);

/**
 * Sets the given LED to specified color and pattern.
 * 
 * @param led LED ID
 * @param pattern Pattern: WS281x_PATTERN_LIGHT, WS281x_PATTERN_BLINK,
 *                         WS281x_PATTERN_FADE_IN, WS281x_PATTERN_FADE_OUT, or
 *                         WS281x_PATTERN_FADE_TOGGLE
 * @param r Red color component
 * @param g Green color component
 * @param b Blue color component
 * @param delay Delay (pattern specific)
 * @param min Minimum value  (pattern specific)
 * @param max Maximum value  (pattern specific)
 */
void WS281x_set(uint8_t led, uint8_t pattern, uint8_t r, uint8_t g, uint8_t b, 
                uint16_t delay, uint8_t min, uint8_t max);

/**
 * Sets the given LED to a specific color.
 * 
 * @param led LED ID
 * @param r Red color component
 * @param g Green  color component
 * @param b Blue color component
 */
void WS281x_RGB(uint8_t led, uint8_t r, uint8_t g, uint8_t b);

#ifdef WS281x_TIMER_PERIOD
/**
 * Sets the given LED to a blinking color.
 * 
 * @param led LED ID
 * @param r Red color component
 * @param g Green  color component
 * @param b Blue color component
 * @param delay Delay between on and off
 */
void WS281x_blink(uint8_t led, uint8_t r, uint8_t g, uint8_t b, uint16_t delay);

/**
 * Sets the given LED to a fade-in color
 * 
 * @param led LED ID
 * @param r Red color component
 * @param g Green  color component
 * @param b Blue color component
 * @param delay Delay between 1% step
 * @param min Minimum %
 * @param max Maximum %
 */
void WS281x_fadeIn(uint8_t led, uint8_t r, uint8_t g, uint8_t b, uint16_t delay, 
                   uint8_t min, uint8_t max);

/**
 * Sets the given LED to a fade-out color.
 * 
 * @param led LED ID
 * @param r Red color component
 * @param g Green  color component
 * @param b Blue color component
 * @param delay Delay between 1% step
 * @param min Minimum %
 * @param max Maximum %
 */
void WS281x_fadeOut(uint8_t led, uint8_t r, uint8_t g, uint8_t b,
                    uint16_t delay,uint8_t min, uint8_t max);

/**
 * Sets the given LED to a fade-in/fade-out toggle color
 * 
 * @param led LED ID
 * @param r Red color component
 * @param g Green  color component
 * @param b Blue color component
 * @param delay Delay between 1% step 
 * @param min Minimum %
 * @param max Maximum %
 */
void WS281x_fadeToggle(uint8_t led, uint8_t r, uint8_t g, uint8_t b,
                       uint16_t delay, uint8_t min, uint8_t max);

/**
 * Set all LEDs to a specific color.
 * 
 * @param r Red color component
 * @param g Green  color component
 * @param b Blue color component
 */
void WS281x_all(uint8_t r, uint8_t g, uint8_t b);
#endif

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* WS2812_H */