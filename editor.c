#include <dirent.h>
#include <ncurses.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "editor.h"
#include "error.h"
#include "highlight.h"

static inline int indent_col(Cursor *curs)
{
    return curs->indent_l * GLOBAL_INDENT_LEN;
}

char *immutable_str = "exit: x";
char *default_str   = "exit: ctrl + x";
char *wo_success    = "wroteout buffer";
char *no_changes    = "nothing to save";
char *wo_failure    = "writeout failed";
char *ctrl_z_str    = "ctrl + z pressed";

void alter_file(Buffer *b, RunTime *rt, Cursor *curs, const char *path,
    int mutable)
{
    // runs a window for viewing, and potentially editing, a file

    // ensure pastes get escaped (reset on exit)
    printf("\033[?2004h");
    fflush(stdout);

    // make a pad at the minimum the size of the window,
    // so smaller files still fill the pad amd terminal
    int y      = getmaxy(stdscr);
    int spad_h = b->n_lines + 1 > y ? b->n_lines + 1 : y;
    rt->pad    = newpad(spad_h, rt->pad_w);

    if (rt->pad == NULL) {
        print_err(edit_src, "failed to initiate new pad", 5);
        return;
    }

    int ch;
    // initiate the status message buffer outside the loop
    char mbuff[MAX_STTM_LEN];
    // set the exit command accordingly
    char *default_msg = (mutable) ? default_str : immutable_str;

    // run once initially with the dirty flag
    walk_explicit_express(b, 1);

    clear();
    refresh();
    keypad(rt->pad, TRUE);

    while (1) {
        /* grow pad if needed before redrawing *
         * without it, lines beyond the original pad size *
         * get corrupted & are not shown when traversed to *
         * if writeout & reopen, they appear because the *
         * pad size was originally made the size of b->n_lines + 1 */
        grow_pad(b, rt);

        // status message: cursor's message if exists else default
        char *status_msg = curs->sprint ? curs->smsg : default_msg;
        // print the status along with the cursor's line : all lines
        snprintf(mbuff, sizeof(mbuff), "%s %d:%d", status_msg, curs->row + 1,
            b->n_lines);
        // move to where the longest status message could start
        // if the new message is shorter, dead text should be cleared
        move(rt->screen_h - 1, MAX_STTM_LEN);
        clrtoeol();
        // print the message from the screen width - message length, if
        // the screen width is greater than the message length, else 0
        int mlen = (int)strlen(mbuff);
        int col  = rt->screen_w > mlen ? rt->screen_w - mlen : 0;
        mvprintw(rt->screen_h - 1, col, "%s", mbuff);
        if (curs->sprint) curs->sprint--;

        // stage changes
        wnoutrefresh(stdscr);
        if (!rt->pad) {
            print_err(edit_src, "NULL rt->pad", 5);
            return;
        }
        werase(rt->pad);

        /* only print the lines that will appear in the terminal window *
         * no need to print the whole file, and similarly, the *
         * regex expressions downstream only run over visible lines. *
         * the multi-line expressions cannot be optimized in *
         * this way, however. they could be affected *or affect* *
         * lines long after or before them. They are run over the *
         * entire buffer, on every iteration. The only 'optimization' *
         * it has is skipping the rescan if there are no edits, which *
         * is also why there is one additional walk with the flag *
         * manually set to dirty *before the loop begins* */
        int i_eo = b->n_lines > rt->screen_h
            ? rt->pad_row + rt->screen_h - STATUS_RROWS + 1
            : b->n_lines;
        for (int i_so = rt->pad_row; i_so < i_eo; i_so++) {
            mvwprintw(rt->pad, i_so, 0, "%s", b->lines[i_so].text);
        }

        // clamp the pad to the cursor's position
        // i.e., scroll if the cursor moves past edge
        if (curs->row < rt->pad_row) rt->pad_row = curs->row;

        if (curs->row >= rt->pad_row + rt->screen_h - STATUS_RROWS) {
            rt->pad_row = curs->row - (rt->screen_h - STATUS_RROWS) + 1;
        }
        if (curs->col < rt->pad_col) rt->pad_col = curs->col;

        if (curs->col >= rt->pad_col + rt->screen_w) {
            rt->pad_col = curs->col - (rt->screen_w) + 1;
        }

        // highlight multi-line expressions (whole buffer scan & color)
        walk_explicit_express(b, b->dirty);

        // highlighting text visible in the terminal
        // window (by each line's regex result)
        if (i_eo > b->n_lines) i_eo = b->n_lines;

        for (int i_so = rt->pad_row; i_so < i_eo; i_so++) {
            Line *line = &b->lines[i_so];
            // checks hlite_NOK, i.e., answers:
            // 'was altered/needs highlighting?'
            refresh_expression(line);
            // for each char in the line, apply its color
            for (int i = 0; i < line->len; i++) {
                // if the char's color code is 0: skip
                if (!line->cells[i].color) continue;
                // else, apply that char's color code
                mvwchgat(rt->pad, i_so, i, 1, A_NORMAL, line->cells[i].color,
                    NULL);
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
        if (curs->action) {
            curs->action = 0;
            break;
        }
    }
    delwin(rt->pad);
    printf("\033[?2004h"); // restore terminal state
    fflush(stdout);
    return;
}

void action_key(Buffer *b, RunTime *rt, Cursor *curs, int ch, const char *path,
    int mutable)
{
    // keystroke digest; movement, chars entered, doc altered, etc.,

    // helper for paste sequence; returns 1 if it consumed the char (matched seq)
    if (paste_sequence(rt, ch)) return;

    // always digest key movement
    if (ch == KEY_LEFT) {
        if (curs->col > 0) curs->col--;

        // if the cursor's at col 0, move to EOL of previous line
        else if (curs->row > 0) {
            curs->row--;
            curs->col = b->lines[curs->row].len;
        } // if at line 0, do nothing
    } else if (ch == KEY_RIGHT) {
        if (curs->col < b->lines[curs->row].len) {
            curs->col++;
            // if the cursor is on the last col of a line,
            // (and its not the last line)
            // move to start of the next line
            // ([index] < [count]) || index + 1
        } else if (curs->row + 1 < b->n_lines) {
            curs->row++;
            curs->col = 0;
        }
    } else if (ch == KEY_UP) {
        // if on first line, move cursor to col = 0
        if (curs->row == 0) curs->col = 0;

        else {
            curs->row--;
            // if the new line's length is less than the cursor's col,
            // then the cursor's column = the new lines' length
            if (curs->col > b->lines[curs->row].len) {
                curs->col = b->lines[curs->row].len;
            }
        }
    } else if (ch == KEY_DOWN) {
        // if on the last line, move cursor to EOL
        // ([index] == [count]) || count - 1
        if (curs->row == b->n_lines - 1) {
            // [index] = [count] || count - 1
            curs->col = b->lines[b->n_lines - 1].len;
        } else {
            // else, move down a row & bind cursor col to new line length
            curs->row++;
            if (curs->col > b->lines[curs->row].len) {
                curs->col = b->lines[curs->row].len;
            }
        }

        // from here on, if the state is not mutable
        // do not go past this block & warn if tried
    } else if (!mutable) {
        if (ch == 'x') curs->action = 1;

        else { // warn about editing & return
            curs->sprint = 1;
            curs->smsg   = no_changes;
        }
        // always, always return if IMMUTABLE
        return;

        // printable ASCII
    } else if (ch >= 32 && ch < 127) {

        // anytime a paste is active, do nothing except insert the char
        if (rt->ps != PASTE_IDLE) {
            buffer_insert_char(b, curs->row, curs->col, (char)ch);
            curs->col++;
            
            // 'smart' indent
            // if that given char (& buffer context) is valid case for dedenting
        } else if (dedentable(b, curs, ch)) {
            // if the indent level is corrupted
            if (curs->col != indent_col(curs)) {
                if (repair_indent(b, curs)) {
                    curs->action = 1;
                    return;
                }
            } // repair it, and dedent
            curs->col = indent_col(curs);
            curs->indent_l--;
            if (curs->indent_l < 0) curs->indent_l = 0;

            buffer_clear_n(b, curs->row, curs->col - GLOBAL_INDENT_LEN,
                GLOBAL_INDENT_LEN);
            curs->col -= GLOBAL_INDENT_LEN;

            buffer_insert_char(b, curs->row, curs->col, (char)ch);
            curs->col++;

        } else { // 'normal' case; insert char to buff & move cursor
            buffer_insert_char(b, curs->row, curs->col, (char)ch);
            curs->col++;
        }
        if (rt->max_line < b->lines[curs->row].len) {
            rt->max_line = b->lines[curs->row].len;
        }

        // utility bindings
    } else if (ch == 27) { // ESC
        curs->action = 1;
        return;

        // resized window
    } else if (ch == KEY_RESIZE) {
        getmaxyx(stdscr, rt->screen_h, rt->screen_w);
        grow_pad(b, rt);
        if (rt->pad == NULL) {
            print_err(edit_src, "NULL pad after RESIZE digest", 4);
            return;
        }

        clear();
        refresh();

        // TAB
    } else if (ch == '\t') {
        buffer_insert_n(b, curs->row, curs->col, (char)32,
            GLOBAL_INDENT_LEN);
        curs->col += GLOBAL_INDENT_LEN;

    } else if (ch == '\n' || ch == KEY_ENTER) {
        /* if the user pressed enter (if not pasting), it could *
         * either be an indentable char (i.e., { or [), which *
         * would increment the indent level & indent to it *
         * otherwise, just indent to indent level *
         * (& clear previous line if empty/all whitespace) */

        if (rt->ps != PASTE_IDLE) { // no smart indent
            buffer_split_line(b, curs->row, curs->col);
            curs->row++;
            curs->col = 0;
            return;
        }

        // 'smart' indent
        // if the char underneath the cursor when this enter was
        // executed matches criteria for an indent, then indent
        if (indentable(b, curs)) {
            buffer_split_line(b, curs->row, curs->col);
            curs->row++;
            curs->col = 0;

            // when indentable, make a new line, go to said line at
            // col 0, and add enough spaces to fill the indent level
            curs->indent_l++;
            int curr_indent = indent_col(curs);
            buffer_insert_n(b, curs->row, curs->col, (char)32, curr_indent);
            curs->col += curr_indent;

        } else { // otherwise, clear out empty lines
            if (all_clear(&b->lines[curs->row])) {
                buffer_clear_n(b, curs->row, 0, b->lines[curs->row].len);
                curs->col = 0;
            }
            // make a new line at the current indent level
            buffer_split_line(b, curs->row, curs->col);
            curs->row++;
            curs->col = 0;
            if (curs->indent_l) {
                // bring the new line up to the current indent
                int indent_len = indent_col(curs);
                buffer_insert_n(b, curs->row, curs->col, (char)32, indent_len);
                curs->col = indent_len;
            }
        }

    } else if (ch == KEY_BACKSPACE || ch == 127) {
        if (curs->col > 0) {
            buffer_delete_char(b, curs->row, curs->col - 1);
            curs->col--;
            // if the cursor's at col = 0, deleting a char joins
            // the current line with the previous
        } else if (curs->row > 0) {
            int prev_len = b->lines[curs->row - 1].len;
            buffer_join_lines(b, curs->row);
            curs->row--;
            curs->col = prev_len; // land cursor at join
        }

        // CTRL keys
    } else if (ch >= 1 && ch <= 26) {
        if (ch == 4) {     // CTRL D
            if (mutable) { // impossible...
                buffer_duplicate_line(b, curs->row);
                // move cursor to EOL of new line
                curs->row++;
            } else {
                curs->sprint = 1;
                curs->smsg   = no_changes;
            }
        } else if (ch == 15) { // CTRL O
            if (mutable) {     // ...but defensive
                curs->sprint = 1;
                if (!b->dirty) {
                    curs->smsg = no_changes;
                } else {
                    clr_empty_lines(b, curs->row);
                    int rc = buffer_writeout(b, path);

                    if (rc) curs->smsg = wo_failure;
                    else
                        curs->smsg = wo_success;
                }
            } else {
                curs->sprint = 1;
                curs->smsg   = no_changes;
                // do nothing & warn
                return;
            }
        } else if (ch == 24) { // CTRL X
            curs->action = 1;
        } else if (ch == 26) { // CTRL Z
            curs->sprint = 1;
            curs->smsg   = ctrl_z_str;
        }
    }
}

int paste_sequence(RunTime *rt, int ch)
{
    // detects the escape sequence for a termnial paste

    switch (rt->ps) {
        case PASTE_IDLE:
            if (ch == 27) {
                rt->ps = SEQ_ESC_OK;
                return 1;
            }
            rt->ps = PASTE_IDLE;
            return 0;
        case SEQ_ESC_OK:
            if (ch == '[') {
                rt->ps = SEQ_BRACK_OK;
                return 1;
            }
            rt->ps = PASTE_IDLE;
            return 0;
        case SEQ_BRACK_OK:
            if (ch == '2') {
                rt->ps = SEQ_TWO_OK;
                return 1;
            }
            rt->ps = PASTE_IDLE;
            return 0;
        case SEQ_TWO_OK:
            if (ch == '0') {
                rt->ps = SEQ_O_OK;
                return 1;
            }
            rt->ps = PASTE_IDLE;
            return 0;
        case SEQ_O_OK:
            if (ch == '0') {
                rt->ps = SEQ_INIT_OK;
                return 1;
            } else if (ch == '1') {
                rt->ps = SEQ_FIN_OK;
                return 1;
            }
            rt->ps = PASTE_IDLE;
            return 0;
        case SEQ_INIT_OK:
            if (ch == '~') {
                rt->ps = PASTE_ON;
                return 1;
            }
            rt->ps = PASTE_IDLE;
            return 0;
        case SEQ_FIN_OK:
            if (ch == '~') {
                rt->ps = PASTE_IDLE;
                return 1;
            }
            rt->ps = PASTE_IDLE;
            return 0;
        case PASTE_ON:
            if (ch == 27) {
                rt->ps = SEQ_ESC_OK;
                return 1;
            }
            return 0;
        case PASTE_FIN:
            rt->ps = PASTE_IDLE;
            return 1;
        default:
            return 0;
        }
}

void walk_explicit_express(Buffer *b, int dirty)
{
    /* walks the entire buffer for intiators of multi-line expressions *
     * when one is encountered, start a span, and highlight until the *
     * opening expressions' closer is found */

    if (!dirty) return;

    int n_exps     = (int)N_GLOBAL_MULTILINE_DEMAND_PAIRS;
    int active_exp = -1;
    int in_span    = 0;

    for (int l = 0; l < b->n_lines; l++) {
        Line *line = &b->lines[l];
        // if (!line->hlite_NOK) continue;
        int len    = line->len;
        int cursor = 0;

        while (1) {
            if (cursor >= len) break;

            if (!in_span) {
                // while not in a span, look for initiators on each line

                int vald_init       = -1;
                CharIndex initiator = {.start = len, .end = len};

                // find the first match of any expression initiator
                for (int exp = 0; exp < n_exps; exp++) {
                    // skip uncompiled regex expressions
                    if (!GLOBAL_MULTILINE_PAIRS[exp].ix.compiled) {
                        continue;
                    }
                    // init the competitor for valid comparisons
                    CharIndex competitor = {.start = len, .end = len};
                    if (regex_search(line->text, cursor,
                            &GLOBAL_MULTILINE_PAIRS[exp].ix.cmp_expression,
                            &competitor)) {
                        // if two expressions match the same char, the longer
                        // expression wins, i.e., <"> is beaten out by <""">
                        if (initiator.start > competitor.start ||
                                (initiator.start == competitor.start
                                && initiator.end < competitor.end)) {
                            initiator = competitor;
                            vald_init = exp;
                        }
                    }
                }

                // if the line matched NONE of the openors
                // i.e., if the was never initiator initiated
                if (vald_init == -1) {
                    for (int c = cursor, n = len; c < n; c++) {
                        if (line->cells[c].is_active) {
                            line->hlite_NOK = 1;
                        }
                        line->cells[c].is_active = 0;
                    }
                    // if no initiator found, the  current line's while loop
                    // requires a hard exit to avoid an infinite loop
                    break; // << load bearing break!!
                }

                // if matched, process the initiator
                // skip if expression len = 0
                if (initiator.start == initiator.end) continue;

                in_span = 1; // set the state to in_span
                // ensure non of the previously unmatched chars are highlighted
                active_exp = vald_init;
                for (int c = cursor, n = initiator.start; c < n; c++) {
                    if (line->cells[c].is_active) {
                        line->hlite_NOK = 1;
                    }
                    line->cells[c].is_active = 0;
                }
                // highlight the opener itself (inclusive highlight)
                for (int h = initiator.start, n = initiator.end; h < n; h++) {
                    line->cells[h].color =
                        GLOBAL_MULTILINE_PAIRS[vald_init].ix.color_code;
                    line->cells[h].is_active = 1;
                }
                // advance the cursor & look for it's closer
                cursor = initiator.end;

            } else {
                /* else, if in_span: the only job is to find the closer. *
                 * if its not found, activate & color the whole line before *
                 * moving to next line. if it is found, color from cursor to the *
                 * close (inclusive) & advance cursor. */
                if (active_exp == -1) {
                    print_err(edit_src, "missing terminator", 5);
                    return;
                }
                if (!GLOBAL_MULTILINE_PAIRS[active_exp].kx.compiled) {
                    continue;
                }
                CharIndex close = {.start = len, .end = len};
                if (!regex_search(line->text, cursor,
                        &GLOBAL_MULTILINE_PAIRS[active_exp].kx.cmp_expression,
                        &close)) {
                    // activate & color the whole line
                    for (int c = cursor, n = len; c < n; c++) {
                        line->cells[c].color =
                            GLOBAL_MULTILINE_PAIRS[active_exp].ix.color_code;
                        line->cells[c].is_active = 1;
                    }
                    break;
                } else {
                    // color from cursor to match end,
                    // advance cursor, & close span
                    if (close.start == close.end) continue;

                    for (int c = cursor, n = close.end; c < n; c++) {
                        line->cells[c].color =
                            GLOBAL_MULTILINE_PAIRS[active_exp].ix.color_code;
                        line->cells[c].is_active = 1;
                    }
                    cursor  = close.end;
                    in_span = 0;
                }
            }
        }
    }
}

int regex_search(const char *text, int pos, const regex_t *regxx,
    CharIndex *index)
{
    // searches the passed text from the pos[ition] for the given expression

    regmatch_t pmatch[1];
    while (regexec(regxx, text + pos, 1, pmatch, 0) == 0) {
        regoff_t start_offset = pmatch[0].rm_so;
        regoff_t end_offset   = pmatch[0].rm_eo;

        index->start = pos + start_offset;
        index->end   = pos + end_offset;
        // return only matches with width > 0
        if (index->start < index->end) return 1;

        pos += pmatch[0].rm_eo;
        if (pmatch[0].rm_so == pmatch[0].rm_eo) break;
    }
    return 0;
}

void refresh_expression(Line *l)
{
    // runs the compiled regex expressions against a given line
    // does nothing if the line's hlite_NOK is OK

    // if highlight is OK, do nothing
    if (!l->hlite_NOK) return;

    // before computing, zero out the line's highlight array
    for (int i = 0; i < l->len; i++) {
        if (l->cells[i].is_active) continue;
        l->cells[i].color = 0;
    }

    for (unsigned int e = N_GLOBAL_DEMANDS; e-- > 0;) {
        if (!GLOBAL_DEMANDS[e].compiled) continue;
        regex_color(l->cells, l->text, l->len,
            &GLOBAL_DEMANDS[e].cmp_expression, GLOBAL_DEMANDS[e].color_code);
    }
    l->hlite_NOK = 0;
}

void regex_color(Cell *cells, const char *text, int len, const regex_t *regxx,
    int code)
{
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
        // if the max capture group has no matches, their rm_so & rm_eo will be
        // -1; check if above 0 to find positive matches to the given expression
        regoff_t so = pmatch[2].rm_so >= 0 ? pmatch[2].rm_so : pmatch[0].rm_so;
        regoff_t eo = pmatch[2].rm_so >= 0 ? pmatch[2].rm_eo : pmatch[0].rm_eo;
        // if the third capture group had no matches, use the first match (if
        // present) if the third capture group had a match, use that match &
        // discard the others

        // for every character from the starting position of the search/computed
        // string plus the start of the match's offset to the end of the match's
        // offset, set the column_color of that char's position to the given
        // expression's color code
        for (regoff_t i = pos + so, x = pos + eo; i < x; i++) {
            if (i >= len) break;

            // if the char's current color is 0, replace it
            if (!cells[i].color) cells[i].color = (short)code;
        }

        // move the position past the entire match (if match)
        pos += pmatch[0].rm_eo;
        // if the regex expression mapped an empty string (i.e., ""), then
        // the match's starting offset will be equal to the ending offset
        // if that is the case, advance past the match (done above) & break out
        if (pmatch[0].rm_eo == pmatch[0].rm_so) break;
    }
}

int repair_indent(Buffer *b, Cursor *curs)
{
    // corrects the cursor's indent to the current indent level
    while (1) {
        // if the indent level doesn't match the column
        if (curs->col == indent_col(curs)) break;
        // repair the indent level before dedenting

        else if (curs->col < indent_col(curs)) {
            buffer_insert_char(b, curs->row, curs->col, (char)32);
            curs->col++;
        } else {
            buffer_delete_char(b, curs->row, curs->col - 1);
            curs->col--;
        }
    }
    if (curs->col == indent_col(curs)) return 0;

    print_err(edit_src, "failed to repair indent indices", 3);
    return 1;
}

int indentable(Buffer *b, Cursor *curs)
{
    /* returns 1 if the row & context 'entered on' is valid for indenting *
     * conditions: line length is above 0 and the last *
     * char of the current line is in the current *
     * indentables list of valid chars */

    int n = b->lines[curs->row].len;
    if (n <= 0) return 0;

    for (int i = 0, x = (int)strlen(GLOBAL_INDENTABLES); i < x; i++) {
        if (b->lines[curs->row].text[n - 1] == GLOBAL_INDENTABLES[i]) {
            return 1;
        }
    }
    return 0;
}

int dedentable(Buffer *b, Cursor *curs, int ch)
{
    /* returns 1 if the entered char & context is valid for dedenting *
     * conditions: cursor is indented, the line's length is *
     * more than 0, and the line contains only whitespace *
     * this is ran before the char is recorded; *
     * technically there is a char in this line now */

    if (!(curs->indent_l)) return 0;

    if (curs->col < GLOBAL_INDENT_LEN) return 0;

    if (!(b->lines[curs->row].len > 0)) return 0;

    if (!all_clear(&b->lines[curs->row])) return 0;

    for (int i = 0, x = (int)strlen(GLOBAL_DEDENTABLES); i < x; i++) {
        if (ch == GLOBAL_DEDENTABLES[i]) return 1;
    }
    return 0;
}

int all_clear(Line *line)
{
    // returns 1 if the line contains only whitespace or tabs

    if (line->len <= 0) return 1;

    for (int i = 0, n = line->len; i < n; i++) {
        if (line->text[i] != ' ' && line->text[i] != '\t') {
            return 0;
        }
    }
    return 1;
}

void clr_empty_lines(Buffer *b, int curr_row)
{
    // wipes all lines filled with only tabs or spaces

    for (int i = 0, n = b->n_lines - 1; i < n; i++) {
        // don't erase the cursor's current line
        if (i == curr_row) continue;

        if (all_clear(&b->lines[i])) {
            b->lines[i].len       = 0;
            b->lines[i].text[0]      = '\0';
            b->lines[i].hlite_NOK = 0;
        }
    }
}

void grow_pad(Buffer *b, RunTime *rt)
{
    /* window management for the file-viewing pad; *
     * same concept as line & buffer reserve: *
     * if inadequate space, double, else OK */

    // minimum height >= b->n_lines || rt->screen_w
    int need_h = b->n_lines + 1;
    if (need_h < rt->screen_h) need_h = rt->screen_h;

    int need_w = rt->screen_w > rt->max_line ? rt->screen_w : rt->max_line;

    int cur_h, cur_w;
    getmaxyx(rt->pad, cur_h, cur_w);
    if (need_h > cur_h || need_w > cur_w) {
        delwin(rt->pad);
        int new_h = need_h > cur_h * 2 ? need_h : cur_h * 2;
        int new_w = need_w > cur_w * 2 ? need_w : cur_w * 2;
        rt->pad   = newpad(new_h, new_w);
        if (rt->pad == NULL) {
            print_err(edit_src, "NULL pad while attempting to grow it", 4);
            return;
        }
        keypad(rt->pad, TRUE);
    }
}
