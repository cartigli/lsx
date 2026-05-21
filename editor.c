#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "editor.h"
#include "error.h"
#include "highlight.h"


char *ctrl_z_str = "ctrl + z pressed"; /* 16 chars + 4 digits + 4 digits + : 1 digit = 25 */
char *default_str = "(exit: ctrl + x)";
char *immutable_str = "(exit: x)";

void stt_handler(WINDOW *w, const char *msg) {
    if (!msg) {
        print_err(edit_src, "NULL *msg passed to stt_handler", 2);
        return;
    }

    int x, y;
    getmaxyx(w, y, x);
    int row = y - STATUS_RROWS;
    int clr_col = MAX_STTM_LEN;
    int str_col = x > (int)strlen(msg) ? x - strlen(msg) : 1;

    wmove(w, row, clr_col);
    wclrtoeol(w);
    mvwprintw(w, row, str_col, "%s", msg);
}


int alter_file(Buffer *b, RunTime *rt, Cursor *curs, int mutable) {
    /* make pad atleast size of window incase file doesn't fill *
     * if condition ? expression if true : expression if false; */
    rt->pad = newpad(b->n_lines + 1, rt->pad_w);
    if (rt->pad == NULL) {
        print_err(edit_src, "failed to initiate new pad for alter file", 5);
        return 1;
    }
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
    char *default_msg = (mutable) ? default_str : immutable_str;

    while (1) {
        char mbuff[MAX_STTM_LEN];
        if (curs->sprint) {
            snprintf(mbuff, sizeof(mbuff), "%d:%d %s",
                    curs->row + 1, b->n_lines, curs->smsg);
        } else {
            snprintf(mbuff, sizeof(mbuff), "%d:%d %s",
                    curs->row + 1, b->n_lines, default_msg);
        }

        /* move to where the longest message could start */
        move(rt->screen_h - 1, MAX_STTM_LEN);
        clrtoeol(); /* clear it */
        /* if the mgs is longer than the screen, print *
         * (or clear) at screen w - msg.len else col 0 */
        int mlen = (int)strlen(mbuff);
        int col = rt->screen_w > mlen ? rt->screen_w - mlen : 0;
        mvprintw(rt->screen_h - 1, col, "%s", mbuff);
        curs->sprint = 0;

        wnoutrefresh(stdscr); /* stage changes */
        if (!rt->pad) {
            print_err(edit_src, "unexpected NULL rt->pad inside the editor's loop", 5);
            return 1;
        }
        werase(rt->pad);

        /* reprint modified buffer, line by line */
        for (int i = 0; i < b->n_lines; i++) {
            mvwprintw(rt->pad, i, 0, "%s", b->lines[i].data);
        }

        /* clamp pad to cursor's position */
        if (curs->row < rt->pad_row) {
            rt->pad_row = curs->row;
        }
        if (curs->row >= rt->pad_row + rt->screen_h - STATUS_RROWS) {
            rt->pad_row = curs->row - (rt->screen_h - STATUS_RROWS) + 1;
        }
        if (curs->col < rt->pad_col) {
            rt->pad_col = curs->col;
        }
        if (curs->col >= rt->pad_col + rt->screen_w) {
            // rt->pad_col = curs->col - (rt->screen_w - STATUS_RROWS) + 1; :: :( why could I not see a difference
            rt->pad_col = curs->col - (rt->screen_w) + 1;
        }

        /* highlight cached RegEx expressions matches */
        int ix = rt->pad_row;
        int xx = rt->pad_row + rt->screen_h - STATUS_RROWS;
        if (xx > b->n_lines) { xx = b->n_lines; }
        for (unsigned int e = 0; e < N_DEMANDS; e++) {
            if (!DEMANDS[e].compiled) {
                print_err(edit_src, "skipping an uncompiled regex expression", 1);
                continue;
            } /* skip invalids */
            /* only scan and color what is currently viewable */
            for (int dx = ix; dx < xx; dx++) {
                regex_color(rt, dx, b->lines[dx].data,
                    &DEMANDS[e].cmp_expression, DEMANDS[e].color_code);
            }
        }

        wmove(rt->pad, curs->row, curs->col);
        pnoutrefresh(rt->pad, rt->pad_row, rt->pad_col, 0, 0,
            rt->screen_h - STATUS_RROWS - 1, rt->screen_w - 1);
        doupdate();

        /* key comprehension factored */
        ch = wgetch(rt->pad);

        action_key(b, rt, curs, ch, mutable);
        if (curs->action) {
            curs->action = 0;
            break;
        }
    }

    delwin(rt->pad);
    return 0;
}


void action_key(Buffer *b, RunTime *rt, Cursor *curs, int ch, int mutable) {
    int indent = 4;

    /* only process if mutable, but do it initially */
    if (mutable) {
        if (ch >= 32 && ch < 127) { /* printable ASCII */
            if (dedentable(b, curs, ch)) { /* 'smart' dedent */
                /* if the indent level is corrupted */
                if (curs->col != curs->indent_l * indent) {
                    if (repair_indent(b, curs, indent)) {
                        curs->action = 1;
                    }
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
        }
    }

    /* movement key digest -- always process */
    if (ch == KEY_LEFT) {
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

    /* 'utility' key bindings */
    } else if (ch == 27) { /* esc */
        curs->action = 1;
    } else if (ch == KEY_RESIZE) { /* terminal window resized */
        getmaxyx(stdscr, rt->screen_h, rt->screen_w);
        grow_pad(b, rt);
        if (rt->pad == NULL) {
            print_err(edit_src, "skipping an uncompiled rt->pad after RESIZE digest", 4);
            return;
        }
        clear();
        refresh();

    /* if in an immutable state, process no other captured keys */
    } else if (!mutable) {
        if (ch == 'x') { curs->action = 1; }
        return;

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
            if (curs->indent_l) {
                /* maintain current level unless otherwise indicated */
                for (int a = 0; a < indent * curs->indent_l; a++) {
                    buffer_insert_char(b, curs->row, curs->col, (char)32);
                }
                curs->col = indent * curs->indent_l;
            }
        }

    } else if (ch == KEY_BACKSPACE || ch == 127) {
        /* if the cursor's not at column 0 */
        if (curs->col > 0) {
            /* if char deleted caused a dedent: */
            if (b->lines[curs->row].len > 0 && dedented(b, curs) &&
                        curs->col == b->lines[curs->row].len - 1) {
                buffer_delete_char(b, curs->row, curs->col - 1);
                for (int a = 0; a < indent; a++) {
                    /* reapply the dedented indent */
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

    /* ctrl key digest */
    } else if (ch >= 1 && ch <= 26) {
        if (ch == 15) { /* ctrl + o */
            if (!mutable) {
                print_err(edit_src, "cannot write out in an immutable state", 3);
                return; /* do nothing */
            }
            curs->wo = -1;
            curs->action = 1;
        } else if (ch == 24) { /* ctrl + x */
            curs->action = 1;
        } else if (ch == 26) { /* ctrl + z */
            curs->sprint = 1;
            curs->smsg = ctrl_z_str;
        }
    }
}


int repair_indent(Buffer *b, Cursor *curs, int indent) {
    while (1) {
        /* if the indent level doesn't match the column */
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
    print_err(edit_src, "failed to repair indent indices", 3);
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
    if (b->lines[curs->row].data[b->lines[curs->row].len] < 1) { return 1; }
    int ch = b->lines[curs->row].data[b->lines[curs->row].len - 1];
    for (int i = 0; i < (int)strlen(curs->dedent); i++) {
        if (ch == curs->dedent[i]) { return 1; }
    }
    return 0;
}


int dedentable(Buffer *b, Cursor *curs, int ch) {
    if (!(curs->indent_l)) { return 0; }
    if (!(b->lines[curs->row].len > 0)) { return 0; }
    if (!whitespace(b, curs->row)) { return 0; }
    /* needs to be longer than 0 & contain only *
     * whitespace (new char has not been entered) */
    for (int i = 0; i < (int)strlen(curs->dedent); i++) {
        if (ch == curs->dedent[i]) { return 1; }
    }
    return 0;
}


int whitespace(Buffer *b, int row) {
    for (int i = 0; i < b->lines[row].len; i++) {
        if (b->lines[row].data[i] != ' ' &&
                    b->lines[row].data[i] != '\t') {
            return 0;
        }
    }
    return 1;
}


void regex_color(RunTime *rt, int row, const char *line,
                const regex_t *regxx, int code) {
    int pos = 0; /* start at position 0 for the given row (string) */
    regmatch_t pmatch[3]; /* max no. of capture groups to be used *
     * regexec args: compiled exp., pointer math for posit. in string, *
     * capture groups, array for capture group results, flags */
    while (regexec(regxx, line + pos, 3, pmatch, 0) == 0) { /* returns 0 for match found (-1 on error) *
         * if the capture groups found no matches, their rm_so/eo will be -1, so check if above 0: */
        regoff_t so = pmatch[2].rm_so >= 0 ? pmatch[2].rm_so : pmatch[0].rm_so;
        regoff_t eo = pmatch[2].rm_so >= 0 ? pmatch[2].rm_eo : pmatch[0].rm_eo;
        /* if not above 0, use first capture group, otherwise, use the third */

        /* move to the posiiton of the first match (pos + start offset: so) 
         * and apply a color pair until the end offset is met (eo - so) */
        mvwchgat(rt->pad, row, pos + so, eo - so, A_NORMAL, code, NULL);

        pos += pmatch[0].rm_eo; /* skip entire match *
         * if regex matched an empty string (start offset == end offset): break out */
        if (pmatch[0].rm_eo == pmatch[0].rm_so) { break; }
    }
}


/* window/pad management with the buffer */

void grow_pad(Buffer *b, RunTime *rt) {
    int need_h = b->n_lines + 1;
    int need_w = rt->screen_w;
    for (int i = 0; i < b->n_lines; i++) {
        if (b->lines[i].len + 1 > need_w) {
            need_w = b->lines[i].len + 1;
        }
    }
    int cur_h, cur_w;
    getmaxyx(rt->pad, cur_h, cur_w);
    if (need_h > cur_h || need_w > cur_w) {
        delwin(rt->pad);
        int new_h = need_h > cur_h * 2 ? need_h : cur_h * 2;
        int new_w = need_w > cur_w * 2 ? need_w : cur_w * 2;
        rt->pad = newpad(new_h, new_w);
        if (rt->pad == NULL) {
            print_err(edit_src, "unexpected NULL pad while attempting to grow it", 4);
            return;
        }
        keypad(rt->pad, TRUE);
    }
}
