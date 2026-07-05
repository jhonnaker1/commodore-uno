#!/usr/bin/env python3
"""Read-only inspection of a ProDOS volume directory, to understand the
template disk's structure before we try writing a new file into it."""
import sys
import struct

def block(data, n):
    return data[n * 512:(n + 1) * 512]

def read_entries(data, block_num, is_first):
    b = block(data, block_num)
    prev_block, next_block = struct.unpack_from('<HH', b, 0)
    off = 4
    entry_len = 0x27
    entries_per_block = 13
    header_len = 4
    if is_first:
        # volume directory header occupies the first entry slot
        storage_type_len = b[4]
        storage_type = storage_type_len >> 4
        name_len = storage_type_len & 0x0F
        name = b[5:5 + name_len].decode('ascii', errors='replace')
        entry_length = b[0x23]
        entries_per_block2 = b[0x24]
        file_count = struct.unpack_from('<H', b, 0x25)[0]
        bit_map_pointer = struct.unpack_from('<H', b, 0x27)[0]
        total_blocks = struct.unpack_from('<H', b, 0x29)[0]
        print(f"Volume header: name={name} storage_type={storage_type} "
              f"entry_length={entry_length} entries_per_block={entries_per_block2} "
              f"file_count={file_count} bit_map_pointer={bit_map_pointer} "
              f"total_blocks={total_blocks}")
        off = 4 + 0x27  # skip past the header entry itself
    while off + 0x27 <= 512:
        entry = b[off:off + 0x27]
        storage_type_len = entry[0]
        storage_type = storage_type_len >> 4
        name_len = storage_type_len & 0x0F
        if storage_type != 0 and name_len > 0:
            name = entry[1:1 + name_len].decode('ascii', errors='replace')
            file_type = entry[0x10]
            key_ptr = struct.unpack_from('<H', entry, 0x11)[0]
            blocks_used = struct.unpack_from('<H', entry, 0x13)[0]
            eof = entry[0x15] | (entry[0x16] << 8) | (entry[0x17] << 16)
            aux_type = struct.unpack_from('<H', entry, 0x1F)[0]
            print(f"  file: {name!r:20} type=${file_type:02x} storage_type={storage_type} "
                  f"key_block={key_ptr} blocks={blocks_used} eof={eof} aux_type=${aux_type:04x}")
        off += 0x27
    return prev_block, next_block

def main():
    path = sys.argv[1]
    data = bytearray(open(path, 'rb').read())
    print(f"disk size: {len(data)} bytes = {len(data)//512} blocks")
    prev_block, next_block = read_entries(data, 2, True)
    print("next_block of dir:", next_block)
    while next_block != 0:
        prev_block, next_block = read_entries(data, next_block, False)

if __name__ == '__main__':
    main()
