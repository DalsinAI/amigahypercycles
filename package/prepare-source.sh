#!/bin/sh
# The upstream source and data, ready to build: package/hc.lock's commit of
# malespiaut/hypercycles is fetched into DIR/upstream (once), then
#   DIR/src   the C sources and headers, named in lower case (as their
#             #includes name them) with DOS's CRs taken out, and
#             amiga/patches applied;
#   DIR/data  the game's files: pictures (*.PCX), maps (LEVEL*.DAT,
#             FM_*.DAT), LEVEL.DEF, OBJECT.DEF, music (*.MDI) and the
#             text files, as upstream names them.
#   sh package/prepare-source.sh DIR
# GPL-3.0-or-later, like Hypercycles. Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/.." && pwd)
. "$HERE/hc.lock"
D=${1:?DIR}
mkdir -p "$D"
U="$D/upstream"
if [ "$(git -C "$U" rev-parse HEAD 2>/dev/null)" != "$HC_COMMIT" ]; then
    rm -rf "$U"; mkdir -p "$U"
    git -C "$U" init -q
    git -C "$U" fetch -q --depth 1 "$HC_GIT" "$HC_COMMIT"
    git -C "$U" checkout -q FETCH_HEAD
fi
[ "$(git -C "$U" rev-parse HEAD)" = "$HC_COMMIT" ] || { echo "hypercycles: commit mismatch" >&2; exit 1; }

rm -rf "$D/src" "$D/data"
mkdir -p "$D/src" "$D/data"
for f in "$U"/*; do
    b=$(basename "$f")
    case "$b" in
    *.C|*.H)
        tr -d '\r' < "$f" > "$D/src/$(echo "$b" | tr 'A-Z' 'a-z')" ;;
    *.PCX|*.DAT|*.DEF|*.MDI|*.TXT)
        cp "$f" "$D/data/$b" ;;
    esac
done
for p in "$ROOT"/amiga/patches/*.patch; do
    patch -s -d "$D/src" -p1 < "$p"
done
