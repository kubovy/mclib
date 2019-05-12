/* 
 * File:   ws281x.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "ws281x.h"

#ifdef WS281x_BUFFER

uint8_t ledTempR[WS281x_LED_COUNT];
uint8_t ledTempG[WS281x_LED_COUNT];
uint8_t ledTempB[WS281x_LED_COUNT];

uint16_t WS281x_counter = 0;
void (*WS281x_Switcher)(bool);

void WS281x_setSwitcher(void (* Switcher)(bool)) {
    WS281x_Switcher = Switcher;
}

void WS281x_show(void) {
    for (uint8_t i = 0; i < WS281x_LED_COUNT; i++) {
        WS281x_BUFFER = ledTempR[i];
        __delay_us(15);
        WS281x_BUFFER = ledTempG[i];
        __delay_us(15);
        WS281x_BUFFER = ledTempB[i];
        __delay_us(15);
    }
}

#ifdef WS281x_TIMER_PERIOD
void WS281x_update(void) {
    uint8_t percent;
    for (uint8_t led = 0; led < WS281x_LED_COUNT; led++) {
        switch(WS281x_ledPattern[led]) {
            case WS281x_PATTERN_LIGHT:
                percent = 255;
                break;
            case WS281x_PATTERN_BLINK:
                percent = (WS281x_counter / WS281x_ledDelay[led]) % 2 ? WS281x_ledMax[led] : WS281x_ledMin[led];
                break;
            case WS281x_PATTERN_FADE_IN:
                percent = (WS281x_counter / WS281x_ledDelay[led]) % 100;
                percent = ((uint16_t) (WS281x_ledMax[led] - WS281x_ledMin[led]) * percent) / 99  + WS281x_ledMin[led];
                break;
            case WS281x_PATTERN_FADE_OUT:
                percent = (WS281x_counter / WS281x_ledDelay[led]) % 100;
                percent = WS281x_ledMax[led] - ((uint16_t) (WS281x_ledMax[led] - WS281x_ledMin[led]) * percent) / 99;
                break;
            case WS281x_PATTERN_FADE_TOGGLE:
                percent = (WS281x_counter / WS281x_ledDelay[led]) % 200;
                percent = percent > 100 ? 200 - percent : percent;
                percent = (WS281x_ledMax[led] - WS281x_ledMin[led]) * percent / 100 + WS281x_ledMin[led];
                break;
        }
        ledTempR[led] = WS281x_ledRed[led] * percent / 255;
        ledTempG[led] = WS281x_ledGreen[led] * percent / 255;
        ledTempB[led] = WS281x_ledBlue[led] * percent / 255;
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

    ledTempR[led] = r;
    ledTempG[led] = g;
    ledTempB[led] = b;

#ifdef WS281x_TIMER_PERIOD
    WS281x_ledDelay[led] = delay / WS281x_TIMER_PERIOD;
#endif
    //ledCounter[led] = 0;
    
    WS281x_ledMin[led] = min;
    WS281x_ledMax[led] = max;
    WS281x_show();
}

void WS281x_off(void) {
    for (uint16_t led = 0; led < WS281x_LED_COUNT; led++) {
        WS281x_all(0x00, 0x00, 0x00);
    }
    WS281x_show();
    if (WS281x_Switcher) WS281x_Switcher(false);
#ifdef WS281x_SW_LAT
    WS281x_SW_LAT = false;
#endif
}

inline void WS281x_all(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t led = 0; led < WS281x_LED_COUNT; led++) {
        WS281x_set(led, WS281x_PATTERN_LIGHT, r, g, b, 0, 0, 0);
    }
}

#endif