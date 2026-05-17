#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ncurses.h>
#include <unistd.h>

#include "buff.h"
#include "highlight.h"
#include "init.h"
#include "menu.h"
#include "utils.h"

#define ST_BLOCK_SIZE 512


/* file system tracking & menu window display */

int DONT_SHOW_SIZES = 0;

// long maxx;
// int MAX_BLOCK_SIZE;
int BLOCK_CUSHION;

int main(int argc, char *argv[]) {
    if (argc > 2) { return 1; }
    if (argc > 1) {
        if (strcmp(argv[1], "no_size") == 0 || strcmp(argv[1], "-ns") == 0) {
            DONT_SHOW_SIZES = 1;
        // } else if (strcmp(argv[1], "read_only") == 0 || strcmp(argv[1], "-r") == 0) {
            // MUTABLE = 0;
        }
    }

    FSNode *cwd = NULL;
    Mstates *ms = NULL;
    // FVWSpecs *fvw = NULL;
    char *ptbuff = NULL;

    /* make the cwd's entry */
    cwd = malloc(sizeof(FSNode));
    if (cwd == NULL) { return 1; }
    if (getcwd(cwd->name, sizeof(FSNode)) == NULL) { goto cleanup; }

    /* reverse (recursive) traversal terminator */
    cwd->parent = NULL;

    /* buffer of full path for descendendants in fls_recurs */
    ptbuff = malloc(MAX_FILENAME);
    if (ptbuff == NULL) {
        free_rfs(cwd);
        return 1;
    }
    strcpy(ptbuff, cwd->name);
    if (sf_strcat(ptbuff, "/", sizeof(ptbuff))) { return 1; }

    ms = malloc(sizeof(Mstates));
    if (ms == NULL) { goto cleanup; }

    // fvw = malloc(sizeof(FVWSpecs));
    // if (fvw == NULL) { goto cleanup; }

    /* index the cwd & its subdirectories */
    fls_recursion(cwd, ptbuff);
    /* order the results by directories first */
    // order_rfs(cwd);
    long longest_block = max_rblocks(cwd);
    if (longest_block == 0) {
        printf("no valid blocks returned \n");
        goto cleanup;
    }
    int cushion = 0;
    while (longest_block != 0) {
        longest_block /= 10;
        cushion++;
    }
    BLOCK_CUSHION = cushion;

    if (init_scr(0)) { goto cleanup; }


    FSNode *nxt = NULL;
    while (1) {
        nxt = menu(cwd, ms);
        if (!nxt) { break; }
        cwd = nxt;
    }
    endwin();  /* kill the window or terminal state will be corrupted */

    cleanup:
    free_assist(cwd, ms, ptbuff);
    return 0;
}


FSNode *menu(FSNode *cd, Mstates *ms) {
    WINDOW *menu_win;
    FSNode** choices = cd->children;

    int ch;
    ms = init_MS(cd, ms, DONT_SHOW_SIZES);

    /* create menu window */
    menu_win = newwin(0, 0, 0, 0); /* fullscreen */
    keypad(menu_win, TRUE);        /* get the key strokes *from the window* */
    box(menu_win, 0, 0);           /* outline the window */
    wrefresh(menu_win);            /* refresh the window to show */

    curs_set(0);

    /* draw choices */
    while (1) {
        int fcx = 0;
        for (int i = 0; i < cd->n_children; i++) {
            /* apply the highlight attribute to the cursor's item */
            if (i == ms->choice) { wattron(menu_win, A_REVERSE); }

            if (!choices[i]->is_dir) {
                int row = (fcx / ms->n_cols) + 4; /* indent slightly deeper */
                int col = (fcx % ms->n_cols) * ms->col_width + 2; /* same right shift */
                if (DONT_SHOW_SIZES) {
                    mvwprintw(menu_win, row, col, "%s", choices[i]->name);
                } else {
                    mvwprintw(menu_win, row, col, "[:%*ld bytes] %s", BLOCK_CUSHION,
                                choices[i]->blocks * ST_BLOCK_SIZE, choices[i]->name);
                }
                fcx++;

            } else { /* every <n_cols> items overflow into the next row */
                int row = (i / ms->n_cols) + 1; /* all get indented + 1 (outline) */
                int col = (i % ms->n_cols) * ms->col_width + 2; /* right +2 (outline) */
                mvwprintw(menu_win, row, col, "%s", choices[i]->name);
            }

            if (i == ms->choice) { wattroff(menu_win, A_REVERSE); }
        }
        wrefresh(menu_win);

        ch = wgetch(menu_win);

        /* map the true choice to the virtual grid */
        if (ms->choice < cd->n_dirs) { ms->v_choice = ms->choice;
        } else { ms->v_choice = ms->fi_init_row + (ms->choice - cd->n_dirs); }

        /* mathmatical traversal like normal */
        switch(ch) {
            case KEY_RIGHT: ms->v_choice++;     break;
            case KEY_LEFT:  ms->v_choice--;     break;
            case KEY_DOWN:  ms->v_choice += ms->n_cols; break;
            case KEY_UP:    ms->v_choice -= ms->n_cols; break;
            case 10: /* Enter/Return to read if its a file or */
            case 13: /* traverse to if a directory */
                ms->Mutable = 0; /* files only editable if opened with 'e' */
                ms->unkn_action = 1;
                goto end;
            case 'x': /* quit and exit */
                ms->choice = -1;
                goto end;
            case 'c': /* 'cd' to the selected directory (if directory) */
                ms->cd_selected = 1; /* not usefully functional atm */
                goto end;
            case 'r': /* read to the selected file */
                ms->Mutable = 0;
                ms->rf_selected = 1;
                goto end;
            case 'e': /* edit the selected file */
                ms->Mutable = 1;
                ms->mf_selected = 1;
                goto end;
            case 'p': /* traverse up 1 (to parent) */
                ms->pd_selected = 1;
                goto end;
        }

        /* if landed in empty dir slot of virtual grid: *
         * if virtual choice is greater than the no. of *
         * directories and less then the first file row */
        if (ms->v_choice >= cd->n_dirs && ms->v_choice < ms->fi_init_row) {
            switch(ch) {
                case KEY_DOWN:  ms->v_choice += ms->n_cols;     break;
                case KEY_UP:    ms->v_choice -= ms->n_cols;     break;
                case KEY_RIGHT: ms->v_choice = ms->fi_init_row; break;
                case KEY_LEFT:  ms->v_choice = cd->n_dirs - 1;   break;
            }
        }

        /* clamp to true boundaries */
        if (ms->v_choice < 0) {
            ms->v_choice = 0;
        }
        if (ms->v_choice >= ms->v_lim) {
            ms->v_choice = ms->v_lim -1;
        }

        /* remap the virtual grid to true array */
        if (ms->v_choice < cd->n_dirs) {
            ms->choice = ms->v_choice;
        } else { 
            ms->choice = cd->n_dirs + 
            (ms->v_choice - ms->fi_init_row);
        }
    }
end:
    if (ms->choice == -1) { return NULL; }
    werase(menu_win);
    /* pressing enter without specifying c or r (ms or read) *
     * so the target type is unknown, at least to us */
    if (ms->unkn_action) {
        if (choices[ms->choice]->is_dir) {
            // menu(choices[ms->choice], ms, fvw);
            return choices[ms->choice];
        }
        else {
            // if (read_from(choices[ms->choice], fvw)) {
            if (pretty_edit(choices[ms->choice], ms->Mutable)) {
                return NULL;
            } else {
                // menu(cd, ms, fvw);
                return cd;
            }
        }
        ms->unkn_action = 0;
    }

    if (ms->cd_selected) {
        ms->cd_selected = 0;
        if (traverse_to(choices[ms->choice])) {
            return NULL;
        } else {
            // menu(cd, ms, fvw);
            return cd;
        }
    } else if (ms->rf_selected) {
        ms->rf_selected = 0;
        // if (read_from(choices[ms->choice], fvw)) {
        if (pretty_edit(choices[ms->choice], ms->Mutable)) {
            return NULL;
        } else {
            // menu(cd, ms, fvw);
            return cd;
        }
    } else if (ms->mf_selected) {
        ms->mf_selected = 0;
        if (pretty_edit(choices[ms->choice], ms->Mutable)) {
            return NULL;
        } else {
            // menu(cd, ms, fvw);
            return cd;
        }
    } else if (ms->pd_selected) {
        ms->pd_selected = 0;
        if (cd->parent) {
            // menu(cd->parent, ms, fvw);
            return cd->parent;
        } else {
            // menu(cd, ms, fvw);
            return cd;
        }
    }
    return NULL;
}


int pretty_edit(FSNode* ff, int Mutable) {
    if (ff->is_dir) {return 0; }
    char *path = malloc(MAX_FILENAME);
    path[0] = '\0';

    untraverse(ff, path);

    int rt_code = pretty_runner(path, Mutable);
    free(path);
    return rt_code;
}


// int read_from(FSNode* ff, FVWSpecs *fvw) {
//     if (ff->is_dir) {return 0; }
//     char *path = malloc(MAX_FILENAME);
//     path[0] = '\0';

//     untraverse(ff, path);

//     int rt_code = view_file(path, fvw);
//     free(path);
//     return rt_code;
// }


// int view_file(char *path, FVWSpecs *fvw) {
//     fvw->n_lines = 0;
//     fvw->max_line = 0;
//     int cur_len = 0; /* counter */
//     int c;

//     FILE *f = fopen(path, "r");
//     if (!f) { return 1; }

//     while ((c = fgetc(f)) != EOF) {
//         if (c == '\n') {
//             fvw->n_lines++;
//             if (cur_len > fvw->max_line) { fvw->max_line = cur_len; }
//             cur_len = 0;
//         } else {
//             cur_len++;
//         }
//     }

//     /* add line w.no trailing \n */
//     if (cur_len > 0) { fvw->n_lines++; }

//     rewind(f);

//     fvw = initFVWS(fvw);
//     if (!fvw) { return 1; }

//     WINDOW *pad = newpad(fvw->n_lines + 1, fvw->pad_w);
//     keypad(pad, TRUE);

//     /* read the file into the pad, line by line */
//     int row = 0;
//     char line[4096];
//     while (fgets(line, sizeof(line), f)) {
//         mvwprintw(pad, row, 0, "%s", line);
//         row++;
//     }

//     fclose(f);

//     clear();
//     refresh();

//     int ch;
//     while (1) {
//         mvprintw(fvw->view_h, 0, "line %d/%d x to quit",
//             fvw->pad_row + 1, fvw->n_lines);
//         clrtoeol();
//         refresh();

//         prefresh(pad, fvw->pad_row, fvw->pad_col, 0, 0,
//             fvw->view_h - 1, fvw->view_w -1);

//         ch = wgetch(pad);
//         switch(ch) {
//             case KEY_DOWN:  fvw->pad_row++; break;
//             case KEY_UP:    fvw->pad_row--; break;
//             case KEY_RIGHT: fvw->pad_col += 8; break;
//             case KEY_LEFT:  fvw->pad_col -= 8; break;
//             case 'x':
//                 goto done;
//         }

//         /* clamp */
//         if (fvw->pad_row > fvw->n_lines - fvw->view_h) {
//             fvw->pad_row = fvw->n_lines - fvw->view_h;
//         }
//         if (fvw->pad_row < 0) {
//             fvw->pad_row = 0;
//         }
//         if (fvw->pad_col > fvw->pad_w - fvw->view_w) {
//             fvw->pad_col = fvw->pad_w - fvw->view_w;
//         }
//         if (fvw->pad_col < 0) {
//             fvw->pad_col = 0;
//         }

//         // int init = fvw->pad_row;
//         // int end = fvw->pad_row + rt->view_h;
//         // if (end > fvw->n_lines) { end = fvw->n_lines; }
//         // for (unsigned int e = 0; e < N_DEMANDS; e++) {
//         //     if (DEMANDS[e].compiled) { continue; }
//         //     for (int d = init; d < end; d++) {
//         //         regex_color(pad, d, line, exp, color);
//         //     }
//         // }
//     }
// done:
//     delwin(pad);
//     return 0;
// }


// void regex_color_a(WINDOW *pad, int row, const char *line,
//     const regex_t *regxx, int code) {
//     int pos = 0;
//     regmatch_t pmatch[3];
//     while (regexec(regxx, line + pos, 3, pmatch, 0) == 0) {
//         regoff_t so = pmatch[2].rm_so >= 0 ? pmatch[2].rm_so : pmatch[0].rm_so;
//         regoff_t eo = pmatch[2].rm_so >= 0 ? pmatch[2].rm_eo : pmatch[0].rm_eo;

//         mvwchgat(pad, row, pos + so, eo - so, A_NORMAL, code, NULL);
//         pos += pmatch[0].rm_eo;

//         if (pmatch[0].rm_so == pmatch[0].rm_eo) { break; }
//     }
// }


void free_assist(FSNode *cd, Mstates *ms, char *ptbuff) {
    if (cd) { free_rfs(cd); }
    if (ms) { free(ms); }
    if (ptbuff) { free(ptbuff); }
}