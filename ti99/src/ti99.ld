/* The game is copied into the 32K expansion's upper 24K block (>A000-FFFF)
   by the cartridge loader and runs from there, so unlike a ROM-resident
   build, .text and .data are simply contiguous in RAM -- no separate ROM copy
   of the initialised data to relocate at startup.

   The stack lives at >4000 growing down (set by the loader), which is the top
   of the *other*, lower 8K expansion block at >2000-3FFF. Keeping the stack in
   a different block from the program means it cannot grow into the code.

   tools/pack.py reads _edata/_ebss to know how much to copy and how much to
   clear. */

ENTRY(main)

SECTIONS
{
  . = 0xA000;

  .text : {
    *(.text)
    *(.text.*)
    *(.rodata)
    *(.rodata.*)
  }

  .data : {
    *(.data)
    *(.data.*)
  }
  _edata = .;

  .bss : {
    *(.bss)
    *(.bss.*)
    *(COMMON)
  }
  _ebss = .;
}
