/* 
 * File:   ws281x_light.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "ws281x_light.h"

#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT
bool WS281xLight_changed = false;
uint8_t WS281xLight_data[WS281x_LED_COUNT * 3];
uint16_t WS281xLight_counter = 0;

WS281xLight_t WS281xLight;

inline void WS281xLight_led(uint8_t led, uint8_t color, uint8_t value) {
    if (WS281xLight_data[led * 3 + color] != value) {
        WS281xLight_data[led * 3 + color] = value;
        WS281xLight_changed = true;
    }
}

inline void WS281xLight_show(void) {
    for (uint16_t i = 0; i < WS281x_LED_COUNT * 3; i++) {
        WS281x_BUFFER = WS281xLight_data[i];
        __delay_us(15);
        //while(!SPI1INTFbits.SRMTIF);
    }
    __delay_us(50);
}

inline Color_t WS281xLight_wheel(uint8_t pos) {
    Color_t color;
    if (pos < 85) {
        color.r = 255 - pos * 3;
        color.g = pos * 3;
        color.b = 0;
    } else if (pos < 170) {
        pos -= 85;
        color.r = 0;
        color.g = 255 - pos * 3;
        color.b = pos * 3;
    } else {
        pos -= 170; 
        color.r = pos * 3;
        color.g = 0;
        color.b = 255 - pos * 3;
    }
    return color;
}

inline uint8_t WS281xLight_isRainbow() {
    uint8_t type = 0;
    for (uint8_t i = 0; i < 7; i++) {
        if (WS281xLight.color[i].r != 0x01) return 0;
        if (WS281xLight.color[i].g != 0x02) return 0;
        
        if (WS281xLight.color[i].b == 0) return 0;
        else if (type == 0) type = WS281xLight.color[i].b;
        else if (WS281xLight.color[i].b != type) return 0;
    }
    return type;
}

inline Color_t WS281xLight_color(uint8_t rainbow, uint8_t led, uint8_t pos, uint8_t shift, Color_t defaultColor) {
    switch(rainbow) {
        case 1:
            return WS281xLight_wheel((pos + shift) & 0xFF);
            break;
        case 2:
            return WS281xLight_wheel(((pos * 256 / WS281x_LIGHT_ROW_COUNT) + shift) & 0xFF);
            break;
        case 3:
            return WS281xLight_wheel((led + shift) & 0xFF);
            break;
        case 4:
            return WS281xLight_wheel(((led * 256 / WS281x_LED_COUNT) + shift) & 0xFF);
            break;
        default:
            return defaultColor;
    }
}

inline void WS281xLight_all(WS281xLight_Pattern_t pattern) {
    uint8_t percent;
    switch (pattern) {
        case WS281x_LIGHT_OFF:
            if (WS281xLight_list.timeout > 0
                    && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
                    && WS281xLight_counter % (WS281xLight.timeout * 100 / WS281x_TIMER_PERIOD) == WS281xLight.timeout * 100 / WS281x_TIMER_PERIOD - 1) {
                // Timeout x 100 ms
                WS281xLight_list.timeout--;
            }
            percent = 0;
            break;
        case WS281x_LIGHT_BLINK:
            if (WS281xLight_list.timeout > 0
                    && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
                    && WS281xLight_counter % (WS281xLight.delay * 2) == WS281xLight.delay * 2 - 1) {
                // Timeout x times
                WS281xLight_list.timeout--;
            }
            percent = (WS281xLight_counter / WS281xLight.delay) % 2 == 0 ? WS281xLight.min : WS281xLight.max;
            break;
        case WS281x_LIGHT_FADE_IN:
            if (WS281xLight_list.timeout > 0
                    && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
                    && WS281xLight_counter % (WS281xLight.delay * 100) == WS281xLight.delay * 100 - 1) {
                // Timeout x times
                WS281xLight_list.timeout--;
            }
            percent = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % 100;
            percent = ((uint16_t) (WS281xLight.max - WS281xLight.min) * percent) / 99 + WS281xLight.min;
            break;
        case WS281x_LIGHT_FADE_OUT:
            if (WS281xLight_list.timeout > 0
                    && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
                    && WS281xLight_counter % (WS281xLight.delay * 100) == WS281xLight.delay * 100 - 1) {
                // Timeout x times
                WS281xLight_list.timeout--;
            }
            percent = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % 100;
            percent = WS281xLight.max - ((uint16_t) (WS281xLight.max - WS281xLight.min) * percent) / 99;
            break;
        case WS281x_LIGHT_FADE_INOUT:
            if (WS281xLight_list.timeout > 0
                    && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
                    && WS281xLight_counter % (WS281xLight.delay * 200) == WS281xLight.delay * 200 - 1) {
                // Timeout x times
                WS281xLight_list.timeout--;
            }
            percent = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % 200;
            percent = percent > 100 ? 200 - percent : percent;
            percent = (WS281xLight.max - WS281xLight.min) * percent / 100 + WS281xLight.min;
            break;
        case WS281x_LIGHT_FADE_TOGGLE:
            if (WS281xLight_list.timeout > 0
                    && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
                    && WS281xLight_counter % (WS281xLight.delay * 200) == WS281xLight.delay * 200 - 1) {
                // Timeout x times
                WS281xLight_list.timeout--;
            }
            percent = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % 200;
            percent = percent > 100 ? 200 - percent : percent;
            percent = (WS281xLight.max - WS281xLight.min) * percent / 100 + WS281xLight.min;
            break;
        default: // WS281x_LIGHT_FULL
            if (WS281xLight_list.timeout > 0
                    && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
                    && WS281xLight_counter % (WS281xLight.timeout * 100 / WS281x_TIMER_PERIOD) == WS281xLight.timeout * 100 / WS281x_TIMER_PERIOD - 1) {
                // Timeout x 100 ms
                WS281xLight_list.timeout--;
            }
            percent = WS281xLight.max;
            break;
    }

    uint8_t rainbow = WS281xLight_isRainbow();
    uint8_t colorShift = rainbow ? (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.fading) : 0; // % 256
    
    // Rows (up to 4 rows, color1..color4 for each row)
    for (uint8_t row = 0; row < WS281x_LIGHT_ROWS; row++) {
        uint8_t p = percent;
        if (pattern == WS281x_LIGHT_FADE_TOGGLE && row < (WS281x_LIGHT_ROWS / 2)) {
            p = WS281xLight.max + WS281xLight.min - percent;
        }

        for (uint8_t i = 0; i < WS281x_LIGHT_ROW_COUNT; i++) {
            uint8_t led = row * WS281x_LIGHT_ROW_COUNT + i;
            Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 4]);
            uint8_t q = p;
            if (q == 0 && pattern != WS281x_LIGHT_OFF) {
                color = WS281xLight.color[6];
                q = 255;
            }
            WS281xLight_led(led, 0, color.r * q / 255);
            WS281xLight_led(led, 1, color.g * q / 255);
            WS281xLight_led(led, 2, color.b * q / 255);
        }
    }

    // Rest LEDs (color5..color6 always 100%)
    for (uint8_t led = WS281x_LED_COUNT - WS281x_LIGHT_REST; led < WS281x_LED_COUNT; led++) {
        WS281xLight_led(led, 0, WS281xLight.color[4 + (led % 2)].r);
        WS281xLight_led(led, 1, WS281xLight.color[4 + (led % 2)].g);
        WS281xLight_led(led, 2, WS281xLight.color[4 + (led % 2)].b);
    }
}

inline void WS281xLight_rotation(void) {
    uint8_t shift = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % WS281x_LIGHT_ROW_COUNT;
    if (WS281xLight_list.timeout > 0
            && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
            && shift == WS281x_LIGHT_ROW_COUNT - 1) {
        // Timeout x times
        WS281xLight_list.timeout--;
    }

    uint8_t rainbow = WS281xLight_isRainbow();
    uint8_t colorShift = rainbow ? (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) : 0; // % 256
    
    // Rows (up to 4 rows, color1..color4 for each row)
    for (uint8_t row = 0; row < WS281x_LIGHT_ROWS; row++) {
        for (uint8_t i = 0; i < WS281x_LIGHT_ROW_COUNT; i++) {
            uint8_t led = row * WS281x_LIGHT_ROW_COUNT + i;
            if ((i >= shift && i < shift + WS281xLight.width)
                    || (shift + WS281xLight.width > WS281x_LIGHT_ROW_COUNT
                    && i < shift + WS281xLight.width - WS281x_LIGHT_ROW_COUNT)) {

                uint8_t percent = i >= shift ? i - shift : WS281x_LIGHT_ROW_COUNT - shift + i;
                percent = WS281xLight.width - percent;
                percent = percent * WS281xLight.fading < WS281xLight.max ? WS281xLight.max - (percent * WS281xLight.fading) : 0;
                percent = percent > WS281xLight.min ? percent : 0;

                Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 4]);
                WS281xLight_led(led, 0, color.r * percent / 255);
                WS281xLight_led(led, 1, color.g * percent / 255);
                WS281xLight_led(led, 2, color.b * percent / 255);
            } else {
                WS281xLight_led(led, 0, WS281xLight.color[6].r);
                WS281xLight_led(led, 1, WS281xLight.color[6].g);
                WS281xLight_led(led, 2, WS281xLight.color[6].b);
            }
        }
    }
    
    // Rest LEDs (color5..color6 always 100%)
    for (uint8_t led = WS281x_LED_COUNT - WS281x_LIGHT_REST; led < WS281x_LED_COUNT; led++) {
        WS281xLight_led(led, 0, WS281xLight.color[4 + (led % 2)].r);
        WS281xLight_led(led, 1, WS281xLight.color[4 + (led % 2)].g);
        WS281xLight_led(led, 2, WS281xLight.color[4 + (led % 2)].b);
    }
}

inline void WS281xLight_wipe(void) {
    uint8_t shift = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % (WS281x_LIGHT_ROW_COUNT * 2);
    if (WS281xLight_list.timeout > 0
            && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
            && shift == WS281x_LIGHT_ROW_COUNT * 2 - 1) {
        // Timeout x times
        WS281xLight_list.timeout--;
    }

    uint8_t rainbow = WS281xLight_isRainbow();
    uint8_t colorShift = rainbow ? (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.fading) : 0; // % 256

    // Rows (up to 4 rows, color1..color4 for each row)
    for (uint8_t row = 0; row < WS281x_LIGHT_ROWS; row++) {
        for (uint8_t i = 0; i < WS281x_LIGHT_ROW_COUNT; i++) {
            uint8_t led = row * WS281x_LIGHT_ROW_COUNT + i;
            bool active = i < (shift % WS281x_LIGHT_ROW_COUNT);
            bool wipe = shift / WS281x_LIGHT_ROW_COUNT > 0;
            bool on = (active && !wipe) || (!active && wipe);
            
            Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 4]);
            uint8_t percent = on ? WS281xLight.max : WS281xLight.min;
            if (!on && percent== 0) {
                color = WS281xLight.color[6];
                percent = WS281xLight.max;
            }
            WS281xLight_led(led, 0, color.r * percent / 255);
            WS281xLight_led(led, 1, color.g * percent / 255);
            WS281xLight_led(led, 2, color.b * percent / 255);
        }
    }

    // Rest LEDs (color5..color6 always 100%)
    for (uint8_t led = WS281x_LED_COUNT - WS281x_LIGHT_REST; led < WS281x_LED_COUNT; led++) {
        WS281xLight_led(led, 0, WS281xLight.color[4 + (led % 2)].r);
        WS281xLight_led(led, 1, WS281xLight.color[4 + (led % 2)].g);
        WS281xLight_led(led, 2, WS281xLight.color[4 + (led % 2)].b);
    }
}

inline void WS281xLight_lighthouse(void) {
    uint8_t shift1 = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % WS281x_LIGHT_ROW_COUNT;
    uint8_t shift2 = (shift1 + WS281x_LIGHT_ROW_COUNT / 2) % WS281x_LIGHT_ROW_COUNT;
    if (WS281xLight_list.timeout > 0
            && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
            && shift1 == WS281x_LIGHT_ROW_COUNT - 1) {
        // Timeout x times
        WS281xLight_list.timeout--;
    }

    uint8_t rainbow = WS281xLight_isRainbow();
    uint8_t colorShift = rainbow ? (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) : 0; // % 256
    
    // Rows (up to 4 rows, color1..color4 for each quarter)
    for (uint8_t row = 0; row < WS281x_LIGHT_ROWS; row++) {
        for (uint8_t i = 0; i < WS281x_LIGHT_ROW_COUNT; i++) {
            uint8_t led = row * WS281x_LIGHT_ROW_COUNT + i;
            if ((i >= shift1 && i < shift1 + WS281xLight.width)
                    || (shift1 + WS281xLight.width > WS281x_LIGHT_ROW_COUNT
                    && i < shift1 + WS281xLight.width - WS281x_LIGHT_ROW_COUNT)) {

                uint8_t percent = i >= shift1 ? i - shift1 : WS281x_LIGHT_ROW_COUNT - shift1 + i;
                percent = WS281xLight.width - percent;
                percent = percent * WS281xLight.fading < WS281xLight.max ? WS281xLight.max - (percent * WS281xLight.fading) : 0;
                percent = percent > WS281xLight.min ? percent : 0;

                Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 2]);
                WS281xLight_led(led, 0, color.r * percent / 255);
                WS281xLight_led(led, 1, color.g * percent / 255);
                WS281xLight_led(led, 2, color.b * percent / 255);

            } else if ((i >= shift2 && i < shift2 + WS281xLight.width)
                    || (shift2 + WS281xLight.width > WS281x_LIGHT_ROW_COUNT
                    && i < shift2 + WS281xLight.width - WS281x_LIGHT_ROW_COUNT)) {

                uint8_t percent = i >= shift2 ? i - shift2 : WS281x_LIGHT_ROW_COUNT - shift2 + i;
                percent = WS281xLight.width - percent;
                percent = percent * WS281xLight.fading < WS281xLight.max ? WS281xLight.max - (percent * WS281xLight.fading) : 0;
                percent = percent > WS281xLight.min ? percent : 0;

                Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 2 + 2]);
                WS281xLight_led(led, 0, color.r * percent / 255);
                WS281xLight_led(led, 1, color.g * percent / 255);
                WS281xLight_led(led, 2, color.b * percent / 255);

            } else {
                WS281xLight_led(led, 0, WS281xLight.color[6].r);
                WS281xLight_led(led, 1, WS281xLight.color[6].g);
                WS281xLight_led(led, 2, WS281xLight.color[6].b);
            }
        }
    }
    
    // Rest LEDs (color5..color6 always 100%)
    for (uint8_t led = WS281x_LED_COUNT - WS281x_LIGHT_REST; led < WS281x_LED_COUNT; led++) {
        WS281xLight_led(led, 0, WS281xLight.color[4 + (led % 2)].r);
        WS281xLight_led(led, 1, WS281xLight.color[4 + (led % 2)].g);
        WS281xLight_led(led, 2, WS281xLight.color[4 + (led % 2)].b);
    }
}

inline void WS281xLight_chaise(void) {
    uint8_t shift = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % WS281x_LIGHT_ROW_COUNT;
    if (WS281xLight_list.timeout > 0
            && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
            && shift == WS281x_LIGHT_ROW_COUNT - 1) {
        // Timeout x times
        WS281xLight_list.timeout--;
    }
    
    uint8_t rainbow = WS281xLight_isRainbow();
    uint8_t colorShift = rainbow ? (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) : 0; // % 256
    
    // Rows (up to 4 rows, color1..color4 for each row)
    for (uint8_t row = 0; row < WS281x_LIGHT_ROWS; row++) {
        for (uint8_t i = 0; i < WS281x_LIGHT_ROW_COUNT; i++) {
            uint8_t led = row * WS281x_LIGHT_ROW_COUNT + i;
            if (row < WS281x_LIGHT_ROWS / 2
                    && ((i >= shift && i < shift + WS281xLight.width)
                    || (shift + WS281xLight.width > WS281x_LIGHT_ROW_COUNT
                    && i < shift + WS281xLight.width - WS281x_LIGHT_ROW_COUNT))) {

                uint8_t percent = i >= shift ? i - shift : WS281x_LIGHT_ROW_COUNT - shift + i;
                percent = WS281xLight.width - percent;
                percent = percent * WS281xLight.fading < WS281xLight.max ? WS281xLight.max - (percent * WS281xLight.fading) : 0;
                percent = percent > WS281xLight.min ? percent : 0;

                Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 4]);
                WS281xLight_led(led, 0, color.r * percent / 255);
                WS281xLight_led(led, 1, color.g * percent / 255);
                WS281xLight_led(led, 2, color.b * percent / 255);
            } else if (row >= WS281x_LIGHT_ROWS / 2
                    && ((i >= WS281x_LIGHT_ROW_COUNT - shift - 1 && i < WS281x_LIGHT_ROW_COUNT - shift + WS281xLight.width - 1)
                    || (WS281x_LIGHT_ROW_COUNT - shift + WS281xLight.width - 1 > WS281x_LIGHT_ROW_COUNT
                    && i < WS281xLight.width - shift - 1))) {

                uint8_t percent = i + shift + 1 >= WS281x_LIGHT_ROW_COUNT 
                    ? i + shift + 1 - WS281x_LIGHT_ROW_COUNT
                    : i + shift + 1;
                percent = percent * WS281xLight.fading < WS281xLight.max ? WS281xLight.max - (percent * WS281xLight.fading) : 0;
                percent = percent > WS281xLight.min ? percent : 0;
                //percent = 255;

                Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 4]);
                WS281xLight_led(led, 0, color.r * percent / 255);
                WS281xLight_led(led, 1, color.g * percent / 255);
                WS281xLight_led(led, 2, color.b * percent / 255);
            } else {
                WS281xLight_led(led, 0, WS281xLight.color[6].r);
                WS281xLight_led(led, 1, WS281xLight.color[6].g);
                WS281xLight_led(led, 2, WS281xLight.color[6].b);
            }
        }
    }
    
    // Rest LEDs (color5..color6 always 100%)
    for (uint8_t led = WS281x_LED_COUNT - WS281x_LIGHT_REST; led < WS281x_LED_COUNT; led++) {
        WS281xLight_led(led, 0, WS281xLight.color[4 + (led % 2)].r);
        WS281xLight_led(led, 1, WS281xLight.color[4 + (led % 2)].g);
        WS281xLight_led(led, 2, WS281xLight.color[4 + (led % 2)].b);
    }
}

inline void WS281xLight_spin(void) {
    uint8_t shift = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % WS281x_LIGHT_ROW_COUNT;
    uint16_t speed = WS281xLight_counter / WS281xLight.timeout;
    if (WS281xLight_list.timeout > 0
            && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
            && shift == WS281x_LIGHT_ROW_COUNT - 1) {
        // Timeout x times
        WS281xLight_list.timeout--;
    }

    shift = (shift + speed * 2) % WS281x_LIGHT_ROW_COUNT;
    uint8_t rainbow = WS281xLight_isRainbow();
    uint8_t colorShift = rainbow ? (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) : 0; // % 256
    
    // Rows (up to 4 rows, color1..color4 for each row)
    for (uint8_t row = 0; row < WS281x_LIGHT_ROWS; row++) {
        for (uint8_t i = 0; i < WS281x_LIGHT_ROW_COUNT; i++) {
            uint8_t led = row * WS281x_LIGHT_ROW_COUNT + i;
            if ((i >= shift && i < shift + WS281xLight.width)
                    || (shift + WS281xLight.width > WS281x_LIGHT_ROW_COUNT
                    && i < shift + WS281xLight.width - WS281x_LIGHT_ROW_COUNT)) {

                uint8_t percent = i >= shift ? i - shift : WS281x_LIGHT_ROW_COUNT - shift + i;
                percent = WS281xLight.width - percent;
                percent = percent * WS281xLight.fading < WS281xLight.max ? WS281xLight.max - (percent * WS281xLight.fading) : 0;
                percent = percent > WS281xLight.min ? percent : 0;

                Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 4]);
                WS281xLight_led(led, 0, color.r * percent / 255);
                WS281xLight_led(led, 1, color.g * percent / 255);
                WS281xLight_led(led, 2, color.b * percent / 255);
            } else {
                WS281xLight_led(led, 0, WS281xLight.color[6].r);
                WS281xLight_led(led, 1, WS281xLight.color[6].g);
                WS281xLight_led(led, 2, WS281xLight.color[6].b);
            }
        }
    }
    
    // Rest LEDs (color5..color6 always 100%)
    for (uint8_t led = WS281x_LED_COUNT - WS281x_LIGHT_REST; led < WS281x_LED_COUNT; led++) {
        WS281xLight_led(led, 0, WS281xLight.color[4 + (led % 2)].r);
        WS281xLight_led(led, 1, WS281xLight.color[4 + (led % 2)].g);
        WS281xLight_led(led, 2, WS281xLight.color[4 + (led % 2)].b);
    }
}

inline void WS281xLight_theater(void) {
    uint8_t shift = (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.delay) % (WS281xLight.width * 2);
    if (WS281xLight_list.timeout > 0
            && WS281xLight_list.timeout < WS281x_LIGHT_INDEFINED
            && shift == WS281xLight.width * 2 - 1) {
        // Timeout x times
        WS281xLight_list.timeout--;
    }
   
    uint8_t rainbow = WS281xLight_isRainbow();
    uint8_t colorShift = rainbow ? (WS281xLight_counter * 2 * WS281x_TIMER_PERIOD / WS281xLight.fading) : 0; // % 256

    // Rows (up to 4 rows, color1..color4 for each row)
    for (uint8_t row = 0; row < WS281x_LIGHT_ROWS; row++) {
        for (uint8_t i = 0; i < WS281x_LIGHT_ROW_COUNT; i++) {
            uint8_t led = row * WS281x_LIGHT_ROW_COUNT + i;
            if ((i + shift / 2) % WS281xLight.width == 0) {
                Color_t color = WS281xLight_color(rainbow, led, i, colorShift, WS281xLight.color[row % 4]);
                WS281xLight_led(led, 0, color.r * (shift % 2 == 0 ? WS281xLight.max : WS281xLight.min) / 255);
                WS281xLight_led(led, 1, color.g * (shift % 2 == 0 ? WS281xLight.max : WS281xLight.min) / 255);
                WS281xLight_led(led, 2, color.b * (shift % 2 == 0 ? WS281xLight.max : WS281xLight.min) / 255);
            } else {
                WS281xLight_led(led, 0, WS281xLight.color[6].r);
                WS281xLight_led(led, 1, WS281xLight.color[6].g);
                WS281xLight_led(led, 2, WS281xLight.color[6].b);
            }
        }
    }
    
    // Rest LEDs (color5..color6 always 100%)
    for (uint8_t led = WS281x_LED_COUNT - WS281x_LIGHT_REST; led < WS281x_LED_COUNT; led++) {
        WS281xLight_led(led, 0, WS281xLight.color[4 + (led % 2)].r);
        WS281xLight_led(led, 1, WS281xLight.color[4 + (led % 2)].g);
        WS281xLight_led(led, 2, WS281xLight.color[4 + (led % 2)].b);
    }
}

void WS281xLight_update(void) {
    WS281xLight_changed = false;
    if (WS281xLight_list.size > 1 && WS281xLight_list.timeout == 0) {
        if (WS281xLight_list.index + 1 < WS281xLight_list.size) WS281xLight_list.index++;
        else WS281xLight_list.index = 0;
        WS281xLight = WS281xLight_items[WS281xLight_list.index];
        WS281xLight_list.timeout = WS281xLight_items[WS281xLight_list.index].timeout;
        WS281xLight_counter = 1; // Starts with 1 not to decrease timer 1st run
    }
    switch (WS281xLight.pattern) {
        case WS281x_LIGHT_OFF:
        case WS281x_LIGHT_FULL:
        case WS281x_LIGHT_BLINK:
        case WS281x_LIGHT_FADE_IN:
        case WS281x_LIGHT_FADE_OUT:
        case WS281x_LIGHT_FADE_INOUT:
        case WS281x_LIGHT_FADE_TOGGLE:
            WS281xLight_all(WS281xLight.pattern);
            break;
        case WS281x_LIGHT_ROTATION:
            WS281xLight_rotation();
            break;
        case WS281x_LIGHT_WIPE:
            WS281xLight_wipe();
            break;
        case WS281x_LIGHT_LIGHTHOUSE:
            WS281xLight_lighthouse();
            break;
        case WS281x_LIGHT_CHAISE:
            WS281xLight_chaise();
            break;
        case WS281x_LIGHT_SPIN:
            WS281xLight_spin();
            break;
        case WS281x_LIGHT_THEATER:
            WS281xLight_theater();
            break;
    }
    WS281xLight_counter++;
    if (WS281xLight_changed) WS281xLight_show();
}

void WS281xLight_off(void) {
    WS281xLight_data[0] = 0x01; // To enforce change.
    WS281xLight_set(WS281x_LIGHT_OFF,
                    0x00, 0x00, 0x00, // Color 1
                    0x00, 0x00, 0x00, // Color 2
                    0x00, 0x00, 0x00, // Color 3
                    0x00, 0x00, 0x00, // Color 4
                    0x00, 0x00, 0x00, // Color 5
                    0x00, 0x00, 0x00, // Color 6
                    0x00, 0x00, 0x00, // Color 7
                    1000,             // Delay
                    0,                // Width
                    0,                // Fading
                    0,                // Min
                    0,                // Max
                    10);              // Timeout (x 100ms)
}

inline void WS281xLight_setInternal(WS281xLight_Pattern_t pattern, 
                    uint8_t red1, uint8_t green1, uint8_t blue1,
                    uint8_t red2, uint8_t green2, uint8_t blue2,
                    uint8_t red3, uint8_t green3, uint8_t blue3,
                    uint8_t red4, uint8_t green4, uint8_t blue4,
                    uint8_t red5, uint8_t green5, uint8_t blue5,
                    uint8_t red6, uint8_t green6, uint8_t blue6,
                    uint8_t red7, uint8_t green7, uint8_t blue7,
                    uint16_t delay, uint8_t width, uint8_t fading,
                    uint8_t min, uint8_t max, uint8_t timeout) {
    WS281xLight.pattern = pattern;
    WS281xLight.color[0].r = red1;
    WS281xLight.color[0].g = green1;
    WS281xLight.color[0].b = blue1;
    WS281xLight.color[1].r = red2;
    WS281xLight.color[1].g = green2;
    WS281xLight.color[1].b = blue2;
    WS281xLight.color[2].r = red3;
    WS281xLight.color[2].g = green3;
    WS281xLight.color[2].b = blue3;
    WS281xLight.color[3].r = red4;
    WS281xLight.color[3].g = green4;
    WS281xLight.color[3].b = blue4;
    WS281xLight.color[4].r = red5;
    WS281xLight.color[4].g = green5;
    WS281xLight.color[4].b = blue5;
    WS281xLight.color[5].r = red6;
    WS281xLight.color[5].g = green6;
    WS281xLight.color[5].b = blue6;
    WS281xLight.color[6].r = red7;
    WS281xLight.color[6].g = green7;
    WS281xLight.color[6].b = blue7;
    WS281xLight.delay = delay / WS281x_TIMER_PERIOD;
    WS281xLight.width = width;
    WS281xLight.fading = fading;
    WS281xLight.min = min;
    WS281xLight.max = max;
    WS281xLight.timeout = timeout;
}

inline void WS281xLight_add(WS281xLight_Pattern_t pattern, 
                    uint8_t red1, uint8_t green1, uint8_t blue1,
                    uint8_t red2, uint8_t green2, uint8_t blue2,
                    uint8_t red3, uint8_t green3, uint8_t blue3,
                    uint8_t red4, uint8_t green4, uint8_t blue4,
                    uint8_t red5, uint8_t green5, uint8_t blue5,
                    uint8_t red6, uint8_t green6, uint8_t blue6,
                    uint8_t red7, uint8_t green7, uint8_t blue7,
                    uint16_t delay, uint8_t width, uint8_t fading,
                    uint8_t min, uint8_t max, uint8_t timeout) {
    WS281xLight_setInternal(pattern, 
            red1, green1, blue1,
            red2, green2, blue2,
            red3, green3, blue3,
            red4, green4, blue4,
            red5, green5, blue5,
            red6, green6, blue6,
            red7, green7, blue7,
            delay, width, fading, min, max, timeout);

    WS281xLight_items[WS281xLight_list.size++] = WS281xLight;
}


inline void WS281xLight_set(WS281xLight_Pattern_t pattern, 
                    uint8_t red1, uint8_t green1, uint8_t blue1,
                    uint8_t red2, uint8_t green2, uint8_t blue2,
                    uint8_t red3, uint8_t green3, uint8_t blue3,
                    uint8_t red4, uint8_t green4, uint8_t blue4,
                    uint8_t red5, uint8_t green5, uint8_t blue5,
                    uint8_t red6, uint8_t green6, uint8_t blue6,
                    uint8_t red7, uint8_t green7, uint8_t blue7,
                    uint16_t delay, uint8_t width, uint8_t fading,
                    uint8_t min, uint8_t max, uint8_t timeout) {
    WS281xLight_setInternal(pattern, 
            red1, green1, blue1,
            red2, green2, blue2,
            red3, green3, blue3,
            red4, green4, blue4,
            red5, green5, blue5,
            red6, green6, blue6,
            red7, green7, blue7,
            delay, width, fading, min, max, timeout);

    WS281xLight_items[0] = WS281xLight;
    WS281xLight_list.index = 0;
    WS281xLight_list.size = 1;
    WS281xLight_list.timeout = timeout;
}

#endif