#!/usr/bin/env python3
"""Writes a Lisa parameter memory (64 bytes, BMLisa's bmlisa.pram) that
starts the Lisa from the floppy, so the boot ROM goes straight to UNO
instead of trying the hard disk and stopping at error 85.

From the boot ROM (341-0175-H/0176-H): the parameter memory is the odd
bytes of $FCC181-$FCC1FF. The high nibble of byte 4 ($FCC189) is the
startup device, and 1 is the floppy. The ROM uses it only if the 32
words pass its checksum: each word is added to a running total that is
then rotated left one bit, and the total must come out 0; the last word
is there to make it so."""
import sys

def rol(v):
    return ((v << 1) | (v >> 15)) & 0xFFFF

def checksum(b, words):
    d = 0
    for i in range(words):
        d = rol((d + ((b[2 * i] << 8) | b[2 * i + 1])) & 0xFFFF)
    return d

b = bytearray(64)
b[4] = 1 << 4  # start from the floppy
w = -checksum(b, 31) & 0xFFFF
b[62], b[63] = w >> 8, w & 0xFF
assert checksum(b, 32) == 0
with open(sys.argv[1], "wb") as f:
    f.write(b)
