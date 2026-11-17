/* hc_plat.h: what amiga/hc_dos.c needs from the machine. Two backends:
 * amiga/hc_acgame.c (GFOS: ACGame's 8-bit screen, keys and pads) and
 * tests/host/hc_host.c (a PC: a frame in memory and scripted keys).
 *
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#ifndef HC_PLAT_H
#define HC_PLAT_H

#include "hc_dos.h"

#define HC_W 320
#define HC_H 200

/* Before the game starts: the data drawer becomes the current directory. */
void hc_plat_start(int argc, char **argv);
/* The 320 x 200 screen (mode 13h). 0 when it is open. */
int hc_plat_open(void);
void hc_plat_close(void);
/* Show the frame: pixels (320 x 200) with pal6 (256 VGA DAC entries, 6 bits
 * each). frame_changed is 0 when only the palette changed. */
void hc_plat_show(const unsigned char *pixels, const unsigned char *pal6, int frame_changed);
/* Every key by its Amiga raw key code (1 held, 0 up), and the first pad. */
void hc_plat_input(unsigned char rawkeys[128], struct hc_pad_in *pad);
/* Milliseconds from any fixed point. */
unsigned long hc_plat_ms(void);
/* Nothing to do for left_ms milliseconds: give the time back if it can. */
void hc_plat_idle(unsigned long left_ms);

/* amiga/hc_keys.c: Amiga raw key code to the PC's scan code (set 1), 0 for
 * a key the PC has no code for. */
unsigned char hc_rawkey_to_scancode(unsigned int rawkey);
/* The ASCII a PC scan code types (shift held or not), 0 for none. */
int hc_scancode_ascii(unsigned char scancode, int shift);

#endif
