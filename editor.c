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


static inline int indent_col(Cursor *curs) {
    return curs->indent_l * GLOBAL_INDENT_LEN;
}


char *ctrl_z_str = "ctrl + z pressed"; /* 16 chars + 4 digits + 4 digits + : 1 digit = 25 */
char *default_str = "exit: ctrl + x";
char *immutable_str = "exit: x";
char *wo_failure = "writeout failed";
char *wo_success = "wrote out buffer";
char *no_changes = "no modifications";


void alter_file(Buffer *b, RunTime *rt, Cursor *curs,
                const char *path, int mutable) {
    // runs a window for viewing, and potentially editing, a file

    // make a pad at the minimum the size of the window, 
    // so smaller files still fill the pad amd terminal
    rt->pad = newpad(b->n_lines + 1, rt->pad_w);
    if (rt->pad == NULL) {
        print_err(edit_src, "failed to initiate new pad for alter file", 5);
        return;
    }

    int ch;
    // initiate the status message buffer outside the loop
    char mbuff[MAX_STTM_LEN];
    // set the exit command accordingly
    char *default_msg = (mutable) ? default_str : immutable_str;
    
    // read the buffer into the pad, line by line
    int row = 0;
    for (int i = 0; i < b->n_lines; i++) {
        mvwprintw(rt->pad, row, 0, "%s", b->lines[i].text);
        row++;
    }

    clear();
    refresh();
    keypad(rt->pad, TRUE);
    while (1) {
        // status message: cursor's message if exists else default
        char *status_msg = curs->sprint ? curs->smsg : default_msg;
        // print the status along with the cursor's line : all lines
        snprintf(mbuff, sizeof(mbuff), "%s %d:%d",
                    status_msg, curs->row + 1, b->n_lines);

        // move to where the longest status message could start
        // if the new message is shorter, dead text should be cleared
        move(rt->screen_h - 1, MAX_STTM_LEN);
        clrtoeol();

        // print the message from the screen width - message length, if
        // the screen width is greater than the message length, else 0
        int mlen = (int)strlen(mbuff);
        int col = rt->screen_w > mlen ? rt->screen_w - mlen : 0;
        mvprintw(rt->screen_h - 1, col, "%s", mbuff);
        if (curs->sprint) { curs->sprint = 0; }

        // stage changes
        wnoutrefresh(stdscr);
        if (!rt->pad) {
            print_err(edit_src, "NULL rt->pad inside the editor's loop", 5);
            return;
        }
        werase(rt->pad);

        // print out the modified buffer, line by line
        for (int i = 0; i < b->n_lines; i++) {
            mvwprintw(rt->pad, i, 0, "%s", b->lines[i].text);
        }

        // clamp the pad to the cursor's position
        // i.e., scroll if the cursor moves past edge
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
            rt->pad_col = curs->col - (rt->screen_w) + 1;
        }

        // for (int i 0; i < b->n_lines; i++) {
        //     Line *line = &b->lines[i];
        //     // find all matches to the initiators of multi-line expressions
        //     check_multiline_inits(l);

        // highlighting text visible in the terminal window
        int ix = rt->pad_row;
        int xx = rt->pad_row + rt->screen_h - STATUS_RROWS;
        if (xx > b->n_lines) { xx = b->n_lines; }

        // for each visible lines:
        for (int dx = ix; dx < xx; dx++) {
            Line *line = &b->lines[dx];
            // checks hlite_NOK, i.e., was edited/needs highlighting?
            refresh_expression(line);
            // for each char in the line, apply its color
            for (int i = 0; i < line->len; i++) {
                // if the char's color code is 0: skip
                if (!line->column_colors[i]) { continue; }
                // else, apply that char's color code
                mvwchgat(rt->pad, dx, i, 1, A_NORMAL,
                            line->column_colors[i], NULL);
            }
        }

        wmove(rt->pad, curs->row, curs->col);
        // exit the staging
        pnoutrefresh(rt->pad, rt->pad_row, rt->pad_col, 0, 0,
            rt->screen_h - STATUS_RROWS - 1, rt->screen_w - 1);
        // update the screen
        doupdate();

        // digest any keypresses
        ch = wgetch(rt->pad);
        action_key(b, rt, curs, ch, path, mutable);
        if (curs->action) { curs->action = 0; break; }
    }

    delwin(rt->pad);
    return;
}


// void check_multiline_inits(Line *line) {
//     if (!line->hlite_NOK) { return; }
//     if (line->multiline) { line->multiline = 0; }
//     for (int i = 0; i < line->len; i++) { line->column_colors[i] = 0; }

//     for (int e = 0; e < (int)N_GLOBAL_MULTILINE_DEMANDS; e++) {
//         if (GLOBAL_MULTILINE_DEMANDS[e].compiled) {
//             regex_indicate(line->column_colors, line->text, line->len,
//                         &GLOBAL_MULTILINE_DEMANDS[e].cmp_expression,
//                         GLOBAL_MULTILINE_DEAMNDS[e].color_code);
//         }


void refresh_expression(Line *l) {
    // runs the compiled regex expressions against a given line
    // does nothing if the line's hlite_NOK is OK

    // if highlight is OK, do nothing
    if (!l->hlite_NOK) { return; }
    // before computing, zero out the line's highlight array (column_colors)
    for (int i = 0; i < l->len; i++) { l->column_colors[i] = 0; }
    for (unsigned int e = 0; e < N_GLOBAL_DEMANDS; e++) {
        if (!GLOBAL_DEMANDS[e].compiled) {
            print_inf(edit_src, "skipping uncompiled regex expression"); continue;
        }
        regex_color(l->column_colors, l->text, l->len,
                    &GLOBAL_DEMANDS[e].cmp_expression, GLOBAL_DEMANDS[e].color_code);
    }
    l->hlite_NOK = 0;
}


void regex_color(short *column_colors, const char *text,
                int len, const regex_t *regxx, int code) {
    // computes a compiled regex expression against a string
    // records all matches

    // starting position is 0
    int pos = 0;
    // maximum number of capture groups used
    regmatch_t pmatch[3];

    // regxx: compiled expression, text + pos: string + starting offset
    // pmatch: capture groups made earlier, flags (0)
    // returns 0 if a match is found & -1 for error
    while (regexec(regxx, text + pos, 3, pmatch, 0) == 0) {
        // if the max capture group has no matches, their rm_so & rm_eo will be -1;
        // check if above 0 to find positive matches to the given expression
        regoff_t so = pmatch[2].rm_so >= 0 ? pmatch[2].rm_so : pmatch[0].rm_so;
        regoff_t eo = pmatch[2].rm_so >= 0 ? pmatch[2].rm_eo : pmatch[0].rm_eo;
        // if the third capture group had no matches, use the first match (if present)
        // if the third capture group had a match, use that match & discard the others

        // for every character from the starting position of the search/computed string
        // plus the start of the match's offset to the end of the match's offset,
        // set the column_color of that char's position to the given expression's color code
        for (regoff_t i = pos + so, x = pos + eo; i < x; i++) {
            if (i >= len) { break; }
            // if (column_colors[i] == 0) {
            column_colors[i] = (short)code;
            // }
        }

        // move the position past the entire match (if match)
        pos += pmatch[0].rm_eo;
        // if the regex expression mapped an empty string (i.e., ""), then
        // the match's starting offset will be equal to the ending offset
        // if that is the case, advance past the match (above) & break out
        if (pmatch[0].rm_eo == pmatch[0].rm_so) { break; }
    }
}


void action_key(Buffer *b, RunTime *rt, Cursor *curs, int ch, const char *path, int mutable) {
    /* only process if mutable, but do it initially */
    if (mutable) {
        if (ch >= 32 && ch < 127) { /* printable ASCII */
            if (dedentable(b, curs, ch)) { /* 'smart' dedent */
                /* if the indent level is corrupted */
                if (curs->col != indent_col(curs)) {
                    if (repair_indent(b, curs)) {
                        curs->action = 1;
                    }
                } /* and then dedent per usual */
                curs->col = indent_col(curs);
                curs->indent_l--;
                if (curs->indent_l < 0) { curs->indent_l = 0; }
                for (int a = 0; a < GLOBAL_INDENT_LEN; a++) {
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

    /* 'utility' key bindings -- escapes & exits, so immutable is hard-stopped after this, always */
    } else if (ch == 27) { /* esc */
        curs->action = 1;
    } else if (ch == KEY_RESIZE) { /* terminal window resized */
        getmaxyx(stdscr, rt->screen_h, rt->screen_w);
        grow_pad(b, rt);
        if (rt->pad == NULL) {
            print_err(edit_src, "NULL pad after RESIZE digest in action_key", 4);
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
            for (int a = 0; a < indent_col(curs); a++) {
                buffer_insert_char(b, curs->row, curs->col, (char)32);
            }
            curs->col += indent_col(curs);

        } else { /* if leaving a line comprised entirely of spaces/tabs, clear it */
            if (whitespace(b, curs->row)) {
                for (int i = 0; i < indent_col(curs); i++) {
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
                for (int a = 0; a < indent_col(curs); a++) {
                    buffer_insert_char(b, curs->row, curs->col, (char)32);
                }
                curs->col = indent_col(curs);
            }
        }

    } else if (ch == KEY_BACKSPACE || ch == 127) {
        /* if the cursor's not at column 0 */
        if (curs->col > 0) {
            buffer_delete_char(b, curs->row, curs->col - 1);
            curs->col--;
        /* otherwise, join the current line w.the previous */
        } else if (curs->row > 0) {
            int prev_len = b->lines[curs->row - 1].len;
            // buffer_join_lines(b, curs->row - 1);
            buffer_join_lines(b, curs->row);
            curs->row--;
            curs->col = prev_len; /* land at join point */
        }

    } else if (ch == '\t') { /* TAB key */
        for (int i = 0; i < GLOBAL_INDENT_LEN; i++) {
            buffer_insert_char(b, curs->row, curs->col, (char)32);
            curs->col++;
        }

    /* ctrl key digest */
    } else if (ch >= 1 && ch <= 26) {
        if (ch == 5) {
            if (mutable) { /* technically, this should be impossible, 
                but being defensive is better than a prayer */
                buffer_duplicate_line(b, curs->row);
                curs->row++; /* move cursor to eol of new line */
            } else {
                curs->sprint = 1;
                curs->smsg = immutable_str;
            }
        } else if (ch == 15) { /* ctrl + o */
            if (mutable) {
                curs->sprint = 1;
                if (!b->dirty) {
                    curs->smsg = no_changes;
                } else if (buffer_writeout(b, path)) {
                    curs->smsg = wo_failure;
                } else {
                    curs->smsg = wo_success;
                }
            } else {
                curs->sprint = 1;
                curs->smsg = immutable_str;
                return; /* do nothing */
            }
        } else if (ch == 24) { /* ctrl + x */
            curs->action = 1;
        } else if (ch == 26) { /* ctrl + z */
            curs->sprint = 1;
            curs->smsg = ctrl_z_str;
        }
    }
}


int repair_indent(Buffer *b, Cursor *curs) {
    // corrects the cursor's indent to the current indent level
    while (1) {
        /* if the indent level doesn't match the column */
        if (curs->col == indent_col(curs)) { break; }
        /* repair the indent level before dedenting */
        else if (curs->col < indent_col(curs)) {
            buffer_insert_char(b, curs->row, curs->col, (char)32);
            curs->col++;
        } else {
            buffer_delete_char(b, curs->row, curs->col - 1);
            curs->col--;
        }
    }
    if (curs->col == indent_col(curs)) { return 0; }
    print_err(edit_src, "failed to repair indent indices", 3);
    return 1;
}


int indentable(Buffer *b, Cursor *curs) {
    // returns 1 if the row & context 'entered on' is valid for indenting
    int n = b->lines[curs->row].len;
    if (!(n > 0)) { return 0; }

    for (int i = 0, x = (int)strlen(GLOBAL_INDENTABLES); i < x; i++) {
        if (b->lines[curs->row].text[n - 1] == GLOBAL_INDENTABLES[i]) {
            return 1;
        }
    }
    return 0;
}


int dedentable(Buffer *b, Cursor *curs, int ch) {
    // returns 1 if the entered char & context is valid for dedenting
    // conditions: cursor is indented, the line's length is
    // more than 0, and the line contains only whitespace
    // this is ran before the char is recorded; technically there is a char
    if (!(curs->indent_l)) { return 0; }
    if (!(b->lines[curs->row].len > 0)) { return 0; }
    if (!whitespace(b, curs->row)) { return 0; }
    for (int i = 0, x = (int)strlen(GLOBAL_DEDENTABLES); i < x; i++) {
        if (ch == GLOBAL_DEDENTABLES[i]) { return 1; }
    }
    return 0;
}


int whitespace(Buffer *b, int row) {
    // returns 1 if the given row contains only whitespace/tabs (no chars)
    for (int i = 0; i < b->lines[row].len; i++) {
        if (b->lines[row].text[i] != ' ' &&
                    b->lines[row].text[i] != '\t') {
            return 0;
        }
    }
    return 1;
}


void grow_pad(Buffer *b, RunTime *rt) {
    // window management for the file-viewing pad;
    // same concept as line & buffer reserve:
    // if inadequate space, double, else OK
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
            print_err(edit_src, "NULL pad while attempting to grow it", 4);
            return;
        }
        keypad(rt->pad, TRUE);
    }
}
