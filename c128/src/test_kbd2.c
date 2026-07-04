#include <conio.h>
#include <ascii_charmap.h>

#define SCREEN ((unsigned char *)0x8000)
#define COLOR ((unsigned char *)0xD800)
#define CIA2_PRA (*(unsigned char *)0xDD00)
#define VIC_MEMCTL (*(unsigned char *)0xD018)

int main(void) {
    unsigned char count = 0;
    unsigned int i;

    /* Same VIC bank-2 switch as vic_init(): screen at $8000, would-be
       charset at $8800 (skipped here, not needed for this test). */
    CIA2_PRA = (CIA2_PRA & 0xFC) | 0x01;
    VIC_MEMCTL = 0x02;

    for (i = 0; i < 1000; i++) {
        SCREEN[i] = 32;
        COLOR[i] = 1;
    }
    SCREEN[0] = 'K' - 64;
    SCREEN[1] = 'B' - 64;
    SCREEN[2] = 'D' - 64;

    for (;;) {
        if (kbhit()) {
            char k = cgetc();
            SCREEN[40 + count] = (unsigned char)k;
            count++;
            if (count > 38) count = 0;
        }
    }
    return 0;
}
