#ifndef LISA_LIBC_H
#define LISA_LIBC_H

/* The little of the C library UNO uses, for a machine with no OS
   (libc.c). rand() and srand() are declared by <stdlib.h>, which the
   shared cards.c includes. gcc may also call memcpy and memset itself,
   for structure copies and clears. */

char *strcpy(char *d, const char *s);
char *strcat(char *d, const char *s);
void *memcpy(void *d, const void *s, unsigned long n);
void *memset(void *d, int c, unsigned long n);

#endif
