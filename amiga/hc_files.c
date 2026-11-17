/* hc_files.c: see hc_files.h.
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#include "hc_files.h"

void hc_ints_le(void *p, int n)
{
    unsigned char *b = (unsigned char *)p;
    int i;
    for (i = 0; i < n; i++, b += 4) {
        unsigned int v = (unsigned int)b[0] | ((unsigned int)b[1] << 8) |
                         ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24);
        unsigned char *d = b;
        /* store it as the machine stores an int */
        union { unsigned int u; unsigned char c[4]; } w;
        w.u = v;
        d[0] = w.c[0]; d[1] = w.c[1]; d[2] = w.c[2]; d[3] = w.c[3];
    }
}

void hc_level_def_le(void *rec)
{
    hc_ints_le((unsigned char *)rec + 66, 5);
    hc_ints_le((unsigned char *)rec + 228, 12);
}

void hc_object_le(void *rec)
{
    hc_ints_le(rec, 22);
}

/* what a wall with no picture loaded reads: see amiga/patches/0001 */
unsigned char hc_blank_texture[64 * 64 + 64];
