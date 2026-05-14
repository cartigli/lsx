#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_FILENAME 1024

typedef struct FileSystemNode {
    char name[MAX_FILENAME];
    int is_dir;
    long blocks;
    int n_children;
    int n_dirs;
    struct FileSystemNode** children;
    struct FileSystemNode* parent;
} FileSystemNode;

typedef struct {
    int unknown_action;
    int choice;
    int v_choice;
    int max_lenfn;
    int padding;
    int col_width;
    int n_cols;
    int fi_init_row;
    int v_lim;
    int f_selected;
    int d_selected;
    int pd_selected;
} RTSelections;

struct dirent *ent;
struct stat st;

/* LX Menu Prototypes */
void fls_recursion(FileSystemNode* di, char *tbuff);
void menu(FileSystemNode* cd, RTSelections *rsm);

// int de_type(char di[]);
int df_type(char di[]);
char *ft_decode(FileSystemNode* dx);
long fl_blocks(char *fd);

void untraverse(FileSystemNode* dd, char* buff);
int traverse_to(FileSystemNode* cd);

// int max_strlen(FileSystemNode *cdi, int n_choices);

void order_rfs(FileSystemNode* ld);
void order_fs(FileSystemNode* dd);
void free_rfs(FileSystemNode* xd);
void free_fs(FileSystemNode* fd);

/* File View Prototypes */
int read_from(FileSystemNode* ff);
int view_file(char *path);

int DONT_SHOW_SIZES = 0;

int main(int argc, char *argv[]) {
    if (argc > 2) { return 1; }
    if (argc == 2) {
        if (strcmp(argv[1], "no_size") == 0 || strcmp(argv[1], "-ns") == 0) {
            DONT_SHOW_SIZES = 1;
        }
    }

    /* make the cwd's entry */
    FileSystemNode* cwd = malloc(sizeof(FileSystemNode));
    if (cwd == NULL) { return 1; } /* if malloc fails */
    if (getcwd(cwd->name, sizeof(FileSystemNode)) == NULL) { return 1; } /* if getcws fails */

    cwd->parent = NULL; /* reverse traversal terminator */

    /* buffer of full path for descendendants in fls_recurs */
    char *ptbuff = malloc(MAX_FILENAME);
    if (ptbuff == NULL) {
        free_rfs(cwd);
        return 1;
    }
    strcpy(ptbuff, cwd->name);
    strcat(ptbuff, "/");

    RTSelections *rsm = malloc(sizeof(RTSelections));
    if (rsm == NULL) {
        free_rfs(cwd);
        free(ptbuff);
        return 1;
    }

    fls_recursion(cwd, ptbuff); /* index the cwd & its subdirectories */
    order_rfs(cwd);             /* order the results by directories first */

    initscr(); /* initiate ncurses screen */
    cbreak();  /* grab all key events (except Ctrl+C) */
    noecho();  /* hide the cursor */

    menu(cwd, rsm);
    endwin();  /* kill the window or terminal state will be corrupted */

    free_rfs(cwd);
    free(ptbuff);
    free(rsm);
    return 0;
}

void fls_recursion(FileSystemNode* cd, char *tbuff) {
    // int children = 0;
    int ix = 0;
    DIR *cd_d = opendir(tbuff);
    if (cd_d == NULL) { 
        printf("couldn't open dir\n");
        return;
    }
    while ((ent = readdir(cd_d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }
        cd->n_children++;
    }

    /* count the current length of the 'parent' str */
    int xterm_pl = strlen(tbuff);

    // cd->n_children = children;
    cd->children = malloc(cd->n_children * sizeof(FileSystemNode*));

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

// int max_strlen(FileSystemNode *cdi, int n_choices) {
//     int xstrlen;
//     int max_lenfn = 0;
//     for (int x = 0; x < n_choices; x++) {
//         xstrlen = strlen(cdi->children[x]->name);
//         if (max_lenfn < xstrlen) { max_lenfn = xstrlen; }
//     }
//     return max_lenfn;
// }

void menu(FileSystemNode *cd, RTSelections *rsm) {
    WINDOW *menu_win;
    // int n_choices = cdi->n_children;
    FileSystemNode** choices = cd->children;
    rsm->unknown_action = 0;
    rsm->choice = 0;
    int ch;

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    // int max_lenfn = max_strlen(cdi, n_choices);
    int xstrlen;
    rsm->max_lenfn = 0;
    for (int x = 0; x < cd->n_children; x++) {
        xstrlen = strlen(cd->children[x]->name);
        if (rsm->max_lenfn < xstrlen) { rsm->max_lenfn = xstrlen; }
    }

    rsm->padding = (DONT_SHOW_SIZES) ? 2 : 17;
    rsm->col_width = rsm->max_lenfn + rsm->padding;
    rsm->n_cols = xMax / rsm->col_width;

    if (rsm->n_cols < 1)         { rsm->n_cols = 1; }
    // if (n_cols > n_choices) { n_cols = n_choices; }
    if (rsm->n_cols > cd->n_children) { rsm->n_cols = cd->n_children; }

    // int rows = rsm->n_children / rsm->n_cols;
    // if (rsm->n_children % rsm->n_cols != 0) { rows++; }

    /* calculate virtual grid of n_choices given n_dirs */
    // int dir_rows;
    // int n_dirs = cdi->n_dirs;
    int dir_rows = (cd->n_dirs) ? ((cd->n_dirs - 1) / rsm->n_cols) + 1 : 0;

    rsm->fi_init_row = dir_rows * rsm->n_cols;            /* first row containing files */
    rsm->v_lim = rsm->fi_init_row + (cd->n_children - cd->n_dirs); /* build from init posit, not first dir posit */

    /* create menu window */
    menu_win = newwin(0, 0, 0, 0); /* fullscreen */
    keypad(menu_win, TRUE);        /* get the key strokes *from the window* */
    box(menu_win, 0, 0);           /* outline the window */
    wrefresh(menu_win);            /* refresh the window to show */

    /* draw choices */
    rsm->d_selected = 0; /* dir to traverse to */
    rsm->f_selected = 0; /* file to read from */
    rsm->pd_selected = 0; /* parent to de-traverse to */
    while (1) {
        int fcx = 0;
        for (int i = 0; i < cd->n_children; i++) {
            if (i == rsm->choice) { wattron(menu_win, A_REVERSE); }
            if (!choices[i]->is_dir) {
                int row = (fcx / rsm->n_cols) + 4; /* indent slightly deeper */
                int col = (fcx % rsm->n_cols) * rsm->col_width + 2; /* same right shift */
                if (DONT_SHOW_SIZES) {
                    mvwprintw(menu_win, row, col, "%s", choices[i]->name);
                } else {
                    mvwprintw(menu_win, row, col, "[ :%lld blocks ] %s", 
                                choices[i]->blocks, choices[i]->name);
                }
                fcx++;
            } else {
                /* every <n_cols> items overflow into the next row */
                int row = (i / rsm->n_cols) + 1; /* all get indented + 1 (outline) */
                int col = (i % rsm->n_cols) * rsm->col_width + 2; /* right +2 (outline) */
                mvwprintw(menu_win, row, col, "%s", choices[i]->name);
            }

            if (i == rsm->choice) { wattroff(menu_win, A_REVERSE); }
        }
        wrefresh(menu_win);

        ch = wgetch(menu_win);
        // int v_choice; /* virtual choice */

        /* map the true choice to the virtual grid */
        if (rsm->choice < cd->n_dirs) {
            rsm->v_choice = rsm->choice;
        } else {
            rsm->v_choice = rsm->fi_init_row + (rsm->choice - cd->n_dirs);
        }

        /* mathmatical traversal like normal */
        switch(ch) {
            case KEY_RIGHT: rsm->v_choice++;     break;
            case KEY_LEFT:  rsm->v_choice--;     break;
            case KEY_DOWN:  rsm->v_choice += rsm->n_cols; break;
            case KEY_UP:    rsm->v_choice -= rsm->n_cols; break;
            case 10: /* Enter/Return to read if its a file or */
            case 13: /* traverse to if a directory */
                rsm->unknown_action = 1;
                goto end;
            case 'q': /* quit and exit */
                rsm->choice = -1;
                goto end;
            case 'c': /* 'rsm' to the selected directory (if directory) */
                rsm->d_selected = 1; /* not usefully functional atm */
                goto end;
            case 'r': /* read to the selected file */
                rsm->f_selected = 1;
                goto end;
            case 'p': /* traverse up 1 (to parent) */
                rsm->pd_selected = 1;
                goto end;
        }

        /* if landed in empty dir slot of virtual grid: *
         * if virtual choice is greater than the no. of *
         * directories and less then the first file row */
        if (rsm->v_choice >= cd->n_dirs && rsm->v_choice < rsm->fi_init_row) {
            switch(ch) {
                case KEY_DOWN:  rsm->v_choice += rsm->n_cols;     break;
                case KEY_UP:    rsm->v_choice -= rsm->n_cols;     break;
                case KEY_RIGHT: rsm->v_choice = rsm->fi_init_row; break;
                case KEY_LEFT:  rsm->v_choice = cd->n_dirs - 1;  break;
            }
        }

        /* clamp to true boundaries */
        if (rsm->v_choice < 0)      { rsm->v_choice = 0; }
        if (rsm->v_choice >= rsm->v_lim) { rsm->v_choice = rsm->v_lim -1; }

        /* remap the virtual grid to true array */
        if (rsm->v_choice < cd->n_dirs) {
            rsm->choice = rsm->v_choice;
        } else { rsm->choice = cd->n_dirs + (rsm->v_choice - rsm->fi_init_row); }
    }
end:
    if (rsm->choice == -1) { return; }
    werase(menu_win);
    /* pressing enter without specifying c or r (rsm or read) *
     * so the target type is unknown, at least to us */
    if (rsm->unknown_action) {
        if (choices[rsm->choice]->is_dir) {
            menu(choices[rsm->choice], rsm);
        }
        else {
            if (read_from(choices[rsm->choice])) {
                return;
            } else {
                menu(cd, rsm);
            }
        }
        rsm->unknown_action = 0;
    }

    if (rsm->d_selected) {
        rsm->d_selected = 0;
        if (!traverse_to(choices[rsm->choice])) { return; }
    } else if (rsm->f_selected) {
        rsm->f_selected = 0;
        if (read_from(choices[rsm->choice])) { return; } else { menu(cd, rsm); }
    } else if (rsm->pd_selected) {
        rsm->pd_selected = 0;
        if (cd->parent != NULL) { menu(cd->parent, rsm); }
        else { menu(cd, rsm); }
    }
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

int read_from(FileSystemNode* ff) {
    if (ff->is_dir) {return 0; }
    char *path = malloc(MAX_FILENAME);
    path[0] = '\0';

    untraverse(ff, path);

    free(path);
    return view_file(path);
}

int view_file(char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { return 1; }

    int n_lines = 0;  /* count the lines */
    int max_line = 0; /* find the longest line */
    int cur_len = 0;  /* counter */
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') {
            n_lines++;
            if (cur_len > max_line) { max_line = cur_len; }
            cur_len = 0;
        } else {
            cur_len++;
        }
    }
    if (cur_len > 0) { n_lines++; } /* add line w.no trailing \n */
    rewind(f);

    int screen_h, screen_w;
    getmaxyx(stdscr, screen_h, screen_w);

    /* make pad atleast size of window incase file doesn't fill */
    /* condition ? expression if true : expression if false */
    int pad_w = (max_line > screen_w) ? max_line + 1 : screen_w;
    WINDOW *pad = newpad(n_lines + 1, pad_w);
    keypad(pad, TRUE);

    /* read the file into the pad, line by line */
    char line[4096];
    int row = 0;
    while (fgets(line, sizeof(line), f)) {
        mvwprintw(pad, row, 0, "%s", line);
        row++;
    }
    fclose(f);

    /* scrolling state : which row/column is in the top-left of the viewport */
    int pad_row = 0;
    int pad_col = 0;
    int view_h = screen_h - 1; /* room for status bar at the bottom */
    int view_w = screen_w;

    clear();
    refresh();

    int ch;
    while (1) {
        mvprintw(view_h, 0, "line %d/%d q to quit", pad_row + 1, n_lines);
        clrtoeol();
        refresh();

        prefresh(pad, pad_row, pad_col, 0, 0, view_h - 1, view_w -1);

        ch = wgetch(pad);
        switch(ch) {
            case KEY_DOWN:  pad_row++; break;
            case KEY_UP:    pad_row--; break;
            case KEY_RIGHT: pad_col += 8; break;
            case KEY_LEFT:  pad_col -= 8; break;
            case 'q':
            case 27:
                goto done;
        }

        /* clamp */
        if (pad_row > n_lines - view_h) { pad_row = n_lines - view_h; }
        if (pad_row < 0)                { pad_row = 0; }
        if (pad_col > pad_w - view_w)   { pad_col = pad_w - view_w; }
        if (pad_col < 0)                { pad_col = 0; }
    }
done:
    delwin(pad);
    return 0;
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