#ifndef IO_BUFF_H
#define IO_BUFF_H

/* buffer initiation, RegEx Expressions building, & RunTime declarations */
// Buffer *buffer_load(const char *path);
// Expressions *init_regex(void);
// RunTime *init_rt_vars(Buffer *b);


/* file_edit */

typedef struct { /* for each line in the file */
    char *data;  /* line contents             */
    int    len;  /* strlen of the line cached */
    int    cap;  /* single line's capacity; allocated bytes; always >= len + 1 */
} Line;

typedef struct {   /* buffer created from file on disk; mutable source of truth */
    Line   *lines; /* array of lines                     */
    int   n_lines; /* lines currently in use             */
    int cap_lines; /* capacity of lines; lines allocated */
    int     dirty; /* unsaved changes                    */
} Buffer;

Buffer *buffer_load(const char *path);

/* regex expressions */

typedef struct v_class { /* RegEx expressions & their colors + cache */
    const char *exp;     /* RegEx expression  */
    int        pair;     /* color pair        */
    regex_t   regxx;     /* cached expression */
    int         val;     /* bool for cached or not    */
} v_class;

typedef struct {        /* all expressions           */
    int         n_exps; /* no. of expressions stored */
    v_class* v_classes; /* array of v_class structs  */
} Expressions;

Expressions *init_regex(void);

/* runtime vars */

typedef struct {  /* cached vars for the ncurses window/pad */
    int max_line; /* longest line */
    int  pad_row; /* top left row of pad */
    int  pad_col; /* top left col of pad */
    int curs_row; /* cursor current row */
    int curs_col; /* cursor current col */
    int screen_h; /* current terminal screen height */
    int screen_w; /* current terminal screen width */
    int   view_h; /* view port height */
    int   view_w; /* view port width */
    int    pad_w; /* pad width */
    int       wo; /* write out (treated as bool) */
    int act_code; /* returned from action key */
    int   sprint; /* status message bool */
    char   *smsg; /* status message */
} RunTime;

RunTime *init_rt_vars(Buffer *b);

/* runs the buffer & ncurses window; main manager */
int alter_file(Buffer *b, Expressions *exps, RunTime *rt);

/* add or expand memory for the modified buffer */
static void line_reserve(Line *l, int need);
static void buffer_reserve(Buffer *b, int need);
WINDOW* grow_pad(WINDOW* pad, Buffer *b, RunTime *rt);

/* the four main modifications: insert, delete, split, join */
void buffer_insert_char(Buffer *b, int row, int col, char c);
void buffer_delete_char(Buffer *b, int row, int col);
void buffer_split_line(Buffer *b, int row, int col);
void buffer_join_lines(Buffer *b, int row);

/* digest key presses */
void action_key(WINDOW* pad, Buffer *b, RunTime *rt, int ch);

/* highlights the syntax from a set of predefined RegEx Expressions */
void regex_color(WINDOW* pad, int row, const char *line,
    const regex_t *regxx, int pair);

/* writes the modified buffer to the disk */
int buffer_writeout(Buffer *b, const char *path);

/* frees allocated memory */
void mfree(Buffer *b, Expressions *exps, RunTime *rt);

/* intializes screen dimensions, attributes, & elements */
int init_scr(void);

/* static: made once in memory and lasts only for runtime *
* const: not mutated; raise a compiler warning if altered *
* RULES[]: defined the struct as an array of structs      */
static const struct { const char *exp; int pair; } RULES[] = {
    { "([^[:space:]()]+)\\(",                              1 }, /* functions */
    { "[*/\\<>%=^+-]",                                     2 }, /* operands */
    { "\"([^\"]*)\"",                                      2 }, /* strings */
    { "#[a-zA-Z_]+",                                       2 }, /* <headers.h> */
    { "[].,!?:;'[{}()]",                                   2 }, /* punctuation */
    { "(^|[^a-zA-Z_])(int|float|double|unsigned"
        "|long|char|NULL|void)([^a-zA-Z_]|$)",             2 }, /* keywords (numerical) */
    { "(^|[^a-zA-Z_])(return|if|while|for)([^a-zA-Z_]|$)", 2 }, /* keywords (flow control) */
    { "(^|[^a-zA-Z_])(typedef|struct)([^a-zA-Z_]|$)",      2 } /* keywords (built-in) */
};

#endif