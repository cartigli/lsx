#ifndef TYPES_H
#define TYPES_H

#include "regex.h"


/* error */
#define LOG_DEST "lx.log"
#define MAX_ERR_LEN 1024

/* editor */
#define MAX_STTM_LEN 25
#define STATUS_RROWS 1

/* fsio */
/* (POSIX) byte size per block on disk */
#define ST_BLOCK_SIZE 512
/* max permissable chars in a filename */
#define MAX_FILENAME 1024

/* note: the cwd found holds the root path & has no parent */
/* self referencing struct for all filesystem instances */
typedef struct FSNode {
    int     is_dir;
    long    blocks;
    int n_children;
    int     n_dirs;
    char  name[MAX_FILENAME];
    struct FSNode** children;
    struct FSNode*    parent;
} FSNode;

/* runtime mode */
typedef enum {
    MENU_MODE,
    EDIT_MODE,
} MODE;

/* runtime config */
typedef struct {
    char *root;
    MODE mode;

    int mutable;
    int hide_size;
    int verbosity;
    int indent_len;

    int colors_loaded;

    const char *indent_chars;
    const char *dedent_chars;
} Config;


/* runtime vars */
typedef struct {
    void    *pad; /* ncurses WINDOW * */
    int max_line; /* longest line           */
    int  pad_row; /* top left row of pad    */
    int  pad_col; /* top left col of pad    */
    int screen_h; /* terminal screen height */
    int screen_w; /* terminal screen width  */
    int    pad_w; /* pad width              */
} RunTime;

/* cursor stats & specs */
typedef struct {
    int      row; /* cursor row */
    int      col; /* cursor column */
    int       wo; /* write out (bool)       */
    char   *smsg; /* status message         */
    int   sprint; /* status message bool    */
    int   action; /* action key result key  */
    int indent_l; /* level of current indent */
    int indent_len;
} Cursor;


/* runtime vars for menu */
typedef struct {
    void    *main; /* ncurses WINDOW * */
    int    action;
    int    choice;
    int  v_choice;
    int max_lenfn;
    int   padding;
    int    n_cols;
    int col_width;
    int     v_lim;
    int    ff_row;
} Mstate;

/* cross file communication & status state */
typedef struct {
    const Config *config;
    FSNode   *root; /* original so tree can still be freed */
    FSNode     *cd; /* current selection to view (dir or file) */
    Mstate     *ms;

    char  *stt_msg; /* status message to show */
    int     frames; /* 'frames' for msg to show */

    int  intention; /* nav to: 1, read: 2, or edit: 3 */
    int  padd_size; /* block_cushion */
} MGMT;


typedef enum {
    c,
    py,
    blank,
    LANG_COUNT
} language;


enum COLORS {
    PINK   = 1,
    GREEN  = 2,
    PURPLE = 3,
    CYAN   = 4,
    LTGRAY = 5,
    YELLOW = 6,
    REDDSH = 7,
    TEAL   = 8,
    ORANGE = 9,
    PY_CYAN = 10,
    PY_PURPLE = 11,
    PY_GREEN = 12,
};

/* enforce the order of expressions *
 * comprehended by the regex engine */
 typedef enum {
    NUMERICALS,  /* 1, 23 */
    VARIABLES,   /* int i; */
    FUNCTIONS,   /* main() */
    OPERANDS,    /* 20 * 5 */
    PREPROCS,    /* #include */
    HEADERS,     /* regex.h */
    KEYWORDS,    /* return; */
    PUNCTUATION, /* , { ; */
    STRINGS,     /* "hello world" */
    SUBSTITUTES, /* "hello %s" */
    COMMENTS,    /* // comment */
} CmpOrder;


// typedef struct {
//     int r;
//     int g;
//     int b;
// } ColorCode;

typedef struct {
    char r[4];
    char g[4];
    char b[4];
} ColorCode;


typedef struct {
    const char *expression; /* RegEx expression (string) */
    regex_t cmp_expression; /* compiled RegEx expression */
    enum COLORS color_code; /* a code to RGB code in hex */
    CmpOrder          type; /* the type of expression */
    int           compiled; /* bool for a valid compile */
} SyntaxDemands;


#endif