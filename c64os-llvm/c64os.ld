/* A C64 OS application, linked by llvm-mos.
 *
 * The contract, read out of the SDK (../c64os/inc/os/s/app.s):
 *   - a PRG whose load address is $0900 (appbase)
 *   - the first ten bytes are five vectors: init, msgcmd, willquit,
 *     REU freeze, REU thaw
 *   - a KERNAL link table of 3-byte records (module id, routine offset)
 *     terminated by $FF, which init hands to initextern at $02FC and
 *     which the OS rewrites in place into JMPs
 *   - the arena is pages $09..$A0; $Cxxx holds C64 OS's own modules and
 *     $E000 is utilbase, so nothing may be placed above $A0FF
 *
 * EVERYTHING IS INSIDE THE LOADED FILE -- .bss, .noinit and the soft
 * stack included. C64 OS allocates the pages an app's file covers and no
 * more, so a NOLOAD stack after the image sits in memory the OS never
 * gave us (ega trek's proof-of-concept measured exactly that). Loading
 * them as zeros also stands in for crt0's zero-bss, which -nostartfiles
 * drops.
 */

/* The imaginary registers live at $4E-$6D: BASIC's floating-point
 * accumulators in C64 OS's own map (//os/docs/memory.t), which nothing on
 * its interrupt path touches. src/app.s swaps them at every OS/C crossing
 * and explains why $02-$21 -- C64 OS's timers -- could not work. */
__rc0 = 0x4E;
INCLUDE imag-regs.ld

MEMORY {
    /* No zero page beyond the registers: a static the compiler put here
     * would be swapped out along with them. */
    zp  : ORIGIN = __rc31 + 1, LENGTH = 0
    ram : ORIGIN = 0x0900,     LENGTH = 0xA100 - 0x0900
}

REGION_ALIAS("c_readonly", ram)
REGION_ALIAS("c_writeable", ram)

SECTIONS {
    .app_header : { KEEP(*(.app_header)) } >ram

    INCLUDE c.ld

    /* A reserved section, not an address somebody believed was free.
     * app.s fills it with $A5 at init so its high-water mark can be read
     * straight out of memory. */
    .stack : {
        __stack_bottom = .;
        . += 512;
        __stack = .;
        /* One real byte at the very end makes the region PROGBITS, so
         * TRIM below writes everything up to here into the file. */
        BYTE(0x00)
    } >ram
}

OUTPUT_FORMAT {
    SHORT(ORIGIN(ram))
    TRIM(ram)
}
