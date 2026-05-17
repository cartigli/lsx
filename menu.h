#ifndef MENU_H
#define MENU_H

#include <ncurses.h>
#include "fsio.h"

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
    int     Mutable;
} Mstates;

// /* additional runtime vars for the file view window */
// typedef struct {
//     int max_line;
//     int  pad_row;
//     int  pad_col;
//     int screen_h;
//     int screen_w;
//     int   view_h;
//     int   view_w;
//     int    pad_w;
//     int  n_lines;
// } FVWSpecs;

/* menu of indexed filesystem entries */
FSNode *menu(FSNode* cd, Mstates *ms);

/* allows calling ef_runn from FSNode instance */
int pretty_edit(FSNode* ff, int Mutable);

/* allows calling view file from FSNode instance */
int read_from(FSNode* ff); //, FVWSpecs *fvw);

/* function to read the contents of a file (static) */
int view_file(char *path); //, FVWSpecs *fvw);

/* free allocated memory */
void free_assist(FSNode* cd, Mstates *ms, char *ptbuff);


#endif