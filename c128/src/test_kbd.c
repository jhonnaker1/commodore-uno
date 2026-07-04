#include <conio.h>
#include <ascii_charmap.h>

#define SCREEN ((unsigned char *)0x0400)
#define COLOR ((unsigned char *)0xD800)

int main(void) {
    unsigned char count = 0;
    unsigned int i;

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
