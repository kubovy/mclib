/* 
 * File:   rgb.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#ifndef RGB_H
#define	RGB_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../../config.h"

#ifdef RGB_R_DUTY_CYCLE
#ifdef RGB_G_DUTY_CYCLE
#ifdef RGB_B_DUTY_CYCLE

#define RGB_ENABLED
#define RGB_INDEFINED 0xFF

#ifndef RGB_TIMER_PERIOD
#ifdef TIMER_PERIOD
#define RGB_TIMER_PERIOD TIMER_PERIOD
#endif
#endif

#ifndef RGB_DUTY_CYCLE_MAX
#define RGB_DUTY_CYCLE_MAX 255
#endif
    
typedef enum {
    RGB_PATTERN_LIGHT = 0x00,
    RGB_PATTERN_BLINK = 0x01,
    RGB_PATTERN_FADE_IN = 0x02,
    RGB_PATTERN_FADE_OUT = 0x03,
    RGB_PATTERN_FADE_TOGGLE = 0x04
} RGB_Pattern_t;

#ifdef RGB_TIMER_PERIOD
/**
 * Updates LED strip.
 * 
 * This should be called periodically by a timer with TIMER_PERIOD period.
 */
void RGB_update(void);
#endif


void RGB_off(void);

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
void RGB_set(uint8_t pattern, uint8_t red, uint8_t green, uint8_t blue, uint16_t delay, uint8_t min, uint8_t max, uint8_t count);

#endif
#endif
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* RGB_H */

