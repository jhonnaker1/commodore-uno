#ifndef TI99_COMPAT_STDLIB_H
#define TI99_COMPAT_STDLIB_H

/* The tms9900 toolchain is a bare cross compiler with no C library at all --
   libti99 supplies hardware access and a few string helpers, but nothing
   declares rand()/srand(). The platform-independent cards.c includes
   <stdlib.h> for them and is shared verbatim across every port in this repo,
   so rather than touch it, an earlier -I puts this shim in the way (same
   trick as the CoCo port's compat/stdlib.h). tirand.c has the implementation. */

int rand(void);
void srand(unsigned int seed);

#define RAND_MAX 0x7FFF

#endif
