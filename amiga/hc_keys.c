/* hc_keys.c: Amiga raw key codes to the PC's scan codes (set 1, what the
 * game's keyboard handler reads from port 0x60), and the ASCII those type.
 * The arrow keys give the keypad's codes (72, 75, 77, 80), as the game's
 * handler expects; Del gives the keypad's 83.
 *
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#include "hc_plat.h"

static const unsigned char raw2pc[0x68] = {
    /* 0x00 ` 1 2 3 4 5 6 7  8  9  0  -  =  \     kp0 */
    41, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 43, 0, 82,
    /* 0x10 q w e r t y u i o p [ ] - kp1 kp2 kp3 */
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 0, 79, 80, 81,
    /* 0x20 a s d f g h j k l ; ' (intl) - kp4 kp5 kp6 */
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 43, 0, 75, 76, 77,
    /* 0x30 (intl <) z x c v b n m , . / - kp. kp7 kp8 kp9 */
    86, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 0, 83, 71, 72, 73,
    /* 0x40 space bs tab kpenter return esc del - - - kp- - up down right left */
    57, 14, 15, 28, 28, 1, 83, 0, 0, 0, 74, 0, 72, 80, 77, 75,
    /* 0x50 F1..F10 kp( kp) kp/ kp* kp+ help */
    59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 0, 0, 53, 55, 78, 0,
    /* 0x60 lshift rshift caps ctrl lalt ralt lamiga ramiga */
    42, 54, 58, 29, 56, 56, 0, 0
};

unsigned char hc_rawkey_to_scancode(unsigned int rawkey)
{
    return rawkey < sizeof(raw2pc) ? raw2pc[rawkey] : 0;
}

/* scan codes 0..57: what they type unshifted and shifted (a US keyboard) */
static const char plain[] =
    "\0\033" "1234567890-=\b\tqwertyuiop[]\r\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
static const char shifted[] =
    "\0\033" "!@#$%^&*()_+\b\tQWERTYUIOP{}\r\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 ";

int hc_scancode_ascii(unsigned char scancode, int shift)
{
    if (scancode >= sizeof(plain) - 1) return 0;
    return (unsigned char)(shift ? shifted : plain)[scancode];
}
