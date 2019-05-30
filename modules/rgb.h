/* 
 * File:   rgb.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * This module implement controlling and LED strip using PWM signal. It enables
 * 3 color component RGB, but can be used also for single color LED strips by
 * enable only one of those component or 2-3 single color LED strips by treating
 * each color component as a separate strip.
 * 
 * Animations can be enabled by defining the RGB_TIMER_PERIOD constant.
 * 
 * By default the value of a duty cycle should range between 0 and 255. If the
 * upper bound is different than the RGB_DUTY_CYCLE_MAX should be overwritten.
 * Maximum upper bound is limited by 1 byte value (255).
 */
#ifndef RGB_H
#define	RGB_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../lib/requirements.h"

#ifdef RGB_ENABLED

#ifndef RGB_LIST_SIZE
#define RGB_LIST_SIZE 20
#endif

#define RGB_INDEFINED 0xFF

#ifndef RGB_TIMER_PERIOD
#ifdef TIMER_PERIOD
#warning "RGB: RGB_TIMER_PERIOD defaults to TIMER_PERIOD"
#define RGB_TIMER_PERIOD TIMER_PERIOD
#endif
#endif
    
#ifndef RGB_SPEED
#define RGB_SPEED 2 * RGB_TIMER_PERIOD
#endif

#ifndef RGB_DUTY_CYCLE_MAX
#define RGB_DUTY_CYCLE_MAX 255
#endif

#define RGB_INDEFINED 0xFF
    
typedef enum {
    RGB_PATTERN_OFF= 0x00,
    RGB_PATTERN_LIGHT = 0x01,
    RGB_PATTERN_BLINK = 0x02,
    RGB_PATTERN_FADE_IN = 0x03,
    RGB_PATTERN_FADE_OUT = 0x04,
    RGB_PATTERN_FADE_INOUT = 0x05
} RGB_Pattern_t;

#ifdef RGB_TIMER_PERIOD
typedef struct {
    RGB_Pattern_t pattern;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t delay;
    uint8_t min;
    uint8_t max;
    uint8_t timeout;
} RGB_t;

RGB_t RGB = {RGB_PATTERN_OFF, 0, 0, 0, 0, 0, 0, RGB_INDEFINED};

struct {
    uint8_t index;
    uint8_t size;
    uint8_t timeout;
} RGB_list = {0, 0, RGB_INDEFINED};

RGB_t RGB_items[RGB_LIST_SIZE];


/**
 * Updates LED strip.
 * 
 * This should be called periodically by a timer with TIMER_PERIOD period.
 */
void RGB_update(void);
#endif

/**
 * Turns LED strip off.
 */
inline void RGB_off(void);

/**
 * Add RGB LED strip configuration.
 * 
 * @param pattern Pattern.
 * @param red Red component.
 * @param green Green component.
 * @param blue Blue component.
 * @param delay Delay.
 * @param min Minimum.
 * @param max Maximum.
 * @param count Count.
 */
void RGB_add(uint8_t pattern, uint8_t red, uint8_t green, uint8_t blue, uint16_t delay, uint8_t min, uint8_t max, uint8_t timeout);

/**
 * Set RGB LED strip configuration.
 * 
 * @param pattern Pattern.
 * @param red Red component.
 * @param green Green component.
 * @param blue Blue component.
 * @param delay Delay.
 * @param min Minimum.
 * @param max Maximum.
 * @param count Count.
 */
void RGB_set(uint8_t pattern, uint8_t red, uint8_t green, uint8_t blue, uint16_t delay, uint8_t min, uint8_t max, uint8_t timeout);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* RGB_H */

