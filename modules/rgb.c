/* 
 * File:   rgb.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "rgb.h"

#ifdef RGB_ENABLED

#ifdef RGB_TIMER_PERIOD
uint16_t RGB_counter = 0;

void RGB_update(void) {
    uint8_t percent;
    
    if (RGB_list.size > 1 && RGB_list.timeout == 0) {
        if (RGB_list.index + 1 < RGB_list.size) RGB_list.index++;
        else RGB_list.index = 0;
        RGB = RGB_items[RGB_list.index];
        RGB_list.timeout = RGB.timeout;
        RGB_counter = 0; // Starts with 1 not to decrease timer 1st run
    }
    
    switch (RGB.pattern) {
        case RGB_PATTERN_OFF:
        case RGB_PATTERN_LIGHT:
            if (RGB_list.size > 1
                    && RGB_list.timeout > 0
                    && RGB_list.timeout < RGB_INDEFINED
                    && RGB_counter % RGB.delay == RGB.delay - 1) {
                // Timeout x 100 ms
                RGB_list.timeout--;
            }
            percent = RGB.pattern == RGB_PATTERN_OFF ? 0 : RGB.max;
            break;
        case RGB_PATTERN_BLINK:
            percent = RGB_counter / RGB.delay % 2;
            if (RGB_list.size > 1
                    && RGB_list.timeout > 0
                    && RGB_list.timeout < RGB_INDEFINED
                    && RGB_counter > 0
                    && percent > (RGB_counter + 1) / RGB.delay % 2) {
                // Timeout x times
                RGB_list.timeout--;
            }
            percent = percent > 0 ? RGB.min: RGB.max;
            break;
        case RGB_PATTERN_FADE_IN:
            percent = RGB_counter * RGB_SPEED / RGB.delay % 100;
            if (RGB_list.size > 1
                    && RGB_list.timeout > 0
                    && RGB_list.timeout < RGB_INDEFINED
                    && RGB_counter > 0
                    && percent > (RGB_counter + 1) * RGB_SPEED / RGB.delay % 100) {
                // Timeout x times
                RGB_list.timeout--;
            }
            percent = ((uint16_t) (RGB.max - RGB.min) * percent) / 99  + RGB.min;
            break;
        case RGB_PATTERN_FADE_OUT:
            percent = RGB_counter * RGB_SPEED / RGB.delay % 100;
            if (RGB_list.size > 1
                    && RGB_list.timeout > 0
                    && RGB_list.timeout < RGB_INDEFINED
                    && RGB_counter > 0
                    && percent > (RGB_counter + 1) * RGB_SPEED / RGB.delay % 100) {
                // Timeout x times
                RGB_list.timeout--;
            }
            percent = RGB.max - ((uint16_t) (RGB.max - RGB.min) * percent) / 99;
            break;
        case RGB_PATTERN_FADE_INOUT:
            percent = RGB_counter * RGB_SPEED / RGB.delay % 200;
            if (RGB_list.size > 1
                    && RGB_list.timeout > 0
                    && RGB_list.timeout < RGB_INDEFINED
                    && RGB_counter > 0
                    && percent > (RGB_counter + 1) * RGB_SPEED / RGB.delay % 200) {
                // Timeout x times
                RGB_list.timeout--;
            }
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
    RGB_set(RGB_PATTERN_OFF, 0, 0, 0, 0, 0, 0, RGB_INDEFINED);
}

inline RGB_t RGB_assemble(uint8_t pattern, uint8_t red, uint8_t green, uint8_t blue,
                     uint16_t delay, uint8_t min, uint8_t max, uint8_t timeout) {
    RGB_t rgb;
    rgb.red = pattern == RGB_PATTERN_OFF ? 0 : red;
    rgb.green = pattern == RGB_PATTERN_OFF ? 0 : green;
    rgb.blue = pattern == RGB_PATTERN_OFF ? 0 : blue;
    rgb.pattern = pattern;
    rgb.delay = delay / RGB_TIMER_PERIOD;
    rgb.min = pattern == RGB_PATTERN_OFF ? 0 : min;
    rgb.max = pattern == RGB_PATTERN_OFF ? 0 : max;
    rgb.timeout = pattern == RGB_PATTERN_OFF ? RGB_INDEFINED : timeout;
    return rgb;
}

#ifdef RGB_TIMER_PERIOD
void RGB_add(uint8_t pattern, uint8_t red, uint8_t green, uint8_t blue,
             uint16_t delay, uint8_t min, uint8_t max, uint8_t timeout) {
    RGB_items[RGB_list.size++] = RGB_assemble(pattern, red, green, blue, delay,
            min, max, timeout);
    if (RGB_list.size <= 1) RGB_counter = 0;
}
#endif

void RGB_set(uint8_t pattern, uint8_t red, uint8_t green, uint8_t blue,
             uint16_t delay, uint8_t min, uint8_t max, uint8_t timeout) {
#ifdef RGB_TIMER_PERIOD
    RGB = RGB_assemble(pattern, red, green, blue, delay, min, max, timeout);
    
    RGB_items[0] = RGB;
    RGB_list.index = 0;
    RGB_list.size = 1;
    RGB_list.timeout = timeout;
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
