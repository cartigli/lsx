#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "iobuff.h"
#include "iofs.h"

// typedef struct { /* for each line in the file */
//     char *data;  /* line contents             */
//     int    len;  /* strlen of the line cached */
//     int    cap;  /* single line's capacity; allocated bytes; always >= len + 1 */
// } Line;

// typedef struct {   /* buffer created from file on disk; mutable source of truth */
//     Line   *lines; /* array of lines                     */
//     int   n_lines; /* lines currently in use             */
//     int cap_lines; /* capacity of lines; lines allocated */
//     int     dirty; /* unsaved changes                    */
// } Buffer;

// typedef struct v_class { /* RegEx expressions & their colors + cache */
//     const char *exp;     /* RegEx expression  */
//     int        pair;     /* color pair        */
//     regex_t   regxx;     /* cached expression */
//     int         val;     /* bool for cached or not    */
// } v_class;

// typedef struct {        /* all expressions           */
//     int         n_exps; /* no. of expressions stored */
//     v_class* v_classes; /* array of v_class structs  */
// } Expressions;

// typedef struct {  /* cached vars for the ncurses window/pad */
//     int max_line; /* longest line */
//     int  pad_row; /* top left row of pad */
//     int  pad_col; /* top left col of pad */
//     int curs_row; /* cursor current row */
//     int curs_col; /* cursor current col */
//     int screen_h; /* current terminal screen height */
//     int screen_w; /* current terminal screen width */
//     int   view_h; /* view port height */
//     int   view_w; /* view port width */
//     int    pad_w; /* pad width */
//     int       wo; /* write out (treated as bool) */
//     int act_code; /* returned from action key */
//     int   sprint; /* status message bool */
//     char   *smsg; /* status message */
// } RunTime;

// /* static: made once in memory and lasts only for runtime *
// * const: not mutated; raise a compiler warning if altered *
// * RULES[]: defined the struct as an array of structs      */
// static const struct { const char *exp; int pair; } RULES[] = {
//     { "([^[:space:]()]+)\\(",                              1 }, /* functions */
//     { "[*/\\<>%=^+-]",                                     2 }, /* operands */
//     { "\"([^\"]*)\"",                                      2 }, /* strings */
//     { "#[a-zA-Z_]+",                                       2 }, /* <headers.h> */
//     { "[].,!?:;'[{}()]",                                   2 }, /* punctuation */
//     { "(^|[^a-zA-Z_])(int|float|double|unsigned"
//         "|long|char|NULL|void)([^a-zA-Z_]|$)",             2 }, /* keywords (numerical) */
//     { "(^|[^a-zA-Z_])(return|if|while|for)([^a-zA-Z_]|$)", 2 }, /* keywords (flow control) */
//     { "(^|[^a-zA-Z_])(typedef|struct)([^a-zA-Z_]|$)",      2 } /* keywords (built-in) */
// };

// /* (the only current) status message */
// char *z_string = "ctrl + z pressed ";

// int ef_runn(char *path);

// /* buffer initiation, RegEx Expressions building, & RunTime declarations */
// Buffer *buffer_load(const char *path);
// Expressions *init_regex(void);
// RunTime *init_rt_vars(Buffer *b);

// /* intializes screen dimensions, attributes, & elements */
// int init_scr(void);

// /* runs the buffer & ncurses window; main manager */
// int alter_file(Buffer *b, Expressions *exps, RunTime *rt);

// /* add or expand memory for the modified buffer */
// static void line_reserve(Line *l, int need);
// static void buffer_reserve(Buffer *b, int need);
// WINDOW* grow_pad(WINDOW* pad, Buffer *b, int screen_w);

// /* the four main modifications: insert, delete, split, join */
// void buffer_insert_char(Buffer *b, int row, int col, char c);
// void buffer_delete_char(Buffer *b, int row, int col);
// void buffer_split_line(Buffer *b, int row, int col);
// void buffer_join_lines(Buffer *b, int row);

// /* digest key presses */
// void action_key(WINDOW* pad, Buffer *b, RunTime *rt, int ch);

// /* highlights the syntax from a set of predefined RegEx Expressions */
// void regex_color(WINDOW* pad, int row, const char *line,
//     const regex_t *regxx, int pair);

// /* writes the modified buffer to the disk */
// int buffer_writeout(Buffer *b, const char *path);

// /* frees allocated memory */
// void mfree(Buffer *b, Expressions *exps, RunTime *rt);

/* (the only current) status message */
char *z_string = "ctrl + z pressed ";

// int main(void) {
int ef_runn(char *path) {
    int exit_code = 1; /* only set to 0 if no errors *
     * otherwise, goto cleanup, & do no collect $200 */

    // char *path = "/Volumes/HomeXx/compuir/lsx/test.txt";
    // char *path = "/home/t/lsx/test.txt";

    Buffer *b = NULL;
    Expressions *exps = NULL;
    RunTime *rt = NULL;

    b = buffer_load(path);
    if (!b) { return 1; }

    exps = init_regex();
    if (!exps) { goto cleanup; }

    if (init_scr()) { goto cleanup; }

    rt = init_rt_vars(b);
    if (rt == NULL) { goto cleanup; }
    
    int edit_code = alter_file(b, exps, rt);
    endwin();

    if (edit_code == -1) {
        if (buffer_writeout(b, path)) { goto cleanup; }
        printf("wrote out modified buffer (OK)\n");
    }
    exit_code = 0;

cleanup:
    mfree(b, exps, rt);
    return exit_code;
}

Expressions *init_regex(void) {
    Expressions *exps = malloc(sizeof(Expressions));
    if (exps == NULL) { return NULL; }

    exps->n_exps = sizeof(RULES) / sizeof(RULES[0]);
    exps->v_classes = malloc(sizeof(v_class) * exps->n_exps);
    if (exps->v_classes == NULL) { return NULL; }
    for (int i = 0; i < exps->n_exps; i++) {
        exps->v_classes[i].pair = RULES[i].pair;
        exps->v_classes[i].exp = RULES[i].exp;
    }

    for (int i = 0; i < exps->n_exps; i++) {
        if (regcomp(&exps->v_classes[i].regxx,
            exps->v_classes[i].exp, REG_EXTENDED) != 0) {
            exps->v_classes[i].val = 0;
        } else {
            exps->v_classes[i].val = 1;
        } /* cleanup */
            // for (int d = 0; d < i; d++) { regfree(&exps->v_classes[d].regxx); }
            // free(exps->v_classes);
            // free(exps);
            // return NULL;
    }
    return exps;
}

RunTime *init_rt_vars(Buffer *b) {
    RunTime *rt = malloc(sizeof(RunTime));
    if (rt == NULL) { return NULL; }
     rt->max_line = 0;
    for (int i = 0; i < b->n_lines; i++) {
        if (b->lines[i].len > rt->max_line) {
            rt->max_line = b->lines[i].len;
        }
    }

    /* intialize the cursor & pad positions */
    rt->pad_row = 0;
    rt->pad_col = 0;
    rt->curs_row = 0;
    rt->curs_col = 0;

    /* find & adjust the given dimensions */
    getmaxyx(stdscr, rt->screen_h, rt->screen_w);
    rt->pad_w = (rt->max_line > rt->screen_w) ? rt->max_line + 1 : rt->screen_w;
    rt->view_h = rt->screen_h - 1;
    rt->view_w = rt->screen_w;

    /* set writeout to FALSE (0) */
    rt->wo = 0;

    /* initialize action to 0 */
    rt->act_code = 0;

    return rt;
}

int init_scr(void) {
    initscr();
    if (has_colors()) {
        start_color();
        /* args: (int: pair_no, fg color, bg color) */
        short skeys_pink = 1;
        short func_green = 2;
        short int_purple = 3;
        short vars_lgray = 4;
        short comm_dgray = 5;
        short strgs_yell = 6;

        init_color(skeys_pink, 980, 600, 803); // 250 , 153 , 205 );
        init_color(func_green, 267, 810, 431); //  68 , 207 , 110 );
        init_color(int_purple, 659, 510, 990); // 168 , 130 , 255 );
        init_color(vars_lgray, 702, 702, 702); // 179 , 179 , 179 );
        init_color(comm_dgray, 400, 400, 400); // 102 , 102 , 102 );
        init_color(strgs_yell, 882, 871, 443); // 225 , 222 , 113 );

        // init_pair(1, COLOR_CYAN, COLOR_BLACK);
        // init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(1, skeys_pink, COLOR_BLACK);
        init_pair(2, func_green, COLOR_BLACK);
        init_pair(3, int_purple, COLOR_BLACK);
        init_pair(4, vars_lgray, COLOR_BLACK);
        init_pair(5, comm_dgray, COLOR_BLACK);
        init_pair(6, strgs_yell, COLOR_BLACK);
    } else {
        endwin();
        return 1;
    }
    raw();                 /* catch ctrl + c as well as ctrl + o & other commons */
    noecho();
    keypad(stdscr, TRUE);  /* we do want to capture key strokes */
    curs_set(1);           /* initialize the cursor */
    leaveok(stdscr, TRUE); /* physical cursor doesn't need to appear on screen */
    return 0;
}

int alter_file(Buffer *b, Expressions *exps, RunTime *rt) {
    /* make pad atleast size of window incase file doesn't fill *
     * if condition ? expression if true : expression if false; */
    WINDOW *pad = newpad(b->n_lines + 1, rt->pad_w);
    if (pad == NULL) { return 1; }
    keypad(pad, TRUE);

    /* read the buffer into the pad, line by line */
    int row = 0;
    for (int i = 0; i < b->n_lines; i++) {
        mvwprintw(pad, row, 0, "%s", b->lines[i].data);
        row++;
    }

    clear();
    refresh();

    int ch;
    while (1) {
        char mbuff[44];
        if (rt->sprint) {
            snprintf(mbuff, sizeof(mbuff), "%s%d:%d (esc to esc)", rt->smsg, rt->curs_row + 1, b->n_lines);
        } else { snprintf(mbuff, sizeof(mbuff), "%d:%d (esc to esc)", rt->curs_row + 1, b->n_lines); }
        move(rt->screen_h - 1, 0);
        clrtoeol();
        int s_posit = rt->screen_w > (int)strlen(mbuff) ? rt->screen_w - (int)strlen(mbuff) : 0;
        mvprintw(rt->screen_h - 1, s_posit, "%s", mbuff);
        rt->sprint = 0;

        wnoutrefresh(stdscr); /* stage changes */

        // pad = grow_pad(pad, b, rt->screen_w);
        if (!pad) { return 1; }

        werase(pad);

        /* reprint modified buffer, line by line */
        for (int i = 0; i < b->n_lines; i++) {
            mvwprintw(pad, i, 0, "%s", b->lines[i].data);
        }

        /* clamp pad to cursor's position */
        if (rt->curs_row < rt->pad_row) {
            rt->pad_row = rt->curs_row;
        }
        if (rt->curs_row >= rt->pad_row + rt->view_h) {
            rt->pad_row = rt->curs_row - rt->view_h + 1;
        }
        if (rt->curs_col < rt->pad_col) {
            rt->pad_col = rt->curs_col;
        }
        if (rt->curs_col >= rt->pad_col + rt->view_w) {
            rt->pad_col = rt->curs_col - rt->view_w + 1;
        }

        /* highlight cached RegEx expressions matches */
        int ix = rt->pad_row;
        int xx = rt->pad_row + rt->view_h;
        if (xx > b->n_lines) { xx = b->n_lines; }
        for (int e = 0; e < exps->n_exps; e++) {
            if (!exps->v_classes[e].val) { continue; } /* skip invalids */
            /* only scan and color what is currently viewable */
            for (int dx = ix; dx < xx; dx++) {
                regex_color(pad, dx, b->lines[dx].data,
                    &exps->v_classes[e].regxx, exps->v_classes[e].pair);
            }
        }

        wmove(pad, rt->curs_row, rt->curs_col);
        pnoutrefresh(pad, rt->pad_row, rt->pad_col, 0, 0,
            rt->view_h - 1, rt->view_w - 1);
        doupdate();

        /* key comprehension exported */
        ch = wgetch(pad);
        action_key(pad, b, rt, ch);
        if (rt->act_code == -1) {
            rt->act_code = 0;
            goto done;
        }
    }
done:
    delwin(pad);
    if (rt->wo) { return -1; }
    return 0;
}

void action_key(WINDOW* pad, Buffer *b, RunTime *rt, int ch) {
    if (ch >= 32 && ch < 127) { /* printable ASCII */
        buffer_insert_char(b, rt->curs_row, rt->curs_col, (char)ch);
        rt->curs_col++;
    } else if (ch == '\n' || ch == KEY_ENTER || ch == 13 || ch == 10) {
        buffer_split_line(b, rt->curs_row, rt->curs_col);
        rt->curs_row++;
        rt->curs_col = 0;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        /* if the cursor's not at column 0 */
        if (rt->curs_col > 0) {
            buffer_delete_char(b, rt->curs_row, rt->curs_col - 1);
            rt->curs_col--;
        /* otherwise, join the current line w.the previous */
        } else if (rt->curs_row > 0) {
            int prev_len = b->lines[rt->curs_row - 1].len;
            buffer_join_lines(b, rt->curs_row - 1);
            rt->curs_row--;
            rt->curs_col = prev_len; /* land at join point */
        }
    /* movement key comprehension */
    } else if (ch == KEY_LEFT) {
        /* if the cursors not at col 0: */
        if (rt->curs_col > 0) {
            rt->curs_col--;
        /* if it is, and isn't at row 0: */
        } else if (rt->curs_row > 0) {
            /* move to EOL of previous row */
            rt->curs_row--;
            rt->curs_col = b->lines[rt->curs_row].len;
        }
    } else if (ch == KEY_RIGHT) {
        /* if the cursors at row less than the current line's length */
        if (rt->curs_col < b->lines[rt->curs_row].len) {
            rt->curs_col++;
        /* otherwise, and isn't at the last line: */
        } else if (rt->curs_row + 1 < b->n_lines) {
            rt->curs_row++; 
            rt->curs_col = 0; /* move to beg. of line */
        }
    } else if (ch == KEY_UP && rt->curs_row > 0) {
        rt->curs_row--; /* if the cursors last col was greater *
        * than the new line's length, move it to the EOL */
        if (rt->curs_col > b->lines[rt->curs_row].len) { 
            rt->curs_col = b->lines[rt->curs_row].len;
        }
    } else if (ch == KEY_DOWN && rt->curs_row + 1 < b->n_lines) {
        rt->curs_row++;
        if (rt->curs_col > b->lines[rt->curs_row].len) {
            rt->curs_col = b->lines[rt->curs_row].len;
        }
    /* ctrl key comprehension */
    } else if (ch >= 1 && ch <= 26) {
        if (ch == 15) { /* ctrl + O (love for Nano) */
            rt->wo = 1;
            rt->act_code = -1;
        } else if (ch == 24) { /* ctrl + X */
            rt->act_code = -1;
        } else if (ch == 26) { /* ctrl + Z */
            rt->sprint = 1;
            rt->smsg = z_string;
        }
    } else if (ch == 27) { /* esc */
        rt->act_code = -1;
    /* if terminal window gets resized */
    } else if (ch == KEY_RESIZE) {
        getmaxyx(stdscr, rt->screen_h, rt->screen_w);
        // pad = grow_pad(pad, b, rt->screen_w);
        pad = grow_pad(pad, b, rt);
        rt->view_h = rt->screen_h - 1;
        rt->view_w = rt->screen_w;
        clear();
        refresh();
    }
}

void mfree(Buffer *b, Expressions *exps, RunTime *rt) {
    for (int i = 0; i < b->n_lines; i++) {
        free(b->lines[i].data);
    }
    free(b->lines);
    free(b);
    if (exps != NULL) {
        for (int e = 0; e < exps->n_exps; e++) {
            if (!exps->v_classes[e].val) { continue; }
            regfree(&exps->v_classes[e].regxx);
        }
        free(exps->v_classes);
        free(exps);
    }
    if (rt != NULL) { free(rt); }
}

void regex_color(WINDOW *pad, int row, const char *line,
                const regex_t *regxx, int pair) {
    int pos = 0; /* start at position 0 for the given row (string) */
    regmatch_t pmatch[3]; /* max no. of capture groups to be used */
    /* regexec args: compiled exp., pointer math for posit. in string, *
     * capture groups, array for capture group results, flags */
    while (regexec(regxx, line + pos, 3, pmatch, 0) == 0) { /* returns 0 for match found (-1 on error) */
        /* if the capture groups found no matches, their rm_so/eo will be -1, so check if above 0: */
        regoff_t so = pmatch[2].rm_so >= 0 ? pmatch[2].rm_so : pmatch[0].rm_so;
        regoff_t eo = pmatch[2].rm_so >= 0 ? pmatch[2].rm_eo : pmatch[0].rm_eo;
        /* if not above 0, use first capture group, otherwise, use the third */

        /* move to the posiiton of the first match (pos + start offset: so) 
         * and apply a color pair until the end offset is met (eo - so) */
        mvwchgat(pad, row, pos + so, eo - so, A_NORMAL, pair, NULL);

        pos += pmatch[0].rm_eo; /* skip entire match */
        /* if regex matched an empty string (start offset == end offset): break out */
        if (pmatch[0].rm_eo == pmatch[0].rm_so) { break; }
    }
}

// WINDOW* grow_pad(WINDOW* pad, Buffer *b, int screen_w) {
WINDOW* grow_pad(WINDOW* pad, Buffer *b, RunTime *rt) {
    int need_h = b->n_lines + 1;
    int need_w = rt->screen_w;
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
        Line *l = &b->lines[b->n_lines]; /* get the last line's address ? */
        l->data = NULL;
        l->len = 0;
        l->cap = 0;

        /* for lines longer than 4096 (identified by having no new line terminator) *
         * increase the line's reserve, update its len, and realloc more memory     */
        for (;;) { 
            int chunk_len = (int)strlen(chunk);
            int has_newline = (chunk_len > 0 && chunk[chunk_len - 1] == '\n');
            if (has_newline) { chunk_len--; } /* strip the trailing '\n' */

            line_reserve(l, l->len + chunk_len + 1);
            memcpy(l->data + l->len, chunk, chunk_len);
            l->len += chunk_len;
            l->data[l->len] = '\0';           /* NULL terminator */

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
    Line *l = &b->lines[row];    /* 'id' the row being modified */
    line_reserve(l, l->len + 2); /* + 1 for c, + 1 for \0 */
    memmove(l->data + col + 1,   /* shift tail right */
        l->data + col,           /* start of text to shift; posit. in line where 'c' will be inserted */
        l->len - col + 1);       /* + 1 copies the \0 as well */
    l->data[col] = c;            /* add the newly inserted char 'c' where there is now space */
    l->len++;                    /* add 1 to the length of the given row */
    b->dirty = 1;                /* mark the changes as unsaved */
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

    line_reserve(dst, tail_len + 1); /* make room for the new tail-end of the original line */
    memcpy(dst->data, src->data + col, tail_len); /* copy the tail end in */
    dst->data[tail_len] = '\0';
    dst->len = tail_len;             /* set it to its split length */

    src->data[col] = '\0';           /* add a null terminator at 'col' of the original line */
    src->len = col;                  /* set it to the length of the line remaining */

    b->n_lines++;                    /* add a line to the total count */
    b->dirty = 1;                    /* mark the unsaved changes */
}

void buffer_join_lines(Buffer *b, int row) {
    if (row + 1 >= b->n_lines) { return; }
    Line *l = &b->lines[row];
    Line *next = &b->lines[row + 1];

    line_reserve(l, l->len + next->len + 1); /* add row 1's length to row 0 */
    memcpy(l->data + l->len, next->data, next->len + 1);
    l->len += next->len;

    free(next->data);

    memmove(&b->lines[row + 1],             /* shift lines above the gap down */
        &b->lines[row + 2],
        (b->n_lines - row - 2) * sizeof(Line));
    b->n_lines--;
    b->dirty = 1;
}