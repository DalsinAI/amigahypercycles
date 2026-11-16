/* hc_sound.c: the Sound Blaster (CTV.C) and AdLib music (MIDIPLAY.C,
 * ADLIB.C) calls the game makes, silent for now: the game's digital
 * effects (*.RAW) are not in the released source, and its music is OPL2
 * FM (*.MDI with the FM_*.DAT instruments), which needs an FM synth
 * feeding ACGame's audio. Until then the game runs with sound and music
 * off (the defaults amiga/patches give HYPER.CFG's mct and sct).
 * GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
 */
#include "hc_dos.h"

int Music_Address, DMA_Channel, io_addr, intr_num;
int volume_flag, digital_speed, MAX_VOLUME = 15, CTV_voice_status;
char musRunning;
unsigned char *digibuf;

void play_vox(char *fname) { (void)fname; }
int CTV_Init(void) { return 0; }
void Volume_OnOff(int on) { (void)on; }
void reset_buf(void) { }
void free_buf(void) { }

int play_song(char *fname) { (void)fname; musRunning = 0; return 0; }
void play_again(void) { }
void Stop_Melo(void) { musRunning = 0; }
void Midi_End(void) { musRunning = 0; }
