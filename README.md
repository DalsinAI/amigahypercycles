# AmigaHypercycles

Hypercycles (1995, Bob Hays / Aclypse Corporation; GPL-3.0) for GFOS
(AmigaOS 3.2 with GFOpenUp) on a 68040 or 68060: the DOS light-cycle game
on ACGame's 320 x 200 256-colour screen (acgame.library), with no SDL and
no GFOpenGPU. Installed as a GFOpenUp part: `SYS:Games/Hypercycles`.

Status: builds and plays on a PC from the same patched sources (see
Tests); the Amiga build is made by the stove, not yet run in a lab. Sound
and music are off (see AMIGA_PORT.md).

AMIGA_PORT.md is the drawer's ReadMe: the keys, what it needs, and how the
port gives the game the DOS machine it was written for.

## Building

    STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out sh package/build-amiga.sh

or in Cradle, stove_build with the os32-gcc16-open6 stove, target
`package`. Upstream (malespiaut/hypercycles) and DalsinAI/acgame are
fetched at the commits `package/hc.lock` pins; nothing of upstream is kept
here. `OUT` gets the archive (`Hypercycles-0.99-gfos1-68040.lha`, the drawer
and Install-Hypercycles) and `Parts/Hypercycles`, installed with

    GFOpenUpTool INSTALL Parts/Hypercycles

## Tests

    sh package/prepare-source.sh /tmp/hc && HC=/tmp/hc sh tests/host/test_hc.sh

on a PC (with AddressSanitizer where the compiler has it):

- `tests/host/unit_test.c`: FIXPOINT.ASM's routines in C bit for bit, the
  Amiga raw key to PC scan code table and the ASCII it types, the
  little-endian records of LEVEL.DEF and OBJECT.DEF (on the real files),
  the VGA DAC ports, the keyboard interrupt, getch, the game port and the
  BIOS clock (81 checks);
- the whole game on `tests/host/hc_host.c` (a frame in memory, virtual
  time, scripted keys): the logo, "presents" and the title menu appear;
  Return through New Game and Rookie reaches level 1, and riding forward
  and turning change the view.

Big-endian, as on the Amiga, with a 68k Linux cross compiler and QEMU:

    HOSTCC="m68k-linux-gnu-gcc -static" HC_RUN=qemu-m68k HC=/tmp/hc \
        HC_SAME_AS=/tmp/hc-pc-frames sh tests/host/test_hc.sh

runs the same tests on an emulated 68040 and checks every frame is byte for
byte the PC's (HC_SAME_AS is a little-endian run's HC_TEST_KEEP).

## Layout

- `amiga/hc_dos.c`, `hc_dos.h`, `include/`: the DOS machine (VGA ports,
  keyboard interrupt, BIOS clock, game port, Watcom calls).
- `amiga/hc_acgame.c`: that machine on ACGame. `tests/host/hc_host.c`: on
  a PC.
- `amiga/hc_keys.c`, `hc_fixpoint.c`, `hc_files.c`, `hc_sound.c`.
- `amiga/patches/0001`: the game's own code, where it had to change.
- `package/`: the lock, the source preparation, the stove build, the part.

Designed, tested and delivered by The AmiCradle Team.
