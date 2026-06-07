#ifndef EDITOR_H
#define EDITOR_H

#include <ncurses.h>
#include <regex.h>

#include "buff.h"
#include "highlight.h"

#define MAX_STTM_LEN 39 // 25 // + 14 for col %d line:
#define STATUS_RROWS 1

// what, if anything, has been detected in a paste sequence
typedef enum {
    PASTE_IDLE,
    SEQ_ESC_OK,
    SEQ_BRACK_OK,
    SEQ_TWO_OK,
    SEQ_O_OK,
    SEQ_INIT_OK,
    SEQ_FIN_OK,
    PASTE_ON,
    PASTE_FIN,
} PasteState;

// runtime vars
typedef struct {
    void *pad;    // ncurses WINDOW *
    int max_line; // longest line
    int pad_row;  // top left row of pad
    int pad_col;  // top left col of pad
    int screen_h; // terminal screen height
    int screen_w; // terminal screen width
    int pad_w;    // pad width
    PasteState ps;
} RunTime;

// cursor stats & specs
typedef struct {
    int row;        // cursor row
    int col;        // cursor column
    int wo;         // write out (bool)
    char *smsg;     // status message
    int sprint;     // status message bool
    int action;     // action key result key
    int indent_l;   // level of current indent
    int indent_len; // indent_length; from config
} Cursor;

// marking the begging & end of expression matches
typedef struct {
    int start;
    int end;
} LIndex;

#define check_rc(rc, op, curs) check_rc_impl((rc), (op), (curs), __FILE__, __func__, __LINE__)

// runs the buffer & ncurses window; main manager
void alter_file(Buffer *b, RunTime *rt, Cursor *curs, LangComponents *lc,
    const char *path, FILE *tmp, int MUTABLE);

// // digest key presses
void action_key(Buffer *b, RunTime *rt, Cursor *curs, LangComponents *lc,
    int ch, const char *path, FILE *tmp, int mutable);

// detects (& consumes) chars detected from a Paste Sequence
int paste_sequence(RunTime *rt, int ch);

// repairs indent levels if corrupted
int repair_indent(Buffer *b, Cursor *curs, LangComponents *lc);

// checks character entered for an indent
int indentable(Buffer *b, Cursor *curs, LangComponents *lc);

// checks character entered for a dedent
int dedentable(Buffer *b, Cursor *curs, LangComponents *lc, int ch);

// runs (or decides not to run) the compiled expressions against lines of text
void refresh_expression(LangComponents *lc, Line *l);

// highlights the syntax from a set of predefined RegEx Expressions
void regex_color(Cell *cells, const char *text, int len, const regex_t *regxx,
    int code, attr_t attr);

/* an 'explicit' walker through the whole buffer *
 * looking for initiations and terminations of *
 * multi-line expressions */
void walk_explicit_express(Buffer *b, LangComponents *lc, int dirty);

void color_span(Line *line, int span_so, int span_eo, short color, attr_t attr);

void clear_span(Line *line, int span_so, int span_eo);

// searches a line from pos = pos for the given expression
int regex_search(const char *text, int pos, const regex_t *regxx,
    LIndex *index);

// wipes all whitespace-only lines from the buffer
// (unless cursor is currently on the row to clear)
void clr_empty_lines(Buffer *b, int curr_row);
int all_clear(Line *line);

void grow_pad(Buffer *b, RunTime *rt);

#endif
