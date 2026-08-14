#!/usr/bin/env python3
"""Turn a framebuffer dump written by the DOS graphics build into a PNG.

The CGA text build can dump its screen as readable text, because a text mode
is characters. A bitmap cannot, so the EGA and VGA builds write raw pixels
and this decodes them -- which keeps the verification loop the same shape
for every DOS build: the program reports what it actually drew, and nothing
has to be screenshotted.

Two formats, told apart by a two-byte magic:

  "EG"  EGA mode 10h. Four bit planes, each row GFX_W/8 bytes, planes
        stored one after another. A pixel's colour is one bit from each
        plane, plane 0 being the least significant.
  "VG"  VGA mode 13h. One byte per pixel, plus a 768-byte palette of
        6-bit RGB triples taken from the DAC.

Usage: dumptopng.py <dump> <out.png>
"""
import struct
import sys
import zlib

# The EGA palette this port displays, as RGB. These are the mode 10h
# defaults with one change: egavid.c reprograms index 2 from the default
# primary green (0,170,0) to secondary green (0,85,0) for a darker felt,
# so the two must be kept in step or the PNGs will not show what the card
# actually showed.
EGA_PALETTE = [
    (0x00, 0x00, 0x00), (0x00, 0x00, 0xAA), (0x00, 0x55, 0x00), (0x00, 0xAA, 0xAA),
    (0xAA, 0x00, 0x00), (0xAA, 0x00, 0xAA), (0xAA, 0x55, 0x00), (0xAA, 0xAA, 0xAA),
    (0x55, 0x55, 0x55), (0x55, 0x55, 0xFF), (0x55, 0xFF, 0x55), (0x55, 0xFF, 0xFF),
    (0xFF, 0x55, 0x55), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0x55), (0xFF, 0xFF, 0xFF),
]


def write_png(path, width, height, rows_rgb):
    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    raw = b"".join(b"\x00" + bytes(r) for r in rows_rgb)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def decode_ega(data):
    w, h = struct.unpack("<HH", data[2:6])
    stride = w // 8
    planes = data[6:]
    plane_size = stride * h
    rows = []
    for y in range(h):
        row = bytearray()
        for x in range(w):
            byte = (x >> 3)
            bit = 7 - (x & 7)
            idx = 0
            for p in range(4):
                b = planes[p * plane_size + y * stride + byte]
                if (b >> bit) & 1:
                    idx |= (1 << p)
            row += bytes(EGA_PALETTE[idx])
        rows.append(row)
    return w, h, rows


def decode_vga(data):
    w, h = struct.unpack("<HH", data[2:6])
    pal = data[6:6 + 768]
    pix = data[6 + 768:]
    # The DAC holds 6-bit values; scale to 8 bits for the PNG.
    table = [((pal[i * 3] * 255) // 63,
              (pal[i * 3 + 1] * 255) // 63,
              (pal[i * 3 + 2] * 255) // 63) for i in range(256)]
    rows = []
    for y in range(h):
        row = bytearray()
        for x in range(w):
            row += bytes(table[pix[y * w + x]])
        rows.append(row)
    return w, h, rows


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    data = open(sys.argv[1], "rb").read()
    magic = data[:2]
    if magic == b"EG":
        w, h, rows = decode_ega(data)
    elif magic == b"VG":
        w, h, rows = decode_vga(data)
    else:
        sys.exit("unrecognised dump magic %r" % magic)
    write_png(sys.argv[2], w, h, rows)
    print("%s: %dx%d" % (sys.argv[2], w, h))


if __name__ == "__main__":
    main()
