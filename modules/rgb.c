/* 
 * File:   rgb.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include <stdbool.h>
#include <stdint.h>
#include "rgb.h"

#ifdef RGB_ENABLED

uint8_t RGB_redValue;
uint8_t RGB_greenValue;
uint8_t RGB_blueValue;
uint8_t RGB_redTemp;
uint8_t RGB_greenTemp;
uint8_t RGB_blueTemp;
uint8_t RGB_pattern;
uint8_t RGB_delay;
uint8_t RGB_min;
uint8_t RGB_max;
uint8_t RGB_count;

uint16_t RGB_counter = 0;

void RGB_show(void) {
    RGB_R_DUTY_CYCLE = RGB_redTemp;
    RGB_G_DUTY_CYCLE = RGB_greenTemp;
    RGB_B_DUTY_CYCLE = RGB_blueTemp;
}

#ifdef RGB_TIMER_PERIOD
void RGB_update(void) {
    uint8_t percent;
    if (RGB_count > 0) switch (RGB_pattern) {
        case RGB_PATTERN_LIGHT:
            percent = 255;
            if (RGB_count < 0xFF) RGB_count--;
            break;
        case RGB_PATTERN_BLINK:
            percent = (RGB_counter / RGB_delay) % 2 ? RGB_max : RGB_min;
            if (RGB_count < 0xFF && percent == 0) RGB_count--;
            break;
        case RGB_PATTERN_FADE_IN:
            percent = (RGB_counter / RGB_delay) % 100;
            if (RGB_count < 0xFF && percent == 99) RGB_count--;
            percent = ((uint16_t) (RGB_max - RGB_min) * percent) / 99  + RGB_min;
            break;
        case RGB_PATTERN_FADE_OUT:
            percent = (RGB_counter / RGB_delay) % 100;
            if (RGB_count < 0xFF && percent == 99) RGB_count--;
            percent = RGB_max - ((uint16_t) (RGB_max - RGB_min) * percent) / 99;
            break;
        case RGB_PATTERN_FADE_TOGGLE:
            percent = (RGB_counter / RGB_delay) % 200;
            if (RGB_count < 0xFF && percent == 199) RGB_count--;
            percent = percent > 100 ? 200 - percent : percent;
            percent = (RGB_max - RGB_min) * percent / 100 + RGB_min;
            break;
    }
    RGB_redTemp = RGB_redValue * percent / 255;
    RGB_greenTemp = RGB_greenValue * percent / 255;
    RGB_blueTemp = RGB_blueValue * percent / 255;
    RGB_show();
    RGB_counter++;
}
#endif

void RGB_off(void) {
    RGB_set(RGB_PATTERN_LIGHT, 0, 0, 0, 0, 0, 0, RGB_INDEFINED);
}

void RGB_set(uint8_t pattern, uint8_t red, uint8_t green, uint8_t blue, uint16_t delay, uint8_t min, uint8_t max, uint8_t count) {
    RGB_pattern = pattern;
    RGB_redValue = red;
    RGB_greenValue = green;
    RGB_blueValue = blue;
    RGB_redTemp = red;
    RGB_greenTemp = green;
    RGB_blueTemp = blue;
#ifdef RGB_TIMER_PERIOD
    RGB_delay = delay / RGB_TIMER_PERIOD;
#endif
    RGB_min = min;
    RGB_max = max;
    RGB_count = count;
#ifndef RGB_TIMER_PERIOD
    RGB_show();
#endif
    RGB_counter = 0;
}

#endif
