#ifndef IOFS_H
#define IOFS_H

/* file system tracking & menu window display */

/* max permissable chars in a filename */
#define MAX_FILENAME 1024

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

/* runtime vars for ncurses window/menu */
typedef struct {
    WINDOW     *pad;
    int unkn_action;
    int      choice;
    int    v_choice;
    int   max_lenfn;
    int     padding;
    int      n_cols;
    int   col_width;
    int       v_lim;
    int fi_init_row;
    int rf_selected;
    int mf_selected;
    int cd_selected;
    int pd_selected;
} RTSpecs;

/* additional runtime vars for the file view window */
typedef struct {
    int  pad_row;
    int  pad_col;
    int    pad_w;
    int screen_h;
    int screen_w;
    int   view_h;
    int   view_w;
    int  n_lines;
    int max_line;
} FVWSpecs;

RTSpecs *init_RTS(FSNode *cd, RTSpecs *rts);

FVWSpecs *initFVWS(char *path, FVWSpecs *fvw);

void menu(FSNode* cd, RTSpecs *rts, FVWSpecs *fvw);

/* recursive filesystem index-r, records findings in FSNode's */
void fls_recursion(FSNode* cd, char *tbuff);

/* returns type as symlink, file, or dir */
int df_type(char *dir);
long fl_blocks(char *dir);

/* build the full path from a given FSNode instance */
void untraverse(FSNode* cd, char* buff);
int traverse_to(FSNode* cd);

/* allows calling view file from FSNode instance */
int read_from(FSNode* ff, FVWSpecs *fvw);
/* function to read the contents of a file (static) */
int view_file(char *path, FVWSpecs *fvw);

/* allows calling ef_runn from FSNode instance */
int edit_de(FSNode* ff);
/* main editor managing function for editing files */
int ef_runn(char *path);

/* orders dirs before files (recursively) */
void order_rfs(FSNode* cd);
void order_fs(FSNode*  cd);

/* free allocated memory */
void free_assist(FSNode* cd, RTSpecs *rts, FVWSpecs *fvw, char *ptbuff);
void free_rfs(FSNode*  cd);
void free_fs(FSNode*   cd);

/* intializes screen dimensions, attributes, & elements */
int init_scr(void);

/* computes color codes to RGB from hexadecimal */
int hex_compr(const char c[]);

#endif