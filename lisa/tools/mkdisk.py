#!/usr/bin/env python3
"""mkdisk.py IMAGE.bin OUT.dc42 - a Lisa 400K boot floppy (Disk Copy 4.2).

IMAGE.bin is the program from $20000: its first 512 bytes are the boot
block, the rest goes in blocks 1, 2, ... in order. The boot block's tag
gets the file ID $AAAA the boot ROM looks for (tag bytes 4-5).

Disk Copy 4.2: an 84 byte header (a Pascal-string name in 64 bytes, then
the data's and the tags' sizes and checksums, big endian, then the disk
format, the format byte and $0100), all 800 blocks' data, then their
12-byte tags. Each checksum adds the big-endian words and rotates the sum
right a bit; the tags' leaves out the first block's tag. IDLE reads the
sizes only; the checksums are right for other tools.
"""
import struct
import sys

BLOCKS, TAG = 800, 12


def checksum(data):
    s = 0
    for i in range(0, len(data), 2):
        s = (s + ((data[i] << 8) | data[i + 1])) & 0xFFFFFFFF
        s = ((s >> 1) | (s << 31)) & 0xFFFFFFFF
    return s


def main(src, dst):
    prog = open(src, "rb").read()
    if len(prog) > BLOCKS * 512:
        sys.exit("mkdisk.py: the program doesn't fit on a 400K floppy")
    data = bytearray(BLOCKS * 512)
    data[:len(prog)] = prog
    tags = bytearray(BLOCKS * TAG)
    tags[4:6] = b"\xAA\xAA"
    name = b"UNO"
    head = bytearray(84)
    head[0] = len(name)
    head[1:1 + len(name)] = name
    head[64:80] = struct.pack(">IIII", len(data), len(tags), checksum(data), checksum(tags[TAG:]))
    head[80:84] = b"\x00\x02\x01\x00"  # 400K, a Lisa disk, then the $0100 marker
    open(dst, "wb").write(bytes(head) + bytes(data) + bytes(tags))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
