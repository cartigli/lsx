#ifndef EDITOR_H
#define EDITOR_H

#include <ncurses.h>
#include <regex.h>

#include "buff.h"
#include "types.h"


/* handles message printing & clearing based on frames */
// void stt_handler(WINDOW *s, const char *msg);

/* add or expand memory for the pad */
void grow_pad(Buffer *b, RunTime *rt);

/* runs the buffer & ncurses window; main manager */
void alter_file(Buffer *b, RunTime *rt, Cursor *curs, const char *path, int MUTABLE);

/* digest key presses */
void action_key(Buffer *b, RunTime *rt, Cursor *curs, int ch, const char *path, int mutable);

/* repairs indent levels if corrupted */
int repair_indent(Buffer *b, Cursor *curs);

/* checks character entered for an indent */
int indentable(Buffer *b, Cursor *curs);

/* checks character entered for a dedent */
int dedentable(Buffer *b, Cursor *curs, int ch);

/* find characters in a given line, if present */
int whitespace(Buffer *b, int row);

// runs (or decides not to run) the compiled expressions against lines of text
void refresh_expression(Line *l);
/* highlights the syntax from a set of predefined RegEx Expressions */
// void regex_color(short *column_colors, const char *text, int len, const regex_t *regxx, int code);
void regex_color(Cell *cells, const char *text, int len, const regex_t *regxx, int code);

void check_multiline_exps(Buffer *b);

void id_multiline_exp_chars(Buffer *b);

void regex_find_inits(Cell *cells, const char *text, int len,
            const regex_t *regxx, int code);
void regex_find_kills(Cell *cells, const char *text, int len,
            const regex_t *regxx);

#endif