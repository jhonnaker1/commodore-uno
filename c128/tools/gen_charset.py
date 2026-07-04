#!/usr/bin/env python3
"""Generates build/charset.bin: a 2048-byte C64 character set.

Chars 0-127 are copied verbatim from a real C64 character ROM (chargen),
so normal PETSCII text/menus render correctly.

Chars 128-255 are custom card-art glyphs (box corners/edges, big digits,
action-card icons, card-back pattern, cursor) defined here as 8x8 bitmaps.
"""
import sys
import struct

CHARGEN_PATH = "/Users/jhonnaker/Downloads/WinVICE-3.1-x64/C64/chargen"
OUT_PATH = "build/charset.bin"
HEADER_PATH = "src/charset_data.h"
CODES_HEADER_PATH = "src/charset_codes.h"

# 8x8 glyphs: '#' = pixel on, '.' or ' ' = pixel off. Row0 = top.
GLYPHS = {
    # Deliberately NOT full-height right-angle strokes -- those visually
    # read as letter fragments (e.g. the old top border looked like "da")
    # at small size, crowding out the actual card value. Small corner
    # blocks + dotted edges read as decoration, not text, so the value
    # in the middle stands out by contrast instead.
    128: ("CORNER_TL", [
        "##......",
        "##......",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    ]),
    129: ("CORNER_TR", [
        "......##",
        "......##",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    ]),
    130: ("CORNER_BL", [
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "##......",
        "##......",
    ]),
    131: ("CORNER_BR", [
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "......##",
        "......##",
    ]),
    132: ("EDGE_TOP", [
        "#.#.#.#.",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
    ]),
    133: ("EDGE_BOTTOM", [
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "........",
        "#.#.#.#.",
    ]),
    134: ("EDGE_LEFT", [
        "#.......",
        "........",
        "#.......",
        "........",
        "#.......",
        "........",
        "#.......",
        "........",
    ]),
    135: ("EDGE_RIGHT", [
        ".......#",
        "........",
        ".......#",
        "........",
        ".......#",
        "........",
        ".......#",
        "........",
    ]),
    136: ("SOLID_BLOCK", [
        "########",
        "########",
        "########",
        "########",
        "########",
        "########",
        "########",
        "########",
    ]),
    137: ("CARDBACK", [
        "#.#.#.#.",
        ".#.#.#.#",
        "#.#.#.#.",
        ".#.#.#.#",
        "#.#.#.#.",
        ".#.#.#.#",
        "#.#.#.#.",
        ".#.#.#.#",
    ]),
    138: ("CURSOR", [
        "...##...",
        "..####..",
        ".######.",
        "########",
        "........",
        "........",
        "........",
        "........",
    ]),
    139: ("ICON_SKIP", [
        "..####..",
        ".######.",
        "##....##",
        "#.##..##",
        "##..##.#",
        "##....##",
        ".######.",
        "..####..",
    ]),
    140: ("ICON_REVERSE", [
        "...##...",
        "..####..",
        ".######.",
        "...##...",
        "...##...",
        ".######.",
        "..####..",
        "...##...",
    ]),
    141: ("ICON_DRAW2", [
        "####....",
        "#..#....",
        "#..#....",
        "####....",
        "....####",
        "....#..#",
        "....#..#",
        "....####",
    ]),
    142: ("ICON_WILD", [
        "..#..#..",
        "#..##..#",
        ".#.##.#.",
        "..####..",
        "..####..",
        ".#.##.#.",
        "#..##..#",
        "..#..#..",
    ]),
    # Big bold digits 0-9
    143: ("DIGIT_0", [
        ".######.",
        "##....##",
        "##....##",
        "##....##",
        "##....##",
        "##....##",
        "##....##",
        ".######.",
    ]),
    144: ("DIGIT_1", [
        "...##...",
        "..###...",
        ".####...",
        "...##...",
        "...##...",
        "...##...",
        "...##...",
        ".######.",
    ]),
    145: ("DIGIT_2", [
        ".#####..",
        "##...##.",
        "....##..",
        "...##...",
        "..##....",
        ".##.....",
        "##......",
        "#######.",
    ]),
    146: ("DIGIT_3", [
        ".#####..",
        "##...##.",
        ".....##.",
        "..####..",
        ".....##.",
        "##...##.",
        "##...##.",
        ".#####..",
    ]),
    147: ("DIGIT_4", [
        "....##..",
        "...###..",
        "..####..",
        ".##.##..",
        "########",
        "....##..",
        "....##..",
        "....##..",
    ]),
    148: ("DIGIT_5", [
        "#######.",
        "##......",
        "##......",
        "######..",
        ".....##.",
        "......##",
        "##...##.",
        ".#####..",
    ]),
    149: ("DIGIT_6", [
        "..####..",
        ".##.....",
        "##......",
        "######..",
        "##...##.",
        "##...##.",
        "##...##.",
        ".#####..",
    ]),
    150: ("DIGIT_7", [
        "########",
        ".....##.",
        "....##..",
        "...##...",
        "..##....",
        "..##....",
        "..##....",
        "..##....",
    ]),
    151: ("DIGIT_8", [
        ".#####..",
        "##...##.",
        "##...##.",
        ".#####..",
        "##...##.",
        "##...##.",
        "##...##.",
        ".#####..",
    ]),
    152: ("DIGIT_9", [
        ".#####..",
        "##...##.",
        "##...##.",
        ".######.",
        ".....##.",
        "......#.",
        "....##..",
        ".####...",
    ]),
}

CUSTOM_BASE = 128


def glyph_to_bytes(rows):
    out = bytearray(8)
    for i, row in enumerate(rows):
        val = 0
        for bit, ch in enumerate(row):
            if ch == "#":
                val |= (1 << (7 - bit))
        out[i] = val
    return bytes(out)


def main():
    with open(CHARGEN_PATH, "rb") as f:
        chargen = f.read()
    if len(chargen) < 2048:
        print("chargen file too small", file=sys.stderr)
        sys.exit(1)

    out = bytearray(2048)
    # Chars 0-127 from the "uppercase/graphics" ROM charset (first 2KB image).
    out[0:1024] = chargen[0:1024]

    # Chars 128-255: custom card art, default blank.
    for code, (name, rows) in GLYPHS.items():
        offset = (code - CUSTOM_BASE) * 8 + 1024
        out[offset:offset + 8] = glyph_to_bytes(rows)

    with open(OUT_PATH, "wb") as f:
        f.write(out)
    print(f"wrote {OUT_PATH} ({len(out)} bytes), {len(GLYPHS)} custom glyphs")

    with open(CODES_HEADER_PATH, "w") as f:
        f.write("/* Generated by tools/gen_charset.py -- do not edit by hand. */\n")
        f.write("#ifndef CHARSET_CODES_H\n#define CHARSET_CODES_H\n\n")
        for code, (name, _rows) in sorted(GLYPHS.items()):
            f.write(f"#define CH_{name} {code}\n")
        f.write("\n#endif\n")
    print(f"wrote {CODES_HEADER_PATH}")

    with open(HEADER_PATH, "w") as f:
        f.write("/* Generated by tools/gen_charset.py -- do not edit by hand. */\n")
        f.write("#ifndef CHARSET_DATA_H\n#define CHARSET_DATA_H\n\n")
        f.write('#include "charset_codes.h"\n\n')
        f.write("static const unsigned char charset_data[2048] = {\n")
        for i in range(0, len(out), 16):
            chunk = ", ".join(str(b) for b in out[i:i + 16])
            f.write(f"    {chunk},\n")
        f.write("};\n\n#endif\n")
    print(f"wrote {HEADER_PATH}")


if __name__ == "__main__":
    main()
