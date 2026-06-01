#ifndef TYPES_H
#define TYPES_H

#include "regex.h"

// error
#define LOG_DEST "lx.log"
#define MAX_ERR_LEN 1024

// editor
#define MAX_STTM_LEN 25
#define STATUS_RROWS 1

// fsio
// POSIZ byte size of a block on disk
#define ST_BLOCK_SIZE 512
// maximum chars in a filename's buffer
#define MAX_FILENAME 1024

// seld referencing struct for recording all filesystem entries found
// note: the root's parents are NULL so traversal above it is bounded
typedef struct FSNode {
    int is_dir;
    long blocks;
    int n_children;
    int n_dirs;
    char name[MAX_FILENAME];
    struct FSNode **children;
    struct FSNode *parent;
} FSNode;

// runtime mode
typedef enum {
    MENU_MODE,
    EDIT_MODE,
} MODE;

// runtime config
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

// runtime vars
typedef struct {
    void *pad;    // ncurses WINDOW *
    int max_line; // longest line
    int pad_row;  // top left row of pad
    int pad_col;  // top left col of pad
    int screen_h; // terminal screen height
    int screen_w; // terminal screen width
    int pad_w;    // pad width
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

// runtime vars for the menu
typedef struct {
    void *main; // ncurses WINDOW *
    int action;
    int choice;
    int v_choice;
    int max_lenfn;
    int padding;
    int block_size;
    int n_cols;
    int col_width;
    int v_lim;
    int ff_row;
} Mstate;

// for sending structs to/from the menu
// **note** root: initial directory; needed to free subsequent nodes
// cwd: the menu's currently active entry for showing options/file contents
typedef struct {
    const Config *config;
    FSNode *root; // original so tree can still be freed
    FSNode *cd;   // current selection to view (dir or file)

    Mstate *ms; // the current Mstate struct

    char *stt_msg; // status message to show
    int frames;    // 'frames' for msg to show

    int intention; // nav to: 1, read: 2, or edit: 3
    int padd_size; // block_cushion - based on largest file size
} MGMT;

// known/supported languages
typedef enum { c, py, blank, LANG_COUNT } language;

// the langauges name as a string
typedef struct {
    language lang;
    const char *l;
} lang_names;

// supported colors
enum COLORS {
    PINK      = 1,
    GREEN     = 2,
    PURPLE    = 3,
    CYAN      = 4,
    LTGRAY    = 5,
    YELLOW    = 6,
    REDDSH    = 7,
    TEAL      = 8,
    ORANGE    = 9,
    PY_CYAN   = 10,
    PY_PURPLE = 11,
    PY_GREEN  = 12,
    ML_GRAY   = 13,
};

// *carefully ordered* list of regex expressions'
// targeted type to find/highlight
typedef enum {
    NUMERICALS,  // 1, 23
    VARIABLES,   // int i, char *s
    FUNCTIONS,   // main()
    OPERANDS,    // 20 * 5
    PREPROCS,    // #include
    HEADERS,     // regex.h
    KEYWORDS,    // return, goto
    PUNCTUATION, // , { ;
    STRINGS,     // "hello, world"
    SUBSTITUTES, // "hello, %s"
    COMMENTS,    // // comment
} CmpOrder;

// a single color's RGB values (hexadecimal format)
typedef struct {
    char r[4];
    char g[4];
    char b[4];
} ColorCode;

typedef struct {
    const char *expression; // RegEx expression (string)
    regex_t cmp_expression; // compiled expression
    enum COLORS color_code; // a code to RGB values
    CmpOrder type;          // the type/order
    int compiled;           // valid compile boolean
} SyntaxDemands;

// pairs of SyntaxDemands that (respectively)
// indicate the beggining and end of a multi-line
// expression (i.e., /*...*/, or python's <""">)
typedef struct {
    SyntaxDemands ix;
    SyntaxDemands kx;
} SyntaxTwins;

#endif
