#ifndef MENU_H
#define MENU_H

#include "fsio.h"
#include <ncurses.h>
// struct FSNode;

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

/* menu of indexed filesystem entries */
FSNode *menu(FSNode* cd, RTSpecs *rts, FVWSpecs *fvw);

/* allows calling view file from FSNode instance */
int read_from(FSNode* ff, FVWSpecs *fvw);

/* function to read the contents of a file (static) */
int view_file(char *path, FVWSpecs *fvw);

/* free allocated memory */
void free_assist(FSNode* cd, RTSpecs *rts, FVWSpecs *fvw, char *ptbuff);


#endif