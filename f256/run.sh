#!/bin/bash
# Copy build/uno.pgz onto a *working copy* of an F256 SD-card image (so the
# original is never touched) and launch the foenix MAME. Then, at the
# SuperBASIC prompt, type:  /- uno
# Usage: run.sh <mame_dir> <sdcard.img> <uno.pgz>
set -e
MAME_DIR="$1"; SDCARD="$2"; PGZ="$3"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cp "$SDCARD" "$WORK/sd.img"

DISK="$(hdiutil attach -nomount "$WORK/sd.img" | head -1 | awk '{print $1}')"
diskutil mount "${DISK}s1" >/dev/null
VOL="$(diskutil info "${DISK}s1" | awk -F': +' '/Mount Point/{print $2}')"
cp "$PGZ" "$VOL/uno.pgz"
diskutil unmount "${DISK}s1" >/dev/null
hdiutil detach "$DISK" >/dev/null

echo ">>> MAME launching. At the SuperBASIC prompt, type:  /- uno"
"$MAME_DIR/mame" f256k -rompath "$MAME_DIR" -window -resolution 800x600 \
    -harddisk "$WORK/sd.img"
