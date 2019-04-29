/* 
 * File:   lcd.c
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 */
#include "lcd.h"

#ifdef LCD_ADDRESS

void _LCD_clearContent(void) {
    for (uint8_t r = 0; r < LCD_ROWS; r++) {
        for (uint8_t c = 0; c < LCD_COLS; c++) {
            LCD_content[r][c] = ' ';
        }
        LCD_content[r][LCD_COLS] = '\0';
    }
}

void LCD_init(void) {
    LCD_sendCommand(LCD_RETURNHOME | LCD_CLEARDISPLAY); // 0b00000010 | 0b00000001 = 0b00000011 (0x03)
    LCD_sendCommand(LCD_RETURNHOME | LCD_CLEARDISPLAY); // 0b00000010 | 0b00000001 = 0b00000011 (0x03)
    LCD_sendCommand(LCD_RETURNHOME | LCD_CLEARDISPLAY); // 0b00000010 | 0b00000001 = 0b00000011 (0x03)
    LCD_sendCommand(LCD_RETURNHOME); // 0b00000010 (0x02)

    
    LCD_sendCommand(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE); // 0b00100000 | 0b00001000 | 0b00000000 | 0b00000000 = 0b00101000 (0x28)
    LCD_sendCommand(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF); // 0b00001000 | 0b00000100 | 0b00000000 | 0b00000000 = 0b00001100 (0x0C)
    LCD_sendCommand(LCD_CLEARDISPLAY); // 0b00000001 (0x01)
    LCD_sendCommand(LCD_ENTRYMODESET | LCD_ENTRYLEFT); // 0b00000100 | 0b00000010 = 0b00000110 (0x06)

    __delay_ms(200);
    
    LCD_sendCommand(LCD_CLEARDISPLAY); // 0b00000001 (0x01)
    LCD_sendCommand(LCD_RETURNHOME); // 0b00000010 (0x02)
    LCD_setBacklight(false);
}

void LCD_clear(void) {
    LCD_sendCommand(LCD_CLEARDISPLAY);
    LCD_sendCommand(LCD_RETURNHOME);
    __delay_ms(200);
    _LCD_clearContent();   
}

void LCD_reset(void) {
    LCD_init();
    LCD_setBacklight(true);
    for (uint8_t i = 0; i < LCD_ROWS; i++) {
        LCD_displayString(LCD_content[i], i);
    }
}

void LCD_setBacklight(bool on) {
    uint8_t value = on ? LCD_BACKLIGHT : LCD_NOBACKLIGHT;
    if (value != LCD_backlight) {
        LCD_backlight = value;
        I2C_write_byte(LCD_ADDRESS, LCD_backlight);
    }
}

void LCD_sendCommand(uint8_t command) {
    unsigned char nibble_lower, nibble_upper;
    // Select lower nibble by moving it to the upper nibble position
    nibble_lower = (command << 4) & 0xF0;
    // Select upper nibble
    nibble_upper = command & 0xF0;      
 
    uint8_t data[4] = {
        nibble_upper | LCD_backlight | En,
        nibble_upper | LCD_backlight,
        nibble_lower | LCD_backlight | En,
        nibble_lower | LCD_backlight
    };

    I2C_write_data(LCD_ADDRESS, data, 4);
}

void LCD_sendData(uint8_t data) {
    unsigned char nibble_lower, nibble_upper;
    // Select lower nibble by moving it to the upper nibble position
    nibble_lower = (data<<4) & 0xF0;
    // Select upper nibble
    nibble_upper = data & 0xF0;
 
    uint8_t raw[8] = {
        nibble_upper | LCD_backlight | En | Rs,
        nibble_upper | LCD_backlight | Rs,
        nibble_lower | LCD_backlight | En | Rs,
        nibble_lower | LCD_backlight | Rs
    };

    I2C_write_data(LCD_ADDRESS, raw, 4);
}

inline void LCD_selectLine(uint8_t line) {
    switch(line) {
        case 1:
            LCD_sendCommand(LCD_LINE2);
            break;
        case 2:
            LCD_sendCommand(LCD_LINE3);
            break;
        case 3:
            LCD_sendCommand(LCD_LINE4);
            break;
        default:
            LCD_sendCommand(LCD_LINE1);
            break;
    }
}

void LCD_displayString(char *str, uint8_t line) {
    while (*str && line < LCD_ROWS) {
        LCD_selectLine(line);
        uint8_t column, prefix = 0;
        uint16_t i, delay = -1;
        
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
        
        for (i = 0; i < prefix; i++) {
            LCD_content[line][i] = ' ';
            LCD_sendData(' ');
        }
        column = prefix;
        while (*str && *str != '\n' && column < LCD_COLS) {
            LCD_content[line][column] = *str;
            LCD_sendData(*str);
            if (delay == -1) __delay_us(500);
            else for(i = 0; i < delay; i++) __delay_ms(1);
            column++;
            str++;
        }
        // Ignore rest of the line or string.
        if (column >= LCD_COLS) while (*str && *str != '\n') str++;
        
        for (i = column; i < LCD_COLS; i++) {
            LCD_content[line][column] = ' ';
            LCD_sendData(' ');
        }

        if (*str == '\n') str++; // Skip new line character

        line++;
    }
}

void LCD_replaceChar(char c, uint8_t position, uint8_t line) {
    if (line < LCD_ROWS && position < LCD_COLS) {
        LCD_content[line][position] = c;
        LCD_displayString(LCD_content[line], line);
    }
}

void LCD_replaceString(char *str, uint8_t position, uint8_t line) {
    if (line < LCD_ROWS) {
        while (*str && position < LCD_COLS) {
            LCD_content[line][position++] = *str++;
        }
        LCD_displayString(LCD_content[line], line);
    }
}

#endif