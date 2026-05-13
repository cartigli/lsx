#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>

#include "fstypes.h"

typedef struct {
    char *data; /* line contents                                              */
    int    len; /* strlen(data) cached                                        */
    int    cap; /* single line's capacity; allocated bytes; always >= len + 1 */
} Line;         /* for each line in the file                                  */

typedef struct {
    Line   *lines; /* array of lines                     */
    int   n_lines; /* lines currently in use             */
    int cap_lines; /* capacity of lines; lines allocated */
    int     dirty; /* unsaved changes                    */
} Buffer;

typedef struct v_class {
    char *exp; /* RegEx expression */
    int pair;  /* color pair */
    regex_t regxx;
} v_class;

typedef struct {
    int n_exps;         /* no. of expressions stored */
    v_class* v_classes; /* pointer to the array of v_class structs */
} Expressions;

typedef struct {
    int max_line;
    int n_lines;
    int pad_row;
    int pad_col;
    int curs_row;
    int curs_col;
    int screen_h;
    int screen_w;
    int view_h;
    int view_w;
    int wo;
} active_state;

// typedef struct {        /* aspects of the altered buffer */
//     int a milli billi; /* one million billion */
// } State;

int alter_file(Buffer *b, Expressions *exps);

static void line_reserve(Line *l, int need);
static void buffer_reserve(Buffer *b, int need);
Buffer *buffer_load(const char *path);

void buffer_insert_char(Buffer *b, int row, int col, char c);
void buffer_delete_char(Buffer *b, int row, int col);
void buffer_split_line(Buffer *b, int row, int col);
void buffer_join_lines(Buffer *b, int row);
WINDOW* grow_pad(WINDOW* pad, Buffer *b, int screen_w);

Expressions *init_regex(void);

void regex_color(WINDOW* pad, int row, char *line, regex_t *regxx, int pair);

int buffer_writeout(Buffer *b, const char *path);
void mfree(Buffer *b, Expressions *exps);

int main(void) {
    char *path = "/Volumes/HomeXx/compuir/lsx/test.txt";
    // char *path = "/home/t/lsx/test.txt";
    Buffer *b = buffer_load(path);
    if (!b) { return 1; }

    initscr();
    if (has_colors()) {
        start_color();
        /* args: pair no, foreground, background */
        init_pair(1, COLOR_RED, COLOR_BLACK);  /* letters */
        init_pair(2, COLOR_CYAN, COLOR_BLACK); /* digits  */
    }
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    Expressions *exps = init_regex();

    int xr = alter_file(b, exps);
    endwin();
    if (xr == 1) { return 1; }
    if (xr == -1) {
        if (buffer_writeout(b, path)) { return 1; }
    }
    if (xr == -1) { printf("wrote buffer to disk (OK)\n"); }

    mfree(b, exps);
    
    return 0;
}

Expressions *init_regex(void) {
    Expressions *exps = malloc(sizeof(Expressions));
    // if not ...
    exps->n_exps = 8;
    exps->v_classes = malloc(sizeof(v_class) * exps->n_exps);
    // if not ...
    exps->v_classes[0].pair = 1; /* functions */
    exps->v_classes[0].exp = "([^[:space:]()]+)\\(";

    exps->v_classes[1].pair = 2; /* operands */
    exps->v_classes[1].exp = "[*/\\<>%=^+-]";

    exps->v_classes[2].pair = 2; /* strings */
    exps->v_classes[2].exp = "\"([^\"]*)\"";

    exps->v_classes[3].pair = 1; /* preprocessors */
    exps->v_classes[3].exp = "#[a-zA-Z_]+";

    exps->v_classes[4].pair = 1; /* <headers.h> */
    exps->v_classes[4].exp = "<([^>]*)>";

    exps->v_classes[5].pair = 1; /* keywords (numerical) */
    exps->v_classes[5].exp = "\b(int|float|double|unsigned|long|char|NULL|void)\b";

    exps->v_classes[6].pair = 1; /* keywords (data flow */
    exps->v_classes[6].exp = "\b(return|if|while|for)\b";

    exps->v_classes[7].pair = 2; /* puncuation */
    exps->v_classes[7].exp = "[].,!?:;'[{}()]";

    for (int i = 0; i < exps->n_exps; i++) {
        if (regcomp(&exps->v_classes[i].regxx, exps->v_classes[i].exp, REG_EXTENDED) != 0) {
            continue;
        }
    }

    return exps;
}

int alter_file(Buffer *b, Expressions *exps) {
    int max_line = 0;
    // active_state.max_line = 0;
    int n_lines = b->n_lines;
    // active_state.n_lines = b->n_lines;
    for (int i = 0; i < n_lines; i++) {
        if (b->lines[i].len > max_line) {
            max_line = b->lines[i].len;
        }
    }

    /* get current screen dimensions */
    int screen_h, screen_w;
    getmaxyx(stdscr, screen_h, screen_w);

    /* make pad atleast size of window incase file doesn't fill *
     * if condition ? expression if true : expression if false; */
    int pad_w = (max_line > screen_w) ? max_line + 1 : screen_w;
    WINDOW *pad = newpad(n_lines + 1, pad_w);
    if (pad == NULL) { return 1; }
    keypad(pad, TRUE);

    /* read the buffer into the pad, line by line */
    int row = 0;
    for (int i = 0; i < n_lines; i++) {
        mvwprintw(pad, row, 0, "%s", b->lines[i].data);
        row++;
    }

    /* making into a typedef struct */
    /* scrolling state : which row/column is in the top-left of the viewport */
    int pad_row = 0;
    int pad_col = 0;
    int curs_row = 0;
    int curs_col = 0;
    int view_h = screen_h - 1; /* room for status bar at the bottom */
    int view_w = screen_w;
    // active_state.pad_row = 0;
    // active_state.pad_col = 0;
    // active_state.curs_row = 0;
    // active_state.curs_col = 0;
    // active_state.view_h = active_state.screen_h - 1;
    // active_State.view_w = active_state.screen_w;

    clear();
    refresh();

    int wo = 0;

    int ch;
    while (1) {
        mvprintw(screen_h - 1, 0, "line %d:%d esc to quit", curs_row + 1, n_lines);
        clrtoeol();
        refresh();

        pad = grow_pad(pad, b, screen_w);
        if (!pad) { return 1; }

        werase(pad);

        for (int i = 0; i < b->n_lines; i++) {
            mvwprintw(pad, i, 0, "%s", b->lines[i].data);
        }

        for (int e = 0; e < exps->n_exps; e++) {
            // if (regcomp(&regx, exps.v_classes[e].exp, REG_EXTENDED) != 0) { continue; }
            for (int l = 0; l < b->n_lines; l++) {
                regex_color(pad, l, b->lines[l].data, &exps->v_classes[e].regxx, exps->v_classes[e].pair);
            }
        }

        // /* instead of processing line by line, char by char let's us highlight specific chars */
        // for (int i = 0; i < b->n_lines; i++) {
        //     wmove(pad, i, 0); /* move window to new row's first column */
        //     for (int n = 0, x = b->lines[i].len; n < x; n++) {
        //         int pair = 0;
        //         unsigned char c = b->lines[i].data[n];
        //         if (c >= '0' && c <= '9') { pair = 2; }
        //         if (pair) { wattron(pad, COLOR_PAIR(pair)); }
        //         waddch(pad, c);
        //         if (pair) {wattroff(pad, COLOR_PAIR(pair)); }
        //     }
        // }

        if (curs_row < pad_row)           pad_row = curs_row;
        if (curs_row >= pad_row + view_h) pad_row = curs_row - view_h + 1;
        if (curs_col < pad_col)           pad_col = curs_col;
        if (curs_col >= pad_col + view_w) pad_col = curs_col - view_w + 1;

        wmove(pad, curs_row, curs_col);
        prefresh(pad, pad_row, pad_col, 0, 0, view_h - 1, view_w - 1);

        ch = wgetch(pad);

        if (ch >= 32 && ch < 127) { /* printable ASCII */
            buffer_insert_char(b, curs_row, curs_col, (char)ch);
            curs_col++;
        } else if (ch == '\n' || ch == KEY_ENTER || ch == 13 || ch == 10) {
            buffer_split_line(b, curs_row, curs_col);
            curs_row++;
            curs_col = 0;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (curs_col > 0) {
                buffer_delete_char(b, curs_row, curs_col - 1);
                curs_col--;
            } else if (curs_row > 0) {
                int prev_len = b->lines[curs_row - 1].len;
                buffer_join_lines(b, curs_row - 1);
                curs_row--;
                curs_col = prev_len;  /* land at join point */
            }
        // } else if (ch == KEY_DC) { /* forward delete : no - 1 compensation */
        //     if (curs_col < b->lines[curs_row].len) {
        //         buffer_delete_char(b, curs_row, curs_col);
        //     } else if (curs_row + 1 < b->n_lines) {
        //         buffer_join_lines(b, curs_row);
        //     }
        } else if (ch == KEY_LEFT) {
            if (curs_col > 0) {                      /* if the cursors not at col 0:  */
                curs_col--;
            } else if (curs_row > 0) {               /* if it is, and isn't at row 0: */
                curs_row--;
                curs_col = b->lines[curs_row].len;   /* move to EOL of previous row   */
            }
        } else if (ch == KEY_RIGHT) {
            if (curs_col < b->lines[curs_row].len) { /* if the cursors at row less than the current line's length */
                curs_col++;
            } else if (curs_row + 1 < b->n_lines) {  /* otherwise, and isn't at the last line: */
                curs_row++; 
                curs_col = 0;                        /* move to beg. of line */
            }
        } else if (ch == KEY_UP && curs_row > 0) {
            curs_row--;                              /* (below) if the cursors last col was greater    */
            if (curs_col > b->lines[curs_row].len) { /* than the new line's length, move it to the EOL */
                curs_col = b->lines[curs_row].len;
            }
        } else if (ch == KEY_DOWN && curs_row + 1 < b->n_lines) {
            curs_row++;
            if (curs_col > b->lines[curs_row].len) {
                curs_col = b->lines[curs_row].len;
            }
        } else if (ch >= 1 && ch <= 26) {            /* ctrl + <char> cases */
            /* ctrl + O (love for Nano) */
            if (ch == 15) {
                wo = 1;
                goto done;
            /* ctrl + X */
            } else if (ch == 24) { goto done; }
        }
        else if (ch == 27) {                         /* esc */
            goto done;
        } else if (ch == KEY_RESIZE) {
            getmaxyx(stdscr, screen_h, screen_w);
            view_h = screen_h - 1;
            view_w = screen_w;
            clear();
            refresh();
        }
    }
done:
    delwin(pad);
    if (wo) { return -1; }
    return 0;
}

void mfree(Buffer *b, Expressions *exps) {
    for (int i = 0; i < b->n_lines; i++) {
        free(b->lines[i].data);
    }
    free(b->lines);
    free(b);
    for (int e = 0; e < exps->n_exps; e++) {
        regfree(&exps->v_classes[e].regxx);
    }
    free(exps->v_classes);
    free(exps);
}

void regex_color(WINDOW* pad, int row, char *line, regex_t *regxx, int pair) {
// void regex_color(WINDOW* pad, int row, char *line) {
    // regex_t regx;
    regmatch_t match;
    // char *exp = "([^[:space:]()]+)\\(";                                /* functions (needs kill_attr - 1 to dehighlight the initial: ( */
    // char *exp = "[*/\\<>%=^+-]";                                            /* operands */
    // char *exp = "\"([^\"]*)\"";                                        /* strings */
    // char *exp = "#[a-zA-Z_]+";                                         /* preprocessors */
    // char *exp = "<([^>]*)>";                                           /* <headers.h> */
    // char *exp = "\b(int|float|double|unsigned|long|char|NULL|void)\b"; /* keywords (numerical) */
    // char *exp = "\b(return|if|while|for)\b";                           /* keywords (flow control) */
    // char *exp = "[].,!?:;'[{}()]";                                     /* punctuation */
    // if (regcomp(&regx, exp, REG_EXTENDED) != 0) { return; }
    int beg = 0;

    while (regexec(regxx, line, 1, &match, 0) == 0) {
        // int init_attr = beg + match.rm_so;
        // int kill_attr = match.rm_eo - match.rm_so;

        mvwchgat(pad, row, beg + match.rm_so, 
            match.rm_eo - match.rm_so, A_NORMAL, pair, NULL);
        line += match.rm_eo;
        beg  += match.rm_eo;
        if (match.rm_eo == match.rm_so) { break; }
    }
    // regfree(&regx);
}

WINDOW* grow_pad(WINDOW* pad, Buffer *b, int screen_w) {
    int need_h = b->n_lines + 1;
    int need_w = screen_w;
    for (int i = 0; i < b->n_lines; i++) {
        if (b->lines[i].len + 1 > need_w) { need_w = b->lines[i].len + 1; }
    }
    int cur_h, cur_w;
    getmaxyx(pad, cur_h, cur_w);
    if (need_h > cur_h || need_w > cur_w) {
        delwin(pad);
        int new_h = need_h > cur_h * 2 ? need_h : cur_h * 2;
        int new_w = need_w > cur_w * 2 ? need_w : cur_w * 2;
        pad = newpad(new_h, new_w);
        if (pad == NULL) { return NULL; }
        keypad(pad, TRUE);
    }
    return pad;
}

int buffer_writeout(Buffer *b, const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "w");
    if (!f) { return 1; }
    for (int i = 0; i < b->n_lines; i++) {
        fputs(b->lines[i].data, f);
        if (i < b->n_lines - 1) { fputc('\n', f); } /* add a trailing \n to the last line */
    }
    fclose(f);

    if (rename(tmp, path) != 0) { return 1; }
    b->dirty = 0;
    return 0;
}

static void line_reserve(Line *l, int need) {
    if (l->cap >= need) { return; }          /* if the lnes capacity is greater than the need, its already fine */
    int new_cap = l->cap ? l->cap : 16;      /* if no current cap, set to 16 */
    while (new_cap < need) { new_cap *= 2; } /* if still inadequate, double until it is */
    l->data = realloc(l->data, new_cap);     /* realloc the new line size */
    l->cap = new_cap;                        /* & update the line's capacity */
}

static void buffer_reserve(Buffer *b, int need) {
    if (b->cap_lines >= need) {return; }       /* same idea here but for no. of lines in the Buffer */
    int new_cap = b->cap_lines ? b->cap_lines : 32;
    while (new_cap < need) { new_cap *= 2; }
    b->lines = realloc(b->lines, new_cap * sizeof(Line));
    b->cap_lines = new_cap;
}

Buffer *buffer_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { return NULL; }
    // if (!f) {
    //     FILE *f = fopen(path, "w")
    // } should initialize & return an empty buffer

    Buffer *b = calloc(1, sizeof(Buffer));
    buffer_reserve(b, 32);

    char chunk[4096];
    while (fgets(chunk, sizeof(chunk), f)) {
        /* the line returned could be truncated if greater than 4096 *
         * checking for the EOL terminator & growing the line resolves this */
        buffer_reserve(b, b->n_lines + 1);
        Line *l = &b->lines[b->n_lines];             /* get the last line's address ? */
        l->data = NULL;
        l->len = 0;
        l->cap = 0;

        /* for lines longer than 4096 (identified by having no new line terminator) *
         * increase the line's reserve, update its len, and realloc more memory     */
        for (;;) { 
            int chunk_len = (int)strlen(chunk);
            int has_newline = (chunk_len > 0 && chunk[chunk_len - 1] == '\n');
            if (has_newline) { chunk_len--; }        /* strip the trailing '\n' */

            line_reserve(l, l->len + chunk_len + 1); /* l->len is 0, no? */
            memcpy(l->data + l->len, chunk, chunk_len);
            l->len += chunk_len;
            l->data[l->len] = '\0';                  /* NULL terminator */

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
    memmove(&b->lines[row + 2],        /* shift lines below 'row' down one */
        &b->lines[row + 1],
        (b->n_lines - row - 1) * sizeof(Line));

    Line *src = &b->lines[row];
    Line *dst = &b->lines[row + 1];
    int tail_len = src->len - col;     /* tail end is the current col - line length */

    dst->data = NULL;
    dst->len = 0;
    dst->cap = 0;

    line_reserve(dst, tail_len + 1);   /* make room for the new tail-end of the original line */
    memcpy(dst->data, src->data + col, tail_len); /* copy the tail end in */
    dst->data[tail_len] = '\0';
    dst->len = tail_len;               /* set it to its split length */

    src->data[col] = '\0';             /* add a null terminator at 'col' of the original line */
    src->len = col;                    /* set it to the length of the line remaining */

    b->n_lines++;                      /* add a line to the total count */
    b->dirty = 1;                      /* mark the unsaved changes */
}

void buffer_join_lines(Buffer *b, int row) {
    if (row + 1 >= b->n_lines) { return; }
    Line *l = &b->lines[row];
    Line *next = &b->lines[row + 1];

    line_reserve(l, l->len + next->len + 1);             /* add row 1's length to row 0 */
    memcpy(l->data + l->len, next->data, next->len + 1); /* + 1 for \0 */
    l->len += next->len;

    free(next->data);

    memmove(&b->lines[row + 1],                          /* shift lines above the gap down */
        &b->lines[row + 2],
        (b->n_lines - row - 2) * sizeof(Line));
    b->n_lines--;
    b->dirty = 1;
}