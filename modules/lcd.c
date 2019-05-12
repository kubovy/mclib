/* 
 * File:   lcd.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "lcd.h"
#include "memory.h"

#ifdef LCD_ADDRESS

#ifdef MEM_LCD_CACHE_START
uint16_t LCD_memAddr;
#else
char LCD_cache[LCD_ROWS][LCD_COLS];
#endif

uint8_t LCD_r, LCD_c;
uint8_t LCD_nibbleUpper, LCD_nibbleLower;
uint8_t LCD_data[4];


inline void LCD_clearContent(void) {
    for (LCD_r = 0; LCD_r < LCD_ROWS; LCD_r++) {
        for (LCD_c = 0; LCD_c < LCD_COLS; LCD_c++) {
            LCD_setCache(LCD_r, LCD_c, ' ');
        }
    }
}

void LCD_send(uint8_t command, uint8_t mode) {
    // Select lower nibble by moving it to the upper nibble position
    LCD_nibbleLower = (command << 4) & 0xF0;
    // Select upper nibble
    LCD_nibbleUpper = command & 0xF0;      
 
    LCD_data[0] = LCD_nibbleUpper | LCD_backlight | En | mode;
    LCD_data[1] = LCD_nibbleUpper | LCD_backlight | mode;
    LCD_data[2] = LCD_nibbleLower | LCD_backlight | En | mode;
    LCD_data[3] = LCD_nibbleLower | LCD_backlight | mode;

    I2C_writeData(LCD_ADDRESS, LCD_data, 4);
    __delay_ms(2);
}

void LCD_init(void) {
    __delay_ms(41); // > 40 ms
    LCD_send(LCD_RETURNHOME | LCD_CLEARDISPLAY, 0);
    __delay_ms(5); // > 4.1 ms
    LCD_send(LCD_RETURNHOME | LCD_CLEARDISPLAY, 0);
    __delay_us(150); // > 100 
    LCD_send(LCD_RETURNHOME | LCD_CLEARDISPLAY, 0);
    LCD_send(LCD_RETURNHOME, 0);

    LCD_send(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS, 0);
    LCD_send(LCD_DISPLAYCONTROL | LCD_DISPLAYOFF | LCD_CURSOROFF | LCD_BLINKOFF, 0);
    LCD_send(LCD_CLEARDISPLAY, 0);
    LCD_send(LCD_ENTRYMODESET | LCD_ENTRY_LEFT, 0);

    //__delay_ms(200);
    
    LCD_send(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF, 0);
    LCD_send(LCD_RETURNHOME, 0);
    LCD_setBacklight(false);
    
    LCD_createChar(0x01, LCD_CHAR_BACKSLASH);
    LCD_send(LCD_RETURNHOME, 0);
}

inline void LCD_clear(void) {
    LCD_send(LCD_CLEARDISPLAY, 0);
    LCD_send(LCD_RETURNHOME, 0);
    __delay_ms(200);
    LCD_clearContent();  
}

inline char LCD_getCache(uint8_t row, uint8_t column) {
#ifdef MEM_LCD_CACHE_START
    LCD_memAddr = MEM_LCD_CACHE_START + (row * LCD_COLS) + column;
    return MEM_read(MEM_ADDRESS, LCD_memAddr >> 8, LCD_memAddr & 0xFF);
#else
    return row < LCD_ROWS && column < LCD_COLS ? LCD_cache[row][column] : 0x00;
#endif
}

inline void LCD_setCache(uint8_t row, uint8_t column, char ch) {
#ifdef MEM_LCD_CACHE_START
    LCD_memAddr = MEM_LCD_CACHE_START + (row * LCD_COLS) + column;
    MEM_write(MEM_ADDRESS, LCD_memAddr >> 8, LCD_memAddr & 0xFF, (uint8_t) ch);
#else
    LCD_cache[row][column] = ch;
#endif
}

inline void LCD_selectLine(uint8_t line) {
    switch(line) {
        case 1:
            LCD_send(LCD_LINE2, 0);
            break;
        case 2:
            LCD_send(LCD_LINE3, 0);
            break;
        case 3:
            LCD_send(LCD_LINE4, 0);
            break;
        default:
            LCD_send(LCD_LINE1, 0);
            break;
    }
}

void LCD_displayCache(void) {
    for (LCD_r = 0; LCD_r < LCD_ROWS; LCD_r++) {
        LCD_selectLine(LCD_r);
        for (LCD_c = 0; LCD_c < LCD_COLS; LCD_c++) {
            LCD_send(LCD_getCache(LCD_r, LCD_c), Rs);
        }
    }
}

inline void LCD_reset(void) {
    LCD_init();
    LCD_setBacklight(true);
    LCD_displayCache();
}

void LCD_setBacklight(bool on) {
    if ((on ? LCD_BACKLIGHT : LCD_NOBACKLIGHT) != LCD_backlight) {
        LCD_backlight = on ? LCD_BACKLIGHT : LCD_NOBACKLIGHT;
        I2C_writeByte(LCD_ADDRESS, LCD_backlight);
    }
}

//void LCD_sendCommand(uint8_t command) {
//    // Select lower nibble by moving it to the upper nibble position
//    LCD_nibbleLower = (command << 4) & 0xF0;
//    // Select upper nibble
//    LCD_nibbleUpper = command & 0xF0;      
// 
//    LCD_data[0] = LCD_nibbleUpper | LCD_backlight | En;
//    LCD_data[1] = LCD_nibbleUpper | LCD_backlight;
//    LCD_data[2] = LCD_nibbleLower | LCD_backlight | En;
//    LCD_data[3] = LCD_nibbleLower | LCD_backlight;
//
//    I2C_writeData(LCD_ADDRESS, LCD_data, 4);
//    __delay_ms(2);
//}

//void LCD_sendData(uint8_t data) {
//    // Select lower nibble by moving it to the upper nibble position
//    LCD_nibbleLower = (data<<4) & 0xF0;
//    // Select upper nibble
//    LCD_nibbleUpper = data & 0xF0;
// 
//    LCD_data[0] = LCD_nibbleUpper | LCD_backlight | En | Rs;
//    LCD_data[1] = LCD_nibbleUpper | LCD_backlight | Rs;
//    LCD_data[2] = LCD_nibbleLower | LCD_backlight | En | Rs;
//    LCD_data[3] = LCD_nibbleLower | LCD_backlight | Rs;
//
//    I2C_writeData(LCD_ADDRESS, LCD_data, 4);
//    __delay_ms(2);
//}

void LCD_displayLine(uint8_t line) {
    LCD_selectLine(line);
    for (LCD_c = 0; LCD_c < LCD_COLS; LCD_c++) {
        LCD_send(LCD_getCache(line, LCD_c), Rs);
    }
}

void LCD_setString(char *str, uint8_t line, bool display) {
#ifdef LCD_EXTENDED_FEATURES
    uint8_t prefix;
    uint16_t i,delay;
#endif

    while (*str && line < LCD_ROWS) {
#ifdef LCD_EXTENDED_FEATURES
        prefix = 0;
        delay = 0xFF;
#endif
        
#ifdef LCD_EXTENDED_FEATURES
        if (*str == '|' && *(str + 2) == '|') {
            // Starting with |c| will center the message
            //               |r| will right align the message
            if (*(str + 1) == 'c' || *(str + 1) == 'r') {
                while(*(str + prefix + 3) && *(str + prefix + 3) != '\n') prefix++;
                if (prefix > LCD_COLS) prefix = LCD_COLS;
                prefix = LCD_COLS - prefix;
                if (*(str + 1) == 'c') prefix /= 2;
                str += 3; // Ignore /\|(c|d)\|/
            }
            if (*(str + 1) == 'l') {
                str += 3; // Ignore /\|l\|/
            }
            // |d|XXX| will type write characters with XXXms delay
            if (*(str + 1) == 'd') {
                i = 0;
                delay = 0;
                while(*(str + i + 3) >= '0' && *(str + i + 3) <= '9') {
                    delay = delay * 10 + (*(str + (i++) + 3) - 48);
                }
                str += i + 4; // Ignore /\|d\|\d+\|/
            }
        }
#endif
        
        if (display) LCD_selectLine(line);
#ifdef LCD_EXTENDED_FEATURES
        for (i = 0; i < prefix; i++) {
            LCD_setCache(line, i, ' ');
            if (display) LCD_send(' ', Rs);
        }
        LCD_c = prefix;
#else
        LCD_c = 0;
#endif
        while (*str && *str != '\n' && LCD_c < LCD_COLS) {
            LCD_setCache(line, LCD_c, *str);
            if (display) LCD_send(*str, Rs);
#ifdef LCD_EXTENDED_FEATURES
            if (delay == 0xFF) __delay_us(500);
            else for(i = 0; i < delay; i++) __delay_ms(1);
#endif
            LCD_c++;
            str++;
        }
        // Ignore rest of the line or string.
        if (LCD_c >= LCD_COLS) while (*str && *str != '\n') str++;

#ifdef LCD_EXTENDED_FEATURES
        for (i = LCD_c; i < LCD_COLS; i++) {
            LCD_setCache(line, i, ' ');
            if (display) LCD_send(' ', Rs);
        }
#endif
        if (*str == '\n') str++; // Skip new line character

        line++;
    }
}

void LCD_replaceChar(char c, uint8_t position, uint8_t line, bool display) {
    if (line < LCD_ROWS && position < LCD_COLS) {
        LCD_setCache(line, position, c);
        if (display) {
            LCD_selectLine(line);
            for (LCD_c = 0; LCD_c < LCD_COLS; LCD_c++) {
                LCD_send(LCD_getCache(line, LCD_c), Rs);
            }
        }
    }
}

void LCD_replaceString(char *str, uint8_t position, uint8_t line, bool display) {
    if (line < LCD_ROWS) {
        while (*str && *str != '\n' && position < LCD_COLS) {
            LCD_setCache(line, position++, *str++);
        }
        if (display) {
            LCD_selectLine(line);
            for (LCD_c = 0; LCD_c < LCD_COLS; LCD_c++) {
                LCD_send(LCD_getCache(line, LCD_c), Rs);
            }
        }
    }
}

void LCD_createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	LCD_send(LCD_SETCGRAMADDR | (location << 3), 0);
	for (LCD_c = 0; LCD_c < 8; LCD_c++) {
		LCD_send(charmap[LCD_c], Rs);
	}
}

#endif