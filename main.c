#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "error.h"
#include "fsio.h"
#include "highlight.h"
#include "main.h"
#include "utils.h"

static int arg_parse_path(char *argv, Config *config)
{
    // returns 1 if the arg could be found as an entry

    int dftype = df_type(argv);
    if (dftype == 0) {
        config->mode = EDIT_MODE;
        return 1;
    } else if (dftype == 1) {
        config->mode = MENU_MODE;
        return 1;
    } else {
        char fp[MAX_FILENAME];
        if (getcwd(fp, MAX_FILENAME) != NULL) {
            if (sf_strcat(fp, "/", MAX_FILENAME) ||
                sf_strcat(fp, argv, MAX_FILENAME)) {
                return 0;
            }
            dftype = df_type(fp);
            if (dftype == 0) {
                config->mode = EDIT_MODE;
                return 1;
            } else if (dftype == 1) {
                config->mode = MENU_MODE;
                return 1;
            }
        } else {
            return 0;
        }
    }
    return 0;
}

static int arg_parse(int argc, char *argv[], Config *config)
{
    // returns 0 if args are valid & parsed correctly; any error is 1

    config->mode = MENU_MODE;
    if (argc == 1) {
        return 0;
    }

    // check if the last flag is a path
    // if it is, iterate through argc - 1
    int flag_end = argc;
    if (arg_parse_path(argv[argc - 1], config)) {
        print_inf(main_src, "path passed as arg OK");
        strcpy(config->root, argv[argc - 1]);
        flag_end--;
    }

    for (int i = 1; i < flag_end; i++) {
        char *argi = argv[i];
        if (strcmp("show_size", argi) == 0 || strcmp("-sz", argi) == 0) {
            config->hide_size = 0;
            print_inf(main_src, "hide sizes disabled");

        } else if (strcmp("immutable", argi) == 0 || strcmp("-im", argi) == 0) {
            config->mutable = 0;
            print_inf(main_src, "mutability disabled");
        } else if (strcmp("no_edit", argi) == 0 || strcmp("-ne", argi) == 0) {
            config->mutable = 0;
            print_inf(main_src, "mutability disabled");

        } else if (strcmp("silent", argi) == 0 || strcmp("-s", argi) == 0) {
            // silent: logging level CRITICAL
            config->verbosity = 5;

        } else if (strcmp("verbose", argi) == 0 || strcmp("-v", argi) == 0) {
            // verbose: logging level DEBUG
            config->verbosity = 0;

            // setting a specific verbosity: -v5, -v1, -v3, etc.,
        } else if (argi[0] == '-' && argi[1] == 'v') {
            if (argi[2] > '5' || argi[2] < '1') {
                print_err(main_src,
                    "invalid flag; verbosity"
                    " must be between 1 - 5",
                    3);
                return 1;
            }
            config->verbosity = atoi(&argi[2]);
        } else {
            print_err(main_src, "invalid arg: flag not recognized", 3);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    init_logging();

    Config config;
    initialize_config(&config);

    if (arg_parse(argc, argv, &config)) {
        usage();
    } else {
        run_win(&config);
    }

    print_inf(main_src, "main fin; flushing logs & freeing config");
    flush_logs(config.verbosity);
    free(config.root);

    return 0;
}

void run_win(Config *config)
{
    initscr();
    keypad(stdscr, TRUE);
    raw();     // capture all keystrokes
    noecho();  // hide entered chars/strokes
    refresh(); // ensure getmaxyx gets current vals

    if (config->mode == EDIT_MODE) {
        print_inf(main_src, "mode: EDIT; running editor");
        pretty_runner(config, config->root, config->mutable);
    } else if (config->mode == MENU_MODE) {
        print_inf(main_src, "mode: MENU; running menu");
        menu_runner(config);
    }

    endwin();
    print_inf(main_src, "runner fin; windows torn down");
}

void menu_runner(Config *config)
{
    MGMT mgmt;
    Mstate ms;
    menu_init(&mgmt, &ms, config);

    while (1) {
        menu(&mgmt);
        char path[MAX_FILENAME];
        path[0] = '\0';
        switch (mgmt.intention) {
            case 0: // quit
                goto fin;
            case 1:
                break; // do nothing
            case 2:    // read
                if (untraverse(mgmt.cd, path)) {
                    goto fin;
                }
                pretty_runner(config, path, 0);

                mgmt.cd = mgmt.cd->parent;
                path[0] = '\0';
                break;
            case 3: // edit
                if (untraverse(mgmt.cd, path)) {
                    goto fin;
                }
                pretty_runner(config, path, 1);

                mgmt.cd = mgmt.cd->parent;
                path[0] = '\0';
                break;
            case 4: // 'cd' (handled in menu)
                break;
        }
        int h, w;
        getmaxyx(stdscr, h, w);
        wresize(mgmt.ms->main, h, w);
        werase(mgmt.ms->main);
        touchwin(mgmt.ms->main);
    }

fin:
    delwin(mgmt.ms->main);
    if (mgmt.root) {
        free_rfs(mgmt.root);
    }
    print_inf(main_src, "menu fin; window deleted & memory freed");
}

void menu_init(MGMT *mgmt, Mstate *ms, Config *config)
{
    if (config->root[0] == '\0') {
        if (getcwd(config->root, MAX_FILENAME) == NULL) {
            print_err(main_src, "failed to find the c.w.d.", 5);
            return;
        }
    }

    FSNode *cd = NULL;
    cd         = calloc(1, sizeof(FSNode));
    if (!cd) {
        print_err(main_src,
            "failed to allocate memory"
            "for c.w.d.'s FSNode",
            5);
        return;
    }

    char buffer[MAX_FILENAME];
    strcpy(cd->name, config->root);
    strcpy(buffer, config->root);

    if (fls_recurse(cd, buffer)) {
        free(cd);
        return;
    }
    order_rfs(cd);

    int block_cushion = max_rblocks(cd);

    new_MS(ms);
    populate_MS(ms, cd, config->hide_size, block_cushion);

    new_management(mgmt);

    WINDOW *w = newwin(0, 0, 0, 0);
    keypad(w, TRUE);
    wrefresh(w);

    ms->main = w;

    mgmt->ms   = ms;
    mgmt->root = cd;
    mgmt->cd   = cd;
    // ^ first menu cycle segment fault gaurd:

    mgmt->config    = config;
    mgmt->padd_size = block_cushion;

    print_inf(main_src, "menu vars initialized");
}

void new_management(MGMT *mgmt)
{
    mgmt->config = NULL;

    mgmt->root = NULL;
    mgmt->cd   = NULL;
    mgmt->ms   = NULL;

    mgmt->stt_msg = NULL;
    mgmt->frames  = 0;

    mgmt->padd_size = 0;
    mgmt->intention = 0;
}

void new_MS(Mstate *ms)
{
    ms->main = NULL;

    ms->action   = 0;
    ms->choice   = 0;
    ms->v_choice = 0;

    ms->max_lenfn = 0;
    ms->padding   = 0;

    ms->n_cols    = 0;
    ms->col_width = 0;

    ms->v_lim  = 0;
    ms->ff_row = 0;
}

void populate_MS(Mstate *ms, FSNode *cd, int hide_size, int block_size)
{
    int xMax = getmaxx(stdscr);

    int xstrlen;
    for (int x = 0; x < cd->n_children; x++) {
        xstrlen = strlen(cd->children[x]->name);
        if (ms->max_lenfn < xstrlen) {
            ms->max_lenfn = xstrlen;
        }
    }

    ms->block_size = block_size;
    ms->padding    = hide_size ? 2 : 10 + ms->block_size;

    ms->col_width = ms->max_lenfn + ms->padding;
    ms->n_cols    = xMax / ms->col_width;

    if (ms->n_cols > cd->n_children) {
        ms->n_cols = cd->n_children;
    }
    if (ms->n_cols < 1) {
        ms->n_cols = 1;
    }

    // calculate a virtual grid of n choices given n dirs
    int dir_rows = (cd->n_dirs) ? ((cd->n_dirs - 1) / ms->n_cols) + 1 : 0;

    // first row containing only files
    ms->ff_row = dir_rows * ms->n_cols;

    // build from the initial position, not the first directory's position
    ms->v_lim = ms->ff_row + (cd->n_children - cd->n_dirs);
}

void pretty_runner(Config *config, const char path[], int mutable)
{
    Buffer *b = NULL;
    RunTime rt;
    Cursor curs;

    b = buffer_load(path);
    if (!b) {
        goto teardown;
    }

    initialize_cursor(&curs, config);

    init_rt_vars(&rt, b);

    if (!config->colors_loaded) {
        if (load_colors()) {
            return;
        }
    }
    config->colors_loaded = 1;

    language l;
    l = detect_lang(path);
    if (compile_regex(l)) return;

    char bf[42];
    snprintf(bf, sizeof(bf), "detected lang: %s", LANG_NAMES[l].l);
    print_inf(main_src, bf);

    curs_set(1);

    print_inf(main_src, "editor runtime vars initialized");

    alter_file(b, &rt, &curs, path, mutable);

teardown:
    free_reg();
    if (b) {
        free(b);
    }

    config->colors_loaded = 0;

    curs_set(0);
    clear(); // clear screen before returning
    refresh();
    touchwin(stdscr); // 'wake' screen; force full repaint next refresh

    print_inf(main_src, "editor cleaned & memory freed");
}

void initialize_cursor(Cursor *curs, Config *config)
{
    curs->row = 0;
    curs->col = 0;

    curs->smsg   = NULL;
    curs->sprint = 0;

    curs->wo     = 0;
    curs->action = 0;

    curs->indent_l   = 0;
    curs->indent_len = config->indent_len;
}

void init_rt_vars(RunTime *rt, Buffer *b)
{
    rt->pad = NULL;

    rt->pad_row = 0;
    rt->pad_col = 0;

    rt->ps = PASTE_IDLE;

    rt->max_line = 0;
    for (int i = 0; i < b->n_lines; i++) {
        if (b->lines[i].len > rt->max_line) {
            rt->max_line = b->lines[i].len;
        }
    }

    // set the pad's width based on screen width
    // (length gets doubled by editor until it meets min.)
    getmaxyx(stdscr, rt->screen_h, rt->screen_w);
    rt->pad_w = (rt->max_line > rt->screen_w) ? rt->max_line + 1 : rt->screen_w;
}

language detect_lang(const char path[])
{
    /* detect a path's 'type' by the file ending present, if *
     * any default: blank, no expressions, no highlighting *
     * checks for first '.'; does not continue if bad ending found */

    language l = blank;
    int len    = strlen(path) - 1;

    for (int i = 0; i < len; i++) {
        if (path[i] == '.') {
            if (path[i + 1] == 'c' || path[i + 1] == 'h') {
                l = c;
            } else if (path[i + 1] == 'p' && path[i + 2] == 'y') {
                l = py;
            }
        }
    }
    return l;
}
