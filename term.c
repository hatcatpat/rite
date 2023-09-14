/* TODO */
/*
 *	[~] mouse support
 *		[] test on kitty
 * 	[] allow for customisable modprefix and mouseprefix/data
 *	[~] find terminal size
 *		[~] find out if terminal size has changed
 *	[] find terminal keymaps
 *	[x] refactor into events
 *	[x] refactor into "while(event = term_poll)"
 *
 * */

#include "rite.h"

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define DIGIT2CHAR(n) ('0' + (n))
#define CHAR2DIGIT(n) ((n) - '0')
#define IS_CTRL(c) (((c) & ~0x1f) == 0)
#define MAKE_CTRL(c) ((c)&0x1f)
#define STRIP_CTRL(c) ((c) | 0x60)

#define QUIT MAKE_CTRL ('q')

#define ESC "\x1b"
#define CSI ESC "["

void term_mouse (bool on);
void term_sig (int sig);

bool term_resized;
int term_rows, term_cols;
struct termios oldtermios, termios;

char *
term_keyname (enum RITE_KEY key)
{
    static char *keynames[RITE_NUM_KEYS_TOTAL] = {
        "HOME", "INSERT", "DELETE", "END", "PGUP", "PGDOWN", "UP",
        "DOWN", "RIGHT",  "LEFT",   NULL,  "TAB",  "ENTER",  "BACKSPACE",
    };
    return keynames[key];
}

char *
term_eventname (enum RITE_EVENT evt)
{
    static char *eventnames[RITE_NUM_EVENTS] = {
        "NONE",
        "KEY",
        "MOUSE",
        "RESIZED",
    };
    return eventnames[evt];
}

char *
term_modname (enum RITE_MOD mod)
{
    static char *modnames[RITE_NUM_MODS - RITE_MOD_SHIFT] = {
        "SHIFT",      "ALT",      "SHIFT_ALT",      "CTRL",
        "SHIFT_CTRL", "ALT_CTRL", "SHIFT_ALT_CTRL",
    };
    return modnames[mod];
}

#define RITE_NUM_REMAPS (RITE_NUM_KEYS_TOTAL - RITE_NUM_KEYS)
int remap[RITE_NUM_REMAPS] = { 0x9, 0xd, 0x7f };

char *keymap_default[RITE_NUM_KEYS] = {
    "\x1b[1~", "\x1b[2~", "\x1b[3~", "\x1b[4~", "\x1b[5~",
    "\x1b[6~", "\x1b[A",  "\x1b[B",  "\x1b[C",  "\x1b[D",
};

char *xterm[] = {
    "\x1b[H", NULL, NULL, "\x1b[F", NULL, NULL, NULL, NULL, NULL, NULL,
};

char *st[] = {
    "\x1b[H", "\x1b[4h", "\x1b[P", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

char **keymap;

char modprefix[] = CSI "1;";
char mouseprefix[] = CSI "M";
char mouseformat[] = "%c%c%c";

void
term_altbuf (bool on)
{
    if (on)
        printf (CSI "?1049h");
    else
        printf (CSI "?1049l");
}

void
term_clear ()
{
    printf (CSI "2J");
}

void
term_cursor_reset ()
{
    printf (CSI "H");
}

void
term_cursor_move (int x, int y)
{
    printf (CSI "%c;%cH", DIGIT2CHAR (x), DIGIT2CHAR (y));
}

void
term_cursor_up (int i)
{
    printf (CSI "%cA", DIGIT2CHAR (i));
}

void
term_cursor_up1 ()
{
    printf (ESC "M");
}

void
term_cursor_down (int i)
{
    printf (CSI "%cB", DIGIT2CHAR (i));
}

void
term_cursor_right (int i)
{
    printf (CSI "%cC", DIGIT2CHAR (i));
}

void
term_cursor_left (int i)
{
    printf (CSI "%cD", DIGIT2CHAR (i));
}

void
term_cursor_save ()
{
    printf (ESC "7");
}

void
term_cursor_restore ()
{
    printf (ESC "8");
}

void
term_cursor_show (bool on)
{
    if (on)
        printf (CSI "?25h");
    else
        printf (CSI "?25l");
}

void
term_mouse (bool on)
{
    if (on)
        printf (CSI "?9h");
    else
        printf (CSI "?9l");
    fflush (stdout);
}

void
term_normal ()
{
    printf (CSI "0m");
}

void
term_color (int c)
{
    printf (CSI "%im", c);
}

void
term_fg (enum TERM_COLOR c)
{
    term_color (c + 30);
}

void
term_bg (enum TERM_COLOR c)
{
    term_color (c + 40);
}

#define TEXT_STYLE(NAME, ON, OFF)                                             \
    void term_##NAME (bool on)                                                \
    {                                                                         \
        if (on)                                                               \
            printf (CSI #ON);                                                 \
        else                                                                  \
            printf (CSI #OFF);                                                \
    }

TEXT_STYLE (bold, 1m, 22m);
TEXT_STYLE (dim, 2m, 23m);
TEXT_STYLE (italic, 3m, 24m);
TEXT_STYLE (underline, 4m, 25m);
TEXT_STYLE (blink, 5m, 26m);
TEXT_STYLE (inverse, 7m, 27m);
TEXT_STYLE (invisible, 8m, 28m);
TEXT_STYLE (strikethrough, 9m, 29m);

#undef TEXT_STYLE

void
term_sig (int sig)
{
    if (sig == SIGWINCH)
        {
            struct winsize w;
            ioctl (1, TIOCGWINSZ, &w);
            term_rows = w.ws_row, term_cols = w.ws_col;
            term_resized = true;
        }
}

int
term_init ()
{
    struct winsize w;
    ioctl (1, TIOCGWINSZ, &w);
    term_rows = w.ws_row, term_cols = w.ws_col;

    signal (SIGWINCH, term_sig);

    tcgetattr (0, &oldtermios);
    termios = oldtermios;
    termios.c_iflag
        &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    termios.c_cflag &= ~(CSIZE | PARENB);
    termios.c_cflag |= CS8;
    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tcsetattr (0, TCSANOW, &termios);

    term_altbuf (true);

    term_normal ();
    term_mouse (true);

    keymap = st;
    return 0;
}

void
term_deinit ()
{
    term_mouse (false);
    tcsetattr (0, TCSANOW, &oldtermios);
    fflush (stdout);
    term_altbuf (false);
}

int
term_poll (event_t *evt)
{
    static char str[16];
    static int i, len;

    memset (evt, 0, sizeof (event_t));

    if ((len = read (STDIN_FILENO, &str, 16)) <= 0)
        {
            if (term_resized)
                {
                    term_resized = false;
                    evt->type = RITE_EVENT_RESIZED;
                }
            return 1;
        }

    /* for (i = 0; i < len; ++i) */
    /*     printf ("[%i]\n", str[i]); */

    if (str[0] == QUIT)
        return 0;

    switch (len)
        {
        case 2:
            if (str[0] == ESC[0])
                {
                    RITE_MOD_SET (evt->mods, RITE_MOD_ALT);
                    str[0] = str[1];
                    len = 1;
                }
        case 1:
            {
                int j;
                for (j = 0; j < RITE_NUM_REMAPS; ++j)
                    if (str[0] == remap[j])
                        {
                            evt->type = RITE_EVENT_KEY;
                            evt->u.k = RITE_NUM_KEYS + 1 + j + 128;
                            return 1;
                        }

                if (isupper (str[0]))
                    RITE_MOD_SET (evt->mods, RITE_MOD_SHIFT);

                if (IS_CTRL (str[0]))
                    {
                        RITE_MOD_SET (evt->mods, RITE_MOD_CTRL);
                        str[0] = STRIP_CTRL (str[0]);
                    }

                evt->type = RITE_EVENT_KEY;
                evt->u.k = str[0];
                return 1;
            }
            break;
        default:
            if (strncmp (str, mouseprefix, strlen (mouseprefix)) == 0)
                {
                    uint8_t b = 0, x = 0, y = 0;
                    if ((sscanf (&str[strlen (mouseprefix)], mouseformat, &b,
                                 &x, &y))
                        == 3)
                        {
                            evt->type = RITE_EVENT_MOUSE;
                            evt->u.m.b = b - 32;
                            evt->u.m.x = y - 32;
                            evt->u.m.x = y - 32;
                            return 1;
                        }
                    break;
                }

            if (strncmp (str, modprefix, strlen (modprefix)) == 0)
                {
                    RITE_MOD_SET (evt->mods,
                                  CHAR2DIGIT (str[strlen (modprefix)]));
                    len -= strlen (modprefix) - 1;
                    for (i = 2; i < len; ++i)
                        str[i] = str[i + strlen (modprefix) + 1 - 2];
                }

            for (i = 0; i < RITE_NUM_KEYS; ++i)
                {
                    char *k
                        = (keymap[i] == NULL ? keymap_default[i] : keymap[i]);
                    if (strncmp (str, k, len) == 0)
                        {
                            evt->type = RITE_EVENT_KEY;
                            evt->u.k = i + 128;
                            return 1;
                        }
                }
            break;
        }

    return 1;
}
