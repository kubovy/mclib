/* 
 * File:   ws281x.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "ws281x.h"

#ifdef WS281x_BUFFER

uint8_t ledDataR[WS281x_LED_COUNT];
uint8_t ledDataG[WS281x_LED_COUNT];
uint8_t ledDataB[WS281x_LED_COUNT];
uint8_t ledTempR[WS281x_LED_COUNT];
uint8_t ledTempG[WS281x_LED_COUNT];
uint8_t ledTempB[WS281x_LED_COUNT];
uint8_t ledPattern[WS281x_LED_COUNT];
uint8_t ledDelay[WS281x_LED_COUNT];
uint8_t ledCounter[WS281x_LED_COUNT];
uint8_t ledMin[WS281x_LED_COUNT];
uint8_t ledMax[WS281x_LED_COUNT];
uint16_t timerPeriod = 10;
uint8_t counter = 0;

void (*WS281x_Switcher)(bool);

void WS281x_init(uint16_t period) {
    timerPeriod = period;
    WS281x_off();
}

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

void WS281x_update(void) {
    bool on;
    uint8_t percent;
    for (uint8_t led = 0; led < WS281x_LED_COUNT; led++) {
        if (ledPattern[led] > WS281x_PATTERN_LIGHT) {
            if (counter % ledDelay[led] == 0) {
                switch(ledPattern[led]) {
                    case WS281x_PATTERN_BLINK:
                        on = ledCounter[led] % 2;
                        ledTempR[led] = on ? ledDataR[led] : 0x00;
                        ledTempG[led] = on ? ledDataG[led] : 0x00;
                        ledTempB[led] = on ? ledDataB[led] : 0x00;
                        ledCounter[led] = ledCounter[led] ? 0 : 1;
                        break;
                    case WS281x_PATTERN_FADE_IN:
                        percent = ((uint16_t) (ledMax[led] - ledMin[led]) * ledCounter[led]) / 100  + ledMin[led];
                        ledTempR[led] = ((uint16_t) ledDataR[led]) * percent / 100;
                        ledTempG[led] = ((uint16_t) ledDataG[led]) * percent / 100;
                        ledTempB[led] = ((uint16_t) ledDataB[led]) * percent / 100;
                        ledCounter[led] = (ledCounter[led] + 1) % 100;
                        break;
                    case WS281x_PATTERN_FADE_OUT:
                        percent = ledMax[led] - ((uint16_t) (ledMax[led] - ledMin[led]) * ledCounter[led]) / 100;
                        ledTempR[led] = ((uint16_t) ledDataR[led]) * percent / 100;
                        ledTempG[led] = ((uint16_t) ledDataG[led]) * percent / 100;
                        ledTempB[led] = ((uint16_t) ledDataB[led]) * percent / 100;
                        ledCounter[led] = (ledCounter[led] + 1) % 100;
                        break;
                    case WS281x_PATTERN_FADE_TOGGLE:
                        percent = ledCounter[led] > 100 ? 200 - ledCounter[led] : ledCounter[led];
                        percent = (ledMax[led] - ledMin[led]) * percent / 100 + ledMin[led];
                        ledTempR[led] = ((uint16_t) ledDataR[led]) * percent / 100;
                        ledTempG[led] = ((uint16_t) ledDataG[led]) * percent / 100;
                        ledTempB[led] = ((uint16_t) ledDataB[led]) * percent / 100;
                        ledCounter[led] = (ledCounter[led] + 1) % 200;
                        break;
                }
            }
        }
    }
    counter++;
    if (counter >= 200) counter = 0;
    WS281x_show();
}

uint8_t WS281x_getPattern(uint8_t led) {
    return ledPattern[led];
}

uint8_t WS281x_getRed(uint8_t led) {
    return ledDataR[led];
}

uint8_t WS281x_getGreen(uint8_t led) {
    return ledDataG[led];
}

uint8_t WS281x_getBlue(uint8_t led) {
    return ledDataB[led];
}

void WS281x_set(uint8_t led, uint8_t pattern, uint8_t r, uint8_t g, uint8_t b,
                uint16_t delay, uint8_t min, uint8_t max) {
    if (WS281x_Switcher) WS281x_Switcher(true);
#ifdef WS281x_SW_LAT
    WS281x_SW_LAT = true;
#endif
    ledPattern[led] = pattern;

    ledDataR[led] = r;
    ledDataG[led] = g;
    ledDataB[led] = b;

    ledTempR[led] = r;
    ledTempG[led] = g;
    ledTempB[led] = b;

    ledDelay[led] = delay / timerPeriod;
    ledCounter[led] = 0;
    
    ledMin[led] = min;
    ledMax[led] = max;
}

void WS281x_off(void) {
    for (uint16_t led = 0; led < WS281x_LED_COUNT; led++) {
        WS281x_RGB(led, 0x00, 0x00, 0x00);
    }
    WS281x_show();
    if (WS281x_Switcher) WS281x_Switcher(false);
#ifdef WS281x_SW_LAT
    WS281x_SW_LAT = false;
#endif
}

void WS281x_RGB(uint8_t led, uint8_t r, uint8_t g, uint8_t b) {
    WS281x_set(led, WS281x_PATTERN_LIGHT, r, g, b, 0, 0, 0);
}

void WS281x_blink(uint8_t led, uint8_t r, uint8_t g, uint8_t b, uint16_t delay) {
    WS281x_set(led, WS281x_PATTERN_BLINK, r, g, b, delay, 0, 0);
}

void WS281x_fadeIn(uint8_t led, uint8_t r, uint8_t g, uint8_t b, uint16_t delay,
                   uint8_t min, uint8_t max) {
    WS281x_set(led, WS281x_PATTERN_FADE_IN, r, g, b, delay, min, max);
}

void WS281x_fadeOut(uint8_t led, uint8_t r, uint8_t g, uint8_t b,
                    uint16_t delay, uint8_t min, uint8_t max) {
    WS281x_set(led, WS281x_PATTERN_FADE_OUT, r, g, b, delay, min, max);
}

void WS281x_fadeToggle(uint8_t led, uint8_t r, uint8_t g, uint8_t b,
                       uint16_t delay, uint8_t min, uint8_t max) {
    WS281x_set(led, WS281x_PATTERN_FADE_TOGGLE, r, g, b, delay, min, max);
}

void WS281x_all(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t led = 0; led < WS281x_LED_COUNT; led++) {
        WS281x_RGB(led, r, g, b);
    }
}

#endif