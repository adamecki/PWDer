#pragma once

#include <Arduino.h>
#include <KeyboardHIDCodes.h>

struct keymap {
    uint8_t usage_id;
    bool shift;
};

const keymap ascii_to_hid[128] = {
    {0, false},            // 00 NUL
    {0, false},            // 01 SOH
    {0, false},            // 02 STX
    {0, false},            // 03 ETX
    {0, false},            // 04 EOT
    {0, false},            // 05 ENQ
    {0, false},            // 06 ACK
    {0, false},            // 07 BEL
    {KEY_BACKSPACE, false},// 08 BS  (Backspace)
    {KEY_TAB, false},      // 09 TAB (Tab)
    {KEY_ENTER, false},    // 10 LF  (Line Feed / Enter)
    {0, false},            // 11 VT
    {0, false},            // 12 FF
    {KEY_ENTER, false},    // 13 CR  (Carriage Return)
    {0, false},            // 14 SO
    {0, false},            // 15 SI
    {0, false},            // 16 DLE
    {0, false},            // 17 DC1
    {0, false},            // 18 DC2
    {0, false},            // 19 DC3
    {0, false},            // 20 DC4
    {0, false},            // 21 NAK
    {0, false},            // 22 SYN
    {0, false},            // 23 ETB
    {0, false},            // 24 CAN
    {0, false},            // 25 EM
    {0, false},            // 26 SUB
    {KEY_ESC, false},      // 27 ESC (Escape)
    {0, false},            // 28 FS
    {0, false},            // 29 GS
    {0, false},            // 30 RS
    {0, false},            // 31 US
    
    // Printable Characters 32-64
    {KEY_SPACE, false},        // 32 Space
    {KEY_1, true},             // 33 !
    {KEY_APOSTROPHE, true},    // 34 "
    {KEY_3, true},             // 35 #
    {KEY_4, true},             // 36 $
    {KEY_5, true},             // 37 %
    {KEY_7, true},             // 38 &
    {KEY_APOSTROPHE, false},   // 39 '
    {KEY_9, true},             // 40 (
    {KEY_0, true},             // 41 )
    {KEY_8, true},             // 42 *
    {KEY_EQUAL, true},         // 43 +
    {KEY_COMMA, false},        // 44 ,
    {KEY_MINUS, false},        // 45 -
    {KEY_DOT, false},          // 46 .
    {KEY_SLASH, false},        // 47 /
    {KEY_0, false},            // 48 0
    {KEY_1, false},            // 49 1
    {KEY_2, false},            // 50 2
    {KEY_3, false},            // 51 3
    {KEY_4, false},            // 52 4
    {KEY_5, false},            // 53 5
    {KEY_6, false},            // 54 6
    {KEY_7, false},            // 55 7
    {KEY_8, false},            // 56 8
    {KEY_9, false},            // 57 9
    {KEY_SEMICOLON, true},     // 58 :
    {KEY_SEMICOLON, false},    // 59 ;
    {KEY_COMMA, true},         // 60 <
    {KEY_EQUAL, false},        // 61 =
    {KEY_DOT, true},           // 62 >
    {KEY_SLASH, true},         // 63 ?
    {KEY_2, true},             // 64 @

    // Uppercase Letters 65-90
    {KEY_A, true}, {KEY_B, true}, {KEY_C, true}, {KEY_D, true},
    {KEY_E, true}, {KEY_F, true}, {KEY_G, true}, {KEY_H, true},
    {KEY_I, true}, {KEY_J, true}, {KEY_K, true}, {KEY_L, true},
    {KEY_M, true}, {KEY_N, true}, {KEY_O, true}, {KEY_P, true},
    {KEY_Q, true}, {KEY_R, true}, {KEY_S, true}, {KEY_T, true},
    {KEY_U, true}, {KEY_V, true}, {KEY_W, true}, {KEY_X, true},
    {KEY_Y, true}, {KEY_Z, true},

    // Symbols 91-96
    {KEY_LEFTBRACE, false},    // 91 [
    {KEY_BACKSLASH, false},    // 92 \ //
    {KEY_RIGHTBRACE, false},   // 93 ]
    {KEY_6, true},             // 94 ^
    {KEY_MINUS, true},         // 95 _
    {KEY_GRAVE, false},        // 96 `

    // Lowercase Letters 97-122
    {KEY_A, false}, {KEY_B, false}, {KEY_C, false}, {KEY_D, false},
    {KEY_E, false}, {KEY_F, false}, {KEY_G, false}, {KEY_H, false},
    {KEY_I, false}, {KEY_J, false}, {KEY_K, false}, {KEY_L, false},
    {KEY_M, false}, {KEY_N, false}, {KEY_O, false}, {KEY_P, false},
    {KEY_Q, false}, {KEY_R, false}, {KEY_S, false}, {KEY_T, false},
    {KEY_U, false}, {KEY_V, false}, {KEY_W, false}, {KEY_X, false},
    {KEY_Y, false}, {KEY_Z, false},

    // Symbols 123-127
    {KEY_LEFTBRACE, true},     // 123 {
    {KEY_BACKSLASH, true},     // 124 |
    {KEY_RIGHTBRACE, true},    // 125 }
    {KEY_GRAVE, true},         // 126 ~
    {KEY_DELETE, false}        // 127 DEL
};
