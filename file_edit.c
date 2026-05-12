#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fstypes.h"

typedef struct {
    char *data; /* line contents */
    int    len; /* strlen(data) cached */
    int    cap; /* single line's capacity; allocated bytes; always >= len + 1 */
} Line; /* for each line in the file */

typedef struct {
    Line *lines; /* array of lines */
    int n_lines; /* lines currently in use */
    int cap_lines; /* capacity of lines; lines allocated */
    int dirty; /* unsaved changes */
} Buffer;

int view_file(char *path);
static void line_reserve(Line *l, int need);
static void buffer_reserve(Buffer *b, int need);
Buffer *buffer_load(const char *path);

void buffer_insert_char(Buffer *b, int row, int col, char c);
void buffer_delete_char(Buffer *b, int row, int col);
void buffer_split_line(Buffer *b, int row, int col);
void buffer_join_lines(Buffer *b, int row);

int main(void) {
    char *path = "/Volumes/HomeXx/compuir/test.txt";
 
    initscr();
    // mvprintw(0, 0, "cursor visibility: %d", curs_set(1));
    // refresh();
    // int ch = getch();
    // while (ch != 100) { continue; }
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(2);
 
    view_file(path);
    endwin();
    return 0;
}

int view_file(char *path) {
    // FILE *f = fopen(path, "r");
    // if (!f) { return 0; }
    Buffer *b = buffer_load(path);

    int max_line = 0;
    int n_lines = b->n_lines;
    for (int i = 0; i < n_lines; i++) {
        if (b->lines[i].len > max_line) { max_line = b->lines[i].len; }
    }

    // curs_set(2);

    int screen_h, screen_w;
    getmaxyx(stdscr, screen_h, screen_w);

    /* make pad atleast size of window incase file doesn't fill */
    /* if condition ? expression if true : expression if false; */
    int pad_w = (max_line > screen_w) ? max_line + 1 : screen_w;
    WINDOW *pad = newpad(n_lines + 1, pad_w);
    keypad(pad, TRUE);

    // char line[4096];
    // int row = 0;
    // while (fgets(line, sizeof(line), f)) {
        //     mvwprintw(pad, row, 0, "%s", line);
        //     row++;
    // }
    // fclose(f);
    /* read the buffer into the pad, line by line */
    int row = 0;
    for (int i = 0; i < n_lines; i++) {
        mvwprintw(pad, row, 0, "%s", b->lines[i].data);
        row++;
    }

    /* scrolling state : which row/column is in the top-left of the viewport */
    int pad_row = 0;
    int pad_col = 0;
    int curs_row = 0;
    int curs_col = 0;
    int view_h = screen_h - 1; /* room for status bar at the bottom */
    int view_w = screen_w;

    clear();
    refresh();

    int ch;
    while (1) {
        mvprintw(screen_h - 1, 0, "line %d/%d q to quit", curs_row + 1, n_lines);
        clrtoeol();
        refresh();

        wmove(pad, curs_row, curs_col);
        prefresh(pad, pad_row, pad_col, 0, 0, view_h - 1, view_w - 1);

        ch = wgetch(pad);
        switch(ch) {
            // case KEY_DOWN: pad_row++; break;
            // case KEY_UP: pad_row--; break;
            // case KEY_RIGHT: pad_col += 8; break;
            // case KEY_LEFT: pad_col -= 8; break;
            case KEY_DOWN: curs_row++; break;
            case KEY_UP: curs_row--; break;
            case KEY_RIGHT: curs_col++; break;
            case KEY_LEFT: curs_col--; break;
            case 'q': case 27: goto done;
        }
        // if (pad_row > n_lines - view_h) { pad_row = n_lines - view_h; }
        // if (pad_row < 0) { pad_row = 0; }
        // if (pad_col > pad_w - view_w) { pad_col = pad_w - view_w; }
        // if (pad_col < 0) { pad_col = 0; }

        /* clamp cursor to pad bounds */
        if (curs_row < 0) { curs_row = 0; }
        if (curs_row > n_lines - 1) { curs_row = n_lines - 1; }
        if (curs_col < 0) { curs_col = 0; }
        if (curs_col > b->lines[curs_row].len) { curs_col = b->lines[curs_row].len; }

        /* clamp viewport to cursor's posit */
        if (curs_row < pad_row) { pad_row = curs_row; }
        if (curs_row >= pad_row + view_h) { pad_row = curs_row - view_h + 1; }
        if (curs_col < pad_col) { pad_col = curs_col; }
        if (curs_col >= pad_col + view_w) { pad_col = curs_col - view_w + 1; }
    }
done:
    delwin(pad);
    return 1;
}

static void line_reserve(Line *l, int need) {
    if (l->cap >= need) { return; } /* if the lnes capacity is greater than the need, its already fine */
    int new_cap = l->cap ? l->cap : 16; /* if no current cap, set to 16 */
    while (new_cap < need) { new_cap *= 2; } /* if still inadequate, double until it is */
    l->data = realloc(l->data, new_cap); /* realloc the new line size */
    l->cap = new_cap; /* & update the line's capacity */
}

static void buffer_reserve(Buffer *b, int need) {
    if (b->cap_lines >= need) {return; } /* same idea here but for no. of lines in the Buffer */
    int new_cap = b->cap_lines ? b->cap_lines : 32;
    while (new_cap < need) { new_cap *= 2; }
    b->lines = realloc(b->lines, new_cap * sizeof(Line));
    b->cap_lines = new_cap;
}

Buffer *buffer_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { return NULL; }

    Buffer *b = calloc(1, sizeof(Buffer));
    buffer_reserve(b, 32);

    char chunk[4096];
    while (fgets(chunk, sizeof(chunk), f)) {
        /* the line returned could be truncated if greater than 4096 *
         * checking for the EOL terminator & growing the line resolves this */
        buffer_reserve(b, b->n_lines + 1); /* blondly ? */
        Line *l = &b->lines[b->n_lines]; /* get the last line's address ? */
        l->data = NULL;
        l->len = 0;
        l->cap = 0;

        /* for lines longer than 4096 (identified by having no new line terminator) */
        for (;;) { /* increase the line's reserve, update its len, and realloc more mem */
            int chunk_len = (int)strlen(chunk);
            int has_newline = (chunk_len > 0 && chunk[chunk_len - 1] == '\n');
            if (has_newline) { chunk_len--; } /* strip the trailing '\n' */

            line_reserve(l, l->len + chunk_len + 1); /* l->len is 0, no? */
            memcpy(l->data + l->len, chunk, chunk_len);
            l->len += chunk_len;
            l->data[l->len] = '\0'; /* NULL terminator */

            if (has_newline) { break; }
            if (!fgets(chunk, sizeof(chunk), f)) { break; }
        }
        b->n_lines++;
    }

    /* empty file - give it one line for the cursor */
    if (b->n_lines == 0) {
        buffer_reserve(b, 1);
        // b->lines[0] = (Line){ .data = calloc(1, 16), .len = 0, .cap = 16 };
        Line tmp_line;
        tmp_line.data = calloc(1, 16);
        tmp_line.len = 0;
        tmp_line.cap = 16;
        b->lines[0] = tmp_line;
        b->n_lines = 1;
    }

    // fclose(f);
    return b;
}

/* insert char 'c' into line 'row' at column 'col' *
 * precondition: 0<= row < n_lines, 0 <= col <= lines[row].len */
void buffer_insert_char(Buffer *b, int row, int col, char c) {
    Line *l = &b->lines[row];       /* 'id' the row being modified */
    line_reserve(l, l->len + 2);    /* + 1 for c, + 1 for \0 */
    memmove(l->data + col + 1,      /* shift tail right */
        l->data + col,              /* start of text to shift; posit. in line where 'c' will be inserted */
        l->len - col + 1);          /* + 1 copies the \0 as well */
    l->data[col] = c;               /* add the newly inserted char 'c' where there is now space */
    l->len++;                       /* add 1 to the length of the given row */
    b->dirty = 1;                   /* mark the changes as unsaved */
}

/* delete the char at (row, col)
 * precondition: 0 <= col < lines[row].len */
void buffer_delete_char(Buffer *b, int row, int col) {
    Line *l = &b->lines[row]; /* twin to above but inverse */
    memmove(l->data + col,
        l->data + col + 1,
        l->len - col);
    l->len--;
    b->dirty = 1;
}

/* split line: row at 'col'; text from 'col' on becomes a new line */
void buffer_split_line(Buffer *b, int row, int col) {
    buffer_reserve(b, b->n_lines + 1); /* add another line to the file's buffer (array of lines) */
    memmove(&b->lines[row + 2], /* shift lines below 'row' down one */
        &b->lines[row + 1],
        (b->n_lines - row - 1) * sizeof(Line));

    Line *src = &b->lines[row];
    Line *dst = &b->lines[row + 1];
    int tail_len = src->len - col; /* tail end is the current col - line length */

    dst->data = NULL;
    dst->len = 0;
    dst->cap = 0;

    line_reserve(dst, tail_len + 1); /* make room for the new tail-end of the original line */
    memcpy(dst->data, src->data + col, tail_len); /* copy the tail end in */
    dst->data[tail_len] = '\0';
    dst->len = tail_len; /* set it to its split length */

    src->data[col] = '\0'; /* add a null terminator at 'col' of the original line */
    src->len = col; /* set it to the length of the line remaining */

    b->n_lines++; /* add a line to the total count */
    b->dirty = 1; /* mark the unsaved changes */
}

void buffer_join_lines(Buffer *b, int row) {
    if (row + 1 >= b->n_lines) { return; }
    Line *l = &b->lines[row];
    Line *next = &b->lines[row + 1];

    line_reserve(l, l->len + next->len + 1); /* add row 1's length to row 0 */
    memcpy(l->data + l->len, next->data, next->len + 1); /* + 1 for \0 */
    l->len += next->len;

    free(next->data);

    /* shift lines above the gap down */
    memmove(&b->lines[row + 1],
        &b->lines[row + 2],
        (b->n_lines - row - 2) * sizeof(Line));
    b->n_lines--;
    b->dirty = 1;
}