/* CMOC's own headers already declare rand()/srand() (see cmoc.h), and it
   doesn't ship a stdlib.h of its own -- its preprocessing step (a plain
   host `cc -E`) falls through to this Mac's system stdlib.h otherwise,
   which on current Xcode drags in libc++'s C++ compatibility headers and
   fails outright (missing __BYTE_ORDER__, bare __has_builtin, etc. --
   all unrelated to this program, just a mismatch between CMOC's minimal
   preprocessing environment and a modern macOS SDK). This shim shadows
   that via an earlier -I so `#include <stdlib.h>` in the platform-
   independent cards.c/game.c (shared verbatim across every port in this
   repo, so left untouched) resolves here instead. */
#include <cmoc.h>
