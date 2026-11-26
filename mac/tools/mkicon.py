#!/usr/bin/env python3
"""Draw UNO's 32x32 Finder icon and print it as Rez 'ICN#' data.

Two cards fanned: a grey one behind, and a black (red-suit, in this port's
1-bit scheme) card in front with the white oval of a real UNO card and a
tiny "UNO" in it. The mask is the two card silhouettes. Run with no
arguments; the output is pasted into src/uno.r.
"""
W = H = 32
img = [[0] * W for _ in range(H)]
mask = [[0] * W for _ in range(H)]

def rounded(x0, y0, x1, y1, r):
    """Cells of an inclusive rounded rect, plus its outline."""
    inside, edge = set(), set()
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            cx = min(max(x, x0 + r), x1 - r)
            cy = min(max(y, y0 + r), y1 - r)
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r + r:
                inside.add((x, y))
    for (x, y) in inside:
        if any((x + dx, y + dy) not in inside for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
            edge.add((x, y))
    return inside, edge

# back card: 50% grey, black outline
back, back_edge = rounded(2, 1, 19, 26, 3)
for (x, y) in back:
    img[y][x] = 1 if (x + y) % 2 == 0 else 0
    mask[y][x] = 1
for (x, y) in back_edge:
    img[y][x] = 1

# front card: solid black, with a white outline ring so it stands off the back card
front, front_edge = rounded(11, 5, 29, 31, 3)
ring = set()
for (x, y) in front_edge:
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            p = (x + dx, y + dy)
            if p not in front and 0 <= p[0] < W and 0 <= p[1] < H:
                ring.add(p)
for (x, y) in ring:
    img[y][x] = 0
    mask[y][x] = 1
for (x, y) in front:
    img[y][x] = 1
    mask[y][x] = 1

# white oval in the middle of the front card
cx, cy, rx, ry = 20, 18, 7.5, 8.5
for y in range(H):
    for x in range(W):
        if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0:
            img[y][x] = 0

# "UNO" in a 3x5 font, centred in the oval
font = {
    "U": ["101", "101", "101", "101", "111"],
    "N": ["101", "111", "111", "111", "101"],
    "O": ["111", "101", "101", "101", "111"],
}
x0, y0 = 15, 16
for i, ch in enumerate("UNO"):
    for r, row in enumerate(font[ch]):
        for c, bit in enumerate(row):
            if bit == "1":
                img[y0 + r][x0 + i * 4 + c] = 1

def rows_hex(bits):
    out = []
    for row in bits:
        v = 0
        for b in row:
            v = (v << 1) | b
        out.append("%08X" % v)
    return out

if __name__ == "__main__":
    import sys
    if "--show" in sys.argv:
        for y in range(H):
            print("".join("#" if img[y][x] else ("." if mask[y][x] else " ") for x in range(W)))
        sys.exit()
    for name, bits in (("icon", img), ("mask", mask)):
        hexrows = rows_hex(bits)
        print("\t\t/* %s */" % name)
        for i in range(0, 32, 4):
            print('\t\t$"%s"' % " ".join(hexrows[i:i + 4]))
