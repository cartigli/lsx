#include <dirent.h>
#include <ncurses.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "editor.h"
#include "error.h"
#include "fsio.h"
#include "menu.h"
#include "utils.h"

void menu(MGMT *mgmt)
{
    // runs and menu and manages interactions thereof

    FSNode *cd = mgmt->cd;
    Mstate *ms = mgmt->ms;

    // the choices are the current directory's FSNode children
    FSNode **choices = cd->children;

    int ch;
    touchwin(ms->main); // 'wake up' window
    werase(ms->main);   // create menu window

    // print the current directory's path to the status bar
    int y   = getmaxy(ms->main);
    int row = y - STATUS_RROWS - 1; // - 1 for border/outline

    // initiate & intialize the status path buffer
    char stt_buff[MAX_FILENAME];
    stt_buff[0] = '\0';
    // untraverse the current FSNode to get its path
    untraverse(cd, stt_buff);

    // wipe the whole line; inside the loop, only clear what
    // *could* contain some previous status, but not this path
    wmove(ms->main, row, 0); // row = last, col = first
    wclrtoeol(ms->main);
    mvwprintw(ms->main, row, 0, "%s", stt_buff);

    wrefresh(ms->main);
    box(ms->main, 0, 0);
    curs_set(0); // hide cursor

    const char *default_stt = "x to exit";

    // draw choices & cursor's selection
    while (1) {
        int fcx = 0;
        for (int i = 0; i < cd->n_children; i++) {
            // apply the highlight attribute to the cursor's item
            if (i == ms->choice) wattron(ms->main, A_REVERSE);

            if (!choices[i]->is_dir) {
                // if not a dir, print on a new 'grid'
                int row = (fcx / ms->n_cols) + 4; // + 4 for down shift
                // adjust the column to fit the window
                int col = (fcx % ms->n_cols) * ms->col_width + 2;
                if (mgmt->config->hide_size) { // if sizes are hidden
                    mvwprintw(ms->main, row, col, "%s", choices[i]->name);
                } else { // else, there are padding & spacing vars to specify
                    mvwprintw(ms->main, row, col, "[:%*ld bytes] %s",
                        (int)ms->block_size, choices[i]->blocks * ST_BLOCK_SIZE,
                        choices[i]->name);
                }
                fcx++;

            } else {
                // if not a file, dirs don't get sizes
                // and are printed above files
                int row = (i / ms->n_cols) + 2; // + 1 down shift
                int col =
                    (i % ms->n_cols) * ms->col_width + 2; // + 2 right shift
                mvwprintw(ms->main, row, col, "%s", choices[i]->name);
            }

            if (i == ms->choice) wattroff(ms->main, A_REVERSE);
        };

        // print the default "exit: x" or print the status message
        const char *stt = mgmt->frames ? mgmt->stt_msg : default_stt;

        int x, y;
        getmaxyx(ms->main, y, x);
        int row     = y - STATUS_RROWS - 1; // - 1 for border/outline
        int clr_col = x - MAX_STTM_LEN;
        int msg_len = (int)strlen(stt);
        int print_col =
            x > msg_len ? x - msg_len - 2 : 0; // - 2 for border cushion

        wmove(ms->main, row, clr_col);
        // manual clear to preserve border elements
        for (int i = 0; i < MAX_STTM_LEN - 1; i++) {
            mvwprintw(ms->main, row, clr_col, " ");
            clr_col++;
        }
        mvwprintw(ms->main, row, print_col, "%s", stt);
        if (mgmt->frames) mgmt->frames--;

        wrefresh(ms->main);

        ch = wgetch(ms->main);

        // map the actual choice to the virtual grid
        if (ms->choice < cd->n_dirs) {
            ms->v_choice = ms->choice;
        } else {
            ms->v_choice = ms->ff_row + (ms->choice - cd->n_dirs);
        }

        // traversal / key digest
        switch (ch) {
            case KEY_RIGHT:
                ms->v_choice++;
                break;
            case KEY_LEFT:
                ms->v_choice--;
                break;
            case KEY_DOWN:
                ms->v_choice += ms->n_cols;
                break;
            case KEY_UP:
                ms->v_choice -= ms->n_cols;
                break;
            case 10: // enter / return: select
            case 13: // if dir, open, if file, read/edit
                mgmt->intention = choices[ms->choice]->is_dir ? 1 : 2;
                goto breakout;
            case 'x': // quit
                mgmt->intention = 0;
                goto breakout;
            case 'c': // 'cd' to selection
                if (choices[ms->choice]->is_dir) {
                    mgmt->intention = 1;
                    goto breakout;
                } else {
                    mgmt->frames  = 2;
                    mgmt->stt_msg = "not a dir";
                    break;
                }
            case 'r': // read the selection
                if (!choices[ms->choice]->is_dir) {
                    mgmt->intention = 2;
                    goto breakout;
                } else {
                    mgmt->frames  = 2;
                    mgmt->stt_msg = "not a file";
                    break;
                }
            case 'e': // edit the selection
                if (!(mgmt->config->mutable)) {
                    // if the current state is immutable,
                    // warn & continue
                    mgmt->frames  = 2;
                    mgmt->stt_msg = "immutable";
                    break;
                } else {
                    mgmt->intention = 3;
                    goto breakout;
                }
            case 'p': // 'cd ..'
                // roots parents are NULL; nowhere to go
                if (cd->parent == NULL) {
                    mgmt->frames  = 2;
                    mgmt->stt_msg = "already at root";
                    break;
                } else {
                    // only block that doesn't go to breakout
                    mgmt->cd        = mgmt->cd->parent;
                    mgmt->intention = 4;
                    werase(ms->main);
                    return;
                }
        }

        /* if the actual choice is an empty dir slot of the *
         * virtual grid and its less than the first file row, *
         * then 'skip' or 'jump' the cursor to the next true file */

        if (ms->v_choice >= cd->n_dirs && ms->v_choice < ms->ff_row) {
            switch (ch) {
                case KEY_DOWN:
                    ms->v_choice += ms->n_cols;
                    break;
                case KEY_UP:
                    ms->v_choice -= ms->n_cols;
                    break;
                case KEY_RIGHT:
                    ms->v_choice = ms->ff_row;
                    break;
                case KEY_LEFT:
                    ms->v_choice = cd->n_dirs - 1;
                    break;
            }
        }

        // clamp to virtual boundaries
        if (ms->v_choice < 0) {
            ms->v_choice = 0;
        }
        if (ms->v_choice >= ms->v_lim) {
            ms->v_choice = ms->v_lim - 1;
        }

        // map to the true array
        if (ms->v_choice < cd->n_dirs) {
            ms->choice = ms->v_choice;
        } else {
            ms->choice = cd->n_dirs + (ms->v_choice - ms->ff_row);
        }
    }

breakout:
    mgmt->cd = choices[ms->choice];
    werase(ms->main);
    return;
}
