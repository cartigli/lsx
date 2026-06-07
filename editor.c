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
#include "fsio.h"
#include "highlight.h"

// status messages
char *immutable_str = "exit: x";
char *default_str   = "exit: ctrl + x";
char *no_changes    = "nothing to save";
char *wo_success    = "wroteout buffer";
char *wo_failure    = "writeout failed";
char *ctrl_z_str    = "ctrl + z pressed";
char *no_edits      = "immutable";

// buffer error messages
char *buf_err       = "edit failed";
char *wo_io         = "disk write failed";
char *stale_buff    = "file-state conflict";
char *wo_perm       = "permission denied";

static inline int indent_col(Cursor *curs, int indent)
{
    return curs->indent_l * indent;
}

static const char *buf_rc_err(int rc)
{
    switch (rc) {
        case BUF_OK: return "OK";
        case BUF_IO: return "IO";
        case BUF_OOB: return "OUT_OF_BOUNDS";
        case BUF_NOMEM: return "OUT_OF_MEMORY";
        case BUF_NOENT: return "FILE_NOT_FOUND";
        case BUF_PERM: return "PERMISSION_DENIED";
        case BUF_STALE: return "STALE_BUFFER";
        default: return "UNKNOWN_ERROR";
    }
}

static int check_rc_impl(int rc, const char *op, Cursor *curs, const char *file, const char *function, int line)
{
    switch (rc) {
        case BUF_OK: return 0;
        case BUF_NOMEM: inter_log(crit, file, function, line, "%s: %s", op, buf_rc_err(rc)); break;
        case BUF_OOB: inter_log(crit, file, function, line, "%s: %s", op, buf_rc_err(rc)); break;
        case BUF_IO: inter_log(crit, file, function, line, "%s: %s", op, buf_rc_err(rc)); break;
        case BUF_NOENT: inter_log(crit, file, function, line, "%s: %s", op, buf_rc_err(rc)); break;
        case BUF_PERM: inter_log(crit, file, function, line, "%s: %s", op, buf_rc_err(rc)); break;
        case BUF_STALE: inter_log(crit, file, function, line, "%s: %s", op, buf_rc_err(rc)); break;
        default: inter_log(crit, file, function, line, "%s: %s", op, buf_rc_err(rc)); break;
    }

    curs->smsg = buf_err;
    return 1;
}

static void wr_status(int rc, Cursor *curs) {
    switch (rc) {
        case BUF_OK: curs->smsg    = wo_success; break;
        case BUF_IO: curs->smsg    = wo_io; break;
        case BUF_PERM: curs->smsg  = wo_perm; break;
        case BUF_STALE: curs->smsg = stale_buff; break;
        default: curs->smsg        = wo_failure; break;
    }
    LOG_INFO("writeout status: %s", curs->smsg);
}

int safe_cp(const char *path, FILE *tmp)
{
    // copy the file before any changes are made

    FILE *src = fopen(path, "r");
    if (!src || ferror(src)) {
        LOG_CRIT("failed to read original file for state saving");
        return 1;
    }

    rewind(tmp); // ensure the pointer is fresh

    char buff[4096];
    size_t bytes;

    while ((bytes = fread(buff, 1, sizeof(buff), src)) > 0) {
        if (fwrite(buff, 1, bytes, tmp) != bytes) {
            fclose(src);
            LOG_ERRO("read error while copying source");
            return 1;
        }
    }

    fflush(tmp);
    fclose(src);
    return 0;
}

void alter_file(Buffer *b, RunTime *rt, Cursor *curs, LangComponents *lc,
    const char *path, FILE *tmp, int mutable)
{
    // runs a window for viewing, and potentially editing, a file

    LOG_DEBUG("editor opened on file: %s", path);
    LOG_DEBUG("mutability: %i", mutable);

    // copy the file to check for mods, if written out
    if (safe_cp(path, tmp)) return;

    // ensure pastes get escaped (reset on exit)
    printf("\033[?2004h");
    fflush(stdout);

    // make a pad at the minimum the size of the window,
    // so smaller files still fill the pad amd terminal
    int y      = getmaxy(stdscr);
    int spad_h = b->n_lines + 1 > y ? b->n_lines + 1 : y;
    rt->pad    = newpad(spad_h, rt->pad_w);

    if (rt->pad == NULL) {
        LOG_CRIT("failed to initiate new pad");
        return;
    }

    int ch;
    // initiate the status message buffer outside the loop
    char mbuff[MAX_STTM_LEN];
    // set the exit command according to the run-mode
    char *default_msg = (mutable) ? default_str : immutable_str;

    // run once initially with the dirty flag
    walk_explicit_express(b, lc, 1);

    clear();
    refresh();
    keypad(rt->pad, TRUE);

    while (1) {
        grow_pad(b, rt);

        // status message: cursor's message if exists else default
        char *status_msg = curs->smsg ? curs->smsg : default_msg;

        // print the status along with the cursor's line : all lines
        snprintf(mbuff, sizeof(mbuff), "%s col: %d line: %d:%d", status_msg,
            curs->col + 1, curs->row + 1, b->n_lines);
        // move to where the longest status message could start
        // if the new message is shorter, dead text should be cleared
        move(rt->screen_h - 1, MAX_STTM_LEN);
        clrtoeol();
        // print the message from the screen width - message length, if
        // the screen width is greater than the message length, else 0
        int mlen = (int)strlen(mbuff);
        int col  = rt->screen_w > mlen ? rt->screen_w - mlen : 0;
        mvprintw(rt->screen_h - 1, col, "%s", mbuff);
        curs->smsg = NULL;

        // stage changes
        wnoutrefresh(stdscr);
        if (!rt->pad) return;
        werase(rt->pad);

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

        // only print lines visible in the terminal window
        // single-line expressions only run on these lines
        // multi-line expressions still run on whole buffer
        int i_eo = b->n_lines > rt->screen_h
            ? rt->pad_row + rt->screen_h - STATUS_RROWS + 1
            : b->n_lines;

        if (i_eo > b->n_lines) i_eo = b->n_lines;
        for (int i_so = rt->pad_row; i_so < i_eo; i_so++) {
            mvwprintw(rt->pad, i_so, 0, "%s", b->lines[i_so].text);
        }

        // highlight multi-line expressions
        // (whole buffer scan & color)
        walk_explicit_express(b, lc, b->dirty);

        // highlighting text visible in the window
        // (by each line's regex match)
        for (int i_so = rt->pad_row; i_so < i_eo; i_so++) {
            Line *line = &b->lines[i_so];
            // checks hlite_NOK, i.e., answers:
            // 'was altered/needs re-highlighting?'
            refresh_expression(lc, line);

            // for each char in the line, apply its color
            for (int i = 0; i < line->len; i++) {
                // if the char's color code is 0: skip
                if (!line->cells[i].color) continue;
                // else, apply that char's color & attr
                mvwchgat(rt->pad, i_so, i, 1, line->cells[i].attr,
                    line->cells[i].color, NULL);
            }
        }

        wmove(rt->pad, curs->row, curs->col); // exit the staging
        pnoutrefresh(rt->pad, rt->pad_row, rt->pad_col, 0, 0,
            rt->screen_h - STATUS_RROWS - 1, rt->screen_w - 1);
        doupdate(); // update the screen

        ch = wgetch(rt->pad); // digest any & all keypresseses
        action_key(b, rt, curs, lc, ch, path, tmp, mutable);
        if (curs->action) {
            curs->action = 0;
            break;
        }
    }
    delwin(rt->pad);
    printf("\033[?2004l"); // restore terminal state
    fflush(stdout);
    return;
}

void action_key(Buffer *b, RunTime *rt, Cursor *curs, LangComponents *lc,
    int ch, const char *path, FILE *tmp, int mutable)
{
    // keystroke digest; movement, chars entered, doc altered, etc.,

    int rc;
    // helper for paste sequence; returns 1
    // if it consumed the char (matched seq)
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
        if (curs->row >= b->n_lines - 1) {
            if (curs->row == b->n_lines - 1) {
                curs->col = b->lines[curs->row].len;
            }
            curs->row = b->n_lines - 1;
        } else {
            // else, move down a row & bind
            // cursor col to new line length
            curs->row++;
            if (curs->col > b->lines[curs->row].len) {
                curs->col = b->lines[curs->row].len;
            }
        }

        // from here on, if the state is not mutable
        // do not go past this block & warn if tried
    } else if (!mutable) {
        if (ch == 'x') curs->action = 1;

        else {
            // warn about editing & return
            curs->smsg = no_edits;
        }
        return; // always return if IMMUTABLE

        // printable ASCII
    } else if (ch >= 32 && ch < 127) {

        // anytime a paste is active, do nothing except insert the char
        if (rt->ps != PASTE_IDLE) {
            rc = buffer_insert_char(b, curs->row, curs->col, (char)ch);
            if (check_rc(rc, "insert_char (paste)", curs)) return;
            curs->col++;

            // 'smart' de-dent
            // if that given char (& buffer context)
            // is valid case for dedenting
        } else if (dedentable(b, curs, lc, ch)) {

            // if the indent level is corrupted
            if (curs->col != indent_col(curs, lc->indent_len)) {
                if (repair_indent(b, curs, lc)) {
                    curs->action = 1;
                    return;
                }
            }

            // repair it, and dedent
            curs->col = indent_col(curs, lc->indent_len);
            curs->indent_l--;
            if (curs->indent_l < 0) curs->indent_l = 0;

            rc = buffer_clear_n(b, curs->row, curs->col - lc->indent_len,
                lc->indent_len);
            if (check_rc(rc, "clear_n (dedent)", curs)) return;
            curs->col -= lc->indent_len;

            rc = buffer_insert_char(b, curs->row, curs->col, (char)ch);
            if (check_rc(rc, "insert_char (dedent)", curs)) return;
            curs->col++;

        } else { // 'normal' case; insert char to buff & move cursor
            rc = buffer_insert_char(b, curs->row, curs->col, (char)ch);
            if (check_rc(rc, "insert_char", curs)) return;
            curs->col++;
        }
        if (rt->max_line < b->lines[curs->row].len) {
            rt->max_line = b->lines[curs->row].len;
        }

    } else if (ch == '\n' || ch == KEY_ENTER) {
        /* if the user pressed enter (if not pasting), it could *
         * either be an indentable char (i.e., { or [), which *
         * would increment the indent level & indent to it *
         * otherwise, just indent to indent level *
         * (& clear previous line if empty/all whitespace) */
        if (rt->ps != PASTE_IDLE) { // no smart indent
            rc = buffer_split_line(b, curs->row, curs->col);
            if (check_rc(rc, "split_line (paste)", curs)) return;
            curs->row++;
            curs->col = 0;
            return;
        }

        // 'smart' indent
        // if the char underneath the cursor when this enter was
        // executed matches criteria for an indent, then indent
        if (indentable(b, curs, lc)) {
            rc = buffer_split_line(b, curs->row, curs->col);
            if (check_rc(rc, "split_line (indent)", curs)) return;
            curs->row++;
            curs->col = 0;
            // when indentable, make a new line, go to said line at
            // col 0, and add enough spaces to fill the indent level
            curs->indent_l++;
            int curr_indent = indent_col(curs, lc->indent_len);
            rc = buffer_insert_n(b, curs->row, curs->col, (char)32, curr_indent);
            if (check_rc(rc, "insert_n (indent)", curs)) return;
            curs->col += curr_indent;

        } else { // otherwise, clear out empty lines
            if (all_clear(&b->lines[curs->row])) {
                rc = buffer_clear_n(b, curs->row, 0, b->lines[curs->row].len);
                if (check_rc(rc, "clear_n (clear whitespace)", curs)) return;
                curs->col = 0;
            }
            // make a new line at the current indent level
            rc = buffer_split_line(b, curs->row, curs->col);
            if (check_rc(rc, "split_line", curs)) return;
            curs->row++;
            curs->col = 0;
            if (curs->indent_l) {
                // bring the new line up to the current indent
                int indent_len = indent_col(curs, lc->indent_len);
                rc = buffer_insert_n(b, curs->row, curs->col, (char)32, indent_len);
                if (check_rc(rc, "insert_n (indent)", curs)) return;
                curs->col = indent_len;
            }
        }

    } else if (ch == KEY_BACKSPACE || ch == 127) {
        if (curs->col > 0) {
            rc = buffer_delete_char(b, curs->row, curs->col - 1);
            if (check_rc(rc, "delete_char", curs)) return;
            curs->col--;
            // if the cursor's at col = 0, deleting a char joins
            // the current line with the previous
        } else if (curs->row > 0) {
            int prev_len = b->lines[curs->row - 1].len;
            rc = buffer_join_lines(b, curs->row);
            if (check_rc(rc, "join_lines", curs)) return;
            curs->row--;
            curs->col = prev_len; // cursor lands at join
        }

        // ESC
    } else if (ch == 27) {
        curs->action = 1;
        return;

        // resized window
    } else if (ch == KEY_RESIZE) {
        getmaxyx(stdscr, rt->screen_h, rt->screen_w);
        grow_pad(b, rt);
        if (rt->pad == NULL) {
            LOG_CRIT("NULL pad after resize digest");
            return;
        }
        clear();
        refresh();

        // TAB
    } else if (ch == '\t') {
        rc = buffer_insert_n(b, curs->row, curs->col, (char)32, lc->indent_len);
        if (check_rc(rc, "insert_n (tab)", curs)) return;
        curs->col += lc->indent_len;

        // CTRL keys
    } else if (ch >= 1 && ch <= 26) {
        // CTRL D - duplicate line
        if (ch == 4) {
            if (mutable) { // impossible...
                rc = buffer_duplicate_line(b, curs->row);
                if (check_rc(rc, "duplicate_line", curs)) return;
                curs->row++;
            } else {
                curs->smsg   = no_edits;
            }
            // CTRL O - writeout/save
        } else if (ch == 15) {
            if (mutable) { // ...but defensive
                if (!b->dirty) curs->smsg = no_changes;
                else {
                    clr_empty_lines(b, curs->row);
                    rc = buffer_writeout(b, path, tmp);
                    wr_status(rc, curs);
                    return;
                }
            } else {
                curs->smsg   = no_edits;
                return; // do nothing & warn
            }
            // CTRL X
        } else if (ch == 24) {
            curs->action = 1;
            // CTRL Z
        } else if (ch == 26) {
            curs->smsg   = ctrl_z_str;
        }
    }
}

int paste_sequence(RunTime *rt, int ch)
{
    // detects the escape sequence for a termnial paste
    // swallows key strokes

    switch (rt->ps) {
        case PASTE_IDLE:
            if (ch == 27) {
                LOG_DEBUG("paste sequence intiator detected");
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
                LOG_DEBUG("paste sequence detected");
                return 1;
            }
            rt->ps = PASTE_IDLE;
            return 0;
        case SEQ_FIN_OK:
            if (ch == '~') {
                rt->ps = PASTE_IDLE;
                LOG_DEBUG("paste sequence terminated");
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

void walk_explicit_express(Buffer *b, LangComponents *lc, int dirty)
{
    /* walks the entire buffer for intiators of multi-line expressions *
     * when one is encountered, start a span, and highlight until the *
     * opening expressions' closer is found */

    if (!dirty) return;

    int n_exps     = lc->n_mx_demands;
    int active_exp = -1;
    int in_span    = 0;

    for (int l = 0; l < b->n_lines; l++) {
        Line *line = &b->lines[l];
        int len    = line->len;
        int cursor = 0;

        while (1) {
            if (cursor >= len) break;

            if (!in_span) {
                // while not in a span, look for initiators on each line
                int vald_init = -1;
                LIndex first  = {.start = len, .end = len};

                // find the first match of any expression initiator
                for (int exp = 0; exp < n_exps; exp++) {
                    // skip uncompiled regex expressions
                    if (!lc->rt_mxi[exp].cached) {
                        LOG_DEBUG("skipping uncompiled regex: %d", exp);
                        continue;
                    }

                    // init the competitor for valid comparisons
                    LIndex comp = {.start = len, .end = len};

                    if (!regex_search(line->text, cursor,
                            &lc->rt_mxi[exp].cache, &comp)) {
                        continue;
                    }
                    // if two expressions match the same char, the longer
                    // expression wins, i.e., * is beaten out by **
                    if (first.start > comp.start ||
                        (first.start == comp.start && first.end < comp.end)) {
                        vald_init = exp;
                        first     = comp;
                    }
                }

                // if the line matched NONE of the openors
                // i.e., if the was never first initiated
                if (vald_init == -1) {
                    clear_span(line, cursor, len);
                    break; // << load bearing break!!
                }

                // if matched, process the first
                // skip if expression len = 0
                if (first.start == first.end) {
                    cursor++;
                    continue;
                }

                in_span = 1; // set the state to in_span
                // ensure non of the previously unmatched chars are highlighted
                active_exp = vald_init;
                clear_span(line, cursor, first.start);

                // highlight the opener itself (inclusive highlight)
                const SyntaxSpan *ss = &lc->mx_demands[vald_init];
                color_span(line, first.start, first.end, ss->color, ss->attr);
                // advance the cursor & look for it's closer
                cursor = first.end;
                continue; // no else indentation
            }

            /* if in_span: the only job is to find the closer.
             * if its not found, activate & color the whole line before
             * moving to next line. if it is found, color from cursor to the
             * sfin (inclusive) & advance cursor. */
            const SyntaxSpan *ss = &lc->mx_demands[active_exp];
            RGXE *mxk            = &lc->rt_mxk[active_exp];

            LIndex sfin = {.start = len, .end = len};
            if (!regex_search(line->text, cursor, &mxk->cache, &sfin)) {
                color_span(line, cursor, len, ss->color, ss->attr);
                break;
            }
            // color from cursor to match end,
            // advance cursor, & close the span
            if (sfin.start == sfin.end) {
                cursor++;
                continue;
            }

            color_span(line, cursor, sfin.end, ss->color, ss->attr);
            cursor  = sfin.end;
            in_span = 0;
        }
    }
}

void color_span(Line *line, int span_so, int span_eo, short color, attr_t attr)
{
    for (int i = span_so; i < span_eo; i++) {
        line->cells[i].color     = color;
        line->cells[i].attr      = attr;
        line->cells[i].is_active = 1;
    }
}

void clear_span(Line *line, int span_so, int span_eo)
{
    for (int i = span_so; i < span_eo; i++) {
        if (line->cells[i].is_active) line->hlite_NOK = 1;
        line->cells[i].is_active = 0;
    }
}

int regex_search(const char *text, int pos, const regex_t *regxx, LIndex *index)
{
    // searches the passed text from the pos[ition] for the given expression

    regmatch_t pmatch[1]; // one capture group

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

void refresh_expression(LangComponents *lc, Line *l)
{
    // runs the compiled regex expressions against a given line
    // does nothing if the line's hlite_NOK is OK

    if (!l->hlite_NOK) return; // if highlight is OK, do nothing

    // before computing, zero out the line's highlight array
    for (int i = 0; i < l->len; i++) {
        // if its an active multi-line expression,
        // it gets precendence
        if (l->cells[i].is_active) continue;

        l->cells[i].color = 0;
        l->cells[i].attr  = A_NORMAL;
    }

    for (int e = lc->n_demands; e-- > 0;) {
        if (!lc->rt_dmds[e].cached) continue;
        regex_color(l->cells, l->text, l->len, &lc->rt_dmds[e].cache,
            lc->demands[e].color, lc->demands[e].attr);
    }

    l->hlite_NOK = 0;
}

void regex_color(Cell *cells, const char *text, int len, const regex_t *regxx,
    int code, attr_t attr)
{
    // computes a compiled regex expression
    // against a string; records all matches

    int pos = 0;          // starting position is 0
    regmatch_t pmatch[3]; // maximum number of capture groups used

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
            if (!cells[i].color) {
                cells[i].color = (short)code;
                cells[i].attr  = attr;
            }
        }

        // move the position past the entire match (if match)
        pos += pmatch[0].rm_eo;
        // if the regex expression mapped an empty string (i.e., ""), then
        // the match's starting offset will be equal to the ending offset
        // if that is the case, advance past the match (done above) & break out
        if (pmatch[0].rm_eo == pmatch[0].rm_so) break;
    }
}

int repair_indent(Buffer *b, Cursor *curs, LangComponents *lc)
{
    // corrects the cursor's indent to the current indent level

    int rc;
    LOG_DEBUG("repairing indent...");
    while (1) {
        // if the indent level matches the column
        if (curs->col == indent_col(curs, lc->indent_len)) break;

        // repair the indent
        else if (curs->col < indent_col(curs, lc->indent_len)) {
            rc = buffer_insert_char(b, curs->row, curs->col, (char)32);
            if (check_rc(rc, "insert_char", curs)) return 1;
            curs->col++;
        } else {
            rc = buffer_delete_char(b, curs->row, curs->col - 1);
            if (check_rc(rc, "delete_char", curs)) return 1;
            curs->col--;
        }
    }
    if (curs->col == indent_col(curs, lc->indent_len)) {
        LOG_DEBUG("indent corrected OK");
        return 0;
    }

    // print_err(edit_src, "failed to repair indent indices", 3);
    LOG_WARN("failed to repair indent indices");
    return 1;
}

int indentable(Buffer *b, Cursor *curs, LangComponents *lc)
{
    /* returns 1 if the row & context 'entered on' is valid for indenting *
     * conditions: line length is above 0 and the last *
     * char of the current line is in the current *
     * indentables list of valid chars */

    int n = b->lines[curs->row].len;
    if (n <= 0) return 0;

    for (int i = 0, x = lc->n_dentables; i < x; i++) {
        if (b->lines[curs->row].text[n - 1] == lc->indentables[i]) {
            return 1;
        }
    }
    return 0;
}

int dedentable(Buffer *b, Cursor *curs, LangComponents *lc, int ch)
{
    /* returns 1 if the entered char & context is valid for dedenting *
     * conditions: cursor is indented, the line's length is *
     * more than 0, and the line contains only whitespace *
     * this is ran before the char is recorded; *
     * technically there is a char in this line now */

    if (!(curs->indent_l)) return 0;

    if (curs->col < lc->indent_len) return 0;

    if (!(b->lines[curs->row].len > 0)) return 0;

    if (!all_clear(&b->lines[curs->row])) return 0;

    for (int i = 0, x = lc->n_dentables; i < x; i++) {
        if (ch == lc->dedentables[i]) return 1;
    }
    return 0;
}

int all_clear(Line *line)
{
    // returns 1 if the line contains only whitespace or tabs

    if (line->len <= 0) return 0;

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
            b->lines[i].hlite_NOK = 0;
            b->lines[i].text[0]   = '\0';
        }
    }
}

void grow_pad(Buffer *b, RunTime *rt)
{
    /* window management for the file-viewing pad;
     * same concept as line & buffer reserve:
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
        LOG_INFO("current_H: %d, new_H: %d; curr_W: %d, new_W: %d", cur_h, new_h, cur_w, new_w);

        rt->pad = newpad(new_h, new_w);
        if (rt->pad == NULL) {
            LOG_CRIT("NULL pad while trying to grow it");
            return;
        }
        keypad(rt->pad, TRUE);
    }
}
