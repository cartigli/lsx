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
    struct FileSystemNode** children;
    struct FileSystemNode* parent;
} FileSystemNode;

FileSystemNode* cwd;

struct dirent *ent;
struct stat st;

int de_type(char di[]);
int df_type(char di[]);
char *ft_decode(FileSystemNode* dx);
void menu(FileSystemNode* cdi);
void fls_recursion(FileSystemNode* di, char *tbuff);
long fl_blocks(char *fd);

int main(void) {
    /* make the cwd's entry */
    cwd = malloc(sizeof(FileSystemNode));
    if (cwd == NULL) { return 1; }
    if (getcwd(cwd->name, sizeof(FileSystemNode)) == NULL) { return 1; }

    cwd->parent = NULL; /* its the full path, nothing to connect */

    char *ptbuff = malloc(MAX_FILENAME);
    strcpy(ptbuff, cwd->name);
    strcat(ptbuff, "/");

    fls_recursion(cwd, ptbuff);
    menu(cwd);

    // free(cwd); don't half ass it
    return 0;
}

void fls_recursion(FileSystemNode* cd, char *tbuff) {
    int children = 0;
    int ix = 0;
    // DIR *cd_d = opendir(cd->name);
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

    DIR *cd_di = opendir(tbuff);
    while ((ent = readdir(cd_di)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }

        
        FileSystemNode* entry = malloc(sizeof(FileSystemNode));
        if (entry == NULL) { return; }
        
        int dtype;
        if (ent->d_type == DT_DIR) { dtype = 1; }
        else if (ent->d_type == DT_REG) {
            dtype = 0;
        }
        else {dtype = -1; }
        
        /* make the full path with / + the new item's name */
        strcat(tbuff, "/");
        strcat(tbuff, ent->d_name);

        strcpy(entry->name, ent->d_name);
        entry->is_dir = dtype;

        if (dtype == 0) {
            long blocks = fl_blocks(tbuff);
            entry->blocks = blocks;
        }
        if (dtype == 1) { fls_recursion(entry, tbuff); }
        tbuff[xterm_pl] = '\0'; /* files & dirs get full paths, so always truncate */

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

void menu(FileSystemNode *cdi) {
    WINDOW *menu_win;
    int n_choices = cdi->n_children;
    FileSystemNode** choices = cdi->children;
    printf("n_children: %i\n", n_choices);
    int choice = 0;
    int fin = 0;
    int ch;

    initscr();
    cbreak();
    noecho();

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    int xstrlen;
    int max_lenfn = 0;
    for (int x = 0; x < n_choices; x++) {
        xstrlen = strlen(cdi->children[x]->name);
        if (max_lenfn < xstrlen) { max_lenfn = xstrlen; }
    }

    int padding = 1;
    int col_width = max_lenfn + padding;
    int n_cols = xMax / col_width;

    if (n_cols < 1) { n_cols = 1; }
    if (n_cols > n_choices) { n_cols = n_choices; }

    int rows = n_choices / n_cols;
    if (rows % n_cols != 0) { rows++; }

    printf("column width: %i | # of columns: %i | rows: %i\n", col_width, n_cols, rows);

    // Create menu window
    menu_win = newwin(0, 0, 0, 0); /* fullscreen */
    keypad(menu_win, TRUE);        /* get the key strokes *from the window* */
    box(menu_win, 0, 0);           /* outline the window */
    wrefresh(menu_win);            /* refresh the window to show */

    /* Draw choices */
    while (!(fin)) {
        for (int i = 0; i < n_choices; i++) {
            if (i == choice) { wattron(menu_win, A_REVERSE); }
            /* every <n_cols> items overflow into the next row */
            int row = (i / n_cols) + 1; /* all get indented + 1 (outline) */
            int col = (i % n_cols) * col_width + 2; /* right +2 (outline) */
            mvwprintw(menu_win, row, col, "%s", choices[i]->name);
            if (i == choice) { wattroff(menu_win, A_REVERSE); }
        }
        wrefresh(menu_win);

        ch = wgetch(menu_win);
        switch (ch) {
            case KEY_RIGHT:
                if (choice < n_choices - 1) { choice++; }
                break;
            case KEY_LEFT:
                if (choice > 0) { choice--; }
                break;
            case KEY_DOWN: /* if the choice + n_columns won't go beyond the bounds */
                /* then move the cursor down one row by adding an entire row to it */
                if (choice + n_cols < n_choices) { choice += n_cols; }
                break;
            case KEY_UP:
                if (choice - n_cols > 0) { choice -= n_cols; }
                break;
            case 10: /* Return (Line Feed <\n>) */
            case 13: /* Return (Std. Carriage) (case 10 inherits) */
                fin++;
            case 27: // Escape
                fin++;
        }
    }
end:
    endwin();
    if (choice) {
        printf("selection: %s\n", choices[choice]->name);
        if (choices[choice]->is_dir) { menu(choices[choice]); }
    } else { printf("no selection\n"); }
}
