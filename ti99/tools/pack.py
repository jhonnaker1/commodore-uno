#!/usr/bin/env python3
"""Build a 16K paged TI-99/4A cartridge image from the loader stub and the game.

The game is bigger than the 8K the cartridge window shows at once, so the cart
is two 8K banks and the ROM-resident stub (src/loader.asm) copies the program
into expansion RAM before running it. Both banks get an identical copy of that
stub, because switching banks changes the memory under the program counter --
the code has to look the same on both sides of the switch. The payload is then
split across whatever space is left in each bank.

Usage: pack.py <loader.elf> <game.elf> <out.bin>
"""
import subprocess
import sys

BANK_SIZE = 0x2000
ROM_BASE = 0x6000
NBANKS = 2

PREFIX = None  # set from argv[0] environment; resolved in tool()


def tool(name):
    return "%s-%s" % (PREFIX, name)


def symbols(elf):
    """name -> address, via nm."""
    out = subprocess.run([tool("nm"), elf], capture_output=True, text=True,
                         check=True).stdout
    syms = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3:
            syms[parts[2]] = int(parts[0], 16)
    return syms


def sections(elf):
    """name -> (addr, offset, size), via readelf."""
    out = subprocess.run([tool("readelf"), "-S", "-W", elf], capture_output=True,
                         text=True, check=True).stdout
    secs = {}
    for line in out.splitlines():
        line = line.strip()
        if not line.startswith("["):
            continue
        # "[ 1] .text  PROGBITS  0000a000 000400 002ec2 00  AX  0 0 2"
        rest = line[line.index("]") + 1:].split()
        if len(rest) < 5:
            continue
        try:
            secs[rest[0]] = (int(rest[2], 16), int(rest[3], 16), int(rest[4], 16))
        except ValueError:
            continue
    return secs


def flat_image(elf, secs, start, end):
    """The loadable bytes of the ELF between two addresses, as one blob."""
    data = bytearray(end - start)
    with open(elf, "rb") as f:
        for name in (".text", ".data"):
            if name not in secs:
                continue
            addr, off, size = secs[name]
            if size == 0:
                continue
            f.seek(off)
            chunk = f.read(size)
            data[addr - start:addr - start + size] = chunk
    return bytes(data)


def main():
    global PREFIX
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    loader_elf, game_elf, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    PREFIX = sys.argv[4] if len(sys.argv) > 4 else "tms9900"

    # --- the game ---
    gsecs = sections(game_elf)
    gsyms = symbols(game_elf)
    text_addr = gsecs[".text"][0]
    edata = gsyms["_edata"]
    ebss = gsyms["_ebss"]
    entry = gsyms["main"]
    payload = flat_image(game_elf, gsecs, text_addr, edata)

    # --- the loader stub ---
    lsecs = sections(loader_elf)
    lsyms = symbols(loader_elf)
    laddr, loff, lsize = lsecs[".text"]
    with open(loader_elf, "rb") as f:
        f.seek(loff)
        stub = bytearray(f.read(lsize))

    stub_len = lsyms["payload"] - laddr
    chunk = BANK_SIZE - stub_len          # payload bytes that fit in bank 0
    capacity = chunk * NBANKS
    if len(payload) > capacity:
        sys.exit("error: program is %d bytes, but the 2-bank cart only holds %d"
                 % (len(payload), capacity))

    def patch(sym, value):
        off = lsyms[sym] - laddr
        stub[off] = (value >> 8) & 0xFF   # TMS9900 is big-endian
        stub[off + 1] = value & 0xFF

    patch("ldest", text_addr)
    patch("lsize", len(payload))
    patch("lbssad", edata)
    patch("lbsssz", ebss - edata)
    patch("lentry", entry)
    patch("lchunk", chunk)

    # --- assemble the two banks ---
    rom = bytearray(b"\x00" * (BANK_SIZE * NBANKS))
    for bank in range(NBANKS):
        base = bank * BANK_SIZE
        rom[base:base + len(stub)] = stub          # identical stub in both
        part = payload[bank * chunk:(bank + 1) * chunk]
        rom[base + stub_len:base + stub_len + len(part)] = part

    with open(out_path, "wb") as f:
        f.write(rom)

    print("wrote %s: %d bytes of program at >%04X (%d free), "
          "bss >%04X+%d, entry >%04X"
          % (out_path, len(payload), text_addr, capacity - len(payload),
             edata, ebss - edata, entry))


if __name__ == "__main__":
    main()
