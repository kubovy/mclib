/* 
 * File:   ws281x.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "ws281x.h"

#if defined WS281x_BUFFER && defined WS281x_INDICATORS

uint8_t WS281x_ledTempR[WS281x_LED_COUNT];
uint8_t WS281x_ledTempG[WS281x_LED_COUNT];
uint8_t WS281x_ledTempB[WS281x_LED_COUNT];

uint16_t WS281x_counter = 0;
void (*WS281x_Switcher)(bool);

void WS281x_setSwitcher(void (* Switcher)(bool)) {
    WS281x_Switcher = Switcher;
}

inline void WS281x_show(void) {
    for (uint8_t i = 0; i < WS281x_LED_COUNT; i++) {
#ifdef WS281x_GRB
        WS281x_BUFFER = WS281x_ledTempG[i];
        __delay_us(15);
        WS281x_BUFFER = WS281x_ledTempR[i];
        __delay_us(15);
        WS281x_BUFFER = WS281x_ledTempB[i];
        __delay_us(15);
#else
        WS281x_BUFFER = WS281x_ledTempR[i];
        __delay_us(15);
        WS281x_BUFFER = WS281x_ledTempG[i];
        __delay_us(15);
        WS281x_BUFFER = WS281x_ledTempB[i];
        __delay_us(15);
#endif
    }
    __delay_us(50);
}

#ifdef WS281x_TIMER_PERIOD
void WS281x_update(void) {
    uint8_t percent; // Percent in bytes (0-255) where 255 = 100%
    for (uint8_t led = 0; led < WS281x_LED_COUNT; led++) {
        switch(WS281x_ledPattern[led]) {
            case WS281x_PATTERN_LIGHT:
                percent = 255;
                break;
            case WS281x_PATTERN_BLINK:
                percent = (WS281x_counter / WS281x_ledDelay[led]) % 2 ? WS281x_ledMax[led] : WS281x_ledMin[led];
                break;
            case WS281x_PATTERN_FADE_IN:
                percent = (WS281x_counter * WS281x_SPEED / WS281x_ledDelay[led]) % 100;
                percent = ((uint16_t) (WS281x_ledMax[led] - WS281x_ledMin[led]) * percent) / 99  + WS281x_ledMin[led];
                break;
            case WS281x_PATTERN_FADE_OUT:
                percent = (WS281x_counter * WS281x_SPEED / WS281x_ledDelay[led]) % 100;
                percent = WS281x_ledMax[led] - ((uint16_t) (WS281x_ledMax[led] - WS281x_ledMin[led]) * percent) / 99;
                break;
            case WS281x_PATTERN_FADE_TOGGLE:
                percent = (WS281x_counter * WS281x_SPEED * 2 / WS281x_ledDelay[led]) % 200;
                percent = percent > 100 ? 200 - percent : percent;
                percent = (WS281x_ledMax[led] - WS281x_ledMin[led]) * percent / 100 + WS281x_ledMin[led];
                break;
        }
        WS281x_ledTempR[led] = ((uint16_t) WS281x_ledRed[led]) * percent / 255 * WS281x_MAX / 255;
        WS281x_ledTempG[led] = ((uint16_t) WS281x_ledGreen[led]) * percent / 255 * WS281x_MAX / 255;
        WS281x_ledTempB[led] = ((uint16_t) WS281x_ledBlue[led]) * percent / 255 * WS281x_MAX / 255;
    }
    WS281x_counter++;
    WS281x_show();
}
#endif

void WS281x_set(uint8_t led, WS281x_Pattern_t pattern, uint8_t r, uint8_t g,
                uint8_t b, uint16_t delay, uint8_t min, uint8_t max) {
    if (WS281x_Switcher) WS281x_Switcher(true);
#ifdef WS281x_SW_LAT
    WS281x_SW_LAT = true;
#endif
    WS281x_ledPattern[led] = pattern;

    WS281x_ledRed[led] = r;
    WS281x_ledGreen[led] = g;
    WS281x_ledBlue[led] = b;

    WS281x_ledTempR[led] = r;
    WS281x_ledTempG[led] = g;
    WS281x_ledTempB[led] = b;

#ifdef WS281x_TIMER_PERIOD
    WS281x_ledDelay[led] = delay / WS281x_TIMER_PERIOD;
#endif
    //ledCounter[led] = 0;
    
    WS281x_ledMin[led] = min;
    WS281x_ledMax[led] = max;
#ifndef WS281x_TIMER_PERIOD
    WS281x_show();
#endif
}

void WS281x_off(void) {
    WS281x_all(0x00, 0x00, 0x00);
//#ifndef WS281x_TIMER_PERIOD
    WS281x_show();
//#endif

    if (WS281x_Switcher) WS281x_Switcher(false);
#ifdef WS281x_SW_LAT
    WS281x_SW_LAT = false;
#endif
}

inline void WS281x_all(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t led = 0; led < WS281x_LED_COUNT; led++) {
        WS281x_set(led, WS281x_PATTERN_LIGHT, r, g, b, 0, 0, 255);
    }
}

#endif
