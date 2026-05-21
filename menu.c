#include <dirent.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ncurses.h>
#include <unistd.h>

#include "fsio.h"
#include "editor.h"
#include "error.h"
#include "menu.h"
#include "utils.h"


void menu(MGMT *mgmt) {
    FSNode *cd = (mgmt->cd) ? mgmt->cd : mgmt->root;
    Mstate *ms = mgmt->ms;
    
    FSNode** choices = cd->children;
    int ch;

    touchwin(ms->main); /* 'wake up' window */
    werase(ms->main); /* create menu window */

    /* print the current directory to the menu's status bar */
    int y = getmaxy(ms->main);
    int row = y - STATUS_RROWS;

    char *stt_buff = calloc(1, MAX_FILENAME);
    if (!stt_buff) {
        print_err(menu_src, "failed to allocate memory for the status message buffer", 4);
        werase(ms->main); return;
    }

    untraverse(cd, stt_buff);

    /* wipe the whole line once and trust status_handler *
     * to only wipe the portion of the status bar it needs */
    wmove(ms->main, row, 0); /* row = last, col = first */
    wclrtoeol(ms->main);
    mvwprintw(ms->main, row, 0, "%s", stt_buff);

    wrefresh(ms->main); /* refresh the window to show updates */
    curs_set(0);

    const char *default_stt = "x to exit";

    /* draw choices */
    while (1) {
        int fcx = 0;
        for (int i = 0; i < cd->n_children; i++) {
            /* apply the highlight attribute to the cursor's item */
            if (i == ms->choice) { wattron(ms->main, A_REVERSE); }

            if (!choices[i]->is_dir) {
                /* indent slightly deeper */
                int row = (fcx / ms->n_cols) + 4;
                /* same right shift */
                int col = (fcx % ms->n_cols) * ms->col_width + 2;
                if (mgmt->hide_size) {
                    mvwprintw(ms->main, row, col, "%s", choices[i]->name);
                } else {
                    mvwprintw(ms->main, row, col,
                                "[:%*ld bytes] %s", (int)mgmt->padd_size,
                                choices[i]->blocks * ST_BLOCK_SIZE,
                                choices[i]->name);
                }
                fcx++;

            } else {
                /* every <n_cols> items overflow into the next row */
                int row = (i / ms->n_cols) + 1; /* all get indented + 1 (outline) */
                int col = (i % ms->n_cols) * ms->col_width + 2; /* right +2 (outline) */
                mvwprintw(ms->main, row, col, "%s", choices[i]->name);
            }

            if (i == ms->choice) { wattroff(ms->main, A_REVERSE); }
        };

        /* print the default "exit: x" or print the status message */
        const char *stt = mgmt->frames ? mgmt->stt_msg : default_stt;
        stt_handler(ms->main, stt);
        if (mgmt->frames) { mgmt->frames--; }
        
        wrefresh(ms->main);

        ch = wgetch(ms->main);

        /* map the true choice to the virtual grid */
        if (ms->choice < cd->n_dirs) {
            ms->v_choice = ms->choice;
        }
        else {
            ms->v_choice = ms->fi_init_row +
                    (ms->choice - cd->n_dirs);
        }

        /* mathmatical traversal like normal */
        switch(ch) {
            case KEY_RIGHT: ms->v_choice++;     break;
            case KEY_LEFT:  ms->v_choice--;     break;
            case KEY_DOWN:  ms->v_choice += ms->n_cols; break;
            case KEY_UP:    ms->v_choice -= ms->n_cols; break;
            case 10: /* Enter/Return to read if its a file or */
            case 13: /* traverse to if a directory */
                /* or open a file in read view (e to edit) */
                mgmt->intention = choices[ms->choice]->is_dir ? 1 : 2;
                goto breakout;
            case 'x': /* quit and exit */
                mgmt->intention = 0;
                goto breakout;
            case 'c': /* 'cd' to the selected dir (if dir) */
                if (choices[ms->choice]->is_dir) {
                    mgmt->intention = 1;
                    goto breakout;
                }
                else {
                    mgmt->frames = 2;
                    mgmt->stt_msg = "not a dir";
                    break;
                }
            case 'r': /* read to the selected file */
                if (!choices[ms->choice]->is_dir) {
                    mgmt->intention = 2;
                    goto breakout;
                } else {
                    mgmt->frames = 2;
                    mgmt->stt_msg = "not a file";
                    break;
                }
            case 'e': /* edit the selected file */
                /* if not in an mutable state, do nothing (& show warning) */
                if (!(mgmt->mutable)) {
                    mgmt->stt_msg = "immutable";
                    mgmt->frames = 2;
                    break;
                } else {
                    mgmt->intention = 3;
                    goto breakout;
                }
            case 'p': /* traverse up 1 (to parent) */
                if (cd == mgmt->root) { /* guard if in root */
                    mgmt->stt_msg = "already at root";
                    mgmt->frames = 2;
                }
                /* this is the only one that's different */
                mgmt->cd = mgmt->cd->parent;
                mgmt->intention = 4;
                werase(ms->main); free(stt_buff); return;
        }

        /* if landed in empty dir slot of virtual grid: *
         * if virtual choice is greater than the no. of *
         * directories and less then the first file row */
        if (ms->v_choice >= cd->n_dirs && ms->v_choice < ms->fi_init_row) {
            switch(ch) {
                case KEY_DOWN:  ms->v_choice += ms->n_cols;      break;
                case KEY_UP:    ms->v_choice -= ms->n_cols;      break;
                case KEY_RIGHT: ms->v_choice  = ms->fi_init_row; break;
                case KEY_LEFT:  ms->v_choice  = cd->n_dirs - 1;  break;
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

breakout:
    mgmt->cd = choices[ms->choice];

    werase(ms->main);
    free(stt_buff);
    return;

}
