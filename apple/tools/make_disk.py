#!/usr/bin/env python3
"""Injects a compiled cc65 ProDOS SYS file into a copy of an existing,
already-bootable ProDOS disk image, since no AppleCommander/cadius-style
tool is available via Homebrew or PyPI.

Strategy: repurpose an existing root-directory file's entry rather than
creating a new one. Creating a brand-new sapling (multi-block, with an
index block) directory entry reliably breaks this disk's boot sequence
("Unable to load ATInit file"), even with an otherwise byte-correct entry
-- confirmed across several isolated tests (tiny seedling files work fine
when newly added, saplings never do, regardless of blocks_used/date/access
field values). The actual root cause was never pinned down. Overwriting an
existing, already-working sapling file's data blocks in place -- keeping
its directory entry, key_block/index block, and all metadata fields except
name/file_type/aux_type/eof -- sidesteps that bug entirely and is proven to
boot and run correctly.

ProDOS block/directory layout referenced throughout is the standard
documented format (blocks are 512 bytes; directory entries are 39 bytes).
"""
import struct
import sys

BLOCK_SIZE = 512
ENTRY_LEN = 0x27


def block_slice(n):
    return slice(n * BLOCK_SIZE, (n + 1) * BLOCK_SIZE)


def find_entry(data, name):
    """Walks the directory block chain starting at block 2, returns
    (block_num, offset_within_block) for the named file's entry."""
    name = name.upper()
    block_num = 2
    is_first = True
    while block_num != 0:
        b = data[block_slice(block_num)]
        prev_block, next_block = struct.unpack_from('<HH', b, 0)
        off = 4 + ENTRY_LEN if is_first else 4
        while off + ENTRY_LEN <= BLOCK_SIZE:
            storage_type_len = b[off]
            storage_type = storage_type_len >> 4
            name_len = storage_type_len & 0x0F
            if storage_type != 0 and name_len > 0:
                entry_name = b[off + 1:off + 1 + name_len].decode('ascii', errors='replace')
                if entry_name == name:
                    return block_num, off
            off += ENTRY_LEN
        block_num = next_block
        is_first = False
    raise RuntimeError(f"file {name!r} not found")


def index_block_pointers(data, key_block):
    idx = data[block_slice(key_block)]
    return [idx[i] | (idx[0x100 + i] << 8) for i in range(256) if idx[i] or idx[0x100 + i]]


def prodos_name(name):
    name = name.upper()
    assert 1 <= len(name) <= 15
    assert name[0].isalpha()
    assert all(c.isalnum() or c == '.' for c in name)
    return name


def main():
    if len(sys.argv) != 5:
        print(f"usage: {sys.argv[0]} <template.po> <file_to_add> "
              f"<existing_file_to_repurpose> <out.po>", file=sys.stderr)
        print("<existing_file_to_repurpose> must be a sapling (multi-block) "
              "file on the template disk with at least as many data blocks "
              "as <file_to_add> needs.", file=sys.stderr)
        sys.exit(1)

    template_path, file_path, repurpose_name, out_path = sys.argv[1:]
    repurpose_name = prodos_name(repurpose_name)

    data = bytearray(open(template_path, 'rb').read())
    file_data = open(file_path, 'rb').read()
    file_len = len(file_data)

    dir_block, dir_off = find_entry(data, repurpose_name)
    b = data[block_slice(dir_block)]
    entry = bytearray(b[dir_off:dir_off + ENTRY_LEN])

    storage_type = entry[0] >> 4
    if storage_type != 2:
        raise RuntimeError(f"{repurpose_name!r} is not a sapling file (storage_type={storage_type})")

    key_block = struct.unpack_from('<H', entry, 0x11)[0]
    data_blocks = index_block_pointers(data, key_block)
    data_blocks_needed = (file_len + BLOCK_SIZE - 1) // BLOCK_SIZE
    if data_blocks_needed > len(data_blocks):
        raise RuntimeError(
            f"{repurpose_name!r} only has {len(data_blocks)} data blocks, "
            f"need {data_blocks_needed} for {file_path}")

    for i, blk in enumerate(data_blocks):
        chunk = file_data[i * BLOCK_SIZE:(i + 1) * BLOCK_SIZE]
        chunk = chunk + b'\x00' * (BLOCK_SIZE - len(chunk))
        data[block_slice(blk)] = chunk

    name = prodos_name("UNO")
    entry[0] = (2 << 4) | len(name)  # storage_type=2 (sapling), new name_len
    entry[1:1 + 15] = b'\x00' * 15
    entry[1:1 + len(name)] = name.encode('ascii')
    # file_type $FF (SYS): the compiled binary is built with __EXEHDR__=0
    # (see Makefile), so it's a plain image loadable/runnable at a single
    # fixed address ($2000) -- exactly what a ProDOS "-NAME" system launch
    # does, no companion loader needed.
    entry[0x10] = 0xFF
    entry[0x15] = file_len & 0xFF
    entry[0x16] = (file_len >> 8) & 0xFF
    entry[0x17] = (file_len >> 16) & 0xFF
    struct.pack_into('<H', entry, 0x1F, 0x2000)  # aux_type: fixed system load address
    # blocks_used, key_block, dates, access, version, min_version, and
    # header_pointer are all left exactly as they were on the known-good
    # entry we're repurposing.

    b[dir_off:dir_off + ENTRY_LEN] = entry
    data[block_slice(dir_block)] = b

    with open(out_path, 'wb') as f:
        f.write(data)
    print(f"wrote {out_path}: repurposed {repurpose_name!r} -> {name!r} "
          f"({file_len} bytes, {data_blocks_needed} of {len(data_blocks)} data blocks used)")


if __name__ == '__main__':
    main()
