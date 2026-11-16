/* hc_acgame.c: amiga/hc_plat.h on GFOS, through ACGame (acgame.library):
 * mode 13h is ACGame's 320 x 200 8-bit screen (AGA, or RTG and GFOpenGfx
 * where there is one; no SDL and no GFOpenGPU), the VGA DAC its palette,
 * the keyboard its raw key table, and the PC joystick its first pad
 * (GFOpenInput, or lowlevel.library's port 2).
 *
 * The game's files are read from its own drawer (PROGDIR:), wherever it is
 * started from.
 *
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <acgame/platform.h>

#include "hc_plat.h"

/* the raycaster and the game's menus keep big tables on the stack */
unsigned long __stack = 128 * 1024;

static BPTR progdir, olddir;
static int have_olddir, screen;

void hc_plat_start(int argc, char **argv)
{
    (void)argc; (void)argv;
    progdir = Lock((CONST_STRPTR)"PROGDIR:", ACCESS_READ);
    if (progdir) {
        olddir = CurrentDir(progdir);
        have_olddir = 1;
    }
}

static void restore_dir(void)
{
    if (have_olddir) {
        CurrentDir(olddir);
        UnLock(progdir);
        have_olddir = 0;
    }
}

int hc_plat_open(void)
{
    acgame_video_config cfg;
    cfg.width = HC_W;
    cfg.height = HC_H;
    cfg.refresh_hz = 70;
    cfg.mode = ACGAME_VIDEO_INDEXED8;
    cfg.display = ACGAME_DISPLAY_AUTO;
    if (acgame_platform_init(&cfg) != 0) {
        printf("Hypercycles: ACGame could not open a 320 x 200 256-colour screen\n");
        return -1;
    }
    acgame_platform_show_pointer(0);
    screen = 1;
    return 0;
}

void hc_plat_close(void)
{
    if (screen) acgame_platform_shutdown();
    screen = 0;
    restore_dir();
}

static void dac_to_rgb(const unsigned char *pal6, acgame_rgb8 *out)
{
    int i;
    for (i = 0; i < 256; i++) {
        const unsigned char *c = pal6 + i * 3;
        out[i].r = (uint8_t)((c[0] << 2) | (c[0] >> 4));
        out[i].g = (uint8_t)((c[1] << 2) | (c[1] >> 4));
        out[i].b = (uint8_t)((c[2] << 2) | (c[2] >> 4));
    }
}

void hc_plat_show(const unsigned char *pixels, const unsigned char *pal6, int frame_changed)
{
    acgame_indexed_surface *s;
    int y;
    if (!screen) return;
    if (!frame_changed) {
        acgame_rgb8 pal[256];
        dac_to_rgb(pal6, pal);
        acgame_platform_set_palette(pal);
        return;
    }
    s = acgame_platform_begin_indexed_frame();
    if (!s) return;
    for (y = 0; y < HC_H && y < (int)s->height; y++)
        memcpy(s->pixels + (size_t)y * s->pitch, pixels + y * HC_W, HC_W);
    dac_to_rgb(pal6, s->palette);
    acgame_platform_present_indexed(s);
}

void hc_plat_input(unsigned char rawkeys[128], struct hc_pad_in *pad)
{
    acgame_input_state st;
    const uint8_t *k;
    memset(pad, 0, sizeof(*pad));
    if (!screen) { memset(rawkeys, 0, 128); return; }
    acgame_platform_poll(&st);
    k = acgame_platform_rawkeys();
    if (k) memcpy(rawkeys, k, 128);
    else memset(rawkeys, 0, 128);
    if (st.pad.connected) {
        pad->connected = 1;
        pad->x = st.pad.axes[ACGAME_PAD_LEFTX];
        pad->y = st.pad.axes[ACGAME_PAD_LEFTY];
        if (st.pad.buttons & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_LEFT)) pad->x = -32768;
        if (st.pad.buttons & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_RIGHT)) pad->x = 32767;
        if (st.pad.buttons & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_UP)) pad->y = -32768;
        if (st.pad.buttons & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_DOWN)) pad->y = 32767;
        pad->button1 = (st.pad.buttons & ACGAME_PAD_BIT(ACGAME_PAD_A)) != 0;
        pad->button2 = (st.pad.buttons & ACGAME_PAD_BIT(ACGAME_PAD_B)) != 0;
    }
}

unsigned long hc_plat_ms(void)
{
    return acgame_platform_ticks_ms();
}

void hc_plat_idle(unsigned long left_ms)
{
    /* a whole 50 Hz tick or more to wait: give it back to the system */
    if (left_ms >= 20) Delay(1);
}
