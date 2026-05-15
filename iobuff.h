#ifndef IO_BUFF_H
#define IO_BUFF_H

/* file editing */

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
int hex_compr(const char c[]);

static const struct {
    // const char color[8];
    // int pair_no;
    const char r[4];
    const char g[4];
    const char b[4];
} COLOR_CODES[] = {
    /* { "pink",     1, */ { "f6", "aa", "d3" }, /* keywords, types, preprocessors */
    /* { "green",    2, */ { "23", "ad", "61" }, /* functions */
    /* { "purple",   3, */ { "c3", "91", "ed" }, /* integers/decimals */ /* & substitutions inside strings */ /* constants & constant builtins */
    /* { "cyan",     4, */ { "87", "ce", "eb" }, /* type-set variables */
    /* { "ltgray",   5, */ { "b3", "b3", "b3" }, /* comments */
    /* { "yellow",   6, */ { "fa", "f1", "87" }, /* strings, headers */
    /* { "reddsh",   7, */ { "e6", "67", "6b" }  /* operands */
};

/* static: made once in memory and lasts only for runtime *
* const: not mutated; raise a compiler warning if altered *
* RULES[]: defined the struct as an array of structs      *
* **order is important now**: if the comment string were not last, *
* things like strings, variables, or otheriwse highlighted text in *
* the comment would show. So consider it intentional. */
static const struct { const char *exp; int pair; } RULES[] = {
    /* integers */
    { "[[:space:]]{0,}[[:digit:]]*[[:space:]]{0,}", 3 },
    /* decimals */
    { "^[[:digit:]][[:digit:].]*",                  3 },
    /* an exact match to the words followed by one or more spaces followed by letters, *
    * asteriks, or undersctores followed by a semicolon, comma, or parenthesis, or space */
    /* type-set variables */
    { "(int|float|double|long|void|char)[[:space:]]{1,}"
        "([[:alnum:]_*][[:alnum:]_]*)[;|,|)|[[:space:]]*]", 4 },

    /* functions */
    /* one or more of anything but a space followed by an open parenthesis */
    { "([^[:space:]()]+)\\(",                       2 },
    /* operands */
    /* list of operands (backslashes are escaping themselves) (all exact matches) */
    { "[*/\\<>%=^+-]",                            7 },
    /* preprocessors */
    /* a hashtag followed by one or more characters including underscores */
    { "#[a-zA-Z_]+",                                1 },
    /* keywords (numerical) */
    /* numeric keywords found with no leading or trailling letters or underscores that match exactly */
    { "(^|[^a-zA-Z_])(int|float|double|unsigned|const|"
        "long|char|NULL|void)([^a-zA-Z_]|$)",       1 },
    /* keywords (flow control) */
    /* ditto as above - builtin keywords */
    { "(^|[^a-zA-Z_])(return|if|else|while|for|"
        "typedef|struct)([^a-zA-Z_]|$)",            1 },

    /* punctuation (gray) (b3b3b3)^ */
    /* punctuations to highlight (exact matches) */
    { "[].,!?:;'[{}()]",                            5 }, 

    /* header specifications (yell)^ */
    /* a < following by any character + . + / except a > one or more times until a > */
    { "<[^>][[:alnum:]./]*>",                       6 },

    /* strings (ran after integers, comments, functions, & specials */
    /* a double quote followed by anything but a double quote until a double quote is found */
    { "\"([^\"]*)\"",                               6 },
    /* substitutions in strings (ran after strings) */
    { "%[s|i|l|p]",                                 3 },

    /* comments (workaround p0) */
    /* from a </-*>, skip everything that's not an asterik, and everything that's not an *
     * asterik immediately followed by backslash until a pair is found | too much for too little; 
     * the current function form can't wrap multi line expressions anyway */
    { "/\\*[^\n]*[\\*|\\*/]",                       5 },
    /* comments (work around p1) */
    { "\\*[[:space:]][^\n]*\\*/",                   5 },
    /* comments 0 */
    /* from any <//> until the end of the line */
    { "//[^\n]*",                                   5 }
};

#endif