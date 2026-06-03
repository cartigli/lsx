#ifndef EDITOR_H
#define EDITOR_H

#include <ncurses.h>
#include <regex.h>

#include "buff.h"
#include "types.h"

// runs the buffer & ncurses window; main manager
void alter_file(Buffer *b, RunTime *rt, Cursor *curs, const char *path,
    int MUTABLE);

// digest key presses
void action_key(Buffer *b, RunTime *rt, Cursor *curs, int ch, const char *path,
    int mutable);

// detects (& consumes) chars detected from a Paste Sequence
int paste_sequence(RunTime *rt, int ch);

// repairs indent levels if corrupted
int repair_indent(Buffer *b, Cursor *curs);

// checks character entered for an indent
int indentable(Buffer *b, Cursor *curs);

// checks character entered for a dedent
int dedentable(Buffer *b, Cursor *curs, int ch);

// runs (or decides not to run) the compiled expressions against lines of text
void refresh_expression(Line *l);

// highlights the syntax from a set of predefined RegEx Expressions
void regex_color(Cell *cells, const char *text, int len, const regex_t *regxx,
    int code);

typedef struct {
    int start;
    int end;
} CharIndex;

typedef struct {
    CharIndex init;
    CharIndex kill;
} CharIndices;

/* an 'explicit' walker through the whole buffer *
 * looking for initiations and terminations of *
 * multi-line expressions */
void walk_explicit_express(Buffer *b, int dirty);

// searches a line from pos = pos for the given expression
int regex_search(const char *text, int pos, const regex_t *regxx,
    CharIndex *index);

// wipes all whitespace-only lines from the buffer
// (unless cursor is currently on the row to clear)
void clr_empty_lines(Buffer *b, int curr_row);
int all_clear(Line *line);

void grow_pad(Buffer *b, RunTime *rt);

#endif
