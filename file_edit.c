#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "highlight.h"
#include "iobuff.h"
#include "iofs.h"

/* (the only current) status message */
char *z_string = "ctrl + z pressed ";

int ef_runn(char *path) {
    int exit_code = 1; /* only set to 0 if no errors *
     * otherwise, goto cleanup, & do no collect $200 */

    Buffer *b = NULL;
    RunTime *rt = NULL;
    Cursor *curs = NULL;

    b = buffer_load(path);
    if (!b) { return 1; }

    curs = init_cursor();
    if (!curs) { goto cleanup; }

    if (compile_regex()) { goto cleanup; }

    if (init_scr()) {
        endwin();
        goto cleanup;
    }

    rt = init_rt_vars(b);
    if (rt == NULL) {
        endwin();
        goto cleanup;
    }

    int edit_code = alter_file(b, rt, curs);
    endwin();

    if (edit_code == -1) {
        if (buffer_writeout(b, path)) { goto cleanup; }
        printf("wrote out modified buffer (OK)\n");
    }
    exit_code = 0;

cleanup:
    mfree(b, rt, curs);
    return exit_code;
}


int compile_regex(void) {
    /* for every demand, compile the RegEx expression & cache the result *
     * additionally, enforce the order with a check before proceeding */
    CmpOrder previous = NUMERICALS;
    for (unsigned int x = 0; x < N_DEMANDS; x++) {
        if (previous > DEMANDS[x].type) {
            printf("DEMAND [%u] OUT OF ORDER | QUITTING\n", x);
            return 1;
        }
        previous = DEMANDS[x].type;

        DEMANDS[x].compiled = (regcomp(&DEMANDS[x].cmp_expression,
                                        DEMANDS[x].expression,
                                        REG_EXTENDED) == 0) ? 1 : 0;
    }
    return 0;
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

    /* intialize the pad positions */
    rt->pad_row = 0;
    rt->pad_col = 0;

    /* find & adjust the given dimensions */
    getmaxyx(stdscr, rt->screen_h, rt->screen_w);
    rt->pad_w = (rt->max_line > rt->screen_w) ? rt->max_line + 1 : rt->screen_w;
    rt->view_h = rt->screen_h - 1;
    rt->view_w = rt->screen_w;

    /* set writeout to FALSE (0) */
    rt->wo = 0;

    /* initialize action to 0 */
    rt->act_code = 0;

    /* set the message & content to empty */
    rt->sprint = 0;
    rt->smsg = NULL;

    return rt;
}


Cursor *init_cursor(void) {
    Cursor *curs = malloc(sizeof(Cursor));
    if (curs == NULL) { return NULL; }
    curs->row = 0;
    curs->col = 0;
    curs->dedent = "}]";
    curs->indent = "{[";
    curs->indent_l = 0;
    return curs;
}


int init_scr(void) {
    initscr();
    if (has_colors()) {
        start_color();
        /* args: (int: pair_no, fg color, bg color) */
        short keys_npres = 14;
        short functions = 15;
        short ints_ndecs = 16;
        short declr_vars = 17;
        short comments = 18;
        short strings = 19;
        short operands = 20;
        short testing = 21;

        init_color(keys_npres, 
            hex_compr(COLOR_CODES[0].r),
            hex_compr(COLOR_CODES[0].g),
            hex_compr(COLOR_CODES[0].b)
        );
        init_color(functions,
            hex_compr(COLOR_CODES[1].r),
            hex_compr(COLOR_CODES[1].g),
            hex_compr(COLOR_CODES[1].b)
        );
        init_color(ints_ndecs,
            hex_compr(COLOR_CODES[2].r),
            hex_compr(COLOR_CODES[2].g),
            hex_compr(COLOR_CODES[2].b)
        );
        init_color(declr_vars,
            hex_compr(COLOR_CODES[3].r),
            hex_compr(COLOR_CODES[3].g),
            hex_compr(COLOR_CODES[3].b)
        );
        init_color(comments,
            hex_compr(COLOR_CODES[4].r),
            hex_compr(COLOR_CODES[4].g),
            hex_compr(COLOR_CODES[4].b)
        );
        init_color(strings,
            hex_compr(COLOR_CODES[5].r),
            hex_compr(COLOR_CODES[5].g),
            hex_compr(COLOR_CODES[5].b)
        );
        init_color(operands,
            hex_compr(COLOR_CODES[6].r),
            hex_compr(COLOR_CODES[6].g),
            hex_compr(COLOR_CODES[6].b)
        );
        init_color(testing,
            hex_compr(COLOR_CODES[7].r),
            hex_compr(COLOR_CODES[7].g),
            hex_compr(COLOR_CODES[7].b)
        );

        init_pair(1, keys_npres, COLOR_BLACK);
        init_pair(2,  functions, COLOR_BLACK);
        init_pair(3, ints_ndecs, COLOR_BLACK);
        init_pair(4, declr_vars, COLOR_BLACK);
        init_pair(5,   comments, COLOR_BLACK);
        init_pair(6,    strings, COLOR_BLACK);
        init_pair(7,   operands, COLOR_BLACK);
        init_pair(8,    testing, COLOR_BLACK);

    } else {
        return 1;
    }
    raw();                 /* catch ctrl + c as well as ctrl + o & other commons */
    noecho();
    keypad(stdscr, TRUE);  /* we do want to capture key strokes */
    curs_set(1);           /* initialize the cursor */
    leaveok(stdscr, TRUE); /* physical cursor doesn't need to appear on screen */
    return 0;
}

int hex_compr(const char c[]) {
    int ccode;
    sscanf(c, "%x", &ccode);
    return (int)((ccode * 1000) / 255.0 );
}

int alter_file(Buffer *b, RunTime *rt, Cursor *curs) {
    /* make pad atleast size of window incase file doesn't fill *
     * if condition ? expression if true : expression if false; */
    WINDOW *pad = newpad(b->n_lines + 1, rt->pad_w);
    // rt->pad = newpad(b->n_lines + 1, rt-pad_w);
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
            snprintf(mbuff, sizeof(mbuff), "%s%d:%d (esc to esc)", rt->smsg, curs->row + 1, b->n_lines);
        } else { snprintf(mbuff, sizeof(mbuff), "%d:%d (esc to esc)", curs->row + 1, b->n_lines); }
        move(rt->screen_h - 1, 0);
        clrtoeol();
        int s_posit = rt->screen_w > (int)strlen(mbuff) ? rt->screen_w - (int)strlen(mbuff) : 0;
        mvprintw(rt->screen_h - 1, s_posit, "%s", mbuff);
        rt->sprint = 0;

        wnoutrefresh(stdscr); /* stage changes */
        if (!pad) { return 1; }
        werase(pad);

        /* reprint modified buffer, line by line */
        for (int i = 0; i < b->n_lines; i++) {
            mvwprintw(pad, i, 0, "%s", b->lines[i].data);
        }

        /* clamp pad to cursor's position */
        if (curs->row < rt->pad_row) {
            rt->pad_row = curs->row;
        }
        if (curs->row >= rt->pad_row + rt->view_h) {
            rt->pad_row = curs->row - rt->view_h + 1;
        }
        if (curs->col < rt->pad_col) {
            rt->pad_col = curs->col;
        }
        if (curs->col >= rt->pad_col + rt->view_w) {
            rt->pad_col = curs->col - rt->view_w + 1;
        }

        /* highlight cached RegEx expressions matches */
        int ix = rt->pad_row;
        int xx = rt->pad_row + rt->view_h;
        if (xx > b->n_lines) { xx = b->n_lines; }
        for (unsigned int e = 0; e < N_DEMANDS; e++) {
            if (!DEMANDS[e].compiled) { continue; } /* skip invalids */
            /* only scan and color what is currently viewable */
            for (int dx = ix; dx < xx; dx++) {
                regex_color(pad, dx, b->lines[dx].data,
                    &DEMANDS[e].cmp_expression, DEMANDS[e].color_code);
            }
        }

        wmove(pad, curs->row, curs->col);
        pnoutrefresh(pad, rt->pad_row, rt->pad_col, 0, 0,
            rt->view_h - 1, rt->view_w - 1);
        doupdate();

        /* key comprehension exported */
        ch = wgetch(pad);
        action_key(pad, b, rt, curs, ch);
        if (rt->act_code) {
            rt->act_code = 0;
            goto done;
        }
    }
done:
    delwin(pad);
    if (rt->wo) { return -1; }
    return 0;
}


void action_key(WINDOW* pad, Buffer *b, RunTime *rt, Cursor *curs, int ch) {
    int indent = 4;

    if (ch >= 32 && ch < 127) { /* printable ASCII */
        if (dedentable(b, curs, ch)) { /* 'smart de-dent' */
            /* if the indent level is corrupted */
            if (curs->col != curs->indent_l * indent) {
                if (repair_indent(b, curs, indent)) { rt->act_code = 1; }
            } /* and then dedent per usual */
            curs->col = curs->indent_l * indent;
            curs->indent_l--;
            if (curs->indent_l < 0) { curs->indent_l = 0; }
            for (int a = 0; a < indent; a++) {
                buffer_delete_char(b, curs->row, curs->col - 1);
                curs->col--;
            }
            buffer_insert_char(b, curs->row, curs->col, (char)ch);
            curs->col++;

        } else { /* else, normal char */
            buffer_insert_char(b, curs->row, curs->col, (char)ch);
            curs->col++;
        }
    } else if (ch == '\n' || ch == KEY_ENTER) {
        /* 'smart indent' */
        if (indentable(b, curs)) {
            //if (curs->col != curs->indent_l * indent) { repair_indent(b, curs, indent); }
            curs->indent_l++; /* increase indent */
            buffer_split_line(b, curs->row, curs->col);
            curs->row++;
            curs->col = 0;
            for (int a = 0; a < indent * curs->indent_l; a++) {
                buffer_insert_char(b, curs->row, curs->col, (char)32);
            }
            curs->col += indent * curs->indent_l;

        } else { /* if leaving a line comprised entirely of spaces/tabs, clear it */
            if (whitespace(b, curs->row)) {
                for (int i = 0; i < curs->indent_l * indent; i++) {
                    buffer_delete_char(b, curs->row, curs->col - 1);
                    curs->col--;
                }
            }
            /* regular new line + matched indent level */
            buffer_split_line(b, curs->row, curs->col);
            curs->row++;
            curs->col = 0;
            if (curs->indent_l) { /* maintain current level unless otherwise indicated */
                for (int a = 0; a < indent * curs->indent_l; a++) {
                    buffer_insert_char(b, curs->row, curs->col, (char)32);
                    // curs->col++;
                }
                curs->col = indent * curs->indent_l;
            }
        }

    } else if (ch == KEY_BACKSPACE || ch == 127) {
        /* if the cursor's not at column 0 */
        if (curs->col > 0) {
            if (dedented(b, curs) && curs->col == b->lines[curs->row].len - 1) { /* if char deleted caused a dedent: */
                buffer_delete_char(b, curs->row, curs->col - 1);
                // curs->col--;
                // if (curs->col != curs->indent_l * indent) { repair_indent(b, curs, indent); }
                for (int a = 0; a < indent; a++) { /* reapply the dedented indent */
                    buffer_insert_char(b, curs->row, curs->col, (char)32);
                    curs->col++;
                }
            } else { /* else, just delete char */
                buffer_delete_char(b, curs->row, curs->col - 1);
                curs->col--;
            }
        /* otherwise, join the current line w.the previous */
        } else if (curs->row > 0) {
            int prev_len = b->lines[curs->row - 1].len;
            buffer_join_lines(b, curs->row - 1);
            curs->row--;
            curs->col = prev_len; /* land at join point */
        }
    /* movement key comprehension */
    } else if (ch == KEY_LEFT) {
        /* if the cursors not at col 0: */
        if (curs->col > 0) {
            curs->col--;
        /* if it is, and isn't at row 0: */
        } else if (curs->row > 0) {
            /* move to EOL of previous row */
            curs->row--;
            curs->col = b->lines[curs->row].len;
        }
    } else if (ch == KEY_RIGHT) {
        /* if the cursors at row less than the current line's length */
        if (curs->col < b->lines[curs->row].len) {
            curs->col++;
        /* otherwise, and isn't at the last line: */
        } else if (curs->row + 1 < b->n_lines) {
            curs->row++; 
            curs->col = 0; /* move to beg. of line */
        }
    } else if (ch == KEY_UP && curs->row > 0) {
        curs->row--; /* if the cursors last col was greater *
        * than the new line's length, move it to the EOL */
        if (curs->col > b->lines[curs->row].len) { 
            curs->col = b->lines[curs->row].len;
        }
    } else if (ch == KEY_DOWN && curs->row + 1 < b->n_lines) {
        curs->row++;
        if (curs->col > b->lines[curs->row].len) {
            curs->col = b->lines[curs->row].len;
        }
    /* ctrl key comprehension */
    } else if (ch >= 1 && ch <= 26) {
        if (ch == 15) { /* ctrl + O (love for Nano) */
            rt->wo = 1;
            rt->act_code = 1;
        } else if (ch == 24) { /* ctrl + X */
            rt->act_code = 1;
        } else if (ch == 26) { /* ctrl + Z */
            rt->sprint = 1;
            rt->smsg = z_string;
        }
    } else if (ch == 27) { /* esc */
        rt->act_code = 1;
    /* if terminal window gets resized */
    } else if (ch == KEY_RESIZE) {
        getmaxyx(stdscr, rt->screen_h, rt->screen_w);
        pad = grow_pad(pad, b, rt);
        rt->view_h = rt->screen_h - 1;
        rt->view_w = rt->screen_w;
        clear();
        refresh();
    }
}


int repair_indent(Buffer *b, Cursor *curs, int indent) {
    while (1) { /* if the indent level doesn't match the column */
        if (curs->col == curs->indent_l * indent) {
            break;
        /* repair the indent level before dedenting */
        } else if (curs->col < curs->indent_l * indent) {
            buffer_insert_char(b, curs->row, curs->col, (char)32);
            curs->col++;
        } else {
            buffer_delete_char(b, curs->row, curs->col - 1);
            curs->col--;
        }
    }
    if (curs->col == curs->indent_l * indent) { return 0; }
    return 1;
}


int indentable(Buffer *b, Cursor *curs) {
    int n = b->lines[curs->row].len;
    if (!(n > 0)) { return 0; }
    for (int i = 0; i < (int)strlen(curs->indent); i++) {
        if (b->lines[curs->row].data[n - 1] == curs->indent[i]) {
            return 1;
        }
    }
    return 0;
}


int dedented(Buffer *b, Cursor *curs) {
    int ch = b->lines[curs->row].data[b->lines[curs->row].len - 1];
    for (int i = 0; i < (int)strlen(curs->dedent); i++) {
        if (ch == curs->dedent[i]) { return 1; }
    }
    return 0;
}


int dedentable(Buffer *b, Cursor *curs, int ch) {
    if (!(curs->indent_l)) { return 0; }
    if (!(b->lines[curs->row].len > 0)) { return 0; }
    if (!whitespace(b, curs->row)) { return 0; } /* needs to be longer than 0 & contain only whitespace (new char not entered yet) */
    for (int i = 0; i < (int)strlen(curs->dedent); i++) {
        if (ch == curs->dedent[i]) { return 1; }
    }
    return 0;
}


int whitespace(Buffer *b, int row) {
    for (int i = 0; i < b->lines[row].len; i++) {
        if (b->lines[row].data[i] != ' ' && b->lines[row].data[i] != '\t') {
            return 0;
        }
    }
    return 1;
}


void regex_color(WINDOW *pad, int row, const char *line,
                const regex_t *regxx, int code) {
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
        // mvwchgat(pad, row, pos + so, eo - so, A_NORMAL, (int)COLOR_CODES[code], NULL);
        mvwchgat(pad, row, pos + so, eo - so, A_NORMAL, code, NULL);

        pos += pmatch[0].rm_eo; /* skip entire match */
        /* if regex matched an empty string (start offset == end offset): break out */
        if (pmatch[0].rm_eo == pmatch[0].rm_so) { break; }
    }
}


Buffer *buffer_load(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { return NULL; }
    // if (!f) { TODO: make new file if not exists
    //     FILE *f = fopen(path, "w")
    // } should initialize & return an empty buffer

    Buffer *b = calloc(1, sizeof(Buffer));
    if (!b) {
        fclose(f);
        return NULL;
    }
    // b[0] = '\0';

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


/* memory management within the buffer */

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


/* four main editors */

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


void mfree(Buffer *b, RunTime *rt, Cursor *curs) {
    for (int i = 0; i < b->n_lines; i++) {
        free(b->lines[i].data);
    }
    free(b->lines);
    free(b);

    if (rt != NULL) { free(rt); }

    for (unsigned int f = 0; f < N_DEMANDS; f++) {
        if (DEMANDS[f].compiled) {
            regfree(&DEMANDS[f].cmp_expression);
        }
    }
    free(curs);
}