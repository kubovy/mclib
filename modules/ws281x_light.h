/* 
 * File:   ws281x_light.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#ifndef WS281X_LIGHT_H
#define	WS281X_LIGHT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../lib/requirements.h"

#if defined WS281x_LIGHT_ROWS && defined WS281x_LIGHT_ROW_COUNT

// Configuration
#ifndef WS281x_LED_COUNT
#error "WS281xLight: WS281x_LED_COUNT needs to be defined!"
#endif

#ifndef WS281x_LIGHT_REST
#define WS281x_LIGHT_REST WS281x_LED_COUNT - (WS281x_LIGHT_ROW_COUNT * WS281x_LIGHT_ROWS)
#endif
    
#ifndef WS281x_TIMER_PERIOD
#ifdef TIMER_PERIOD
#warning "WS281xLight: WS281x_TIMER_PERIOD defaults to TIMER_PERIOD"
#define WS281x_TIMER_PERIOD TIMER_PERIOD
#endif
#endif
    
#ifndef WS281x_LIGHT_SPEED
#define WS281x_LIGHT_SPEED 2 * WS281x_TIMER_PERIOD
#endif
    
#ifndef WS281x_LIGHT_LIST_SIZE
#define WS281x_LIGHT_LIST_SIZE 20
#endif

#define WS281x_LIGHT_INDEFINED 0xFF

typedef enum {
    WS281x_LIGHT_OFF = 0x00,
    WS281x_LIGHT_FULL = 0x01,
    WS281x_LIGHT_BLINK = 0x02,
    WS281x_LIGHT_FADE_IN = 0x03,
    WS281x_LIGHT_FADE_OUT = 0x04,
    WS281x_LIGHT_FADE_INOUT = 0x05,
    WS281x_LIGHT_FADE_TOGGLE = 0x06,
    WS281x_LIGHT_ROTATION = 0x07,
    WS281x_LIGHT_WIPE = 0x08,
    WS281x_LIGHT_LIGHTHOUSE = 0x09,
    WS281x_LIGHT_CHAISE = 0x0A,
    WS281x_LIGHT_THEATER = 0x0B,
} WS281xLight_Pattern_t;

typedef struct { // 8+16+8+8+8+8+(6*8)=16+40+48=104 bytes
    WS281xLight_Pattern_t pattern;
    uint16_t delay;
    uint8_t width;
    uint8_t fading;
    uint8_t min;
    uint8_t max;
    Color_t color[7]; // 7x8=56
    uint8_t timeout;
} WS281xLight_t;

struct {
    uint8_t index;
    uint8_t size;
    uint8_t timeout;
} WS281xLight_list = {0, 0, WS281x_LIGHT_INDEFINED};

WS281xLight_t WS281xLight_items[WS281x_LIGHT_LIST_SIZE];

void WS281xLight_update(void);

void WS281xLight_off(void);

inline void WS281xLight_add(WS281xLight_Pattern_t pattern, 
                    uint8_t red1, uint8_t green1, uint8_t blue1,
                    uint8_t red2, uint8_t green2, uint8_t blue2,
                    uint8_t red3, uint8_t green3, uint8_t blue3,
                    uint8_t red4, uint8_t green4, uint8_t blue4,
                    uint8_t red5, uint8_t green5, uint8_t blue5,
                    uint8_t red6, uint8_t green6, uint8_t blue6,
                    uint8_t red7, uint8_t green7, uint8_t blue7,
                    uint16_t delay, uint8_t width, uint8_t fading,
                    uint8_t min, uint8_t max, uint8_t timeout);

inline void WS281xLight_set(WS281xLight_Pattern_t pattern, 
                    uint8_t red1, uint8_t green1, uint8_t blue1,
                    uint8_t red2, uint8_t green2, uint8_t blue2,
                    uint8_t red3, uint8_t green3, uint8_t blue3,
                    uint8_t red4, uint8_t green4, uint8_t blue4,
                    uint8_t red5, uint8_t green5, uint8_t blue5,
                    uint8_t red6, uint8_t green6, uint8_t blue6,
                    uint8_t red7, uint8_t green7, uint8_t blue7,
                    uint16_t delay, uint8_t width, uint8_t fading,
                    uint8_t min, uint8_t max, uint8_t timeout);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* WS281X_LIGHT_H */

