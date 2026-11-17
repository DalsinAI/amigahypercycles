/* hc_files.h: the game's data files hold little-endian 32-bit ints (Watcom on
 * a 386); these turn the records read whole into the machine's own ints.
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#ifndef HC_FILES_H
#define HC_FILES_H

/* LEVEL.DEF: 276 bytes a level, ints at 66..85 and 228..275 */
#define HC_LEVEL_DEF_SIZE 276
/* OBJECT.DEF: 22 ints (88 bytes) an object */
#define HC_OBJECT_SIZE 88

void hc_level_def_le(void *rec);
void hc_object_le(void *rec);
/* n little-endian ints at p, in place */
void hc_ints_le(void *p, int n);
/* 64 x 64, all colour 0: the texture of a wall no tile set gives */
extern unsigned char hc_blank_texture[64 * 64 + 64];

#endif
