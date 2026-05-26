#ifndef EDITOR_H
#define EDITOR_H

#include <ncurses.h>
#include <regex.h>

#include "buff.h"
#include "types.h"


/* handles message printing & clearing based on frames */
void stt_handler(WINDOW *s, const char *msg);

/* add or expand memory for the pad */
void grow_pad(Buffer *b, RunTime *rt);

/* runs the buffer & ncurses window; main manager */
int alter_file(Buffer *b, RunTime *rt, Cursor *curs, const char *path, int MUTABLE);

/* digest key presses */
void action_key(Buffer *b, RunTime *rt, Cursor *curs, int ch, const char *path, int MUTABLE);

/* repairs indent levels if corrupted */
int repair_indent(Buffer *b, Cursor *curs);

/* checks character entered for an indent */
int indentable(Buffer *b, Cursor *curs);

/* checks character entered for a dedent */
int dedentable(Buffer *b, Cursor *curs, int ch);

/* find characters in a given line, if present */
int whitespace(Buffer *b, int row);

/* highlights the syntax from a set of predefined RegEx Expressions */
void regex_color(RunTime *rt, int row, const char *line,
            const regex_t *regxx, int code);


#endif