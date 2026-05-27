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


        // identify and highlight multi-line expressions
        check_multiline_exps(b);

        // highlighting text visible in the terminal window (by line)
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
                if (!line->cells[i].color) { continue; }
                // else, apply that char's color code
                mvwchgat(rt->pad, dx, i, 1, A_NORMAL,
                            line->cells[i].color, NULL);
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


void check_multiline_exps(Buffer *b) {
    // scans, indexes, and highlights multi-line expressions throughout the buffer
    for (int i = 0; i < b->n_lines; i++) {
        Line *line = &b->lines[i];
        // multiline could need reprocessing
        // regardless of whether a single line
        // has been edited since the last regex
        // computation & color application, so
        // wipe all current inits & kills before
        for (int c = 0; c < line->len; c++) {
            line->cells[c].mx_line_initr = 0;
            line->cells[c].mx_line_killr = 0;
        }
    }
    for (int i = 0; i < b->n_lines; i++) {
        int demand_pairs = N_GLOBAL_MULTILINE_DEMAND_PAIRS;
        Line *line = &b->lines[i];

        SyntaxDemands *init_demands = GLOBAL_MULTILINE_PAIRS->ix;
        SyntaxDemands *kill_demands = GLOBAL_MULTILINE_PAIRS->kx;

        for (int e = 0; e < demand_pairs; e++) {
            if (init_demands[e].compiled) {
                regex_find_inits(line->cells, line->text, line->len,
                            &init_demands[e].cmp_expression,
                            init_demands[e].color_code);
            }
        }

        for (int o = 0; o < demand_pairs; o++) {
            if (kill_demands[o].compiled) {
                regex_find_kills(line->cells, line->text, line->len,
                            &kill_demands[o].cmp_expression);
            }
        }
    }

    id_multiline_exp_chars(b);
}


void id_multiline_exp_chars(Buffer *b) {
    // PROBLEM: A rogue intiiator without a killer changes the color
    // of every char from there on. This is intended & ideal, but
    // when the initiator is removed, or a killer is added, the text
    // that was previously colored is no longer a part of the multi-
    // line expression, and so each of their colors should be set to
    // 0 and their lines should be flagged for refresh & highlight.
    // Deciding when to reset a char's color & when to label it as an
    // active component of the highlight is the trick & intention here
    //
    // add_astra+ added the new color ML_GRAY for distinction between
    //     single line comments or other gray text in the buffer
    short color = 0; // highlight ? highlight color : 0
    int terminate_ = 0; // is an active highlight ? 1 : 0
    for (int l = 0; l < b->n_lines; l++) {
        Line *line = &b->lines[l];
        for (int c = 0, n = line->len; c < n; c++) {
            Cell *cell = &line->cells[c];
            int was_active = cell->is_active;

            // only one of three can be satisfied
            // for a given character
            // first priority is initiators, then
            // terminators, then color only gets
            // reset *after* the terminator is not
            // detected anymore so it gets colored
            if (cell->mx_line_initr) {
                color = cell->color;
                cell->is_active = 1;
                terminate_ = 0;
            } else if (cell->mx_line_killr) {
                terminate_ = 1; // throw the signal
            } else if (terminate_) {
                color = 0; // reset
                terminate_ = 0; // no term, no init
            }

            if (color) {
                cell->color = color; // highlight it, and
                cell->is_active = 1; // don't erase its highlight
            }
            if (!color) { // if not in an active highlight, show that
                cell->is_active = 0; // but don't set/change its color
                // if the char was an active highlight, but the
                // highlight is currently *inactive*, then refresh
                // that line. but if not, it had no changes, and
                // refreshing means every single line gets refreshed
                if (was_active) { line->hlite_NOK = 1; }
            }
        }
    }
}


void regex_find_inits(Cell *cells, const char *text, int len,
            const regex_t *regxx, int code) {
    int pos = 0;
    regmatch_t pmatch[1]; // only one match for multi-line expressions
    while (regexec(regxx, text + pos, 1, pmatch, 0) == 0) {
        regoff_t start_offset = pmatch[0].rm_so;
        regoff_t end_offset = pmatch[0].rm_eo;
        for (regoff_t i = pos + start_offset; i < pos + end_offset; i++) {
            if (i >= len) { break; }
            cells[i].color = (short)code; // color to highlight match
            cells[i].mx_line_initr = 1; // mark the match as the start of a multi-line expression
        }
        pos += pmatch[0].rm_eo;
        if (pmatch[0].rm_so == pmatch[0].rm_eo) { break; }
    }
}


void regex_find_kills(Cell *cells, const char *text, int len,
            const regex_t *regxx) {
    int pos = 0;
    regmatch_t pmatch[1];
    while (regexec(regxx, text + pos, 1, pmatch, 0) == 0) {
        regoff_t start_offset = pmatch[0].rm_so;
        regoff_t end_offset = pmatch[0].rm_eo;
        for (regoff_t i = pos + start_offset; i < pos + end_offset; i++) {
            if (i >= len) { break; }
            cells[i].mx_line_killr = 1; // mark as the multi-line expression's terminator
        }
        pos += pmatch[0].rm_eo;
        if (pmatch[0].rm_so == pmatch[0].rm_eo) { break; }
    }
}


void refresh_expression(Line *l) {
    // runs the compiled regex expressions against a given line
    // does nothing if the line's hlite_NOK is OK

    // if highlight is OK, do nothing
    if (!l->hlite_NOK) { return; }
    // before computing, zero out the line's highlight array
    for (int i = 0; i < l->len; i++) {
        // if (l->cells[i].color == ML_GRAY) { continue; }
        if (l->cells[i].is_active) { continue; }
        l->cells[i].color = 0;
    }
    for (unsigned int e = 0; e < N_GLOBAL_DEMANDS; e++) {
        if (!GLOBAL_DEMANDS[e].compiled) {
            print_inf(edit_src, "skipping uncompiled regex expression"); continue;
        }
        regex_color(l->cells, l->text, l->len,
                    &GLOBAL_DEMANDS[e].cmp_expression, GLOBAL_DEMANDS[e].color_code);
    }
    l->hlite_NOK = 0;
}


// void regex_color(short *column_colors, const char *text,
void regex_color(Cell *cells, const char *text, int len,
                const regex_t *regxx, int code) {
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
            // if the char's current color is 0, replace it
            if (!cells[i].color) { cells[i].color = (short)code; }
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
