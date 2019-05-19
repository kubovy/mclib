/* 
 * File:   ascii.h
 * Author: Jan Kubovy &lt;jan@kubovy.eu&gt;
 * 
 * This file acts as ASCII code helper. 
 */
#ifndef ASCII_H
#define	ASCII_H

#ifdef	__cplusplus
extern "C" {
#endif

////                          HEX     DEC CATEGORY   DESCRIPTION
//#define ASCII_NUL                 0x00 // 000 SYSTEM     NULL
//#define ASCII_SOH                 0x01 // 001 SYSTEM     Start of heading
//#define ASCII_STX                 0x02 // 002 SYSTEM     Start of text
//#define ASCII_ETX                 0x03 // 003 SYSTEM     End of text
//#define ASCII_EOT                 0x04 // 004 SYSTEM     End of transmission
//#define ASCII_ENQ                 0x05 // 005 SYSTEM     Enquiry
//#define ASCII_ACK                 0x06 // 006 SYSTEM     Acknowledge
//#define ASCII_BEL                 0x07 // 007 SYSTEM     Bell
//#define ASCII_BS                  0x08 // 008 SYSTEM     Backspace
//#define ASCII_TAB                 0x09 // 009 SYSTEM     Horizontal tab
//#define ASCII_LF                  0x0A // 010 SYSTEM     NL line feed, new line
//#define ASCII_NL                  0x0A // 010 SYSTEM     NL line feed, new line
//#define ASCII_VT                  0x0B // 011 SYSTEM     Vertical tab
//#define ASCII_FF                  0x0C // 012 SYSTEM     NP form feed, new page
//#define ASCII_CR                  0x0D // 013 SYSTEM     Carriage return
//#define ASCII_SO                  0x0E // 014 SYSTEM     Shift out
//#define ASCII_SI                  0x0F // 015 SYSTEM     Shift in
//
//#define ASCII_DLE                 0x10 // 016 SYSTEM     Data link escape
//#define ASCII_DC1                 0x11 // 017 SYSTEM     Device control 1
//#define ASCII_DC2                 0x12 // 018 SYSTEM     Device control 2
//#define ASCII_DC3                 0x13 // 019 SYSTEM     Device control 3
//#define ASCII_DC4                 0x14 // 020 SYSTEM     Device control 4
//#define ASCII_NAK                 0x15 // 021 SYSTEM     Negative acknowledge
//#define ASCII_SYN                 0x16 // 022 SYSTEM     Synchronous idle
//#define ASCII_ETB                 0x17 // 023 SYSTEM     End of transmission block
//#define ASCII_CAN                 0x18 // 024 SYSTEM     Cancel
//#define ASCII_EM                  0x19 // 025 SYSTEM     End of medium
//#define ASCII_SUB                 0x1A // 026 SYSTEM     Substitute
//#define ASCII_ESC                 0x1B // 027 SYSTEM     Escape
//#define ASCII_FS                  0x1C // 028 SYSTEM     File separator
//#define ASCII_GS                  0x1D // 029 SYSTEM     Group separator
//#define ASCII_RS                  0x1E // 030 SYSTEM     Record separator
//#define ASCII_US                  0x1F // 031 SYSTEM     Unit separator
//
//#define ASCII_SPACE               0x20 // 032 SYSTEM     Space
//#define ASCII_EXLAMATION_MARK     0x21 // 033 PUNCTATION !
//#define ASCII_QUOTES              0x22 // 034 PUNCTATION "
//#define ASCII_HASH                0x23 // 035 PUNCTATION #
//#define ASCII_DOLLAR              0x24 // 036 SYMBOL     $
//#define ASCII_PERCENT             0x25 // 037 PUNCTATION %
//#define ASCII_AND                 0x26 // 038 PUNCTATION &
//#define ASCII_APOSTROPHE          0x27 // 039 PUNCTATION '
//#define ASCII_PARENTHES_OPEN      0x28 // 040 PUNCTATION (
//#define ASCII_PARENTHES_CLOSE     0x29 // 041 PUNCTATION )
//#define ASCII_STAR                0x2A // 042 PUNCTATION *
//#define ASCII_PLUS                0x2B // 043 SYMBOL     +
//#define ASCII_COMMA               0x2C // 044 PUNCTATION ,
//#define ASCII_MINUS               0x2D // 045 PUNCTATION -
//#define ASCII_PERIOD              0x2E // 046 PUNCTATION .
//#define ASCII_SLASH               0x2F // 047 PUNCTATION /
//
//#define ASCII_0               0x30 // 048 NUMBER     0
//#define ASCII_1               0x31 // 049 NUMBER     1
//#define ASCII_2               0x32 // 050 NUMBER     2
//#define ASCII_3               0x33 // 051 NUMBER     3
//#define ASCII_4               0x34 // 052 NUMBER     4
//#define ASCII_5               0x35 // 053 NUMBER     5
//#define ASCII_6               0x36 // 054 NUMBER     6
//#define ASCII_7               0x37 // 055 NUMBER     7
//#define ASCII_8               0x38 // 056 NUMBER     8
//#define ASCII_9               0x39 // 057 NUMBER     9
//#define ASCII_COLON               0x3A // 058 PUNCTATION :
//#define ASCII_SEMICOLON           0x3B // 059 PUNCTATION ;
//#define ASCII_LESS_THAN           0x3C // 060 SYMBOL     <
//#define ASCII_EQUAL               0x3D // 061 SYMBOL     =
//#define ASCII_GREATER_THAN        0x3E // 062 SYMBOL     >
//#define ASCII_QUESTION_MARK       0x3F // 063 PUNCTATION ?
//
//#define ASCII_AT_SIGN             0x40 // 064 PUNCTATION @
//#define ASCII_UPPER_A            0x41 // 065 LETTER     A
//#define ASCII_UPPER_B            0x42 // 066 LETTER     B
//#define ASCII_UPPER_C            0x43 // 067 LETTER     C
//#define ASCII_UPPER_D            0x44 // 068 LETTER     D
//#define ASCII_UPPER_E            0x45 // 069 LETTER     E
//#define ASCII_UPPER_F            0x46 // 070 LETTER     F
//#define ASCII_UPPER_G            0x47 // 071 LETTER     G
//#define ASCII_UPPER_H            0x48 // 072 LETTER     H
//#define ASCII_UPPER_I            0x49 // 073 LETTER     I
//#define ASCII_UPPER_J            0x4A // 074 LETTER     J
//#define ASCII_UPPER_K            0x4B // 075 LETTER     K
//#define ASCII_UPPER_L            0x4C // 076 LETTER     L
//#define ASCII_UPPER_M            0x4D // 077 LETTER     M
//#define ASCII_UPPER_N            0x4E // 078 LETTER     N
//#define ASCII_UPPER_O            0x4F // 079 LETTER     O
//
//#define ASCII_UPPER_P            0x50 // 080 LETTER     P
//#define ASCII_UPPER_Q            0x51 // 081 LETTER     Q
//#define ASCII_UPPER_R            0x52 // 082 LETTER     R
//#define ASCII_UPPER_S            0x53 // 083 LETTER     S
//#define ASCII_UPPER_T            0x54 // 084 LETTER     T
//#define ASCII_UPPER_U            0x55 // 085 LETTER     U
//#define ASCII_UPPER_V            0x56 // 086 LETTER     V
//#define ASCII_UPPER_W            0x57 // 087 LETTER     W
//#define ASCII_UPPER_X            0x58 // 088 LETTER     X
//#define ASCII_UPPER_Y            0x59 // 089 LETTER     Y
//#define ASCII_UPPER_Z            0x5A // 090 LETTER     Z
//#define ASCII_BRACKET_OPEN        0x5B // 091 PUNCTATION [
//#define ASCII_BACKSLASH           0x5C // 092 PUNCTATION 
//#define ASCII_BRACKET_CLOSE       0x5D // 093 PUNCTATION ]
//#define ASCII_CARRET              0x5E // 094 SYMBOL     ^
//#define ASCII_UNDERSCORE          0x5F // 095 PUNCTATION _
//
//#define ASCII_GRAVE_ACCENT        0x60 // 096 SYMBOL     `
//#define ASCII_LOWER_A            0x61 // 097 LETTER     a
//#define ASCII_LOWER_B            0x62 // 098 LETTER     b
//#define ASCII_LOWER_C            0x63 // 099 LETTER     c
//#define ASCII_LOWER_D            0x64 // 100 LETTER     d
//#define ASCII_LOWER_E            0x65 // 101 LETTER     e
//#define ASCII_LOWER_F            0x66 // 102 LETTER     f
//#define ASCII_LOWER_G            0x67 // 103 LETTER     g
//#define ASCII_LOWER_H            0x68 // 104 LETTER     h
//#define ASCII_LOWER_I            0x69 // 105 LETTER     i
//#define ASCII_LOWER_J            0x6A // 106 LETTER     j
//#define ASCII_LOWER_K            0x6B // 107 LETTER     k
//#define ASCII_LOWER_L            0x6C // 108 LETTER     l
//#define ASCII_LOWER_M            0x6D // 109 LETTER     m
//#define ASCII_LOWER_N            0x6E // 110 LETTER     n
//#define ASCII_LOWER_O            0x6F // 111 LETTER     o
//
//#define ASCII_LOWER_P            0x70 // 112 LETTER     p
//#define ASCII_LOWER_Q            0x71 // 113 LETTER     q
//#define ASCII_LOWER_R            0x72 // 114 LETTER     r
//#define ASCII_LOWER_S            0x73 // 115 LETTER     s
//#define ASCII_LOWER_T            0x74 // 116 LETTER     t
//#define ASCII_LOWER_U            0x75 // 117 LETTER     u
//#define ASCII_LOWER_V            0x76 // 118 LETTER     v
//#define ASCII_LOWER_W            0x77 // 119 LETTER     w
//#define ASCII_LOWER_X            0x78 // 120 LETTER     x
//#define ASCII_LOWER_Y            0x79 // 121 LETTER     y
//#define ASCII_LOWER_Z            0x7A // 122 LETTER     z
//#define ASCII_CURLY_BRACKET_OPEN  0x7B // 123 PUNCTATION {
//#define ASCII_PIPE                0x7C // 124 SYMBOL     |
//#define ASCII_CURLY_BRACKET_CLOSE 0x7D // 125 PUNCTATION }
//#define ASCII_TILDE               0x7E // 126 SYMBOL     ~
//#define ASCII_DEL                 0x7F // 127 SYSTEM     DEL

#ifdef	__cplusplus
}
#endif

#endif	/* ASCII_H */

