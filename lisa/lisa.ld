/* The program is one flat image from $20000: the boot block (512 bytes,
   which the ROM loads) and then everything boot.S loads after it. */
OUTPUT_FORMAT("elf32-m68k")
ENTRY(_start)

SECTIONS
{
    . = 0x20000;
    .boot : { KEEP(*(.boot)) }
    ASSERT(SIZEOF(.boot) <= 512, "the boot block is over 512 bytes")
    . = 0x20200;
    .text : { *(.text.start) *(.text .text.*) *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    . = ALIGN(4);
    __load_end = .;
    .bss (NOLOAD) : { __bss_start = .; *(.bss .bss.*) *(COMMON); . = ALIGN(4); __bss_end = .; }
    __load_blocks = (__load_end - 0x20200 + 511) / 512;
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
