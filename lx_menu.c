#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fstypes.h"

int SHOW_SIZES = 1;

int main(int argc, char *argv[]) {
    if (argc > 2) { return 1; }
    if (argc == 2) {
        if (strcmp(argv[1], "no_size") == 0) { SHOW_SIZES = 0; }
        else if (strcmp(argv[1], "-ns") == 0) { SHOW_SIZES = 0; }
    }

    /* make the cwd's entry */
    cwd = malloc(sizeof(FileSystemNode));
    if (cwd == NULL) { return 1; } /* if malloc fails */
    if (getcwd(cwd->name, sizeof(FileSystemNode)) == NULL) { return 1; } /* if getcws fails */

    /* reverse traversal terminator */
    cwd->parent = NULL;

    /* buffer of full path for descendendants in fls_recurs */
    char *ptbuff = malloc(MAX_FILENAME);
    strcpy(ptbuff, cwd->name);
    strcat(ptbuff, "/");

    fls_recursion(cwd, ptbuff); /* index the cwd & its subdirectories */
    order_rfs(cwd);             /* order the results by directories first */

    initscr(); /* initiate ncurses screen */
    cbreak(); /* grab all key events (except Ctrl+C) */
    noecho(); /* hide the cursor */

    menu(cwd);

    endwin(); /* kill the window or terminal state will be corrupted */

    free_rfs(cwd);
    return 0;
}

void fls_recursion(FileSystemNode* cd, char *tbuff) {
    int children = 0;
    int ix = 0;
    DIR *cd_d = opendir(tbuff);
    if (cd_d == NULL) { 
        printf("couldn't open dir\n");
        return;
    }
    while ((ent = readdir(cd_d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }
        children++;
    }

    /* count the current length of the 'parent' str */
    int xterm_pl = strlen(tbuff);

    cd->n_children = children;
    cd->children = malloc(children * sizeof(FileSystemNode*));

    rewinddir(cd_d);
    if (cd_d == NULL) { return; }
    while ((ent = readdir(cd_d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }

        FileSystemNode* entry = malloc(sizeof(FileSystemNode));
        if (entry == NULL) { return; }

        entry->parent = cd; /* point it at the parent */

        int dtype;
        if (ent->d_type == DT_DIR) {
            dtype = 1;
            if (!(cd->n_dirs)) { cd->n_dirs = 1; }
            else { cd->n_dirs++; }
        }
        else if (ent->d_type == DT_REG) {
            dtype = 0;
        } else { dtype = -1; }

        /* make the full path with / + the new item's name */
        strcat(tbuff, "/");
        strcat(tbuff, ent->d_name);

        strcpy(entry->name, ent->d_name);
        entry->is_dir = dtype;

        if (dtype == 0) {
            long blocks = fl_blocks(tbuff);
            entry->blocks = blocks;
        } else if (dtype == 1) {
            fls_recursion(entry, tbuff);
        }

        tbuff[xterm_pl] = '\0'; /* reset buffer to parent's path */
        cd->children[ix] = entry;
        ix++;
    }
}

int de_type(char di[]) {
    char *s = malloc(MAX_FILENAME);
    strcpy(s, cwd->name);
    strcat(s, "/");
    strcat(s, di);
    if (lstat(s, &st) ==-1) { return -2; }
    if (S_ISLNK(st.st_mode)) { return -1; }
    if (S_ISREG(st.st_mode)) { return 0; }
    if (S_ISDIR(st.st_mode)) { return 1; }
    return -3; /* god forbid */
}

int df_type(char di[]) {
    if (lstat(di, &st) ==-1) { return -2; }
    if (S_ISLNK(st.st_mode)) { return -1; }
    if (S_ISREG(st.st_mode)) { return 0; }
    if (S_ISDIR(st.st_mode)) { return 1; }
    return -3; /* god forbid */
}

long fl_blocks(char *fd) {
    if (stat(fd, &st) == -1) { return -2; }
    long sz = st.st_blocks;
    return sz;
}

char *ft_decode(FileSystemNode* dx) {
    int is_dir = dx->is_dir;
    if (is_dir) { return "dir "; }
    else if (is_dir == 0) { return "file"; }
    return "NaN";
}

int max_strlen(FileSystemNode *cdi, int n_choices) {
    int xstrlen;
    int max_lenfn = 0;
    for (int x = 0; x < n_choices; x++) {
        xstrlen = strlen(cdi->children[x]->name);
        if (max_lenfn < xstrlen) { max_lenfn = xstrlen; }
    }

    return max_lenfn;
}

void menu(FileSystemNode *cdi) {
    WINDOW *menu_win;
    int n_choices = cdi->n_children;
    FileSystemNode** choices = cdi->children;
    int choice = 0;
    int ch;

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    int max_lenfn = max_strlen(cdi, n_choices);

    int padding = 2;
    if (SHOW_SIZES) { padding = padding + 15; }
    int col_width = max_lenfn + padding;
    int n_cols = xMax / col_width;

    if (n_cols < 1) { n_cols = 1; }
    if (n_cols > n_choices) { n_cols = n_choices; }

    int rows = n_choices / n_cols;
    if (n_choices % n_cols != 0) { rows++; }

    /* calculate virtual grid of n_choices given n_dirs */
    int dir_rows;
    int n_dirs = cdi->n_dirs;
    if (n_dirs) { dir_rows = ((n_dirs - 1) / n_cols) + 1; } else { dir_rows = 0; }
    int fi_init_row = dir_rows * n_cols;            /* first row containing files */
    int v_lim = fi_init_row + (n_choices - n_dirs); /* build from init posit, not first dir posit */

    /* create menu window */
    menu_win = newwin(0, 0, 0, 0); /* fullscreen */
    keypad(menu_win, TRUE);        /* get the key strokes *from the window* */
    box(menu_win, 0, 0);           /* outline the window */
    wrefresh(menu_win);            /* refresh the window to show */

    /* draw choices */
    int running = 1;
    int d_selected = 0; /* dir to traverse to */
    int f_selected = 0; /* file to read from */
    int dd_selected = 0; /* parent to de-traverse to */
    while (running) {
        int fcx = 0;
        for (int i = 0; i < n_choices; i++) {
            if (i == choice) { wattron(menu_win, A_REVERSE); }

            /* every <n_cols> items overflow into the next row */
            int row = (i / n_cols) + 1; /* all get indented + 1 (outline) */
            int col = (i % n_cols) * col_width + 2; /* right +2 (outline) */
            if (!(choices[i]->is_dir) && SHOW_SIZES) { /* if its not a dir, show the size */
                int row = (fcx / n_cols) + 4; /* indent slightly deeper */
                int col = (fcx % n_cols) * col_width + 2; /* same right shift */
                mvwprintw(menu_win, row, col, "[ :%lld blocks ] %s", choices[i]->blocks, choices[i]->name);
                fcx++;
            } else { mvwprintw(menu_win, row, col, "%s", choices[i]->name); }

            if (i == choice) { wattroff(menu_win, A_REVERSE); }
        }
        wrefresh(menu_win);

        ch = wgetch(menu_win);
        int v_choice; /* virtual choice */

        /* map the true choice to the virtual grid */
        if (choice < n_dirs) {
            v_choice = choice;
        } else {
            v_choice = fi_init_row + (choice - n_dirs);
        }

        /* mathmatical traversal like normal */
        switch(ch) {
            case KEY_RIGHT: v_choice++;     break;
            case KEY_LEFT:  v_choice--;     break;
            case KEY_DOWN:  v_choice += n_cols; break;
            case KEY_UP:    v_choice -= n_cols; break;
            case 10:
            case 13:
                goto end;
            case 27:
            case 'q':
                choice = -1;
                running = 0;
                break;
            case 'c': /* 'cd' to the selected directory (if directory) */
                d_selected = 1; /* not usefully functional atm */
                running = 0;
                break;
            case 'r': /* read to the selected file */
                f_selected = 1;
                running = 0;
                break;
            case 'p': /* traverse up 1 */
                dd_selected = 1;
                running = 0;
                break;
        }

        /* if landed in empty dir slot of virtual grid: *
         * if virtual choice is greater than the no. of *
         * directories and less then the first file row */
        if (v_choice >= n_dirs && v_choice < fi_init_row) {
            switch(ch) {
                case KEY_DOWN:  v_choice += n_cols;     break;
                case KEY_UP:    v_choice -= n_cols;     break;
                case KEY_RIGHT: v_choice = fi_init_row; break;
                case KEY_LEFT:  v_choice = n_dirs - 1;  break;
            }
        }

        /* clamp to true boundaries */
        if (v_choice < 0) { v_choice = 0; }
        if (v_choice >= v_lim) { v_choice = v_lim -1; }

        /* remap the virtual grid to true array */
        if (v_choice < n_dirs) {
            choice = v_choice;
        } else {
            choice = n_dirs + (v_choice - fi_init_row);
        }
    }
end:
    werase(menu_win);
    if (choice != -1) {
        if (d_selected) {
            if (!traverse_to(choices[choice])) {
                printf("traversal failed\n");
                return;
            } else { menu(cwd); } /* or could be menu(choices[choice]->parent), no upwards traversal fx rn though */
        } else if (f_selected) {
            if (!read_from(choices[choice])) {
                printf("failed to read file\n");
                return;
            } else { menu(cwd); }
        } else if (dd_selected) { if (cdi->parent != NULL) { menu(cdi->parent); } else { menu(cdi); } }
        printf("selection: %s\n", choices[choice]->name);
        if (choices[choice]->is_dir) { menu(choices[choice]); }
    } else { return; }
}

void untraverse(FileSystemNode* dd, char* buff) {
    if (dd->parent != NULL) {
        untraverse(dd->parent, buff);
        strcat(buff, "/");
    }
    strcat(buff, dd->name);
}

int traverse_to(FileSystemNode* cd) {
    if (!cd->is_dir) { return 0; }
    char *path = malloc(MAX_FILENAME);
    path[0] = '\0';

    untraverse(cd, path);
    printf("path: %s\n", path);

    printf("full path: %s\n", path);

    if (chdir(path) == -1) { return 0; }
    free(path);
    return 1;
}

void order_rfs(FileSystemNode* ld) {
    if (ld == NULL) { return; }
    int n = ld->n_children;
    for (int i = 0; i < n; i++) { order_fs(ld->children[i]); }
    order_fs(ld);
}

void order_fs(FileSystemNode* dd) {
    if (dd == NULL) { return; }
    if (dd->n_children) {
        int ndirs = 0;
        int n = dd->n_children;
        FileSystemNode** tmp = malloc(sizeof(FileSystemNode*) * n);
        for (int i = 0; i < n; i++) {
            if (dd->children[i]->is_dir) {
                tmp[ndirs] = dd->children[i];
                ndirs++;
            }
        }
        for (int l = 0; l < n; l++) {
            if (!(dd->children[l]->is_dir)) {
                tmp[ndirs] = dd->children[l];
                ndirs++;
            }
        }
        dd->children = tmp;
    }
}

void free_rfs(FileSystemNode* xd) {
    if (xd == NULL) { return; }
    int n = xd->n_children;
    for (int i = 0; i < n; i++) {
        free_rfs(xd->children[i]);
    }
    free_fs(xd);
}

void free_fs(FileSystemNode* fd) {
    if (fd == NULL) { return; }
    if (fd->n_children) { free(fd->children); }
    free(fd);
}