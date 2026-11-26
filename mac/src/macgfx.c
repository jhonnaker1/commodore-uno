/* Macintosh platform layer -- see macgfx.h. Written against the original
   Toolbox (Inside Macintosh I-III) so it runs from a Mac Plus on System 6
   up: GetNextEvent + SystemTask rather than WaitNextEvent, and a plain
   1-bit BitMap to draw into offscreen.

   Colour is an extra, taken only when it is all there: Color QuickDraw,
   32-bit QuickDraw (for NewGWorld -- the 6.0.x INIT or System 7), and a
   main screen set to 16 colours or more. Then the window is a colour one
   and the offscreen buffer a GWorld at the screen's depth; otherwise
   everything stays 1-bit, which is also what a Mac II with its monitor
   set to black-and-white gets. */
#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Events.h>
#include <OSUtils.h>
#include <ToolUtils.h>
#include <Memory.h>
#include <SegLoad.h>
#include <Gestalt.h>
#include <string.h>
#include "macgfx.h"

enum { MENU_APPLE = 128, MENU_FILE = 129, MENU_EDIT = 130, MENU_GAME = 131 };
enum { APPLE_ABOUT = 1 };
enum { FILE_QUIT = 1 };
enum { EDIT_UNDO = 1, EDIT_CUT = 3, EDIT_COPY, EDIT_PASTE, EDIT_CLEAR };
enum { GAME_NEW = 1 };

static void (*new_game_handler)(void);

static WindowPtr win;
static GrafPort off_port;
static BitMap off_bits;
static short dirty_top = GFX_H, dirty_bot = 0; /* empty when top >= bot */
static GrafPtr saved_port;

static unsigned char color;  /* drawing into a colour GWorld */
static GWorldPtr off_gw;
static CGrafPtr saved_cport;
static GDHandle saved_dev;

unsigned char gfx_has_color(void) {
    return color;
}

#define QLEN 16
static int keyq[QLEN];
static unsigned char key_head, key_tail;
static short clickq_x[QLEN], clickq_y[QLEN];
static unsigned char click_head, click_tail;

static void push_key(int k) {
    unsigned char next = (unsigned char)((key_tail + 1) % QLEN);
    if (next == key_head) return; /* full: drop */
    keyq[key_tail] = k;
    key_tail = next;
}

static void push_click(short x, short y) {
    unsigned char next = (unsigned char)((click_tail + 1) % QLEN);
    if (next == click_head) return;
    clickq_x[click_tail] = x;
    clickq_y[click_tail] = y;
    click_tail = next;
}

int gfx_next_key(void) {
    int k;
    if (key_head == key_tail) return 0;
    k = keyq[key_head];
    key_head = (unsigned char)((key_head + 1) % QLEN);
    return k;
}

int gfx_next_click(short *x, short *y) {
    if (click_head == click_tail) return 0;
    *x = clickq_x[click_head];
    *y = clickq_y[click_head];
    click_head = (unsigned char)((click_head + 1) % QLEN);
    return 1;
}

void gfx_flush_input(void) {
    key_head = key_tail;
    click_head = click_tail;
}

static void copy_rows(short top, short bottom) {
    Rect r;
    GrafPtr old;
    SetRect(&r, 0, top, GFX_W, bottom);
    GetPort(&old);
    SetPort(win);
    if (color) {
        ForeColor(blackColor); /* so CopyBits doesn't colourise */
        BackColor(whiteColor);
        CopyBits((BitMap *)*off_gw->portPixMap, &win->portBits, &r, &r, srcCopy, NULL);
    } else {
        CopyBits(&off_bits, &win->portBits, &r, &r, srcCopy, NULL);
    }
    SetPort(old);
}

static void present(void) {
    copy_rows(dirty_top, dirty_bot);
    dirty_top = GFX_H;
    dirty_bot = 0;
}

void gfx_on_new_game(void (*handler)(void)) {
    new_game_handler = handler;
}

static void setup_menus(void) {
    MenuHandle m;
    m = NewMenu(MENU_APPLE, (ConstStringPtr)"\p\024"); /* the Apple glyph */
    AppendMenu(m, (ConstStringPtr)"\pAbout UNO...;(-");
    AppendResMenu(m, 'DRVR'); /* desk accessories */
    InsertMenu(m, 0);
    m = NewMenu(MENU_FILE, (ConstStringPtr)"\pFile");
    AppendMenu(m, (ConstStringPtr)"\pQuit/Q");
    InsertMenu(m, 0);
    /* Edit is here for desk accessories (Note Pad, Calculator...), which
       get its commands through SystemEdit; the game itself has nothing to
       cut or paste. */
    m = NewMenu(MENU_EDIT, (ConstStringPtr)"\pEdit");
    AppendMenu(m, (ConstStringPtr)"\pUndo/Z;(-;Cut/X;Copy/C;Paste/V;Clear");
    InsertMenu(m, 0);
    m = NewMenu(MENU_GAME, (ConstStringPtr)"\pGame");
    AppendMenu(m, (ConstStringPtr)"\pNew Game/N");
    InsertMenu(m, 0);
    DrawMenuBar();
}

/* ---- About box: a modal window drawn in code (no DLOG/DITL resources),
   dismissed by a click or a key. Its own little event loop keeps the game
   window's update events answered while it is up. */

#define ABOUT_W 360
#define ABOUT_H 172

static void about_line(short y, short size, short face, const char *s) {
    short n = (short)strlen(s);
    TextFont(systemFont);
    TextSize(size);
    TextFace(face);
    MoveTo((short)((ABOUT_W - TextWidth((Ptr)s, 0, n)) / 2), y);
    DrawText((Ptr)s, 0, n);
}

static void about_draw(void) {
    about_line(34, 24, bold, "U N O");
    about_line(62, 12, 0, "One player against three CPU players.");
    about_line(86, 12, 0, "Part of UNO for Vintage Computers: the");
    about_line(102, 12, 0, "same game on 8- and 16-bit machines.");
    about_line(126, 12, 0, "Built with Retro68 for any Mac from the Plus up.");
    about_line(156, 12, bold, "Click to continue");
}

static void about_box(void) {
    Rect r, sb = qd.screenBits.bounds;
    WindowPtr box;
    GrafPtr old;
    EventRecord e;
    short left = (short)((sb.right - ABOUT_W) / 2), top = (short)((sb.bottom - ABOUT_H) / 2 + 10);

    GetPort(&old);
    SetRect(&r, left, top, left + ABOUT_W, top + ABOUT_H);
    box = NewWindow(NULL, &r, (ConstStringPtr)"\p", true, dBoxProc, (WindowPtr)-1, false, 0);
    SetPort(box);
    about_draw();
    for (;;) {
        SystemTask();
        if (!GetNextEvent(everyEvent, &e)) continue;
        if (e.what == mouseDown || e.what == keyDown) break;
        if (e.what == updateEvt) {
            WindowPtr w = (WindowPtr)e.message;
            BeginUpdate(w);
            if (w == box) { SetPort(box); about_draw(); }
            else if (w == win) copy_rows(0, GFX_H);
            EndUpdate(w);
        }
    }
    DisposeWindow(box); /* the game window gets an update event for the hole */
    SetPort(old);
}

static void do_menu(long choice) {
    short menu = (short)(choice >> 16), item = (short)(choice & 0xFFFF);
    HiliteMenu(0); /* before anything below that may not return */
    if (menu == MENU_APPLE) {
        if (item == APPLE_ABOUT) {
            about_box();
        } else {
            Str255 name;
            GrafPtr old;
            GetPort(&old);
            GetMenuItemText(GetMenuHandle(MENU_APPLE), item, name);
            OpenDeskAcc(name);
            SetPort(old);
        }
    } else if (menu == MENU_FILE && item == FILE_QUIT) {
        ExitToShell();
    } else if (menu == MENU_EDIT) {
        SystemEdit((short)(item - 1)); /* a desk accessory's, if one is in front */
    } else if (menu == MENU_GAME && item == GAME_NEW) {
        gfx_flush_input();
        if (new_game_handler) new_game_handler(); /* does not return */
    }
}

static void handle_event(EventRecord *e) {
    WindowPtr w;
    short part;
    switch (e->what) {
    case mouseDown:
        part = FindWindow(e->where, &w);
        if (part == inMenuBar) {
            do_menu(MenuSelect(e->where));
        } else if (part == inSysWindow) {
            SystemClick(e, w);
        } else if (part == inDrag) {
            Rect limit = qd.screenBits.bounds;
            InsetRect(&limit, 4, 4);
            DragWindow(w, e->where, &limit);
        } else if (part == inContent && w == win) {
            if (w != FrontWindow()) {
                SelectWindow(w);
            } else {
                Point p = e->where;
                GrafPtr old;
                GetPort(&old);
                SetPort(win);
                GlobalToLocal(&p);
                SetPort(old);
                push_click(p.h, p.v);
            }
        }
        break;
    case keyDown:
    case autoKey: {
        char ch = (char)(e->message & charCodeMask);
        if (e->modifiers & cmdKey) {
            if (e->what == keyDown) do_menu(MenuKey(ch));
        } else {
            push_key((unsigned char)ch);
        }
        break;
    }
    case updateEvt:
        if ((WindowPtr)e->message == win) {
            BeginUpdate(win);
            copy_rows(0, GFX_H); /* clipped to what was uncovered */
            EndUpdate(win);
        }
        break;
    default:
        break;
    }
}

void wait_vsync(void) {
    EventRecord e;
    unsigned long t = TickCount();
    do {
        SystemTask();
        if (GetNextEvent(everyEvent, &e)) handle_event(&e);
    } while (TickCount() == t);
    if (dirty_top < dirty_bot) present();
}

void gfx_begin(void) {
    if (color) {
        GetGWorld(&saved_cport, &saved_dev);
        SetGWorld(off_gw, NULL);
    } else {
        GetPort(&saved_port);
        SetPort(&off_port);
    }
}

void gfx_end(short top, short bottom) {
    if (top < dirty_top) dirty_top = top;
    if (bottom > dirty_bot) dirty_bot = bottom;
    if (color) SetGWorld(saved_cport, saved_dev);
    else SetPort(saved_port);
}

/* True when this Mac can and should draw in colour: see the top of the
   file. Gestalt itself only exists from System 6.0.4, so its trap is
   checked for before it is called. */
static unsigned char want_color(void) {
    SysEnvRec env;
    long qdv;
    GDHandle dev;
    if (SysEnvirons(1, &env) != noErr || !env.hasColorQD) return 0;
    if (GetOSTrapAddress(_Gestalt) == GetToolTrapAddress(_Unimplemented)) return 0;
    if (Gestalt(gestaltQuickdrawVersion, &qdv) != noErr || qdv < gestalt32BitQD) return 0;
    dev = GetMainDevice();
    return (unsigned char)((*(*dev)->gdPMap)->pixelSize >= 4);
}

void gfx_init(void) {
    Rect wr, sb;
    short sw, sh, left, top;

    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
    FlushEvents(everyEvent, 0);

    setup_menus();

    /* Centre the window below the menu bar. On a Plus (512x342) this leaves
       just room for the title bar; bigger screens get margins. */
    sb = qd.screenBits.bounds;
    sw = sb.right - sb.left;
    sh = sb.bottom - sb.top;
    left = (short)((sw - GFX_W) / 2);
    top = (short)(20 + 18 + (sh - 20 - 18 - GFX_H) / 2);
    SetRect(&wr, left, top, left + GFX_W, top + GFX_H);
    color = want_color();
    if (color) {
        Rect b;
        SetRect(&b, 0, 0, GFX_W, GFX_H);
        if (NewGWorld(&off_gw, 0, &b, NULL, NULL, 0) != noErr ||
            !LockPixels(off_gw->portPixMap)) {
            color = 0; /* not enough memory for it: fall back to 1-bit */
        }
    }
    if (color) {
        win = NewCWindow(NULL, &wr, (ConstStringPtr)"\pUNO", true, noGrowDocProc,
                         (WindowPtr)-1, false, 0);
        gfx_begin();
        EraseRect(&off_gw->portRect);
        gfx_end(0, 0);
        SetPort(win);
        return;
    }
    win = NewWindow(NULL, &wr, (ConstStringPtr)"\pUNO", true, noGrowDocProc,
                    (WindowPtr)-1, false, 0);

    /* The offscreen port: a GrafPort whose bits are our own buffer. */
    off_bits.rowBytes = ((GFX_W + 15) / 16) * 2;
    off_bits.baseAddr = NewPtrClear((long)off_bits.rowBytes * GFX_H);
    SetRect(&off_bits.bounds, 0, 0, GFX_W, GFX_H);
    OpenPort(&off_port);
    SetPortBits(&off_bits);
    PortSize(GFX_W, GFX_H);
    RectRgn(off_port.visRgn, &off_bits.bounds);
    ClipRect(&off_bits.bounds);
    EraseRect(&off_bits.bounds);

    SetPort(win);
}
