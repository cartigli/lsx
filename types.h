#ifndef TYPES_H
#define TYPES_H

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
    char name[MAX_FILENAME];
    int     is_dir;
    long    blocks;
    int n_children;
    int     n_dirs;
    struct FSNode** children;
    struct FSNode*    parent;
} FSNode;


/* runtime mode */
typedef enum {
    MENU_MODE,
    EDIT_MODE,
    FULLFAULT,
} MODE;

/* runtime config */
typedef struct {
    char *root;
    MODE mode;

    int hide_size;
    int mutable;
    int verbosity;

    const char *indent_chars;
    const char *dedent_chars;
} Config;


/* runtime vars */
typedef struct {
    // WINDOW  *pad;
    void    *pad;
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
    int indent_l; /* level of current indent */
    const char *indent; /* chars to trigger an indent */
    const char *dedent; /* chars to trigger a dedent */
    int       wo; /* write out (bool)       */ /* this & action should be 86'd */
    char   *smsg; /* status message         */
    int   sprint; /* status message bool    */
    int   action; /* action key result key  */
} Cursor;


/* runtime vars for menu */
typedef struct {
    // WINDOW    *main;
    void      *main;
    int      action;
    int      choice;
    int    v_choice;
    int   max_lenfn;
    int     padding;
    int      n_cols;
    int   col_width;
    int       v_lim;
    int fi_init_row;
} Mstate;

/* cross file communication & status state */
typedef struct {
    FSNode   *root; /* original so tree can still be freed */
    FSNode     *cd; /* current selection to view (dir or file) */
    Mstate     *ms;

    char  *stt_msg; /* status message to show */
    int     frames; /* 'frames' for msg to show */

    int  intention; /* nav to: 1, read: 2, or edit: 3 */
    int  hide_size; /* dont_show_sizes */
    int  padd_size; /* block_cushion */
    int   mutable; /* Mutable bool */
} MGMT;


#endif