#!/bin/sh
# Host tests: the port's own pieces (tests/host/unit_test.c), then the whole
# game built for this PC from the same patched sources and amiga/hc_dos.c,
# on tests/host/hc_host.c (a frame in memory, virtual time, scripted keys),
# with AddressSanitizer where the compiler has it:
#  - attract: the Aclypse logo, "presents", then the title menu, each a
#    different picture, in many colours;
#  - play: Return through the menu and the difficulty (Rookie), level 1's
#    briefing, then the cycle rides: up held, then left; the frames differ
#    as it moves and turns, and nothing reads or writes out of bounds.
#   HC=/path/to/prepared (package/prepare-source.sh) sh tests/host/test_hc.sh
#   HC_TEST_KEEP=DIR keeps the frames (PPM) there.
# GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
HC=${HC:?HC=prepared source (package/prepare-source.sh DIR)}
CC=${HOSTCC:-cc}
T=$(mktemp -d "${TMPDIR:-/tmp}/hc-host.XXXXXX")
trap 'rm -rf "$T"' EXIT INT TERM

SAN=""
if printf 'int main(void){return 0;}\n' > "$T/s.c" && $CC -fsanitize=address -o "$T/s" "$T/s.c" 2>/dev/null; then
    SAN="-fsanitize=address -fno-omit-frame-pointer"
fi
UP="-x c -std=gnu89 -w -O1 -g -funsigned-char -fno-strict-aliasing -Dmain=hc_game_main -I$ROOT/amiga/include -I$ROOT/amiga -I$HC/src $SAN"
MINE="-std=gnu99 -Wall -Werror -O1 -g -I$ROOT/amiga $SAN"
for f in hc_dos hc_keys hc_fixpoint hc_sound hc_files; do $CC $MINE -c -o "$T/$f.o" "$ROOT/amiga/$f.c"; done
$CC $MINE -c -o "$T/hc_host.o" "$HERE/hc_host.c"
$CC $MINE -c -o "$T/unit_test.o" "$HERE/unit_test.c"
OURS="$T/hc_dos.o $T/hc_keys.o $T/hc_fixpoint.o $T/hc_sound.o $T/hc_files.o $T/hc_host.o"

# the data drawer: upstream's names, and lower case too (the game asks for
# both; AmigaDOS doesn't mind, a PC's file system does)
mkdir -p "$T/data"
for f in "$HC"/data/*; do
    b=$(basename "$f"); cp "$f" "$T/data/$b"
    l=$(echo "$b" | tr 'A-Z' 'a-z'); [ "$l" = "$b" ] || ln -s "$b" "$T/data/$l"
done

# shellcheck disable=SC2086
$CC $SAN -o "$T/unit" "$T/unit_test.o" $OURS -lm
HC_TEST_DATA="$T/data" HC_END=100000 "$T/unit"

for f in hyper6 raycast trig h3d_gfx1 hypadt; do $CC $UP -c -o "$T/$f.o" "$HC/src/$f.c"; done
# shellcheck disable=SC2086
$CC $SAN -o "$T/hc" "$T"/hyper6.o "$T"/raycast.o "$T"/trig.o "$T"/h3d_gfx1.o "$T"/hypadt.o $OURS -lm

fail=0
summary() { sed -n "s/^$2 //p" "$1/summary.txt"; }
differ() { ! cmp -s "$1" "$2"; }
ppm_colours() {   # distinct colours in a PPM frame
    python3 -c 'import sys
d=open(sys.argv[1],"rb").read(); i=d.index(b"255\n")+4; px=d[i:]
print(len({px[j:j+3] for j in range(0,len(px),3)}))' "$1"
}

run() {   # run NAME VAR=VALUE...: the game until HC_END
    mkdir -p "$T/$1"; n=$1; shift
    if ! env HC_DATA="$T/data" HC_OUT="$T/$n" "$@" "$T/hc" > "$T/$n/stdout.txt" 2> "$T/$n/stderr.txt"; then
        echo "FAIL $n: the game stopped"; tail -30 "$T/$n/stderr.txt"; fail=1; return 1
    fi
    [ -f "$T/$n/summary.txt" ] || { echo "FAIL $n: no summary"; fail=1; return 1; }
}

# attract: logo, presents, title
if run attract HC_END=16000 HC_SHOTS=2000,5000,15000; then
    [ "$(summary "$T/attract" opened)" = 1 ] || { echo "FAIL attract: the screen was not opened once"; fail=1; }
    [ "$(summary "$T/attract" shows)" -gt 20 ] || { echo "FAIL attract: few frames shown"; fail=1; }
    differ "$T/attract/shot-5000.ppm" "$T/attract/shot-15000.ppm" || { echo "FAIL attract: the title never came"; fail=1; }
    c=$(ppm_colours "$T/attract/shot-15000.ppm"); [ "$c" -ge 20 ] || { echo "FAIL attract: the title has $c colours"; fail=1; }
    grep -q "HYPERCYCLES" "$T/attract/stdout.txt" || { echo "FAIL attract: no banner"; fail=1; }
    echo "attract: logo, presents and title shown ($c colours on the title)"
fi

# play: New Game, Rookie, the briefing, then ride (Return is raw 0x44,
# up 0x4C, left 0x4F)
KEYS="16000:68:1,16100:68:0,20000:68:1,20100:68:0,24000:68:1,24100:68:0,28000:68:1,28100:68:0"
KEYS="$KEYS,40000:76:1,46000:76:0,46000:79:1,47000:79:0"
if run play HC_END=50000 HC_KEYS="$KEYS" HC_SHOTS=36000,40000,45000,49000; then
    differ "$T/play/shot-40000.ppm" "$T/play/shot-45000.ppm" || { echo "FAIL play: riding forward changed nothing"; fail=1; }
    differ "$T/play/shot-45000.ppm" "$T/play/shot-49000.ppm" || { echo "FAIL play: turning changed nothing"; fail=1; }
    c=$(ppm_colours "$T/play/shot-45000.ppm"); [ "$c" -ge 30 ] || { echo "FAIL play: the grid has $c colours"; fail=1; }
    echo "play: level 1 rides forward and turns ($c colours in the view)"
fi

if [ -n "${HC_TEST_KEEP:-}" ]; then mkdir -p "$HC_TEST_KEEP"; cp -R "$T/attract" "$T/play" "$HC_TEST_KEEP/"; fi
[ $fail = 0 ] && echo "host tests: all passed${SAN:+ (AddressSanitizer)}"
exit $fail
