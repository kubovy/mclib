/* 
 * File:   rgb.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include <stdbool.h>
#include <stdint.h>
#include "rgb.h"

#ifdef RGB_ENABLED

#ifdef RGB_TIMER_PERIOD
uint16_t RGB_counter = 0;

void RGB_update(void) {
    uint8_t percent;
    if (RGB.count > 0) switch (RGB.pattern) {
        case RGB_PATTERN_LIGHT:
            percent = 255;
            if (RGB.count < 0xFF) RGB.count--;
            break;
        case RGB_PATTERN_BLINK:
            percent = (RGB_counter / RGB.delay) % 2 ? RGB.max : RGB.min;
            if (RGB.count < 0xFF && percent == 0) RGB.count--;
            break;
        case RGB_PATTERN_FADE_IN:
            percent = (RGB_counter / RGB.delay) % 100;
            if (RGB.count < 0xFF && percent == 99) RGB.count--;
            percent = ((uint16_t) (RGB.max - RGB.min) * percent) / 99  + RGB.min;
            break;
        case RGB_PATTERN_FADE_OUT:
            percent = (RGB_counter / RGB.delay) % 100;
            if (RGB.count < 0xFF && percent == 99) RGB.count--;
            percent = RGB.max - ((uint16_t) (RGB.max - RGB.min) * percent) / 99;
            break;
        case RGB_PATTERN_FADE_TOGGLE:
            percent = (RGB_counter / RGB.delay) % 200;
            if (RGB.count < 0xFF && percent == 199) RGB.count--;
            percent = percent > 100 ? 200 - percent : percent;
            percent = (RGB.max - RGB.min) * percent / 100 + RGB.min;
            break;
    }
#ifdef RGB_R_DUTY_CYCLE
    RGB_R_DUTY_CYCLE = RGB.red * percent / RGB_DUTY_CYCLE_MAX;
#endif
#ifdef RGB_G_DUTY_CYCLE
    RGB_G_DUTY_CYCLE = RGB.green * percent / RGB_DUTY_CYCLE_MAX;
#endif
#ifdef RGB_B_DUTY_CYCLE
    RGB_B_DUTY_CYCLE = RGB.blue * percent / RGB_DUTY_CYCLE_MAX;
#endif
    RGB_counter++;
}
#endif

inline void RGB_off(void) {
    RGB_set(RGB_PATTERN_LIGHT, 0, 0, 0, 0, 0, 0, RGB_INDEFINED);
}

void RGB_set(uint8_t pattern, uint8_t red, uint8_t green, uint8_t blue, uint16_t delay, uint8_t min, uint8_t max, uint8_t count) {
#ifdef RGB_TIMER_PERIOD
    RGB.red = red;
    RGB.green = green;
    RGB.blue = blue;
    RGB.pattern = pattern;
    RGB.delay = delay / RGB_TIMER_PERIOD;
    RGB.min = min;
    RGB.max = max;
    RGB.count = count;
    RGB_counter = 0;
#else
#ifdef RGB_R_DUTY_CYCLE
    RGB_R_DUTY_CYCLE = red * 255 / RGB_DUTY_CYCLE_MAX;
#endif
#ifdef RGB_G_DUTY_CYCLE
    RGB_G_DUTY_CYCLE = green * 255 / RGB_DUTY_CYCLE_MAX;
#endif
#ifdef RGB_B_DUTY_CYCLE
    RGB_B_DUTY_CYCLE = blue * 255 / RGB_DUTY_CYCLE_MAX;
#endif
#endif
}

#endif
