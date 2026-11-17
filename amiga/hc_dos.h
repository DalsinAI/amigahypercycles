/* hc_dos.h: the DOS and Watcom C calls Hypercycles makes, on GFOS.
 *
 * Hypercycles (1995) is a DOS4GW program: it writes the VGA's palette and
 * reads its retrace through I/O ports, hooks the keyboard interrupt and
 * reads scan codes from port 0x60, times the joystick through port 0x201,
 * and reads the BIOS clock at 0x46C. Here those ports are a small
 * emulation (amiga/hc_dos.c) over ACGame's 8-bit screen, keys and pads
 * (amiga/hc_acgame.c), so the game's own code stays as it was.
 *
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#ifndef HC_DOS_H
#define HC_DOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

/* the game has its own random(min, max) */
#define random hc_game_random

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_TEXT
#define O_TEXT 0
#endif

/* Watcom's keywords for interrupt handlers and far pointers */
#define _interrupt
#define _far
#ifndef __far
#define __far
#endif
#define far
#define near
#define huge

/* the ports: the VGA DAC (0x3C6-0x3C9) and status (0x3DA), the keyboard
 * (0x60, 0x61), the PIC (0x20) and the game port (0x201) */
unsigned int inp(unsigned int port);
unsigned int outp(unsigned int port, unsigned int value);
#define inpw(p) inp(p)
#define outpw(p, v) outp((p), (v))

/* Interrupts: _enable() is where the emulation runs (the game calls it in
 * every wait loop and once a frame), _disable() does nothing. */
void _enable(void);
void _disable(void);

/* Vector 9 (the keyboard) is the only one taken: each key the player
 * presses or lets go reaches the game's handler as a PC scan code. */
typedef void (*hc_isr)(void);
hc_isr _dos_getvect(unsigned int vector);
void _dos_setvect(unsigned int vector, hc_isr handler);

void delay(unsigned int ms);
void sound(unsigned int hz);
void nosound(void);

int kbhit(void);
int getch(void);
int getche(void);
int putch(int c);
int cprintf(const char *fmt, ...);

/* the program's command line, as Watcom's getcmd() gives it */
char *getcmd(char *buf);

/* int386 for the BIOS calls left in the code: none does anything here */
struct hc_wordregs { unsigned short ax, bx, cx, dx, si, di, cflag; };
struct hc_byteregs { unsigned char al, ah, bl, bh, cl, ch, dl, dh; };
union REGS { struct hc_wordregs w; struct hc_byteregs h; struct hc_wordregs x; };
struct SREGS { unsigned short es, cs, ss, ds, fs, gs; };
int int386(int intno, union REGS *in, union REGS *out);
int int386x(int intno, union REGS *in, union REGS *out, struct SREGS *s);
void segread(struct SREGS *s);

/* Watcom's string calls */
char *strupr(char *s);
#if !defined(__amigaos__) && !defined(AMIGA)
char *strlwr(char *s);   /* libnix has its own */
#endif
char *itoa(int value, char *buf, int radix);

/* what the game points at in place of 0x410, 0x46C and 0xA0000 */
extern unsigned int hc_bios_equip;      /* bit 12: a game port (a pad is there) */
extern unsigned int hc_bios_clock;      /* 18.2 ticks a second, from the start */
extern unsigned char hc_vga[64000 + 256]; /* mode 13h: 320 x 200, one byte a pixel */

/* the emulation's state, for amiga/hc_dos.c and the tests */
struct hc_pad_in {
    int connected;
    int x, y;                           /* -32768 .. 32767 */
    int button1, button2;
};
void hc_dos_pump(void);                 /* what _enable() runs */
const unsigned char *hc_dos_palette(void); /* 768 bytes, the DAC's 6-bit values */
int hc_dos_key_event(unsigned char scancode); /* one scan code to the game's handler */

#endif
