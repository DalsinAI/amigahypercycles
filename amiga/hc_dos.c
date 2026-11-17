/* hc_dos.c: the DOS machine Hypercycles was written for, as far as the game
 * uses it, over amiga/hc_plat.h.
 *
 *  - mode 13h: the game's vga_ram is hc_vga (320 x 200 bytes); the VGA DAC
 *    (ports 0x3C6-0x3C9) keeps a 256-entry palette. Whatever is in them is
 *    shown at most every 20 ms (50 a second), when it changed.
 *  - the keyboard interrupt: the game's handler (vector 9) is called once
 *    for every key pressed (its make code) or let go (make code + 0x80),
 *    and reads that code from port 0x60.
 *  - the BIOS clock at 0x46C: hc_bios_clock, 18.2 ticks a second.
 *  - the game port (0x201): a write starts the four one-shots, and each read
 *    counts down the time they stay high: the pad's stick gives the count,
 *    as a PC joystick's resistance did, and its buttons pull bits 4 and 5
 *    low.
 *  - the text screen calls (set_vmode, prints, getns, clrscrn) and getch.
 *
 * All of it runs in hc_dos_pump(), called from _enable(): the game calls
 * that in every wait loop and once a frame, as it did to let interrupts in.
 *
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#include "hc_plat.h"

unsigned int hc_bios_equip;
unsigned int hc_bios_clock;
unsigned char hc_vga[64000 + 256];

static unsigned char dac[768], shown_dac[768];
static unsigned char shown[64000];
static int dac_w, dac_r, dac_mask = 0xff;
static int screen_open, ever_shown;
static unsigned long t_start, t_input, t_show;
static hc_isr key_isr;
static unsigned char port60;
static unsigned char keys_now[128];
static struct hc_pad_in pad;
static unsigned int js_reads, js_x, js_y;
static int pumping;
static char cmdline[256];

/* typed characters, for getch/kbhit */
static unsigned char typed[64];
static int typed_head, typed_tail;

static void type_char(int c)
{
    int next = (typed_tail + 1) % (int)sizeof(typed);
    if (c && next != typed_head) {
        typed[typed_tail] = (unsigned char)c;
        typed_tail = next;
    }
}

const unsigned char *hc_dos_palette(void) { return dac; }

int hc_dos_key_event(unsigned char scancode)
{
    port60 = scancode;
    if (!(scancode & 0x80)) {
        int shift = keys_now[0x60] || keys_now[0x61];
        type_char(hc_scancode_ascii(scancode, shift));
    }
    if (key_isr) { key_isr(); return 1; }
    return 0;
}

static unsigned int stick_count(int axis)
{
    /* -32768 .. 32767 to 8 .. 264 reads: the middle (136) is what a centred
     * PC stick timed on a 486; the game calibrates it anyway */
    return (unsigned int)((axis + 32768) >> 8) + 8;
}

static void show(void)
{
    int frame_changed = memcmp(shown, hc_vga, sizeof(shown)) != 0;
    int pal_changed = memcmp(shown_dac, dac, sizeof(dac)) != 0;
    if (!screen_open || (!frame_changed && !pal_changed && ever_shown)) return;
    hc_plat_show(hc_vga, dac, frame_changed || !ever_shown);
    memcpy(shown, hc_vga, sizeof(shown));
    memcpy(shown_dac, dac, sizeof(dac));
    ever_shown = 1;
}

void hc_dos_pump(void)
{
    unsigned long now;
    if (pumping) return;
    pumping = 1;
    now = hc_plat_ms();
    hc_bios_clock = (unsigned int)((now - t_start) * 182 / 10000);
    if (now - t_input >= 5) {
        unsigned char k[128];
        int i;
        t_input = now;
        hc_plat_input(k, &pad);
        for (i = 0; i < 128; i++) {
            if (k[i] != keys_now[i]) {
                unsigned char sc = hc_rawkey_to_scancode((unsigned int)i);
                keys_now[i] = k[i];
                if (sc) hc_dos_key_event(k[i] ? sc : (unsigned char)(sc | 0x80));
            }
        }
    }
    if (now - t_show >= 20) {
        t_show = now;
        show();
    }
    pumping = 0;
}

void _enable(void) { hc_dos_pump(); }
void _disable(void) { }

unsigned int outp(unsigned int port, unsigned int value)
{
    value &= 0xff;
    switch (port) {
    case 0x3c6: dac_mask = (int)value; break;
    case 0x3c7: dac_r = (int)value * 3; break;
    case 0x3c8: dac_w = (int)value * 3; break;
    case 0x3c9:
        dac[dac_w] = (unsigned char)(value & 63);
        dac_w = (dac_w + 1) % 768;
        break;
    case 0x201:
        js_reads = 0;
        js_x = pad.connected ? stick_count(pad.x) : 0xffffffffu;
        js_y = pad.connected ? stick_count(pad.y) : 0xffffffffu;
        break;
    default: break;
    }
    return value;
}

unsigned int inp(unsigned int port)
{
    switch (port) {
    case 0x60: return port60;
    case 0x3c9: {
        unsigned int v = dac[dac_r];
        dac_r = (dac_r + 1) % 768;
        return v;
    }
    case 0x3da: {
        /* bit 3: the vertical retrace, 1 ms in every 14 (70 a second) */
        hc_dos_pump();
        return (hc_plat_ms() % 14) == 0 ? 0x09 : 0x00;
    }
    case 0x201: {
        unsigned int v = 0xf0;
        if (js_reads < js_x) v |= 1;
        if (js_reads < js_y) v |= 2;
        js_reads++;
        if ((js_reads & 63) == 0) hc_dos_pump();
        if (pad.button1) v &= ~0x10u;
        if (pad.button2) v &= ~0x20u;
        return v;
    }
    default: return 0;
    }
}

hc_isr _dos_getvect(unsigned int vector)
{
    return vector == 9 ? key_isr : (hc_isr)0;
}

void _dos_setvect(unsigned int vector, hc_isr handler)
{
    if (vector == 9) key_isr = handler;
}

void delay(unsigned int ms)
{
    unsigned long end = hc_plat_ms() + ms;
    for (;;) {
        unsigned long now;
        hc_dos_pump();
        now = hc_plat_ms();
        if ((long)(end - now) <= 0) break;
        hc_plat_idle(end - now);
    }
}

void sound(unsigned int hz) { (void)hz; }
void nosound(void) { }

int kbhit(void)
{
    hc_dos_pump();
    return typed_head != typed_tail;
}

int getch(void)
{
    int c;
    while (!kbhit()) hc_plat_idle(5);
    c = typed[typed_head];
    typed_head = (typed_head + 1) % (int)sizeof(typed);
    return c;
}

int putch(int c) { return putchar(c); }
int getche(void) { return putch(getch()); }

int cprintf(const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    return n;
}

/* The game does strcpy(cl, getcmd(cl)): the copy it gets back is ours, so
 * the two never overlap. */
char *getcmd(char *buf)
{
    strcpy(buf, cmdline);
    return cmdline;
}

int int386(int intno, union REGS *in, union REGS *out)
{
    (void)intno;
    if (out != in) *out = *in;
    return 0;
}

int int386x(int intno, union REGS *in, union REGS *out, struct SREGS *s)
{
    (void)s;
    return int386(intno, in, out);
}

void segread(struct SREGS *s) { memset(s, 0, sizeof(*s)); }

/* C_MOD1.C's text screen calls: mode 13h opens the screen; the text modes
 * leave it as it is (what the game prints then goes to the Shell). */
void set_vmode(int vmode)
{
    if (vmode == 0x13) {
        if (!screen_open && hc_plat_open() == 0) {
            screen_open = 1;
            /* the pads are known once the screen is open */
            hc_plat_input(keys_now, &pad);
            hc_bios_equip = pad.connected ? 0x1000u : 0u;
        }
        memset(hc_vga, 0, sizeof(hc_vga));
        ever_shown = 0;
    }
}

void gotoxy(int row, int column) { (void)row; (void)column; }
void clrscrn(void) { }

void prints(int row, int column, unsigned char *txtstr, int attr)
{
    (void)row; (void)column; (void)attr;
    printf("%s\n", (const char *)txtstr);
}

int getns(char *str, int maxchars)
{
    int n = (int)strlen(str);
    for (;;) {
        int c = getch();
        if (c == 27) { str[0] = 0; return 1; }
        if (c == '\r' || c == '\n') { str[n] = 0; return 0; }
        if (c == '\b') { if (n > 0) str[--n] = 0; continue; }
        if (n < maxchars) { str[n++] = (char)c; str[n] = 0; }
    }
}

/* MEMUSE.C: DOS memory reports; GFOS has the memory or the malloc fails */
void memuse(void) { }
int mem_ok(void) { return 1; }
int Get_EMS_Mem(void) { return 0; }

/* the low memory DOS4GW gave the Sound Blaster's DMA: plain memory here */
static unsigned char *dosmem;
unsigned char *D32DosMemAlloc(unsigned int sz)
{
    free(dosmem);
    dosmem = calloc(1, sz);
    return dosmem;
}
unsigned int D32DosMemFree(void) { free(dosmem); dosmem = 0; return 0; }

void Show_Notice(void) { }

static void at_exit(void)
{
    if (screen_open) hc_plat_close();
    screen_open = 0;
}

int hc_game_main(void);

int main(int argc, char **argv)
{
    int i;
    cmdline[0] = 0;
    for (i = 1; i < argc; i++) {
        if (strlen(cmdline) + strlen(argv[i]) + 2 >= sizeof(cmdline)) break;
        if (i > 1) strcat(cmdline, " ");
        strcat(cmdline, argv[i]);
    }
    hc_plat_start(argc, argv);
    t_start = t_input = t_show = hc_plat_ms();
    hc_plat_input(keys_now, &pad);
    hc_bios_equip = pad.connected ? 0x1000u : 0u;
    atexit(at_exit);
    hc_game_main();
    return 0;
}

char *strupr(char *s)
{
    char *p;
    for (p = s; *p; p++) if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 32);
    return s;
}

/* libnix (the Amiga's C library) has its own strlwr */
#if !defined(__amigaos__) && !defined(AMIGA)
char *strlwr(char *s)
{
    char *p;
    for (p = s; *p; p++) if (*p >= 'A' && *p <= 'Z') *p = (char)(*p + 32);
    return s;
}
#endif

/* Watcom's itoa */
char *itoa(int value, char *buf, int radix)
{
    char tmp[34];
    unsigned int v = (value < 0 && radix == 10) ? (unsigned int)-value : (unsigned int)value;
    int i = 0, j = 0;
    if (radix < 2 || radix > 36) radix = 10;
    do { int d = (int)(v % (unsigned int)radix); tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); v /= (unsigned int)radix; } while (v);
    if (value < 0 && radix == 10) buf[j++] = '-';
    while (i) buf[j++] = tmp[--i];
    buf[j] = 0;
    return buf;
}
