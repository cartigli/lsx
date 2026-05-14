#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "iofs.h"

// #define MAX_FILENAME 1024

// typedef struct FSNode {
//     char name[MAX_FILENAME];
//     int     is_dir;
//     long    blocks;
//     int n_children;
//     int     n_dirs;
//     struct FSNode** children;
//     struct FSNode*    parent;
// } FSNode;

// typedef struct {
//     int unkn_action;
//     int      choice;
//     int    v_choice;
//     int   max_lenfn;
//     int     padding;
//     int   col_width;
//     int      n_cols;
//     int fi_init_row;
//     int       v_lim;
//     int  rf_selected;
//     int  d_selected;
//     int pd_selected;
// } RTSpecs;

// typedef struct {
//     int  pad_row;
//     int  pad_col;
//     int    pad_w;
//     int screen_h;
//     int screen_w;
//     int   view_h;
//     int   view_w;
//     int  n_lines;
//     int max_line;
// } FVWSpecs;

// struct dirent *ent;
// struct stat     st;

// void menu(FSNode* cd, RTSpecs *rts, FVWSpecs *fvw);

// void fls_recursion(FSNode* cd, char *tbuff);

// int df_type(char *dir);
// long fl_blocks(char *dir);

// void untraverse(FSNode* cd, char* buff);
// int traverse_to(FSNode* cd);

// int read_from(FSNode* ff, FVWSpecs *fvw);
// int view_file(char *path, FVWSpecs *fvw);

// void order_rfs(FSNode* cd);
// void order_fs(FSNode*  cd);
// void free_rfs(FSNode*  cd);
// void free_fs(FSNode*   cd);


int DONT_SHOW_SIZES = 0;

int main(int argc, char *argv[]) {
    if (argc > 2) { return 1; }
    if (argc > 1) {
        if (strcmp(argv[1], "no_size") == 0 || strcmp(argv[1], "-ns") == 0) {
            DONT_SHOW_SIZES = 1;
        }
    }

    /* make the cwd's entry */
    FSNode* cwd = malloc(sizeof(FSNode));
    if (cwd == NULL) { return 1; }
    if (getcwd(cwd->name, sizeof(FSNode)) == NULL) {
        free(cwd);
        return 1;
    }

    cwd->parent = NULL; /* reverse traversal terminator */

    /* buffer of full path for descendendants in fls_recurs */
    char *ptbuff = malloc(MAX_FILENAME);
    if (ptbuff == NULL) {
        free_rfs(cwd);
        return 1;
    }
    strcpy(ptbuff, cwd->name);
    strcat(ptbuff, "/");

    RTSpecs *rts = malloc(sizeof(RTSpecs));
    if (rts == NULL) {
        free_rfs(cwd);
        free(ptbuff);
        return 1;
    }

    FVWSpecs *fvw = malloc(sizeof(FVWSpecs));
    if (fvw == NULL) {
        free_rfs(cwd);
        free(ptbuff);
        free(rts);
        return 1;
    }

    fls_recursion(cwd, ptbuff); /* index the cwd & its subdirectories */
    order_rfs(cwd);             /* order the results by directories first */

    initscr(); /* initiate ncurses screen */
    cbreak();  /* grab all key events (except Ctrl+C) */
    noecho();  /* hide the cursor */

    menu(cwd, rts, fvw);
    endwin();  /* kill the window or terminal state will be corrupted */

    free_rfs(cwd);
    free(ptbuff);
    free(rts);
    free(fvw);
    return 0;
}


// void fls_recursion(FSNode* cd, char *tbuff) {
//     cd->n_children = 0;

//     int ix = 0;
//     DIR *cd_d = opendir(tbuff);
//     if (cd_d == NULL) { 
//         printf("couldn't open dir\n");
//         return;
//     }
//     while ((ent = readdir(cd_d)) != NULL) {
//         if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }
//         cd->n_children++;
//     }

//     /* count 'parent' str to index & truncate later */
//     int xterm_pl = strlen(tbuff);

//     cd->children = malloc(cd->n_children * sizeof(FSNode*));

//     rewinddir(cd_d);
//     if (cd_d == NULL) { return; }
//     while ((ent = readdir(cd_d)) != NULL) {
//         if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }

//         FSNode* entry = malloc(sizeof(FSNode));
//         if (entry == NULL) { return; }

//         entry->parent = cd; /* point it at the parent */

//         int dtype;
//         if (ent->d_type == DT_DIR) {
//             dtype = 1;
//             if (!(cd->n_dirs)) { cd->n_dirs = 1; }
//             else { cd->n_dirs++; }
//         }
//         else if (ent->d_type == DT_REG) {
//             dtype = 0;
//         } else { dtype = -1; }

//         /* make the full path with / + the new item's name */
//         strcat(tbuff, "/");
//         strcat(tbuff, ent->d_name);

//         strcpy(entry->name, ent->d_name);
//         entry->is_dir = dtype;

//         if (dtype == 0) {
//             long blocks = fl_blocks(tbuff);
//             entry->blocks = blocks;
//         } else if (dtype == 1) {
//             fls_recursion(entry, tbuff);
//         }

//         tbuff[xterm_pl] = '\0'; /* reset buffer to parent's path */
//         cd->children[ix] = entry;
//         ix++;
//     }
// }


// int df_type(char *dir) {
//     if (lstat(dir, &st) ==-1) { return -2; }
//     if (S_ISLNK(st.st_mode)) { return -1; }
//     if (S_ISREG(st.st_mode)) { return 0; }
//     if (S_ISDIR(st.st_mode)) { return 1; }
//     return -3; /* god forbid */
// }

// long fl_blocks(char *dir) {
//     if (stat(dir, &st) == -1) { return -2; }
//     return (long)st.st_blocks;
// }



// void order_rfs(FSNode* cd) {
//     if (cd == NULL) { return; }
//     int n = cd->n_children;
//     for (int i = 0; i < n; i++) { order_fs(cd->children[i]); }
//     order_fs(cd);
// }

// void order_fs(FSNode* cd) {
//     if (cd == NULL) { return; }
//     if (cd->n_children) {
//         int ndirs = 0;
//         int n = cd->n_children;
//         FSNode** tmp = malloc(sizeof(FSNode*) * n);
//         for (int i = 0; i < n; i++) {
//             if (cd->children[i]->is_dir) {
//                 tmp[ndirs] = cd->children[i];
//                 ndirs++;
//             }
//         }
//         for (int l = 0; l < n; l++) {
//             if (!(cd->children[l]->is_dir)) {
//                 tmp[ndirs] = cd->children[l];
//                 ndirs++;
//             }
//         }
//         cd->children = tmp;
//     }
// }

// void free_rfs(FSNode* cd) {
//     if (cd == NULL) { return; }
//     int n = cd->n_children;
//     for (int i = 0; i < n; i++) {
//         free_rfs(cd->children[i]);
//     }
//     free_fs(cd);
// }

// void free_fs(FSNode* cd) {
//     if (cd == NULL) { return; }
//     if (cd->n_children) { free(cd->children); }
//     free(cd);
// }

// void untraverse(FSNode* cd, char* buff) {
//     if (cd->parent != NULL) {
//         untraverse(cd->parent, buff);
//         strcat(buff, "/");
//     }
//     strcat(buff, cd->name);
// }

// int traverse_to(FSNode* cd) {
//     if (!cd->is_dir) { return 0; }
//     char *path = malloc(MAX_FILENAME);
//     path[0] = '\0';

//     untraverse(cd, path);
//     printf("path: %s\n", path);

//     printf("full path: %s\n", path);

//     if (chdir(path) == -1) { return 0; }
//     free(path);
//     return 1;
// }



void menu(FSNode *cd, RTSpecs *rts, FVWSpecs *fvw) {
    WINDOW *menu_win;
    FSNode** choices = cd->children;
    rts->unkn_action = 0;
    rts->choice = 0;
    int ch;

    // int yMax, xMax;
    // getmaxyx(stdscr, yMax, xMax);
    int xMax = getmaxx(stdscr);

    int xstrlen;
    rts->max_lenfn = 0;
    for (int x = 0; x < cd->n_children; x++) {
        xstrlen = strlen(cd->children[x]->name);
        if (rts->max_lenfn < xstrlen) { rts->max_lenfn = xstrlen; }
    }

    rts->padding = (DONT_SHOW_SIZES) ? 2 : 17;
    rts->col_width = rts->max_lenfn + rts->padding;
    rts->n_cols = xMax / rts->col_width;

    if (rts->n_cols < 1)              { rts->n_cols = 1; }
    if (rts->n_cols > cd->n_children) { rts->n_cols = cd->n_children; }

    /* calculate virtual grid of n_choices given n_dirs */
    int dir_rows = (cd->n_dirs) ? ((cd->n_dirs - 1) / rts->n_cols) + 1 : 0;

    rts->fi_init_row = dir_rows * rts->n_cols;            /* first row containing files */
    rts->v_lim = rts->fi_init_row + (cd->n_children - cd->n_dirs); /* build from init posit, not first dir posit */

    /* create menu window */
    menu_win = newwin(0, 0, 0, 0); /* fullscreen */
    keypad(menu_win, TRUE);        /* get the key strokes *from the window* */
    box(menu_win, 0, 0);           /* outline the window */
    wrefresh(menu_win);            /* refresh the window to show */

    /* draw choices */
    rts->d_selected  = 0; /* dir to traverse to */
    rts->rf_selected  = 0; /* file to read from */
    rts->pd_selected = 0; /* parent to de-traverse to */

    while (1) {
        int fcx = 0;
        for (int i = 0; i < cd->n_children; i++) {
            if (i == rts->choice) { wattron(menu_win, A_REVERSE); }

            if (!choices[i]->is_dir) {
                int row = (fcx / rts->n_cols) + 4; /* indent slightly deeper */
                int col = (fcx % rts->n_cols) * rts->col_width + 2; /* same right shift */
                if (DONT_SHOW_SIZES) {
                    mvwprintw(menu_win, row, col, "%s", choices[i]->name);
                } else {
                    mvwprintw(menu_win, row, col, "[ :%ld blocks ] %s", 
                                choices[i]->blocks, choices[i]->name);
                }
                fcx++;

            } else { /* every <n_cols> items overflow into the next row */
                int row = (i / rts->n_cols) + 1; /* all get indented + 1 (outline) */
                int col = (i % rts->n_cols) * rts->col_width + 2; /* right +2 (outline) */
                mvwprintw(menu_win, row, col, "%s", choices[i]->name);
            }

            if (i == rts->choice) { wattroff(menu_win, A_REVERSE); }
        }
        wrefresh(menu_win);

        ch = wgetch(menu_win);

        /* map the true choice to the virtual grid */
        if (rts->choice < cd->n_dirs) {
            rts->v_choice = rts->choice;
        } else {
            rts->v_choice = rts->fi_init_row + (rts->choice - cd->n_dirs);
        }

        /* mathmatical traversal like normal */
        switch(ch) {
            case KEY_RIGHT: rts->v_choice++;     break;
            case KEY_LEFT:  rts->v_choice--;     break;
            case KEY_DOWN:  rts->v_choice += rts->n_cols; break;
            case KEY_UP:    rts->v_choice -= rts->n_cols; break;
            case 10: /* Enter/Return to read if its a file or */
            case 13: /* traverse to if a directory */
                rts->unkn_action = 1;
                goto end;
            case 'q': /* quit and exit */
                rts->choice = -1;
                goto end;
            case 'c': /* 'cd' to the selected directory (if directory) */
                rts->d_selected = 1; /* not usefully functional atm */
                goto end;
            case 'r': /* read to the selected file */
                rts->rf_selected = 1;
                goto end;
            case 'e': /* edit the selected file */
                rts->mf_selected = 1;
                goto end;
            case 'p': /* traverse up 1 (to parent) */
                rts->pd_selected = 1;
                goto end;
        }

        /* if landed in empty dir slot of virtual grid: *
         * if virtual choice is greater than the no. of *
         * directories and less then the first file row */
        if (rts->v_choice >= cd->n_dirs && rts->v_choice < rts->fi_init_row) {
            switch(ch) {
                case KEY_DOWN:  rts->v_choice += rts->n_cols;     break;
                case KEY_UP:    rts->v_choice -= rts->n_cols;     break;
                case KEY_RIGHT: rts->v_choice = rts->fi_init_row; break;
                case KEY_LEFT:  rts->v_choice = cd->n_dirs - 1;  break;
            }
        }

        /* clamp to true boundaries */
        if (rts->v_choice < 0) {
            rts->v_choice = 0;
        }
        if (rts->v_choice >= rts->v_lim) {
            rts->v_choice = rts->v_lim -1;
        }

        /* remap the virtual grid to true array */
        if (rts->v_choice < cd->n_dirs) {
            rts->choice = rts->v_choice;
        } else { 
            rts->choice = cd->n_dirs + (rts->v_choice - rts->fi_init_row);
        }
    }
end:
    if (rts->choice == -1) { return; }
    werase(menu_win);
    /* pressing enter without specifying c or r (rts or read) *
     * so the target type is unknown, at least to us */
    if (rts->unkn_action) {
        if (choices[rts->choice]->is_dir) {
            menu(choices[rts->choice], rts, fvw);
        }
        else {
            if (read_from(choices[rts->choice], fvw)) {
                return;
            } else {
                menu(cd, rts, fvw);
            }
        }
        rts->unkn_action = 0;
    }

    if (rts->d_selected) {
        rts->d_selected = 0;
        if (traverse_to(choices[rts->choice]))    { return; } else { menu(cd, rts, fvw); }
    } else if (rts->rf_selected) {
        rts->rf_selected = 0;
        if (read_from(choices[rts->choice], fvw)) { return; } else { menu(cd, rts, fvw); }
    } else if (rts->mf_selected) {
        rts->mf_selected = 0;
        if (edit_de(choices[rts->choice]))        { return; } else { menu(cd, rts, fvw); }
    } else if (rts->pd_selected) {
        rts->pd_selected = 0;
        if (cd->parent) { menu(cd->parent, rts, fvw); }       else { menu(cd, rts, fvw); }
    }
}

int edit_de(FSNode* ff) {
    if (ff->is_dir) {return 0; }
    char *path = malloc(MAX_FILENAME);
    path[0] = '\0';

    untraverse(ff, path);

    free(path);
    return ef_runn(path);
}

int read_from(FSNode* ff, FVWSpecs *fvw) {
    if (ff->is_dir) {return 0; }
    char *path = malloc(MAX_FILENAME);
    path[0] = '\0';

    untraverse(ff, path);

    free(path);
    return view_file(path, fvw);
}


int view_file(char *path, FVWSpecs *fvw) {
    FILE *f = fopen(path, "r");
    if (!f) { return 1; }

    fvw->n_lines = 0;
    fvw->max_line = 0;
    int cur_len = 0; /* counter */
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') {
            fvw->n_lines++;
            if (cur_len > fvw->max_line) { fvw->max_line = cur_len; }
            cur_len = 0;
        } else {
            cur_len++;
        }
    }

    if (cur_len > 0) { fvw->n_lines++; } /* add line w.no trailing \n */

    rewind(f);

    getmaxyx(stdscr, fvw->screen_h, fvw->screen_w);

    /* make pad atleast size of window incase file doesn't fill */
    /* condition ? expression if true : expression if false */
    fvw->pad_w = (fvw->max_line > fvw->screen_w) ? fvw->max_line + 1 : fvw->screen_w;
    WINDOW *pad = newpad(fvw->n_lines + 1, fvw->pad_w);
    keypad(pad, TRUE);

    /* read the file into the pad, line by line */
    int row = 0;
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        mvwprintw(pad, row, 0, "%s", line);
        row++;
    }

    fclose(f);

    /* scrolling state : which row/column is in the top-left of the viewport */
    fvw->pad_row = 0;
    fvw->pad_col = 0;
    fvw->view_h = fvw->screen_h - 1; /* room for status bar at the bottom */
    fvw->view_w = fvw->screen_w;

    clear();
    refresh();

    int ch;
    while (1) {
        mvprintw(fvw->view_h, 0, "line %d/%d q to quit",
            fvw->pad_row + 1, fvw->n_lines);
        clrtoeol();
        refresh();

        prefresh(pad, fvw->pad_row, fvw->pad_col, 0, 0,
            fvw->view_h - 1, fvw->view_w -1);

        ch = wgetch(pad);
        switch(ch) {
            case KEY_DOWN:  fvw->pad_row++; break;
            case KEY_UP:    fvw->pad_row--; break;
            case KEY_RIGHT: fvw->pad_col += 8; break;
            case KEY_LEFT:  fvw->pad_col -= 8; break;
            case 'q':
                goto done;
        }

        /* clamp */
        if (fvw->pad_row > fvw->n_lines - fvw->view_h) {
            fvw->pad_row = fvw->n_lines - fvw->view_h;
        }
        if (fvw->pad_row < 0) {
            fvw->pad_row = 0;
        }
        if (fvw->pad_col > fvw->pad_w - fvw->view_w) {
            fvw->pad_col = fvw->pad_w - fvw->view_w;
        }
        if (fvw->pad_col < 0) {
            fvw->pad_col = 0;
        }
    }
done:
    delwin(pad);
    return 0;
}
