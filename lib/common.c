/* 
 * File:   common.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "common.h"

uint8_t waitingLoaderCounter = 0;

unsigned char dec2hex(uint8_t dec) {
    return dec > 9 ? dec + 55 : dec + 48;
}

uint8_t hex2dec(unsigned char hex) {
    return hex >= 65 ? hex - 55 : hex - 48;
}

uint8_t strlen(char *str) {
    uint8_t i = 0;
    while (str[i++] != '\0');
    return i;
}

bool strcmp(char *str1, char *str2) {
    uint8_t i = 0;
    while (str1[i] != '\0') {
        if (str1[i] != str2[i]) {
            return false;
        }
        i++;
    }
    return str1[i] == str2[i];
}

void strcpy(char *dest, char* src, uint8_t len) {
    uint8_t i = 0;
    while (i < len && *(src + i) != '\0') {
        *(dest + i) = *(src + i);
        i++;
    }
    *(dest + i) = '\0';
}

void (*TriggerRef)(void);

void watchDogTrigger(uint8_t *counter, uint8_t period, void (* Trigger)(void)) {
    if (*counter == 0) {
        *counter = period;
        TriggerRef = Trigger;
        if (TriggerRef) TriggerRef();
#ifdef WATCH_DOG_LED_LAT
        WATCH_DOG_LED_LAT = ~WATCH_DOG_LED_LAT;
#endif
    }
}

#ifdef LCD_ADDRESS
void printProgress(char* title, uint16_t value, uint16_t max) {
    uint8_t i;
    LCD_displayString("                    ", 0);
    LCD_displayString(title, 1);

    if (value < max) {
        char bar[LCD_COLS + 1] = "                    \0";
        char message[LCD_COLS + 1] = "|c|   %\0";
        while(value > 100) {
            value /= 10;
            max /= 10;
        }
        uint16_t percent = value * 100 / max;
        uint16_t progress = value * 20 / max;

        for (i = 0; i < progress; i++) {
            bar[i] = 0xFF;
        }
        LCD_displayString(bar, 2);

        if (percent >= 100) message[3] = dec2hex(percent / 100 % 10);
        if (percent >= 10) message[4] = dec2hex(percent / 10 % 10);
        message[5] = dec2hex(percent % 10);
        LCD_displayString(message, 3);
    } else {
        //LCD_send_string("                    ", 2);
        LCD_displayString("|c|DONE", 2);
        LCD_displayString("                    ", 3);
    }
}

void watingLoaderDots(void) {
    switch(waitingLoaderCounter) {
        case 0:
            LCD_displayString("|c|     ", 1);
            break;
        case 1:
            LCD_displayString("|c|.    ", 1);
            break;
        case 2:
            LCD_displayString("|c|..   ", 1);
            break;
        case 3:
            LCD_displayString("|c|...  ", 1);
            break;
        case 4:
            LCD_displayString("|c|.... ", 1);
            break;
        case 5:
            LCD_displayString("|c|.....", 1);
            break;
    }
    waitingLoaderCounter++;
    if (waitingLoaderCounter > 5) waitingLoaderCounter = 0;
}
#endif

#ifdef LCD_ADDRESS
void watingLoaderWave(void) {
    switch(waitingLoaderCounter) {
        case 0:
            LCD_displayString("|c|  .  ", 1);
            break;
        case 1:
            LCD_displayString("|c| .o. ", 1);
            break;
        case 2:
            LCD_displayString("|c|.oOo.", 1);
            break;
        case 3:
            LCD_displayString("|c|oOoOo", 1);
            break;
        case 4:
            LCD_displayString("|c|Oo.oO", 1);
            break;
        case 5:
            LCD_displayString("|c|o. .o", 1);
            break;
        case 6:
            LCD_displayString("|c|.   .", 1);
            break;
    }
    waitingLoaderCounter++;
    if (waitingLoaderCounter > 6) waitingLoaderCounter = 0;
}
#endif