#!/usr/bin/env python3
"""Wrap a raw TI-99/4A cartridge ROM image in an RPK.

MAME's TI-99 cartridge connector doesn't take a bare ROM dump -- it wants an
RPK, which is just a zip holding the ROM plus a layout.xml naming which sockets
the ROM plugs into.

Note the pcb names here are the RPK loader's, which are not the same as the
`feature name="pcb"` values in MAME's own software list: a two-bank 16K cart is
"paged16k" in the softlist but "paged" in an RPK, and it wants the two banks as
two separate 8K images in two sockets rather than one 16K blob.

Usage: mkrpk.py <rom.bin> <out.rpk> [standard|paged]
"""
import sys
import os
import zipfile

BANK_SIZE = 0x2000

HEADER = """<?xml version="1.0" encoding="utf-8"?>
<romset version="1.0">
    <resources>
"""
FOOTER = """    </resources>
    <configuration>
        <pcb type="{pcb}">
{sockets}        </pcb>
    </configuration>
</romset>
"""


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    rom_path, rpk_path = sys.argv[1], sys.argv[2]
    pcb = sys.argv[3] if len(sys.argv) > 3 else "standard"
    stem = os.path.splitext(os.path.basename(rom_path))[0]

    with open(rom_path, "rb") as f:
        rom = f.read()

    if pcb == "paged":
        banks = [("%s_b0.bin" % stem, rom[:BANK_SIZE]),
                 ("%s_b1.bin" % stem, rom[BANK_SIZE:BANK_SIZE * 2])]
        socket_ids = ["rom_socket", "rom2_socket"]
    else:
        banks = [("%s.bin" % stem, rom)]
        socket_ids = ["rom_socket"]

    resources = ""
    sockets = ""
    for i, (name, _data) in enumerate(banks):
        resources += '        <rom id="rom%dimage" file="%s"/>\n' % (i, name)
        sockets += '            <socket id="%s" uses="rom%dimage"/>\n' % (
            socket_ids[i], i)

    layout = HEADER + resources + FOOTER.format(pcb=pcb, sockets=sockets)

    with zipfile.ZipFile(rpk_path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("layout.xml", layout)
        for name, data in banks:
            z.writestr(name, data)

    print("wrote %s (%s, %d bank(s), %d bytes of ROM)"
          % (rpk_path, pcb, len(banks), len(rom)))


if __name__ == "__main__":
    main()
