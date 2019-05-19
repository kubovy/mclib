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
#include "../lib/requirements.h"

#ifdef WS281x_BUFFER

// Configuration
#ifndef WS281x_LED_COUNT
#error "WS281x: WS281x_LED_COUNT needs to be defined!"
#endif

#ifndef WS281x_TIMER_PERIOD
#ifdef TIMER_PERIOD
#warning "WS281x: WS281x_TIMER_PERIOD defaults to TIMER_PERIOD"
#define WS281x_TIMER_PERIOD TIMER_PERIOD
#endif
#endif

typedef enum {
#ifndef WS281x_TIMER_PERIOD
    WS281x_PATTERN_LIGHT = 0x00       // Simple light
#else
    WS281x_PATTERN_LIGHT = 0x00,      // Simple light
    WS281x_PATTERN_BLINK = 0x01,      // Blink 50/50
    WS281x_PATTERN_FADE_IN = 0x02,    // Fade in 0>1
    WS281x_PATTERN_FADE_OUT = 0x03,   // Fade out 1>0
    WS281x_PATTERN_FADE_TOGGLE = 0x04 // Fade toggle 0>1>0
#endif
} WS281x_Pattern_t;

uint8_t WS281x_ledRed[WS281x_LED_COUNT];
uint8_t WS281x_ledGreen[WS281x_LED_COUNT];
uint8_t WS281x_ledBlue[WS281x_LED_COUNT];
uint8_t WS281x_ledPattern[WS281x_LED_COUNT];
uint8_t WS281x_ledDelay[WS281x_LED_COUNT];
uint8_t WS281x_ledMin[WS281x_LED_COUNT];
uint8_t WS281x_ledMax[WS281x_LED_COUNT];


/**
 * Sets switcher to turn the LED strip on or off.
 * 
 * @param Switcher Switcher function.
 */
void WS281x_setSwitcher(void (* Switcher)(bool));

/**
 * Show/populate configuration buffer on WS281x LEDs
 */
inline void WS281x_show(void);

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
void WS281x_set(uint8_t led, WS281x_Pattern_t pattern, uint8_t r, uint8_t g,
                uint8_t b, uint16_t delay, uint8_t min, uint8_t max);

/**
 * Set all LEDs to a specific color.
 * 
 * @param r Red color component
 * @param g Green  color component
 * @param b Blue color component
 */
inline void WS281x_all(uint8_t r, uint8_t g, uint8_t b);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* WS2812_H */