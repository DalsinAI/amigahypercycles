Hypercycles 0.99 for GFOS
=========================

Hypercycles (1995, Aclypse Corporation, by Bob Hays): ride a light cycle
through thirty levels of the Grid, collect the gate keys, fight the other
riders, tanks, stalkers and carriers, and face Dr. Devaro at the end. Its
author released the source and data under the GPL (version 3).

Keys
----
  Cursor keys      steer, speed up and slow down; up and down in menus
  Return           choose in the menus; W picks the weapon
  Ctrl             fire the weapon, Alt the gun
  Space            the wall projector on and off
  1 to 8, R        cycle options, the radar
  P, O             pause, go on
  Esc              the menu
The game's own "Read Me!" and "How to play" pages are in its menu.
A pad (GFOpenInput, or a joystick in port 2) is the PC's joystick: choose
it in the game's Options, which calibrate it as they did on a PC.

What it needs
-------------
GFOS (AmigaOS 3.2 with GFOpenUp) on a 68040 or 68060; built soft float,
no FPU needed. The game draws on ACGame's 320 x 200 256-colour screen: on
RTG (and GFOpenGfx where it is installed) or AGA. It needs no SDL and no
GFOpenGPU. It ships acgame.library in the drawer's Libs/.

Sound: not yet. The released source has no digital effects (the *.RAW
files were in the shareware archives' HYPER1.ADT/HYPER2.ADT, which are not
in it), and the music is AdLib (OPL2 FM) songs; both wait for an FM synth
on ACGame's audio. The game runs with sound and music off.

The game keeps its settings (HYPER.CFG) and saved games (HYPER.SAV) in its
drawer.

Installing
----------
From the archive: double-click Install-Hypercycles; it asks where the
drawer goes (Work:Games by default). With GFOpenUp:

  GFOpenUpTool INSTALL Parts/Hypercycles

puts it in SYS:Games/Hypercycles.

How it was ported
-----------------
Hypercycles is a DOS4GW program: it draws into the VGA's memory at
0xA0000, sets the palette through the VGA's ports, hooks the keyboard
interrupt and reads scan codes from port 0x60, reads the BIOS clock at
0x46C and times a PC joystick through port 0x201. The port keeps the
game's code as it was and gives it that machine (amiga/hc_dos.c):

  - mode 13h is a 320 x 200 buffer; it and the 256-entry palette are shown
    through ACGame (acgame.library) when they change, at most 50 times a
    second;
  - each Amiga key pressed or let go reaches the game's own keyboard
    handler as the PC's scan code (amiga/hc_keys.c);
  - the BIOS clock ticks 18.2 times a second from ACGame's milliseconds;
  - the game port's one-shots are timed from the pad's stick, its buttons
    are the pad's A and B;
  - FIXPOINT.ASM is C (amiga/hc_fixpoint.c), bit for bit;
  - the data files' little-endian ints (LEVEL.DEF, OBJECT.DEF) are turned
    into the 68k's own (amiga/hc_files.c).

amiga/patches/0001 changes the game's own code only where it has to: the
data files are read loose from the drawer (not from .ADT archives), no
HYPER.CFG means the defaults, the map reader skips DOS's CRs, a byte
variable the game stored an int in is an int (on a big-endian 68k the int
overwrote its neighbours), a wall with no picture reads an empty texture
(DOS read memory at 0), the speed test's textures are freed once (the game
freed some twice), and pictures have a little room past their end for the
sprite grabs that read there.

Designed, tested and delivered by The AmiCradle Team.
