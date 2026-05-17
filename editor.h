#ifndef EDITOR_H
#define EDITOR_H

// #include <ncurses.h>
#include <regex.h>

#include "buff.h"


/* file editing */

/* runtime vars */
typedef struct {
    WINDOW  *pad;
    int max_line; /* longest line           */
    int  pad_row; /* top left row of pad    */
    int  pad_col; /* top left col of pad    */
    int screen_h; /* terminal screen height */
    int screen_w; /* terminal screen width  */
    int   view_h; /* view port height       */
    int   view_w; /* view port width        */
    int    pad_w; /* pad width              */
    int       wo; /* write out (bool)       */
    int act_code; /* action key result key  */
    int   sprint; /* status message bool    */
    char   *smsg; /* status message         */
} RunTime;

/* cursor stats & specs */
typedef struct {
    int      row; /* cursor row */
    int      col; /* cursor column */
    int indent_l; /* level of current indent */
    char *indent; /* chars to trigger an indent */
    char *dedent; /* chars to trigger a dedent */
} Cursor;

// /* add or expand memory for the pad */
void grow_pad(Buffer *b, RunTime *rt);

/* runs the buffer & ncurses window; main manager */
int alter_file(Buffer *b, RunTime *rt, Cursor *curs);

/* digest key presses */
void action_key(Buffer *b, RunTime *rt, Cursor *curs, int ch);

/* repairs indent levels if corrupted */
int repair_indent(Buffer *b, Cursor *curs, int indent);

/* checks character entered for an indent */
int indentable(Buffer *b, Cursor *curs);

/* checks if character deleted caused a dedent */
int dedented(Buffer *b, Cursor *curs);

/* checks character entered for a dedent */
int dedentable(Buffer *b, Cursor *curs, int ch);

/* find characters in a given line, if present */
int whitespace(Buffer *b, int row);

/* highlights the syntax from a set of predefined RegEx Expressions */
void regex_color(RunTime *rt, int row, const char *line,
    const regex_t *regxx, int code);

/* frees allocated memory */
void mfree(Buffer *b, RunTime *rt, Cursor *curs);

#endif