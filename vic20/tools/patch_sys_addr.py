#!/usr/bin/env python3
"""Patches cc65's auto-generated "10 SYS nnnn" BASIC stub in a compiled
.prg to point at the correct address.

Needed for linker configs (like vic20-highmem.cfg) where the real code
doesn't start immediately after the stub -- there's a deliberate gap so
the $1000-$1FFF range stays free for the VIC-20's screen memory. cc65's
stub generator assumes a contiguous layout and computes the wrong address,
so the ASCII digits it emits have to be overwritten by hand after linking.
"""
import sys


def main():
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} <file.prg> <correct_address>", file=sys.stderr)
        sys.exit(1)
    path, addr_str = sys.argv[1], sys.argv[2]
    correct = str(int(addr_str))

    data = bytearray(open(path, 'rb').read())
    # PRG layout: load addr (2 bytes) + BASIC line: next-line ptr (2) +
    # line number (2) + SYS token ($9e) + ASCII digits + line terminator ($00).
    if data[6] != 0x9E:
        print(f"error: byte 6 is 0x{data[6]:02x}, not the SYS token (0x9e) -- "
              f"stub layout doesn't match what this tool expects", file=sys.stderr)
        sys.exit(1)
    start = 7
    end = start
    while data[end] != 0:
        end += 1
    old = data[start:end].decode('ascii')
    if len(correct) != len(old):
        print(f"error: replacement {correct!r} is a different length than "
              f"original {old!r} -- can't patch in place without shifting "
              f"the rest of the file", file=sys.stderr)
        sys.exit(1)
    data[start:end] = correct.encode('ascii')
    open(path, 'wb').write(data)
    print(f"patched SYS address in {path}: {old} -> {correct}")


if __name__ == '__main__':
    main()
