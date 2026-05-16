#ifndef IO_BUFF_H
#define IO_BUFF_H

/* file editing */

/* for each line in the file */
typedef struct {
    char *data; /* line contents              */
    int    len; /* strlen of the line cached  */
    int    cap; /* single line's capacity; allocated bytes; always >= len + 1 */
} Line;

/* buffer created from file on disk; mutable source of truth */
typedef struct {
    Line   *lines; /* array of lines                     */
    int   n_lines; /* lines currently in use             */
    int cap_lines; /* capacity of lines; lines allocated */
    int     dirty; /* unsaved changes                    */
} Buffer;

/* runtime vars */
typedef struct {
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

Buffer *buffer_load(const char *path);

RunTime *init_rt_vars(Buffer *b);

Cursor *init_cursor(void);

/* compiles and caches regex expressions */
int compile_regex(void);

/* add or expand memory for the modified buffer */
static void line_reserve(Line *l, int need);
static void buffer_reserve(Buffer *b, int need);
WINDOW* grow_pad(WINDOW* pad, Buffer *b, RunTime *rt);

/* runs the buffer & ncurses window; main manager */
int alter_file(Buffer *b, RunTime *rt, Cursor *curs);

/* the four main modifications: insert, delete, split, join */
void buffer_insert_char(Buffer *b, int row, int col, char c);
void buffer_delete_char(Buffer *b, int row, int col);
void buffer_split_line(Buffer *b, int row, int col);
void buffer_join_lines(Buffer *b, int row);

/* digest key presses */
void action_key(WINDOW* pad, Buffer *b, RunTime *rt, Cursor *curs, int ch);

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
void regex_color(WINDOW* pad, int row, const char *line,
    const regex_t *regxx, int code);

/* writes the modified buffer to the disk (saves file) */
int buffer_writeout(Buffer *b, const char *path);

/* frees allocated memory */
void mfree(Buffer *b, RunTime *rt, Cursor *curs);

#endif