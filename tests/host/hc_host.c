/* hc_host.c: amiga/hc_plat.h on a PC, for the host tests: the screen is a
 * frame in memory, time is virtual (a wait takes no time at all, a busy loop
 * moves the clock on by 1 ms every 64 reads), and keys come from a script.
 *
 *   HC_DATA   the game's data drawer (it becomes the current directory)
 *   HC_KEYS   "MS:RAW:1,MS:RAW:0,...": Amiga raw key RAW goes down (1) or
 *             up (0) at virtual time MS
 *   HC_PAD    "MS:X:Y:B1" the same for the pad (connected from the start)
 *   HC_SHOTS  "MS,MS,...": the frame shown at that time is written to
 *             HC_OUT/shot-MS.ppm
 *   HC_END    the virtual time the run ends (exit 0), after writing
 *             HC_OUT/summary.txt: how many frames were shown, the colours
 *             in the last one, and whether the screen was ever opened
 *
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#include "hc_plat.h"
#include <unistd.h>

static unsigned long vt, reads;
static unsigned char frame[HC_W * HC_H], pal[768];
static int opened, shows, closed;
static const char *out_dir = ".";

static unsigned long env_ul(const char *name, unsigned long dflt)
{
    const char *v = getenv(name);
    return v && *v ? strtoul(v, 0, 10) : dflt;
}

static void write_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", HC_W, HC_H);
    for (i = 0; i < HC_W * HC_H; i++) {
        const unsigned char *c = pal + frame[i] * 3;
        fputc((c[0] << 2) | (c[0] >> 4), f);
        fputc((c[1] << 2) | (c[1] >> 4), f);
        fputc((c[2] << 2) | (c[2] >> 4), f);
    }
    fclose(f);
}

static int colours_used(void)
{
    static unsigned char seen[256];
    int i, n = 0;
    memset(seen, 0, sizeof(seen));
    for (i = 0; i < HC_W * HC_H; i++) {
        if (!seen[frame[i]]) { seen[frame[i]] = 1; n++; }
    }
    return n;
}

static void finish(void)
{
    char path[512];
    FILE *f;
    snprintf(path, sizeof(path), "%s/summary.txt", out_dir);
    f = fopen(path, "w");
    if (f) {
        fprintf(f, "opened %d\nshows %d\ncolours %d\nvt %lu\n", opened, shows, colours_used(), vt);
        fclose(f);
    }
    snprintf(path, sizeof(path), "%s/last.ppm", out_dir);
    write_ppm(path);
}

static void check_time(void)
{
    static unsigned long end;
    const char *shots = getenv("HC_SHOTS");
    if (!end) end = env_ul("HC_END", 60000);
    if (shots) {
        static unsigned long done_upto;
        const char *p = shots;
        while (*p) {
            unsigned long t = strtoul(p, (char **)&p, 10);
            if (t > done_upto && t <= vt) {
                char path[512];
                snprintf(path, sizeof(path), "%s/shot-%lu.ppm", out_dir, t);
                write_ppm(path);
                done_upto = t;
            }
            if (*p == ',') p++; else break;
        }
    }
    if (vt >= end) {
        finish();
        fflush(stdout);
        _exit(0);
    }
}

void hc_plat_start(int argc, char **argv)
{
    const char *d = getenv("HC_DATA");
    (void)argc; (void)argv;
    if (getenv("HC_OUT")) out_dir = getenv("HC_OUT");
    if (d && chdir(d) != 0) { perror(d); exit(2); }
}

int hc_plat_open(void) { opened++; return 0; }
void hc_plat_close(void) { closed++; }

void hc_plat_show(const unsigned char *pixels, const unsigned char *pal6, int frame_changed)
{
    if (frame_changed) memcpy(frame, pixels, sizeof(frame));
    memcpy(pal, pal6, sizeof(pal));
    shows++;
}

static void apply_script(const char *s, unsigned char rawkeys[128], struct hc_pad_in *pad, int is_pad)
{
    const char *p = s;
    while (p && *p) {
        unsigned long t = strtoul(p, (char **)&p, 10);
        long a = 0, b = 0, c = 0;
        if (*p == ':') a = strtol(p + 1, (char **)&p, 10);
        if (*p == ':') b = strtol(p + 1, (char **)&p, 10);
        if (is_pad && *p == ':') c = strtol(p + 1, (char **)&p, 10);
        if (t <= vt) {
            if (is_pad) { pad->x = (int)a; pad->y = (int)b; pad->button1 = (int)c; }
            else if (a >= 0 && a < 128) rawkeys[a] = (unsigned char)b;
        }
        while (*p && *p != ',') p++;
        if (*p == ',') p++;
    }
}

void hc_plat_input(unsigned char rawkeys[128], struct hc_pad_in *pad)
{
    memset(rawkeys, 0, 128);
    memset(pad, 0, sizeof(*pad));
    pad->connected = getenv("HC_PAD") != 0;
    apply_script(getenv("HC_KEYS"), rawkeys, pad, 0);
    apply_script(getenv("HC_PAD"), rawkeys, pad, 1);
}

unsigned long hc_plat_ms(void)
{
    if ((++reads & 63) == 0) vt++;
    check_time();
    return vt;
}

void hc_plat_idle(unsigned long left_ms)
{
    vt += left_ms ? left_ms : 1;
    check_time();
}
