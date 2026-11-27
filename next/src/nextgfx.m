/* The NeXT platform layer (see nextgfx.h): NeXTSTEP 3.3's AppKit and
   Display PostScript.

   main() makes the application, its menu and a delegate, and runs it.
   When the application has finished starting up, the delegate's
   appDidInit: opens the window and calls the game (uno_main() in main.c),
   which never returns: from then on the game's own loop runs, and
   wait_vsync() hands the AppKit its events once a frame. Quit and a
   closed window end the program from inside that event handling.

   The window is buffered, so drawing goes to its off-screen copy and only
   reaches the screen when wait_vsync() flushes it: a card is never seen
   half drawn, and the Window Server repaints the window by itself when
   something that covered it moves away. */
#import <appkit/appkit.h>
#import <sys/time.h>
#import <stdlib.h>
#import <string.h>
#import "nextgfx.h"

void uno_main(void); /* main.c */

#define SCALE 1.5

static id win, view;
static unsigned char has_color, dirty;
static void (*new_game_handler)(void);
static id fonts[4];

/* ---- input queues ---- */

#define KEYQ 32
static int keyq[KEYQ];
static unsigned char kq_head, kq_tail;

#define CLICKQ 8
static short clickq[CLICKQ][2];
static unsigned char cq_head, cq_tail;

static void push_key(int c)
{
    unsigned char next = (unsigned char)((kq_tail + 1) % KEYQ);
    if (next == kq_head) return;
    keyq[kq_tail] = c;
    kq_tail = next;
}

static void push_click(short x, short y)
{
    unsigned char next = (unsigned char)((cq_tail + 1) % CLICKQ);
    if (next == cq_head) return;
    clickq[cq_tail][0] = x;
    clickq[cq_tail][1] = y;
    cq_tail = next;
}

int gfx_next_key(void)
{
    int c;
    if (kq_head == kq_tail) return 0;
    c = keyq[kq_head];
    kq_head = (unsigned char)((kq_head + 1) % KEYQ);
    return c;
}

int gfx_next_click(short *x, short *y)
{
    if (cq_head == cq_tail) return 0;
    *x = clickq[cq_head][0];
    *y = clickq[cq_head][1];
    cq_head = (unsigned char)((cq_head + 1) % CLICKQ);
    return 1;
}

void gfx_flush_input(void)
{
    kq_head = kq_tail;
    cq_head = cq_tail;
}

void gfx_on_new_game(void (*handler)(void))
{
    new_game_handler = handler;
}

/* ---- the view: takes keys and clicks ---- */

@interface UnoView : View
@end

@implementation UnoView

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)acceptsFirstMouse
{
    return YES;
}

/* Everything is drawn as the game goes; the buffered window keeps it. */
- drawSelf:(const NXRect *)rects :(int)count
{
    return self;
}

- keyDown:(NXEvent *)e
{
    int c = e->data.key.charCode;
    if (e->flags & NX_COMMANDMASK) return [super keyDown:e];
    if (e->data.key.charSet == NX_SYMBOLSET) {
        /* the arrow keys, as the Mac's codes */
        switch (c) {
        case 0xAC: push_key(0x1C); break; /* left */
        case 0xAD: push_key(0x1E); break; /* up */
        case 0xAE: push_key(0x1D); break; /* right */
        case 0xAF: push_key(0x1F); break; /* down */
        default: break;
        }
        return self;
    }
    if (c) push_key(c);
    return self;
}

- mouseDown:(NXEvent *)e
{
    NXPoint p = e->location;
    [self convertPoint:&p fromView:nil];
    push_click((short)p.x, (short)p.y);
    return self;
}

@end

/* ---- the application's delegate ---- */

@interface UnoDelegate : Object
@end

@implementation UnoDelegate

- appDidInit:sender
{
    uno_main();
    return self;
}

- newGame:sender
{
    if (new_game_handler) new_game_handler();
    return self;
}

/* Info > Info Panel...: an alert panel, which shows the application's
   icon beside the text. It runs its own event loop until OK is pressed. */
- showInfo:sender
{
    NXRunAlertPanel("UNO",
        "UNO for the NeXT, one of the ports in UNO for Vintage Computers.\n\n"
        "You against three CPU players, with Skip, Reverse, Draw Two, Wild "
        "and Wild Draw Four (which you can challenge).",
        "OK", NULL, NULL);
    return self;
}

- windowWillClose:sender
{
    [NXApp terminate:self];
    return self;
}

@end

int main(int argc, char *argv[])
{
    id menu, info, delegate;

    NXApp = [Application new];
    delegate = [[UnoDelegate alloc] init];
    [NXApp setDelegate:delegate];

    /* the usual NeXT order: Info first, Hide and Quit last */
    menu = [[Menu alloc] initTitle:"UNO"];
    info = [[Menu alloc] initTitle:"Info"];
    [[info addItem:"Info Panel..." action:@selector(showInfo:) keyEquivalent:'\0']
        setTarget:delegate];
    [info sizeToFit];
    [menu setSubmenu:info
             forItem:[menu addItem:"Info" action:@selector(submenuAction:) keyEquivalent:'\0']];
    [[menu addItem:"New Game" action:@selector(newGame:) keyEquivalent:'n']
        setTarget:delegate];
    [menu addItem:"Hide" action:@selector(hide:) keyEquivalent:'h'];
    [menu addItem:"Quit" action:@selector(terminate:) keyEquivalent:'q'];
    [menu sizeToFit];
    [NXApp setMainMenu:menu];

    [NXApp run];
    [NXApp free];
    return 0;
}

/* ---- the window ---- */

void gfx_init(void)
{
    const NXScreen *screen = [NXApp colorScreen];
    NXRect r;

    has_color = screen &&
        NXNumberOfColorComponents(NXColorSpaceFromDepth(screen->depth)) > 1;

    fonts[FONT_UI] = [Font newFont:"Helvetica" size:12.0 matrix:NX_FLIPPEDMATRIX];
    fonts[FONT_BOLD] = [Font newFont:"Helvetica-Bold" size:12.0 matrix:NX_FLIPPEDMATRIX];
    fonts[FONT_SMALL] = [Font newFont:"Helvetica" size:9.0 matrix:NX_FLIPPEDMATRIX];
    fonts[FONT_BIG] = [Font newFont:"Helvetica-Bold" size:24.0 matrix:NX_FLIPPEDMATRIX];

    NXSetRect(&r, 180.0, 230.0, GFX_W * SCALE, GFX_H * SCALE);
    win = [[Window alloc] initContent:&r
                                style:NX_TITLEDSTYLE
                              backing:NX_BUFFERED
                           buttonMask:NX_MINIATURIZEBUTTONMASK | NX_CLOSEBUTTONMASK
                                defer:NO];
    [win setTitle:"UNO"];
    [win setDelegate:[NXApp delegate]];

    NXSetRect(&r, 0.0, 0.0, GFX_W * SCALE, GFX_H * SCALE);
    view = [[UnoView alloc] initFrame:&r];
    [view setFlipped:YES];
    [view setDrawSize:(NXCoord)GFX_W :(NXCoord)GFX_H];
    [win setContentView:view];
    [win makeFirstResponder:view];
    [win display]; /* the title bar and frame */

    gfx_begin();
    gfx_gray(NX_WHITE);
    gfx_fill_rect(0, 0, GFX_W, GFX_H);
    gfx_end();

    [win makeKeyAndOrderFront:nil];
    [NXApp activateSelf:YES];
    wait_vsync();
}

unsigned char gfx_has_color(void)
{
    return has_color;
}

void gfx_begin(void)
{
    [view lockFocus];
}

void gfx_end(void)
{
    [view unlockFocus];
    dirty = 1;
}

static double now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void wait_vsync(void)
{
    static double next;
    double t = now();
    NXEvent *e;

    if (dirty) {
        [win flushWindow];
        NXPing();
        dirty = 0;
    }
    /* one frame on from the last, unless the game fell behind */
    if (next < t - 0.1) next = t;
    next += 1.0 / 60.0;
    for (;;) {
        double wait = next - now();
        if (wait < 0.0) wait = 0.0;
        e = [NXApp getNextEvent:NX_ALLEVENTS waitFor:wait threshold:NX_BASETHRESHOLD];
        if (e) [NXApp sendEvent:e];
        else if (now() >= next) break;
    }
}

/* ---- drawing ---- */

void gfx_gray(float v)
{
    PSsetgray(v);
}

void gfx_rgb(float r, float g, float b)
{
    PSsetrgbcolor(r, g, b);
}

void gfx_fill_rect(short l, short t, short r, short b)
{
    PSrectfill(l, t, r - l, b - t);
}

void gfx_frame_rect(short l, short t, short r, short b)
{
    PSsetlinewidth(1.0);
    PSrectstroke(l + 0.5, t + 0.5, r - l - 1, b - t - 1);
}

static void round_path(float l, float t, float r, float b, float rad)
{
    PSnewpath();
    PSmoveto(l + rad, t);
    PSarct(r, t, r, b, rad);
    PSarct(r, b, l, b, rad);
    PSarct(l, b, l, t, rad);
    PSarct(l, t, r, t, rad);
    PSclosepath();
}

void gfx_fill_round(short l, short t, short r, short b, float radius)
{
    round_path(l, t, r, b, radius);
    PSfill();
}

void gfx_frame_round(short l, short t, short r, short b, float radius, float w)
{
    float h = w / 2;
    PSsetlinewidth(w);
    round_path(l + h, t + h, r - h, b - h, radius - h > 0 ? radius - h : 0);
    PSstroke();
}

void gfx_fill_oval(short l, short t, short r, short b)
{
    PSgsave();
    PStranslate((l + r) / 2.0, (t + b) / 2.0);
    PSscale((r - l) / 2.0, (b - t) / 2.0);
    PSnewpath();
    PSarc(0.0, 0.0, 1.0, 0.0, 360.0);
    PSfill();
    PSgrestore();
}

void gfx_clip_round(short l, short t, short r, short b, float radius)
{
    PSgsave();
    round_path(l, t, r, b, radius);
    PSclip();
}

void gfx_unclip(void)
{
    PSgrestore();
}

static int cur_font;

void gfx_font(int which)
{
    cur_font = which;
    [fonts[which] set];
}

void gfx_text(short x, short y, const char *s)
{
    PSmoveto(x, y);
    PSshow(s);
}

short gfx_text_w(const char *s)
{
    return (short)([fonts[cur_font] getWidthOf:s] + 0.5);
}
