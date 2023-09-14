#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define ABS(x) ((x) < 0 ? -(x) : (x))

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int int16_t;
typedef unsigned short int uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long int int64_t;
typedef unsigned long int uint64_t;
typedef unsigned int uint_t;

typedef unsigned char bool;
enum
{
    false,
    true
};

enum TERM_COLOR
{
    TERM_BLACK,
    TERM_RED,
    TERM_GREEN,
    TERM_YELLOW,
    TERM_BLUE,
    TERM_MAGENTA,
    TERM_CYAN,
    TERM_WHITE,
    TERM_DEFAULT = 9,
};

enum RITE_MOD
{
    RITE_MOD_SHIFT = 2,
    RITE_MOD_ALT,
    RITE_MOD_SHIFT_ALT,
    RITE_MOD_CTRL,
    RITE_MOD_SHIFT_CTRL,
    RITE_MOD_ALT_CTRL,
    RITE_MOD_SHIFT_ALT_CTRL,
    RITE_NUM_MODS
};
#define RITE_MOD_BIT(f) (1 << ((f)-RITE_MOD_SHIFT))
#define RITE_MOD_SET(m, f) ((m) |= RITE_MOD_BIT (f))
#define RITE_MOD_GET(m, f) ((m)&RITE_MOD_BIT (f))
#define RITE_MOD_CLEAR(m, f) ((m) &= ~(1 << RITE_MOD_BIT (f)))

enum RITE_KEY
{
    RITE_KEY_HOME,
    RITE_KEY_INSERT,
    RITE_KEY_DELETE,
    RITE_KEY_END,
    RITE_KEY_PGUP,
    RITE_KEY_PGDOWN,
    RITE_KEY_UP,
    RITE_KEY_DOWN,
    RITE_KEY_RIGHT,
    RITE_KEY_LEFT,
    RITE_NUM_KEYS,

    RITE_KEY_TAB,
    RITE_KEY_ENTER,
    RITE_KEY_BACKSPACE,
    RITE_NUM_KEYS_TOTAL
};

enum RITE_EVENT
{
    RITE_EVENT_NONE,
    RITE_EVENT_KEY,
    RITE_EVENT_MOUSE,
    RITE_EVENT_RESIZED,
    RITE_NUM_EVENTS
};

typedef struct
{
    void *data;
    int len, size, pad, itemsize;
} vector_t;

void vector_init (vector_t *vec, int itemsize, int pad);
void vector_deinit (vector_t *vec);
void *vector_get (vector_t *vec, int i);
void *vector_head (vector_t *vec);
void *vector_tail (vector_t *vec);
void vector_resize (vector_t *vec);
void *vector_append (vector_t *vec);
void *vector_prepend (vector_t *vec);
void *vector_insert (vector_t *vec, int i);
void vector_remove (vector_t *vec, int i);
void vector_split (vector_t *dest, vector_t *src, int i);
void vector_join (vector_t *dest, vector_t *src);

typedef struct
{
    union
    {
        struct
        {
            uint8_t b, x, y;
        } m;
        uint16_t k;
    } u;
    enum RITE_EVENT type;
    uint8_t mods;
} event_t;

typedef struct
{
    vector_t text;
} line_t;

typedef struct
{
    bool dirty;
    int len;
    char *text, *name;
    vector_t lines;
} page_t;

typedef struct
{
    int row, col;
    page_t *page;
    char status[64];
} rite_t;

/**/
/* rite.c */
/**/
extern rite_t rite;

void line_init (line_t *line);
void line_deinit (line_t *line);
void line_read (page_t *page, line_t *line, int start, int end);
void line_print (line_t *line);

void page_init (page_t *page);
void page_deinit (page_t *page);
int page_read (page_t *page, char *filename);
int page_write (page_t *page);

void type (char c);
void move_row (int row);
void move_col (int col);
void end ();
void home ();
void erase ();
void enter ();
void jump_forward ();
void jump_back ();

void status (char *str);

void draw_ui ();
void draw_page ();
void draw_line (line_t *line, bool hi);
void draw_status ();
void draw ();
/**/

/**/
/* term.c */
/**/
extern int term_rows, term_cols;
int term_init ();
void term_deinit ();

void term_altbuf (bool on);

void term_clear ();
void term_cursor_reset ();
void term_cursor_up (int i);
void term_cursor_up1 ();
void term_cursor_down (int i);
void term_cursor_left (int i);
void term_cursor_right (int i);
void term_cursor_move (int x, int y);
void term_cursor_restore ();
void term_cursor_save ();
void term_cursor_show (bool on);

void term_normal ();
void term_color (int c);
void term_fg (enum TERM_COLOR c);
void term_bg (enum TERM_COLOR c);
void term_bold (bool on);
void term_dim (bool on);
void term_italic (bool on);
void term_underline (bool on);
void term_blink (bool on);
void term_inverse (bool on);
void term_invisible (bool on);
void term_strikethrough (bool on);

char *term_modname (enum RITE_MOD mod);
char *term_eventname (enum RITE_EVENT evt);
char *term_keyname (enum RITE_KEY key);

int term_poll (event_t *evt);
/**/
