/* A C library's worth for UNO on a bare Lisa: string copies, the memory
   routines gcc calls on its own, and rand(), plus the start-up that clears
   BSS and calls main(). */
#include <stdlib.h>
#include "libc.h"

extern unsigned long __bss_start[], __bss_end[];
int main(void);

/* boot.S jumps here once the whole program is loaded; .text.start puts it
   first in the image. */
void __attribute__((section(".text.start"), noreturn)) _cstart(void) {
    unsigned long *p;
    for (p = __bss_start; p < __bss_end; p++) *p = 0;
    main();
    for (;;) {}
}

char *strcpy(char *d, const char *s) {
    char *r = d;
    while ((*d++ = *s++) != 0) {}
    return r;
}

char *strcat(char *d, const char *s) {
    char *r = d;
    while (*d) d++;
    while ((*d++ = *s++) != 0) {}
    return r;
}

void *memcpy(void *d, const void *s, unsigned long n) {
    unsigned char *dd = d;
    const unsigned char *ss = s;
    while (n--) *dd++ = *ss++;
    return d;
}

void *memset(void *d, int c, unsigned long n) {
    unsigned char *dd = d;
    while (n--) *dd++ = (unsigned char)c;
    return d;
}

/* The C standard's example generator: 15 bits, which is all cards.c's
   rand() % (i + 1) needs. */
static unsigned long next = 1;

int rand(void) {
    next = next * 1103515245UL + 12345;
    return (int)((next >> 16) & 0x7FFF);
}

void srand(unsigned int seed) {
    next = seed;
}
