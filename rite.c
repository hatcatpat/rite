#include "rite.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MEMORY_PADDING 0x10

#define HI_FG TERM_RED
#define HI_BG TERM_WHITE

void
vector_init (vector_t *vec, int itemsize, int pad)
{
    memset (vec, 0, sizeof (vector_t));
    vec->pad = pad;
    vec->itemsize = itemsize;
}

void
vector_deinit (vector_t *vec)
{
    if (vec->data != NULL)
        free (vec->data);
    memset (vec, 0, sizeof (vector_t));
}

void *
vector_get (vector_t *vec, int i)
{
    if (vec->data == NULL || i < 0 || i >= vec->len)
        return NULL;
    else
        return vec->data + vec->itemsize * i;
}

void *
vector_head (vector_t *vec)
{
    if (vec->data == NULL)
        return NULL;
    else
        return vec->data;
}

void *
vector_tail (vector_t *vec)
{
    if (vec->data == NULL)
        return NULL;
    else
        return vec->data + vec->itemsize * (vec->len - 1);
}

void
vector_resize (vector_t *vec)
{
    if (vec->data != NULL && vec->len <= 0 && vec->size >= 0)
        {
            free (vec->data);
            vec->data = NULL;
            vec->len = vec->size = 0;
        }
    else if (vec->size < vec->len || vec->len < vec->size - vec->pad)
        {
            vec->size = vec->pad * (1 + vec->len / vec->pad);
            vec->data = realloc (vec->data, vec->size * vec->itemsize);
        }
}

void *
vector_append (vector_t *vec)
{
    vec->len++;
    vector_resize (vec);
    return vector_tail (vec);
}

void *
vector_prepend (vector_t *vec)
{
    vec->len++;
    vector_resize (vec);
    memmove (vec->data + vec->itemsize, vec->data,
             vec->itemsize * (vec->len - 1));
    return vector_head (vec);
}

void *
vector_insert (vector_t *vec, int i)
{
    if (i <= 0)
        return vector_prepend (vec);
    else if (i >= vec->len)
        return vector_append (vec);
    else
        {
            vec->len++;
            vector_resize (vec);
            memmove (vec->data + vec->itemsize * (i + 1),
                     vec->data + vec->itemsize * i,
                     vec->itemsize * (vec->len - 1 - i));
            return vector_get (vec, i);
        }
}

void
vector_remove (vector_t *vec, int i)
{
    if (vec->len)
        {
            if (i <= 0)
                memmove (vec->data, vec->data + vec->itemsize,
                         vec->itemsize * (vec->len - 1));
            else if (i < vec->len)
                memmove (vec->data + vec->itemsize * i,
                         vec->data + vec->itemsize * (i + 1),
                         vec->itemsize * (vec->len - 1 - i));
            vec->len--;
            vector_resize (vec);
        }
}

void
vector_split (vector_t *dest, vector_t *src, int i)
{
    int len = dest->len;
    if (i <= 0)
        {
            dest->len += src->len;
            vector_resize (dest);
            memcpy (vector_get (dest, len), src->data,
                    src->itemsize * src->len);
            src->len = 0;
            vector_resize (src);
        }
    else if (i < src->len)
        {
            dest->len += src->len - i;
            vector_resize (dest);
            memcpy (vector_get (dest, len), vector_get (src, i),
                    src->itemsize * (src->len - i));
            src->len = i;
            vector_resize (src);
        }
}

void
vector_join (vector_t *dest, vector_t *src)
{
    int len = dest->len;
    dest->len += src->len;
    vector_resize (dest);
    memcpy (vector_get (dest, len), vector_head (src),
            src->itemsize * src->len);
    vector_deinit (src);
}

rite_t rite;

int
main (int argc, char *argv[])
{
    event_t evt;

    if (term_init ())
        return -1;

    rite.page = malloc (sizeof (page_t));
    page_read (rite.page, "test");
    move_row (rite.row), move_col (rite.col);
    term_cursor_show (false);

    status ("howdy!");
    draw ();
    while (term_poll (&evt))
        {
            switch (evt.type)
                {
                case RITE_EVENT_KEY:
                    if (evt.u.k >= 128)
                        {
                            evt.u.k -= 128;
                            switch (evt.u.k)
                                {
                                case RITE_KEY_UP:
                                    move_row (rite.row - 1);
                                    break;
                                case RITE_KEY_DOWN:
                                    move_row (rite.row + 1);
                                    break;
                                case RITE_KEY_LEFT:
                                    if (RITE_MOD_GET (evt.mods, RITE_MOD_CTRL))
                                        jump_back ();
                                    else
                                        move_col (rite.col - 1);
                                    break;
                                case RITE_KEY_RIGHT:
                                    if (RITE_MOD_GET (evt.mods, RITE_MOD_CTRL))
                                        jump_forward ();
                                    else
                                        move_col (rite.col + 1);
                                    break;
                                case RITE_KEY_BACKSPACE:
                                    erase ();
                                    break;
                                case RITE_KEY_END:
                                    end ();
                                    break;
                                case RITE_KEY_HOME:
                                    home ();
                                    break;
                                case RITE_KEY_ENTER:
                                    enter ();
                                    break;
                                }
                        }
                    else if (evt.u.k == 's'
                             && RITE_MOD_GET (evt.mods, RITE_MOD_CTRL))
                        page_write (rite.page);
                    else if (isprint (evt.u.k))
                        type (evt.u.k);
                    break;

                default:
                    break;
                }

            draw ();
            status (NULL);
        }

    term_cursor_show (true);
    page_deinit (rite.page);
    free (rite.page);
    term_deinit ();
    return 0;
}

void
line_init (line_t *line)
{
    memset (line, 0, sizeof (line_t));
    vector_init (&line->text, sizeof (char), LINE_MEMORY_PADDING);
}

void
line_deinit (line_t *line)
{
    vector_deinit (&line->text);
    memset (line, 0, sizeof (line_t));
}

void
line_read (page_t *page, line_t *line, int start, int end)
{
    line->text.len = end - start + 1;

    if (line->text.len > 1)
        {
            vector_resize (&line->text);
            strncpy (line->text.data, page->text + start, line->text.len - 1);
            line_print (line);
        }
    else
        line->text.len = 0;
}

void
line_print (line_t *line)
{
    if (line->text.data == NULL)
        printf ("[cols: %i, size: %i] = %s\n", line->text.len - 1,
                line->text.size, "(NULL)");
    else
        printf ("[cols: %i, size: %i] = \"%s\", %i\n", line->text.len - 1,
                line->text.size, (char *)vector_head (&line->text),
                *(char *)vector_tail (&line->text));
}

void
page_init (page_t *page)
{
    memset (page, 0, sizeof (page_t));
}

void
page_deinit (page_t *page)
{
    int i;
    for (i = 0; i < page->lines.len; ++i)
        line_deinit (vector_get (&page->lines, i));
    vector_deinit (&page->lines);

    if (page->name != NULL)
        free (page->name);

    if (page->text != NULL)
        free (page->text);

    memset (page, 0, sizeof (page_t));
}

int
page_read (page_t *page, char *filename)
{
    int len = 0;
    FILE *f = fopen (filename, "r");
    if (f == NULL)
        return -1;

    page_init (page);

    page->name = malloc (strlen (filename) + 1);
    strncpy (page->name, filename, strlen (filename));
    page->name[strlen (filename)] = '\0';

    printf ("page: \"%s\"\n", page->name);

    fseek (f, 0, SEEK_END);
    len = ftell (f);
    fseek (f, 0, SEEK_SET);

    if (len)
        {
            int i, start;

            page->text = malloc (len);
            fread (page->text, 1, len, f);

            vector_init (&page->lines, sizeof (line_t), 0x10);

            for (i = 0, start = 0; i < len; ++i)
                if (page->text[i] == '\n')
                    {
                        line_t *l = vector_append (&page->lines);
                        if (l != NULL)
                            {
                                line_init (l);
                                line_read (page, l, start, i);
                            }
                        start = i + 1;
                    }
        }

    fclose (f);
    return 0;
}

int
page_write (page_t *page)
{
    int i;
    FILE *f;
    if (page->dirty == false || page->name == NULL || page->lines.data == NULL)
        return -1;

    f = fopen (page->name, "w");
    if (f == NULL)
        return -1;

    for (i = 0; i < page->lines.len; ++i)
        {
            line_t *l = vector_get (&page->lines, i);
            if (l->text.data != NULL)
                fputs (vector_head (&l->text), f);
            fputc ('\n', f);
        }

    page->dirty = false;

    fclose (f);
    return 0;
}

void
type (char c)
{
    char *ptr;
    line_t *l = vector_get (&rite.page->lines, rite.row);
    if (l == NULL)
        return;

    if (l->text.data == NULL)
        {
            ptr = vector_append (&l->text);
            *ptr = c;
            ptr = vector_append (&l->text);
            *ptr = '\0';
        }
    else
        {
            if (rite.col >= l->text.len - 1)
                {
                    ptr = vector_tail (&l->text);
                    *ptr = c;
                    ptr = vector_append (&l->text);
                    *ptr = '\0';
                }
            else
                {
                    ptr = vector_insert (&l->text, rite.col);
                    *ptr = c;
                }
        }

    rite.page->dirty = true;
    move_col (rite.col + 1);
}

void
move_row (int row)
{
    if (rite.page->lines.data != NULL)
        {
            rite.row = CLAMP (row, 0, MAX (rite.page->lines.len - 1, 0));
            move_col (rite.col);
        }
    else
        rite.row = 0;
}

void
move_col (int col)
{
    line_t *l = vector_get (&rite.page->lines, rite.row);
    if (l != NULL)
        rite.col = CLAMP (col, 0, MAX (l->text.len - 1, 0));
}

void
end ()
{
    line_t *l = vector_get (&rite.page->lines, rite.row);
    if (l != NULL)
        move_col (l->text.len - 1);
}

void
home ()
{
    move_col (0);
}

void
erase ()
{
    line_t *curr = vector_get (&rite.page->lines, rite.row);
    if (curr == NULL)
        return;

    if (curr->text.data == NULL && rite.row > 0)
        {
            line_deinit (curr);
            vector_remove (&rite.page->lines, rite.row);
            move_row (rite.row - 1);
            end ();
        }
    else if (rite.col == 0)
        {
            line_t *prev = vector_get (&rite.page->lines, rite.row - 1);
            if (prev != NULL)
                {
                    int len = prev->text.len;
                    vector_remove (&prev->text, prev->text.len);
                    vector_join (&prev->text, &curr->text);
                    line_deinit (curr);
                    vector_remove (&rite.page->lines, rite.row);
                    move_row (rite.row - 1);
                    move_col (rite.col + len - 1);
                }
        }
    else
        {
            vector_remove (&curr->text, rite.col - 1);
            if (curr->text.len == 1)
                vector_remove (&curr->text, 0);
            move_col (rite.col - 1);
        }
}

void
enter ()
{
    line_t *prev, *new;

    if (rite.col == 0)
        {
            if ((new = vector_insert (&rite.page->lines, rite.row)) == NULL)
                return;

            line_init (new);
        }
    else
        {
            if ((new = vector_insert (&rite.page->lines, rite.row + 1))
                == NULL)
                return;

            line_init (new);

            if ((prev = vector_get (&rite.page->lines, rite.row)) != NULL)
                {
                    if (rite.col < prev->text.len - 1)
                        {
                            char *c;
                            vector_split (&new->text, &prev->text, rite.col);
                            c = vector_append (&prev->text);
                            *c = '\0';
                        }
                }
        }

    move_col (0);
    move_row (rite.row + 1);
}

void
jump_forward ()
{
    line_t *l;
    char c;

    if ((l = vector_get (&rite.page->lines, rite.row)) == NULL)
        return;

    if (l->text.data == NULL)
        return;

    if (rite.col == l->text.len - 1)
        return;

    c = *(char *)vector_get (&l->text, rite.col);

    if (c == ' ')
        {
            while (rite.col < l->text.len - 1)
                if (*(char *)vector_get (&l->text, ++rite.col) != ' ')
                    break;
        }
    else
        {
            while (rite.col < l->text.len - 1)
                if (*(char *)vector_get (&l->text, ++rite.col) == ' ')
                    break;
        }
}

void
jump_back ()
{
    line_t *l;
    char c;

    if (rite.col == 0)
        return;

    if ((l = vector_get (&rite.page->lines, rite.row)) == NULL)
        return;

    if (l->text.data == NULL)
        return;

    c = *(char *)vector_get (&l->text, rite.col - 1);

    if (c == ' ')
        {
            while (rite.col > 0)
                if (*(char *)vector_get (&l->text, rite.col - 1) != ' ')
                    break;
                else
                    rite.col--;
        }
    else
        {
            while (rite.col > 0)
                if (*(char *)vector_get (&l->text, rite.col - 1) == ' ')
                    break;
                else
                    rite.col--;
        }
}

void
status (char *str)
{
    if (str == NULL)
        rite.status[0] = '\0';
    else
        strncpy (rite.status, str, 64);
}

void
draw_ui ()
{
    static char fmt[] = "[%02x_%02x] :: %s%c";

    term_clear (), term_cursor_reset ();

    if (rite.page == NULL)
        return;

    term_bold (true);
    term_underline (true);

    printf (fmt, rite.col, rite.row, rite.page->name,
            rite.page->dirty ? '+' : ' ');

    /* { */
    /*     int i; */
    /*     for (i = 0; i < term_cols + 7 */
    /*                         - (strlen (fmt) */
    /*                            + (rite.page->name == NULL */
    /*                                   ? 6 */
    /*                                   : strlen (rite.page->name))); */
    /*          ++i) */
    /*         putchar (' '); */
    /* } */
    putchar ('\n');

    term_normal ();
}

void
draw_page ()
{
    if (rite.page == NULL)
        return;
    else
        {
            int i;
            for (i = 0; i < MIN (term_rows - 1, rite.page->lines.len); ++i)
                draw_line (vector_get (&rite.page->lines, i), i == rite.row);
            term_normal ();
        }
}

void
draw_line (line_t *line, bool hi)
{
    if (line->text.data != NULL)
        {
            char *str = vector_head (&line->text);

            printf ("[%02x,%02x]__", line->text.len - 1, line->text.size);

            if (hi)
                {
                    int i = MIN (line->text.len - 1, rite.col);
                    printf ("%.*s", i, str);
                    str += i;

                    term_fg (HI_FG), term_bg (HI_BG);
                    if (line->text.data == NULL || i == line->text.len - 1)
                        putchar (' ');
                    else
                        putchar (*str++);
                    term_fg (TERM_DEFAULT), term_bg (TERM_DEFAULT);
                }

            if (str != NULL)
                printf ("%s", str);

            if (line->text.data != NULL)
                {
                    char end;
                    if ((end = *(char *)vector_tail (&line->text)) != '\0')
                        {
                            term_fg (TERM_WHITE), term_bg (TERM_RED);
                            printf ("__%i", end);
                        }
                }
        }
    else
        {
            printf ("[NA,NA]__");
            if (hi)
                {
                    term_fg (HI_FG), term_bg (HI_BG);
                    putchar (' ');
                    term_fg (TERM_DEFAULT), term_bg (TERM_DEFAULT);
                }
        }

    putchar ('\n');
}

void
draw_status ()
{
    printf ("--------------------\n%s\n", rite.status);
}

void
draw ()
{
    draw_ui ();
    draw_page ();
    draw_status ();
}
