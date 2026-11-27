#!/usr/bin/env python3
"""Draws UNO's application icon and writes it as a NeXTSTEP icon TIFF.

    python3 tools/mkicon.py UNO.tiff

The icon is three UNO cards fanned out, green and yellow behind and red in
front, with "UNO" in the front card's white oval. It is 48x48, drawn four
times over and averaged down so the edges are smooth.

The file holds two images, laid out as NeXTSTEP 3.3's own icons are (the
TIFF in Edit.app's __ICON segment was read to get this right):
  1. 2-bit grey with a 2-bit alpha plane, planar: for greyscale NeXTs;
  2. 4-bit RGBA, one strip: for colour NeXTs.
Both are big-endian, uncompressed, with premultiplied alpha, and mark
their alpha with NeXT's Matteing tag (32995) rather than ExtraSamples.
The build runs on the NeXT, which has no Python, so the TIFF this writes
is kept in the repository beside this script."""
import struct
import sys

N = 48
SS = 4  # supersampling
W = N * SS

RED = (0.88, 0.08, 0.08)
YELLOW = (0.97, 0.81, 0.00)
GREEN = (0.00, 0.63, 0.22)
WHITE = (1.0, 1.0, 1.0)
BLACK = (0.0, 0.0, 0.0)

# "UNO" in a 3x5 font, one string per row
GLYPHS = {
    "U": ["X.X", "X.X", "X.X", "X.X", "XXX"],
    "N": ["XXX", "X.X", "X.X", "X.X", "X.X"],
    "O": ["XXX", "X.X", "X.X", "X.X", "XXX"],
}

img = [[(0.0, 0.0, 0.0, 0.0) for _ in range(W)] for _ in range(W)]


def put(x, y, c):
    if 0 <= x < W and 0 <= y < W:
        img[y][x] = (c[0], c[1], c[2], 1.0)


def in_round(px, py, l, t, r, b, rad):
    """Is the point inside the rounded rectangle l,t,r,b?"""
    if not (l <= px < r and t <= py < b):
        return False
    cx = min(max(px, l + rad), r - rad)
    cy = min(max(py, t + rad), b - rad)
    return (px - cx) ** 2 + (py - cy) ** 2 <= rad * rad


def card(ox, oy, color):
    """A 24x36 card (in icon pixels) with its top left at ox, oy: a black
    edge, the suit colour, a white oval."""
    l, t, r, b = ox * SS, oy * SS, (ox + 24) * SS, (oy + 36) * SS
    rad = 4 * SS
    for y in range(t, b):
        for x in range(l, r):
            px, py = x + 0.5, y + 0.5
            if not in_round(px, py, l, t, r, b, rad):
                continue
            inner = in_round(px, py, l + SS, t + SS, r - SS, b - SS, rad - SS)
            put(x, y, color if inner else BLACK)
    cx, cy = (l + r) / 2, (t + b) / 2
    ax, ay = 9 * SS, 12 * SS  # the oval's half-sizes
    for y in range(t, b):
        for x in range(l, r):
            d = ((x + 0.5 - cx) / ax) ** 2 + ((y + 0.5 - cy) / ay) ** 2
            if d <= 1.0:
                put(x, y, WHITE)


def text(ox, oy, s, color):
    for i, ch in enumerate(s):
        for row, bits in enumerate(GLYPHS[ch]):
            for col, bit in enumerate(bits):
                if bit == "X":
                    for y in range((oy + row) * SS, (oy + row + 1) * SS):
                        for x in range((ox + i * 4 + col) * SS, (ox + i * 4 + col + 1) * SS):
                            put(x, y, color)


card(2, 10, GREEN)
card(11, 6, YELLOW)
card(21, 2, RED)
text(27, 18, "UNO", RED)

# average down to 48x48, premultiplying the colour by its coverage
pix = []
for y in range(N):
    row = []
    for x in range(N):
        r = g = b = a = 0.0
        for yy in range(y * SS, (y + 1) * SS):
            for xx in range(x * SS, (x + 1) * SS):
                pr, pg, pb, pa = img[yy][xx]
                r += pr * pa
                g += pg * pa
                b += pb * pa
                a += pa
        k = SS * SS
        row.append((r / k, g / k, b / k, a / k))
    pix.append(row)


def q(v, levels):
    return min(levels - 1, int(v * (levels - 1) + 0.5))


# image 1: 2-bit grey + 2-bit alpha, planar
def plane(values):
    out = bytearray()
    for row in values:
        byte = 0
        for i, v in enumerate(row):
            byte = (byte << 2) | v
            if i % 4 == 3:
                out.append(byte)
                byte = 0
    return bytes(out)


grey = [[q(min(0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2], p[3]), 4) for p in row] for row in pix]
alpha = [[q(p[3], 4) for p in row] for row in pix]
g_plane, a_plane = plane(grey), plane(alpha)

# image 2: 4-bit RGBA, one strip, two samples a byte
rgba = bytearray()
for row in pix:
    nib = []
    for p in row:
        a = q(p[3], 16)
        nib += [min(q(p[0], 16), a), min(q(p[1], 16), a), min(q(p[2], 16), a), a]
    for i in range(0, len(nib), 2):
        rgba.append((nib[i] << 4) | nib[i + 1])
rgba = bytes(rgba)

# --- write the TIFF ---
SHORT, LONG, RATIONAL = 3, 4, 5


def ifd(entries, next_off):
    """entries: (tag, type, count, value-or-offset), sorted by tag."""
    out = struct.pack(">H", len(entries))
    for tag, typ, count, val in sorted(entries):
        if typ == SHORT and count == 1:
            out += struct.pack(">HHIHH", tag, typ, count, val, 0)
        elif typ == SHORT and count == 2 and isinstance(val, tuple):
            out += struct.pack(">HHIHH", tag, typ, count, val[0], val[1])
        else:
            out += struct.pack(">HHII", tag, typ, count, val)
    return out + struct.pack(">I", next_off)


def ifd_size(n):
    return 2 + 12 * n + 4


data = bytearray(b"MM\x00\x2a\x00\x00\x00\x00")
# image data first, then the directories and their out-of-line values
off_g = len(data); data += g_plane
off_a = len(data); data += a_plane
off_rgba = len(data); data += rgba
if len(data) % 2:
    data += b"\x00"

# out-of-line values for image 1: strip offsets/counts, resolution
off1_vals = len(data)
data += struct.pack(">II", off_g, off_a)               # +0 strip offsets
data += struct.pack(">II", len(g_plane), len(a_plane))  # +8 strip byte counts
data += struct.pack(">II", 72, 1)                       # +16 x resolution
data += struct.pack(">II", 72, 1)                       # +24 y resolution
# image 2: bits per sample (4 shorts), resolution
off2_vals = len(data)
data += struct.pack(">HHHH", 4, 4, 4, 4)                # +0 bits per sample
data += struct.pack(">II", 72, 1)                       # +8 x resolution
data += struct.pack(">II", 72, 1)                       # +16 y resolution

img1 = [
    (256, SHORT, 1, N), (257, SHORT, 1, N),
    (258, SHORT, 2, (2, 2)), (259, SHORT, 1, 1), (262, SHORT, 1, 1),
    (273, LONG, 2, off1_vals), (277, SHORT, 1, 2), (278, SHORT, 1, N),
    (279, LONG, 2, off1_vals + 8), (282, RATIONAL, 1, off1_vals + 16),
    (283, RATIONAL, 1, off1_vals + 24), (284, SHORT, 1, 2),
    (296, SHORT, 1, 2), (32995, SHORT, 1, 1),
]
img2 = [
    (256, SHORT, 1, N), (257, SHORT, 1, N),
    (258, SHORT, 4, off2_vals), (259, SHORT, 1, 1), (262, SHORT, 1, 2),
    (273, LONG, 1, off_rgba), (277, SHORT, 1, 4), (278, SHORT, 1, N),
    (279, LONG, 1, len(rgba)), (282, RATIONAL, 1, off2_vals + 8),
    (283, RATIONAL, 1, off2_vals + 16), (284, SHORT, 1, 1),
    (296, SHORT, 1, 2), (32995, SHORT, 1, 1),
]
ifd1_off = len(data)
ifd2_off = ifd1_off + ifd_size(len(img1))
data += ifd(img1, ifd2_off)
data += ifd(img2, 0)
data[4:8] = struct.pack(">I", ifd1_off)

with open(sys.argv[1], "wb") as f:
    f.write(data)
