#ifndef MENU_H
#define MENU_H

#include "config.h"

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

/* for sending structs to/from the menu from main *
 * **note** root: initial directory, *
 * needed to free subsequent nodes *
 * cwd: the menu's currently active *
 * entry for showing options/file contents */
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

// menu of indexed filesystem entries
void menu(MGMT *mgmt);

// allows calling view file from FSNode instance
int read_from(FSNode *ff);

// function to read the contents of a file
int view_file(char *path);

#endif
