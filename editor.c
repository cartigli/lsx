#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "init.h"
#include "editor.h"
#include "highlight.h"
#include "fsio.h"

/* (the only current) status message */
char *ctrl_z_str = "ctrl + z pressed ";
char *default_str = "(exit: ctrl + x)";
char *immutable_str = "(exit: x)";

char *default_msg;

int pretty_runner(char *path, int MUTABLE) {
    int exit_code = 1; /* only set to 0 if no errors *
     * otherwise, goto cleanup, & do no collect $200 */

    Buffer *b = NULL;
    RunTime *rt = NULL;
    Cursor *curs = NULL;

    b = buffer_load(path);
    if (!b) { return 1; }

    curs = init_cursor(MUTABLE);
    if (!curs) { goto cleanup; }
    default_msg = (curs->Mutable) ? default_str : immutable_str;

    if (compile_regex()) { goto cleanup; }

    curs_set(1);

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


int alter_file(Buffer *b, RunTime *rt, Cursor *curs) {
    /* make pad atleast size of window incase file doesn't fill *
     * if condition ? expression if true : expression if false; */
    rt->pad = newpad(b->n_lines + 1, rt->pad_w);
    if (rt->pad == NULL) { return 1; }
    keypad(rt->pad, TRUE);

    /* read the buffer into the pad, line by line */
    int row = 0;
    for (int i = 0; i < b->n_lines; i++) {
        mvwprintw(rt->pad, row, 0, "%s", b->lines[i].data);
        row++;
    }

    clear();
    refresh();

    int ch;
    char *default_msg = "(esc to esc)";
    while (1) {
        char mbuff[44];
        if (curs->sprint) {
            snprintf(mbuff, sizeof(mbuff), "%d:%d %s", curs->row + 1, b->n_lines, curs->smsg);
        } else { snprintf(mbuff, sizeof(mbuff), "%d:%d %s", curs->row + 1, b->n_lines, default_msg); }

        move(rt->screen_h - 1, 0);
        clrtoeol();
        int s_posit = rt->screen_w > (int)strlen(mbuff) ? rt->screen_w - (int)strlen(mbuff) : 0;
        mvprintw(rt->screen_h - 1, s_posit, "%s", mbuff);
        curs->sprint = 0;

        wnoutrefresh(stdscr); /* stage changes */
        if (!rt->pad) { return 1; }
        werase(rt->pad);

        /* reprint modified buffer, line by line */
        for (int i = 0; i < b->n_lines; i++) {
            mvwprintw(rt->pad, i, 0, "%s", b->lines[i].data);
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
                regex_color(rt, dx, b->lines[dx].data,
                    &DEMANDS[e].cmp_expression, DEMANDS[e].color_code);
            }
        }

        wmove(rt->pad, curs->row, curs->col);
        pnoutrefresh(rt->pad, rt->pad_row, rt->pad_col, 0, 0,
            rt->view_h - 1, rt->view_w - 1);
        doupdate();

        /* key comprehension exported */
        ch = wgetch(rt->pad);

        action_key(b, rt, curs, ch);
        if (curs->act_code) {
            curs->act_code = 0;
            goto done;
        }
    }
done:
    delwin(rt->pad);
    if (curs->wo) { return -1; }
    return 0;
}


void action_key(Buffer *b, RunTime *rt, Cursor *curs, int ch) {
    int indent = 4;

    if (!curs->Mutable) {
        if (ch == 'x') {
            curs->act_code = 1;
            return;
        } else if (ch >= 32 && ch < 127) {
            return;
        }
    }

    if (ch >= 32 && ch < 127) { /* printable ASCII */
        if (dedentable(b, curs, ch)) { /* 'smart de-dent' */
            /* if the indent level is corrupted */
            if (curs->col != curs->indent_l * indent) {
                if (repair_indent(b, curs, indent)) { curs->act_code = 1; }
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
        if (indentable(b, curs)) { /* 'smart indent' */
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
                }
                curs->col = indent * curs->indent_l;
            }
        }

    } else if (ch == KEY_BACKSPACE || ch == 127) {
        /* if the cursor's not at column 0 */
        if (curs->col > 0) {
            if (dedented(b, curs) && curs->col == b->lines[curs->row].len - 1) { /* if char deleted caused a dedent: */
                buffer_delete_char(b, curs->row, curs->col - 1);
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
            if (curs->Mutable) {
                curs->wo = 1;
                curs->act_code = 1;
            }; // else { continue; }
        } else if (ch == 24) { /* ctrl + X */
            curs->act_code = 1;
        } else if (ch == 26) { /* ctrl + Z */
            curs->sprint = 1;
            curs->smsg = ctrl_z_str;
        }
    } else if (ch == 27) { /* esc */
        curs->act_code = 1;
    /* if terminal window gets resized */
    } else if (ch == KEY_RESIZE) {
        getmaxyx(stdscr, rt->screen_h, rt->screen_w);
        grow_pad(b, rt);
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


void regex_color(RunTime *rt, int row, const char *line,
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
        mvwchgat(rt->pad, row, pos + so, eo - so, A_NORMAL, code, NULL);

        pos += pmatch[0].rm_eo; /* skip entire match */
        /* if regex matched an empty string (start offset == end offset): break out */
        if (pmatch[0].rm_eo == pmatch[0].rm_so) { break; }
    }
}


/* window/pad management with the buffer */

void grow_pad(Buffer *b, RunTime *rt) {
    int need_h = b->n_lines + 1;
    int need_w = rt->screen_w;
    for (int i = 0; i < b->n_lines; i++) {
        if (b->lines[i].len + 1 > need_w) { need_w = b->lines[i].len + 1; }
    }
    int cur_h, cur_w;
    getmaxyx(rt->pad, cur_h, cur_w);
    if (need_h > cur_h || need_w > cur_w) {
        delwin(rt->pad);
        int new_h = need_h > cur_h * 2 ? need_h : cur_h * 2;
        int new_w = need_w > cur_w * 2 ? need_w : cur_w * 2;
        rt->pad = newpad(new_h, new_w);
        if (rt->pad == NULL) { return; }
        keypad(rt->pad, TRUE);
    }
}


void mfree(Buffer *b, RunTime *rt, Cursor *curs) {
    free_buff(b);

    if (rt != NULL) { free(rt); }

    for (unsigned int f = 0; f < N_DEMANDS; f++) {
        if (DEMANDS[f].compiled) {
            regfree(&DEMANDS[f].cmp_expression);
        }
    }
    free(curs);
}