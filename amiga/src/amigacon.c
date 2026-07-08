#include <exec/types.h>
#include <exec/io.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include "amigacon.h"

/* The simple Open("CON:...") shortcut only ever gave us the default
   Workbench screen, whose palette rendered every one of our colors as
   the same handful of blue/gray shades (confirmed empirically -- see
   git history). Getting a real 8-color palette means owning our own
   screen, which means going through Intuition + console.device directly
   instead of the dos.library CON: handler. Text output still goes
   through console.device (so the same ANSI CUP/SGR escape codes work),
   but keyboard input comes from Intuition's own IDCMP_VANILLAKEY
   messages on our window instead -- simpler than console.device reads,
   and we don't need cursor-key decoding at all since movement is mapped
   to ordinary keys (comma/period/'U', see input.c). */

/* 816x240, not the naively-expected 640x200: console.device's default
   font isn't 8x8 the way COLS/ROWS in amigacon.h assumed. Confirmed
   empirically (via a temporary diagnostic printing the ConUnit's real
   cu_XMax/cu_YMax) that a 640x200 window only gave 63 columns x 21 rows
   -- about 10.16 x 9.52 px per character. 816x240 gives console.device's
   internal model exactly 80x25 at that real character size.

   The real DISPLAYED view is still narrower than that -- confirmed with
   a column-marker test that only columns up to ~60 reliably show
   (marker digits at every 10th column: 0-6 visible, 7 i.e. column 70
   wasn't). Tried fixing that with SA_Overscan=OSCAN_MAX (made it worse:
   only columns 10-30 visible) and an explicit SA_DClip rectangle (no
   better). Given neither made things wider, plain OpenScreen() -- which
   gave the widest confirmed-safe range of everything tried -- is kept,
   and the UI layout (ui.c) works within that ~60-column safe width
   instead (wrapping the hand to a second row after 9 cards rather than
   11, since 11 needed columns past the safe range). */
static struct NewScreen ns = {
    0, 0, 816, 240, 3,
    0, 1,
    HIRES,
    CUSTOMSCREEN,
    NULL, (UBYTE *)"UNO", NULL, NULL
};

static struct NewWindow nw = {
    0, 0, 816, 240,
    0, 1,
    IDCMP_VANILLAKEY,
    ACTIVATE | BORDERLESS | RMBTRAP,
    NULL, NULL,
    NULL,
    NULL, NULL,
    0, 0, 0, 0,
    CUSTOMSCREEN
};

static struct Screen *scr;
static struct Window *win;
static struct MsgPort *ioport;
static struct IOStdReq *io;

static void con_write(char *data, unsigned int len) {
    io->io_Command = CMD_WRITE;
    io->io_Data = (APTR)data;
    io->io_Length = (LONG)len;
    DoIO((struct IORequest *)io);
}

void amiga_init(void) {
    scr = OpenScreen(&ns);

    SetRGB4(&scr->ViewPort, 0, 0, 0, 0);    /* black */
    SetRGB4(&scr->ViewPort, 1, 15, 0, 0);   /* red */
    SetRGB4(&scr->ViewPort, 2, 0, 15, 0);   /* green */
    SetRGB4(&scr->ViewPort, 3, 15, 15, 0);  /* yellow */
    SetRGB4(&scr->ViewPort, 4, 0, 0, 15);   /* blue */
    SetRGB4(&scr->ViewPort, 5, 15, 0, 15);  /* magenta */
    SetRGB4(&scr->ViewPort, 6, 0, 15, 15);  /* cyan */
    SetRGB4(&scr->ViewPort, 7, 15, 15, 15); /* white */

    nw.Screen = scr;
    win = OpenWindow(&nw);

    ioport = CreatePort(NULL, 0);
    io = (struct IOStdReq *)CreateExtIO(ioport, sizeof(struct IOStdReq));
    io->io_Data = (APTR)win;
    io->io_Length = sizeof(struct Window);
    OpenDevice("console.device", 0, (struct IORequest *)io, 0);

    scr_clear();
}

void amiga_shutdown(void) {
    if (io) {
        CloseDevice((struct IORequest *)io);
        DeleteExtIO((struct IORequest *)io);
    }
    if (ioport) DeletePort(ioport);
    if (win) CloseWindow(win);
    if (scr) CloseScreen(scr);
}

/* No memory-mapped raster register to poll here (unlike every other port
   in this project) -- pace with the OS's own tick timer instead. One
   tick is 1/50 second. */
void wait_vsync(void) {
    Delay(1);
}

/* Tiny local strlen so this file doesn't have to pull in a full <string.h>
   dependency chain just for one call site. */
static unsigned int strlen_(const char *s) {
    unsigned int n = 0;
    while (s[n]) n++;
    return n;
}

/* Writes the decimal digits of n (no leading zeros, 1-3 digits for our
   range) into buf, returns how many bytes were written. */
static unsigned char put_num(char *buf, unsigned int n) {
    char tmp[5];
    unsigned char i = 0, len;
    if (n == 0) {
        buf[0] = '0';
        return 1;
    }
    while (n > 0 && i < 5) {
        tmp[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    len = i;
    while (i > 0) {
        i--;
        buf[len - 1 - i] = tmp[i];
    }
    return len;
}

/* ANSI CUP/SGR sequences built by hand instead of via sprintf(): newlib's
   sprintf pulls in float formatting support, which on AmigaOS means an
   unconditional open of mathieeedoubbas.library at startup -- fine on a
   full Workbench install, but fails outright ("mathieeedoubbas.library
   failed to load") on a plain Kickstart boot with no Workbench disk,
   confirmed empirically. Nothing here actually uses floating point, so
   avoiding sprintf sidesteps the dependency entirely. */
static void goto_xy(unsigned char x, unsigned char y) {
    char buf[16];
    unsigned char len = 0;
    buf[len++] = 0x1B;
    buf[len++] = '[';
    len = (unsigned char)(len + put_num(buf + len, (unsigned)y + 1));
    buf[len++] = ';';
    len = (unsigned char)(len + put_num(buf + len, (unsigned)x + 1));
    buf[len++] = 'H';
    con_write(buf, len);
}

static void set_color(unsigned char color) {
    char buf[5];
    buf[0] = 0x1B;
    buf[1] = '[';
    buf[2] = '3';
    buf[3] = (char)('0' + (color & 7));
    buf[4] = 'm';
    con_write(buf, 5);
}

void scr_clear(void) {
    con_write("\x1b[0m\x1b[2J\x1b[H", 11);
}

void scr_put(unsigned char x, unsigned char y, char ch, unsigned char color) {
    goto_xy(x, y);
    set_color(color);
    con_write(&ch, 1);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    goto_xy(x, y);
    set_color(color);
    con_write((char *)s, strlen_(s));
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color) {
    char buf[6];
    unsigned char i = 0;
    if (n == 0) {
        scr_put(x, y, '0', color);
        return;
    }
    while (n > 0 && i < 5) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) {
        i--;
        scr_put(x, y, buf[i], color);
        x++;
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color) {
    unsigned char row, col;
    for (row = 0; row < h; row++) {
        goto_xy(x, (unsigned char)(y + row));
        set_color(color);
        for (col = 0; col < w; col++) {
            con_write(&ch, 1);
        }
    }
}

int con_getkey(void) {
    struct IntuiMessage *msg;
    int result = -1;

    msg = (struct IntuiMessage *)GetMsg(win->UserPort);
    if (msg) {
        if (msg->Class == IDCMP_VANILLAKEY) {
            result = (int)(unsigned char)msg->Code;
        }
        ReplyMsg((struct Message *)msg);
    }
    return result;
}
