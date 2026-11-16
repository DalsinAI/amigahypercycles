/* unit_test.c: the port's own pieces on a PC, one check at a time:
 *  - hc_fixpoint.c gives FIXPOINT.ASM's bits;
 *  - hc_keys.c: Amiga raw keys to the PC scan codes the game reads, and
 *    the ASCII they type;
 *  - hc_files.c: LEVEL.DEF and OBJECT.DEF records (little-endian ints)
 *    read as the machine's own, on the real files when HC_DATA names them;
 *  - hc_dos.c: the VGA DAC ports, the keyboard interrupt, getch, the game
 *    port with no pad, the BIOS clock.
 * Linked with hc_dos.c (whose main calls hc_game_main: these tests) and
 * tests/host/hc_host.c.
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#include "hc_plat.h"
#include "hc_files.h"

int fixmul(int, int);
int fixmul2(int, int);
int fixdiv(int, int);
int shf1(int);
int qrshift(int, int);

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_fixpoint(void)
{
    CHECK(fixmul(3 << 16, 2 << 16) == 6 << 16);
    CHECK(fixmul(-65536, 32768) == -32768);
    CHECK(fixmul(0x7fffffff, 2) == 0xffff);              /* bits 16..47 of the product */
    CHECK(fixmul2(1280, 1792) == 8960);
    CHECK(fixmul2(-256, 256) == -256);
    CHECK(fixdiv(3 << 16, 2 << 16) == 98304);
    CHECK(fixdiv(-(3 << 16), 2 << 16) == -98304);
    CHECK(fixdiv(0, 5) == 0);
    CHECK(fixdiv(5, 0) == 0);
    CHECK(shf1((int)0xffc00000u) == 0x3ff);
    CHECK(shf1(1 << 22) == 1);
    CHECK(qrshift((int)0x80000000u, 31) == 1);
    CHECK(qrshift(256, 4) == 16);
}

static void test_keys(void)
{
    CHECK(hc_rawkey_to_scancode(0x4c) == 72);   /* up */
    CHECK(hc_rawkey_to_scancode(0x4d) == 80);   /* down */
    CHECK(hc_rawkey_to_scancode(0x4e) == 77);   /* right */
    CHECK(hc_rawkey_to_scancode(0x4f) == 75);   /* left */
    CHECK(hc_rawkey_to_scancode(0x45) == 1);    /* Esc */
    CHECK(hc_rawkey_to_scancode(0x44) == 28);   /* Return */
    CHECK(hc_rawkey_to_scancode(0x43) == 28);   /* Enter */
    CHECK(hc_rawkey_to_scancode(0x40) == 57);   /* space */
    CHECK(hc_rawkey_to_scancode(0x10) == 16);   /* q */
    CHECK(hc_rawkey_to_scancode(0x20) == 30);   /* a */
    CHECK(hc_rawkey_to_scancode(0x31) == 44);   /* z */
    CHECK(hc_rawkey_to_scancode(0x01) == 2);    /* 1 */
    CHECK(hc_rawkey_to_scancode(0x0a) == 11);   /* 0 */
    CHECK(hc_rawkey_to_scancode(0x50) == 59);   /* F1 */
    CHECK(hc_rawkey_to_scancode(0x59) == 68);   /* F10 */
    CHECK(hc_rawkey_to_scancode(0x63) == 29);   /* Ctrl */
    CHECK(hc_rawkey_to_scancode(0x64) == 56);   /* Alt */
    CHECK(hc_rawkey_to_scancode(0x60) == 42);   /* shift */
    CHECK(hc_rawkey_to_scancode(0x66) == 0);    /* Amiga key: none */
    CHECK(hc_rawkey_to_scancode(0x7f) == 0);
    CHECK(hc_scancode_ascii(16, 0) == 'q');
    CHECK(hc_scancode_ascii(16, 1) == 'Q');
    CHECK(hc_scancode_ascii(2, 0) == '1');
    CHECK(hc_scancode_ascii(2, 1) == '!');
    CHECK(hc_scancode_ascii(28, 0) == '\r');
    CHECK(hc_scancode_ascii(1, 0) == 27);
    CHECK(hc_scancode_ascii(57, 0) == ' ');
    CHECK(hc_scancode_ascii(14, 0) == '\b');
    CHECK(hc_scancode_ascii(50, 0) == 'm');
    CHECK(hc_scancode_ascii(53, 0) == '/');
    CHECK(hc_scancode_ascii(29, 0) == 0);       /* Ctrl types nothing */
    CHECK(hc_scancode_ascii(58, 0) == 0);       /* past the table */
    {
        /* every letter's scan code types its letter */
        static const unsigned char raw_letters[26] = {
            0x20, 0x35, 0x33, 0x22, 0x12, 0x23, 0x24, 0x25, 0x17, 0x26, 0x27, 0x28, 0x37,
            0x36, 0x18, 0x19, 0x10, 0x13, 0x21, 0x14, 0x16, 0x34, 0x11, 0x32, 0x15, 0x31 };
        int i, ok = 1;
        for (i = 0; i < 26; i++)
            if (hc_scancode_ascii(hc_rawkey_to_scancode(raw_letters[i]), 0) != 'a' + i) ok = 0;
        CHECK(ok);
    }
}

static int rd(const unsigned char *p) { return *(const int *)(const void *)p; }

static void test_files(void)
{
    unsigned char rec[HC_OBJECT_SIZE];
    int i;
    for (i = 0; i < 22; i++) {
        rec[i * 4] = (unsigned char)(i + 1);
        rec[i * 4 + 1] = 0x02;
        rec[i * 4 + 2] = 0;
        rec[i * 4 + 3] = (unsigned char)(i == 21 ? 0x80 : 0);
    }
    hc_object_le(rec);
    CHECK(rd(rec) == 0x0201);
    CHECK(rd(rec + 4 * 20) == 0x0215);
    CHECK((unsigned int)rd(rec + 4 * 21) == 0x80000216u);

    if (getenv("HC_TEST_DATA")) {
        char path[512];
        unsigned char lv[HC_LEVEL_DEF_SIZE];
        FILE *f;
        snprintf(path, sizeof(path), "%s/LEVEL.DEF", getenv("HC_TEST_DATA"));
        f = fopen(path, "rb");
        CHECK(f != 0);
        if (f) {
            CHECK(fread(lv, sizeof(lv), 1, f) == 1);
            fclose(f);
            hc_level_def_le(lv);
            CHECK(lv[0] == 1);                                  /* level_type */
            CHECK(strcmp((char *)lv + 1, "WELCOME TO THE GRID") == 0);
            CHECK(strcmp((char *)lv + 27, "LEVEL1.DAT") == 0);  /* the wall map */
            CHECK(rd(lv + 66) == 2080 && rd(lv + 70) == 2080);   /* x, y */
            CHECK(rd(lv + 82) == 60);                           /* exit_ymaze_sq */
            CHECK(strcmp((char *)lv + 99, "RMH2001.MDI") == 0);
            CHECK(rd(lv + 228) == 14 && rd(lv + 232) == 3);      /* skycolor, rings_req */
        }
        snprintf(path, sizeof(path), "%s/OBJECT.DEF", getenv("HC_TEST_DATA"));
        f = fopen(path, "rb");
        CHECK(f != 0);
        if (f) {
            long n;
            fseek(f, 0, SEEK_END);
            n = ftell(f);
            CHECK(n % HC_OBJECT_SIZE == 0);
            fseek(f, 0, SEEK_SET);
            CHECK(fread(rec, sizeof(rec), 1, f) == 1);
            fclose(f);
            hc_object_le(rec);
            CHECK(rd(rec) >= 1 && rd(rec) <= 30);               /* level_num */
            CHECK(rd(rec + 12) >= 0 && rd(rec + 12) < 64 * 64); /* x in the world */
        }
    }
}

static int isr_calls;
static unsigned int isr_seen;
static void test_isr(void)
{
    _enable();          /* the game's handler lets interrupts in first */
    isr_seen = inp(0x60);
    isr_calls++;
}

static void test_dos(void)
{
    const unsigned char *pal = hc_dos_palette();
    unsigned int t0;

    outp(0x3c8, 5);
    outp(0x3c9, 63); outp(0x3c9, 32); outp(0x3c9, 1);
    outp(0x3c9, 0x7f);                          /* the next entry, 6 bits kept */
    CHECK(pal[15] == 63 && pal[16] == 32 && pal[17] == 1);
    CHECK(pal[18] == 0x3f);
    outp(0x3c7, 5);
    CHECK(inp(0x3c9) == 63 && inp(0x3c9) == 32 && inp(0x3c9) == 1);
    outp(0x3c8, 255);
    outp(0x3c9, 1); outp(0x3c9, 2); outp(0x3c9, 3); outp(0x3c9, 4);
    CHECK(pal[765] == 1 && pal[767] == 3 && pal[0] == 4);   /* wraps to entry 0 */

    CHECK(_dos_getvect(9) == 0);
    _dos_setvect(9, test_isr);
    CHECK(_dos_getvect(9) == test_isr);
    CHECK(hc_dos_key_event(30) == 1);
    CHECK(isr_calls == 1 && isr_seen == 30);
    CHECK(hc_dos_key_event(30 | 0x80) == 1);
    CHECK(isr_calls == 2 && isr_seen == (30 | 0x80));
    CHECK(kbhit());
    CHECK(getch() == 'a');                      /* the make code typed it, the break didn't */
    CHECK(!kbhit());
    hc_dos_key_event(28);
    hc_dos_key_event(28 | 0x80);
    CHECK(getch() == '\r');
    _dos_setvect(9, 0);
    CHECK(hc_dos_key_event(57) == 0);
    (void)getch();

    /* no pad: the one-shots never fall, the buttons stay up */
    outp(0x201, 255);
    CHECK((inp(0x201) & 0x33) == 0x33);

    t0 = hc_bios_clock;
    delay(1000);
    CHECK(hc_bios_clock - t0 >= 17 && hc_bios_clock - t0 <= 19);
    CHECK(hc_bios_equip == 0);
}

int hc_game_main(void)
{
    test_fixpoint();
    test_keys();
    test_files();
    test_dos();
    printf("unit tests: %d of %d checks passed\n", checks - fails, checks);
    exit(fails ? 1 : 0);
    return 0;
}
