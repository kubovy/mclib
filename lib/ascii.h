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

//                          HEX     DEC CATEGORY   DESCRIPTION
#define NUL                 0x00 // 000 SYSTEM     NULL
#define SOH                 0x01 // 001 SYSTEM     Start of heading
#define STX                 0x02 // 002 SYSTEM     Start of text
#define ETX                 0x03 // 003 SYSTEM     End of text
#define EOT                 0x04 // 004 SYSTEM     End of transmission
#define ENQ                 0x05 // 005 SYSTEM     Enquiry
#define ACK                 0x06 // 006 SYSTEM     Acknowledge
#define BEL                 0x07 // 007 SYSTEM     Bell
#define BS                  0x08 // 008 SYSTEM     Backspace
#define TAB                 0x09 // 009 SYSTEM     Horizontal tab
#define LF                  0x0A // 010 SYSTEM     NL line feed, new line
#define NL                  0x0A // 010 SYSTEM     NL line feed, new line
#define VT                  0x0B // 011 SYSTEM     Vertical tab
#define FF                  0x0C // 012 SYSTEM     NP form feed, new page
#define CR                  0x0D // 013 SYSTEM     Carriage return
#define SO                  0x0E // 014 SYSTEM     Shift out
#define SI                  0x0F // 015 SYSTEM     Shift in

#define DLE                 0x10 // 016 SYSTEM     Data link escape
#define DC1                 0x11 // 017 SYSTEM     Device control 1
#define DC2                 0x12 // 018 SYSTEM     Device control 2
#define DC3                 0x13 // 019 SYSTEM     Device control 3
#define DC4                 0x14 // 020 SYSTEM     Device control 4
#define NAK                 0x15 // 021 SYSTEM     Negative acknowledge
#define SYN                 0x16 // 022 SYSTEM     Synchronous idle
#define ETB                 0x17 // 023 SYSTEM     End of transmission block
#define CAN                 0x18 // 024 SYSTEM     Cancel
#define EM                  0x19 // 025 SYSTEM     End of medium
#define SUB                 0x1A // 026 SYSTEM     Substitute
#define ESC                 0x1B // 027 SYSTEM     Escape
#define FS                  0x1C // 028 SYSTEM     File separator
#define GS                  0x1D // 029 SYSTEM     Group separator
#define RS                  0x1E // 030 SYSTEM     Record separator
#define US                  0x1F // 031 SYSTEM     Unit separator

#define SPACE               0x20 // 032 SYSTEM     Space
#define EXLAMATION_MARK     0x21 // 033 PUNCTATION !
#define QUOTES              0x22 // 034 PUNCTATION "
#define HASH                0x23 // 035 PUNCTATION #
#define DOLLAR              0x24 // 036 SYMBOL     $
#define PERCENT             0x25 // 037 PUNCTATION %
#define AND                 0x26 // 038 PUNCTATION &
#define APOSTROPHE          0x27 // 039 PUNCTATION '
#define PARENTHES_OPEN      0x28 // 040 PUNCTATION (
#define PARENTHES_CLOSE     0x29 // 041 PUNCTATION )
#define STAR                0x2A // 042 PUNCTATION *
#define PLUS                0x2B // 043 SYMBOL     +
#define COMMA               0x2C // 044 PUNCTATION ,
#define MINUS               0x2D // 045 PUNCTATION -
#define PERIOD              0x2E // 046 PUNCTATION .
#define SLASH               0x2F // 047 PUNCTATION /

#define NUM_0               0x30 // 048 NUMBER     0
#define NUM_1               0x31 // 049 NUMBER     1
#define NUM_2               0x32 // 050 NUMBER     2
#define NUM_3               0x33 // 051 NUMBER     3
#define NUM_4               0x34 // 052 NUMBER     4
#define NUM_5               0x35 // 053 NUMBER     5
#define NUM_6               0x36 // 054 NUMBER     6
#define NUM_7               0x37 // 055 NUMBER     7
#define NUM_8               0x38 // 056 NUMBER     8
#define NUM_9               0x39 // 057 NUMBER     9
#define COLON               0x3A // 058 PUNCTATION :
#define SEMICOLON           0x3B // 059 PUNCTATION ;
#define LESS_THAN           0x3C // 060 SYMBOL     <
#define EQUAL               0x3D // 061 SYMBOL     =
#define GREATER_THAN        0x3E // 062 SYMBOL     >
#define QUESTION_MARK       0x3F // 063 PUNCTATION ?

#define AT_SIGN             0x40 // 064 PUNCTATION @
#define LETTER_A            0x41 // 065 LETTER     A
#define LETTER_B            0x42 // 066 LETTER     B
#define LETTER_C            0x43 // 067 LETTER     C
#define LETTER_D            0x44 // 068 LETTER     D
#define LETTER_E            0x45 // 069 LETTER     E
#define LETTER_F            0x46 // 070 LETTER     F
#define LETTER_G            0x47 // 071 LETTER     G
#define LETTER_H            0x48 // 072 LETTER     H
#define LETTER_I            0x49 // 073 LETTER     I
#define LETTER_J            0x4A // 074 LETTER     J
#define LETTER_K            0x4B // 075 LETTER     K
#define LETTER_L            0x4C // 076 LETTER     L
#define LETTER_M            0x4D // 077 LETTER     M
#define LETTER_N            0x4E // 078 LETTER     N
#define LETTER_O            0x4F // 079 LETTER     O

#define LETTER_P            0x50 // 080 LETTER     P
#define LETTER_Q            0x51 // 081 LETTER     Q
#define LETTER_R            0x52 // 082 LETTER     R
#define LETTER_S            0x53 // 083 LETTER     S
#define LETTER_T            0x54 // 084 LETTER     T
#define LETTER_U            0x55 // 085 LETTER     U
#define LETTER_V            0x56 // 086 LETTER     V
#define LETTER_W            0x57 // 087 LETTER     W
#define LETTER_X            0x58 // 088 LETTER     X
#define LETTER_Y            0x59 // 089 LETTER     Y
#define LETTER_Z            0x5A // 090 LETTER     Z
#define BRACKET_OPEN        0x5B // 091 PUNCTATION [
#define BACKSLASH           0x5C // 092 PUNCTATION 
#define BRACKET_CLOSE       0x5D // 093 PUNCTATION ]
#define CARRET              0x5E // 094 SYMBOL     ^
#define UNDERSCORE          0x5F // 095 PUNCTATION _

#define GRAVE_ACCENT        0x60 // 096 SYMBOL     `
#define LETTER_a            0x61 // 097 LETTER     a
#define LETTER_b            0x62 // 098 LETTER     b
#define LETTER_c            0x63 // 099 LETTER     c
#define LETTER_d            0x64 // 100 LETTER     d
#define LETTER_e            0x65 // 101 LETTER     e
#define LETTER_f            0x66 // 102 LETTER     f
#define LETTER_g            0x67 // 103 LETTER     g
#define LETTER_h            0x68 // 104 LETTER     h
#define LETTER_i            0x69 // 105 LETTER     i
#define LETTER_j            0x6A // 106 LETTER     j
#define LETTER_k            0x6B // 107 LETTER     k
#define LETTER_l            0x6C // 108 LETTER     l
#define LETTER_m            0x6D // 109 LETTER     m
#define LETTER_n            0x6E // 110 LETTER     n
#define LETTER_o            0x6F // 111 LETTER     o

#define LETTER_p            0x70 // 112 LETTER     p
#define LETTER_q            0x71 // 113 LETTER     q
#define LETTER_r            0x72 // 114 LETTER     r
#define LETTER_s            0x73 // 115 LETTER     s
#define LETTER_t            0x74 // 116 LETTER     t
#define LETTER_u            0x75 // 117 LETTER     u
#define LETTER_v            0x76 // 118 LETTER     v
#define LETTER_w            0x77 // 119 LETTER     w
#define LETTER_x            0x78 // 120 LETTER     x
#define LETTER_y            0x79 // 121 LETTER     y
#define LETTER_z            0x7A // 122 LETTER     z
#define CURLY_BRACKET_OPEN  0x7B // 123 PUNCTATION {
#define PIPE                0x7C // 124 SYMBOL     |
#define CURLY_BRACKET_CLOSE 0x7D // 125 PUNCTATION }
#define TILDE               0x7E // 126 SYMBOL     ~
#define DEL                 0x7F // 127 SYSTEM     DEL

#ifdef	__cplusplus
}
#endif

#endif	/* ASCII_H */

