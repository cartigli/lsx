#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "highlight.h"
#include "iofs.h"


/* file system tracking & menu window display */

int DONT_SHOW_SIZES = 0;

int MUTABLE = 1;

int main(int argc, char *argv[]) {
    if (argc > 2) { return 1; }
    if (argc > 1) {
        if (strcmp(argv[1], "no_size") == 0 || strcmp(argv[1], "-ns") == 0) {
            DONT_SHOW_SIZES = 1;
        } else if (strcmp(argv[1], "read_only") == 0 || strcmp(argv[1], "-r") == 0) {
            MUTABLE = 0;
        }
    }

    /* make the cwd's entry */
    FSNode* cwd = malloc(sizeof(FSNode));
    if (cwd == NULL) { return 1; }
    if (getcwd(cwd->name, sizeof(FSNode)) == NULL) { goto cleanup; }

    /* reverse (recursive) traversal terminator */
    cwd->parent = NULL;

    /* buffer of full path for descendendants in fls_recurs */
    char *ptbuff = malloc(MAX_FILENAME);
    if (ptbuff == NULL) {
        free_rfs(cwd);
        return 1;
    }
    /* TODO: safe strcat */
    strcpy(ptbuff, cwd->name);
    strcat(ptbuff, "/");

    RTSpecs *rts = malloc(sizeof(RTSpecs));
    if (rts == NULL) { goto cleanup; }

    FVWSpecs *fvw = malloc(sizeof(FVWSpecs));
    if (fvw == NULL) { goto cleanup; }

    /* index the cwd & its subdirectories */
    fls_recursion(cwd, ptbuff);
    /* order the results by directories first */
    order_rfs(cwd);

    initscr(); /* initiate ncurses screen */
    cbreak();  /* grab all key events (except Ctrl+C) */
    noecho();  /* hide the cursor */

    menu(cwd, rts, fvw);
    endwin();  /* kill the window or terminal state will be corrupted */
cleanup:
    free_assist(cwd, rts, fvw, ptbuff);
    return 0;
}


RTSpecs *init_RTS(FSNode *cd, RTSpecs *rts) {
    rts->unkn_action = 0;
    rts->choice = 0;
    int ch;

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

    rts->cd_selected = 0;
    rts->rf_selected = 0;
    rts->pd_selected = 0;
    return rts;
}


void menu(FSNode *cd, RTSpecs *rts, FVWSpecs *fvw) {
    WINDOW *menu_win;
    FSNode** choices = cd->children;

    int ch;
    rts = init_RTS(cd, rts);

    /* create menu window */
    menu_win = newwin(0, 0, 0, 0); /* fullscreen */
    keypad(menu_win, TRUE);        /* get the key strokes *from the window* */
    box(menu_win, 0, 0);           /* outline the window */
    wrefresh(menu_win);            /* refresh the window to show */

    // /* draw choices */
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
        if (rts->choice < cd->n_dirs) { rts->v_choice = rts->choice;
        } else { rts->v_choice = rts->fi_init_row + (rts->choice - cd->n_dirs); }

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
            case 'x': /* quit and exit */
                rts->choice = -1;
                goto end;
            case 'c': /* 'cd' to the selected directory (if directory) */
                rts->cd_selected = 1; /* not usefully functional atm */
                goto end;
            case 'r': /* read to the selected file */
                rts->rf_selected = 1;
                goto end;
            case 'e': /* edit the selected file */
                if (MUTABLE) {
                    rts->mf_selected = 1;
                    goto end;
                } else { continue; }
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

    if (rts->cd_selected) {
        rts->cd_selected = 0;
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

    int rt_code = ef_runn(path);
    free(path);
    return rt_code;
}


int read_from(FSNode* ff, FVWSpecs *fvw) {
    if (ff->is_dir) {return 0; }
    char *path = malloc(MAX_FILENAME);
    path[0] = '\0';

    untraverse(ff, path);

    int rt_code = view_file(path, fvw);
    free(path);
    return rt_code;
}


FVWSpecs *initFVWS(char *path, FVWSpecs *fvw) {
    FILE *f = fopen(path, "r");
    if (!f) { return NULL; }

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

    // rewind(f);
    fclose(f);

    getmaxyx(stdscr, fvw->screen_h, fvw->screen_w);

    /* make pad atleast size of window incase file doesn't fill */
    /* condition ? expression if true : expression if false */
    fvw->pad_w = (fvw->max_line > fvw->screen_w) ? fvw->max_line + 1 : fvw->screen_w;

    /* scrolling state : which row/column is in the top-left of the viewport */
    fvw->pad_row = 0;
    fvw->pad_col = 0;
    fvw->view_h = fvw->screen_h - 1; /* room for status bar at the bottom */
    fvw->view_w = fvw->screen_w;

    return fvw;
}


int view_file(char *path, FVWSpecs *fvw) {
    fvw = initFVWS(path, fvw);
    if (!fvw) { return 1; }

    WINDOW *pad = newpad(fvw->n_lines + 1, fvw->pad_w);
    keypad(pad, TRUE);

    FILE *f = fopen(path, "r");
    if (!f) { return 1; }

    /* read the file into the pad, line by line */
    int row = 0;
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        mvwprintw(pad, row, 0, "%s", line);
        row++;
    }

    fclose(f);

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