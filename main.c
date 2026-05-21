#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>

#include "config.h"
#include "error.h"
#include "fsio.h"
#include "highlight.h"
#include "main.h"
#include "utils.h"


int main(int argc, char *argv[]) {
    if (argc > 5) {
        usage();
        return 1;
    } else if (argc == 2) { if (strstr(argv[1], "h")) { usage(); return 0; } }

    Config *config;
    config = initialize_config();
    if (!config) {
        print_err(main_src, "failed to initiate the config file from disk", 4);
        return 1;
    }

    if (argc == 1) { config->mode = MENU_MODE; }
    else { /* start at 1 to skip 'lx' arg */
        for (int i = 1; i < argc; i++) {
            char *argi = argv[i];

            /* arguments and running states - overrides default config values */
            if (strcmp(ARGS[nsl], argi) == 0 ||
                        strcmp(ARGS[nss], argi) == 0) {
                config->hide_size = 0;

            } else if (strcmp(ARGS[mtl], argi) == 0 ||
                        strcmp(ARGS[mts], argi) == 0) {
                config->mutable = 0;

            /* turning logs on or off (silenced or verbose) */
            } else if (strcmp(ARGS[sil], argi) == 0) {
                /* silent; logging level CRITICAL */
                config->verbosity = 5;
            } else if (strcmp(ARGS[ver], argi) == 0) {
                /* verbose; logging level DEBUG */
                config->verbosity = 0;

            /* setting a specific verbosity level */
            } else if (argi[0] == '-' && argi[1] == 'v') {
                char v = argi[2]; /* -v5, -v1, v3, etc., */
                if (v > '5' || v < '1') {
                    print_err(main_src, "invalid flag; verbosity must be between 1 - 5", 3);
                    config->mode = FULLFAULT; continue;
                }
                config->verbosity = atoi(&argi[2]);
            
            /* run modes and immediate intentions */
            } else if (i == argc - 1) {
                /* its a path & the last arg; if it doesn't exist, *
                * there's an error, otherwise, proceed based on df_type */
                int root_type;
                char *root = argv[i];
                root_type = df_type(root);
                switch(root_type) {
                    case -2: /* doesn't exist / permission error */
                    case -1: /* symlinks - not accepted */
                        config->mode = FULLFAULT;
                        print_err(main_src, "symlinked files or directories are not accepted", 3);
                        break;
                    case  0: /* file - read or edit, but always EDIT_MODE */
                        config->mode = EDIT_MODE;
                        strcpy(config->root, root);
                        break;
                    case  1: /* directory - always MENU_MODE */
                        config->mode = MENU_MODE;
                        strcpy(config->root, root);
                        break;
                    default: /* something unholy */
                        config->mode = FULLFAULT;
                        break;
                }

            } else { config->mode = FULLFAULT; }
        }
    }

    if (config->mode == FULLFAULT) { free(config->root); free(config); return 1; }

    run(config);

    free_reg(); /* compiled for menu & editor */
    endwin();
    
    flush_logs(config->verbosity);
    
    free(config->root);
    free(config);
    
    return 0;
}


void run(Config *config) {
    init_logging();
    if (spinup_window()) { return; }

    else if (config->mode == EDIT_MODE) {
        // pretty_runner(config->root, config->mutable);
        pretty_runner(config->root, config->mutable, config->indent_chars, config->dedent_chars);
    } else if (config->mode == MENU_MODE) { menu_runner(config); }
}


int spinup_window(void) {
    initscr();
    keypad(stdscr, TRUE);
    raw(); /* capture all keystrokes and */
    noecho(); /* hide the characters from the screen */
    refresh(); /* update changes for getmaxyx & related */
    
    if (load_colors() || compile_regex()) { return 1; }
    return 0;
}


void menu_runner(Config *config) {
    MGMT *mgmt = NULL;
    mgmt = menu_init(config);
    if (!mgmt) { return; }

    while(1) {
        menu(mgmt);
        char path[MAX_FILENAME];
        switch(mgmt->intention) {
            case 0: /* quit / done */
                goto fin;
            case 1:
                break; /* do nothing */
            case 2: /* read (immutable edit) */
                untraverse(mgmt->cd, path);
                // pretty_runner(path, 0);
                pretty_runner(path, 0, config->indent_chars, config->dedent_chars);
                mgmt->cd = mgmt->cd->parent;
                path[0] = '\0';
                break;
            case 3: /* edut (mutable read) */
                untraverse(mgmt->cd, path);
                // pretty_runner(path, 1);
                pretty_runner(path, 1, config->indent_chars, config->dedent_chars);
                mgmt->cd = mgmt->cd->parent;
                path[0] = '\0';
                break;
            case 4: /* 'cd', handling in menu.c */
                break;
        }
        int h, w;
        getmaxyx(stdscr, h, w);
        wresize(mgmt->ms->main, h, w);
        werase(mgmt->ms->main);
        touchwin(mgmt->ms->main);
    }

fin:
    delwin(mgmt->ms->main);

    if (mgmt->root) { free_rfs(mgmt->root); }
    if (mgmt->ms) { free(mgmt->ms); }
    free(mgmt);
}


MGMT *menu_init(Config *config) {
    MGMT *mgmt = NULL;

    if (config->root[0] == '\0') {
        if (getcwd(config->root, MAX_FILENAME) == NULL) {
            print_err(main_src, "failed to find the c.w.d.", 5);
            return NULL;
        }
    }

    FSNode *cd = NULL;
    cd = calloc(1, sizeof(FSNode));
    if (!cd) {
        print_err(main_src, "failed to allocate memory for c.w.d.'s FSNode", 5);
        return NULL;
    }

    char *buffer = NULL;
    buffer = calloc(1, MAX_FILENAME);
    if (!buffer) {
        print_err(main_src, "failed to allocate memory for the recursion buffer", 5);
        free(cd);
        return NULL;
    }

    strcpy(cd->name, config->root);
    strcpy(buffer, config->root);

    if (fls_recurse(cd, buffer)) {
        free(cd);
        free(buffer);
        return NULL;
    }
    free(buffer);
    order_rfs(cd);

    int block_cushion = max_rblocks(cd);

    Mstate *ms = NULL;
    ms = new_MS();
    if (!ms) {
        // free(cd);
        free_rfs(cd);
        return NULL;
    }
    populate_MS(ms, cd, config->hide_size);

    mgmt = new_management();
    if (!mgmt) {
        // free(cd);
        free_rfs(cd);
        free(ms);
        return NULL;
    }

    WINDOW *w = newwin(0, 0, 0, 0);
    keypad(w, TRUE);
    box(w, 0, 0);
    wrefresh(w);
    ms->main = w;

    mgmt->ms = ms;
    mgmt->root = cd;

    mgmt->padd_size = block_cushion;
    mgmt->mutable = config->mutable;
    mgmt->hide_size = config->hide_size;

    return mgmt;
}


MGMT *new_management(void) {
    MGMT *mgmt = NULL;
    mgmt = calloc(1, sizeof(MGMT));
    if (!mgmt) { return NULL; }

    mgmt->root = NULL;
    mgmt->cd = NULL;
    mgmt->ms = NULL;

    mgmt->stt_msg = NULL;
    mgmt->frames = 0;

    mgmt->hide_size = 0;
    mgmt->mutable = 0;
    mgmt->padd_size = 0;
    mgmt->intention = 0;

    return mgmt;
}


Mstate *new_MS(void) {
    Mstate *ms = NULL;
    ms = calloc(1, sizeof(Mstate));
    if (!ms) { return NULL; }

    ms->main = NULL;

    ms->action = 0;
    ms->choice = 0;
    ms->v_choice = 0;

    ms->max_lenfn = 0;
    ms->padding = 0;

    ms->n_cols = 0;
    ms->col_width = 0;

    ms->v_lim = 0;
    ms->fi_init_row = 0;

    return ms;
}


void populate_MS(Mstate *ms, FSNode *cd, int hide_size) {
    int xMax = getmaxx(stdscr);

    int xstrlen;
    for (int x = 0; x < cd->n_children; x++) {
        xstrlen = strlen(cd->children[x]->name);
        if (ms->max_lenfn < xstrlen) { ms->max_lenfn = xstrlen; }
    }

    ms->padding = hide_size ? 2 : 12;
    ms->col_width = ms->max_lenfn + ms->padding;
    ms->n_cols = xMax / ms->col_width;

    if (ms->n_cols > cd->n_children) { ms->n_cols = cd->n_children; }
    if (ms->n_cols < 1)              { ms->n_cols = 1; }

    /* calculate virtual grid of n_choices given n_dirs */
    int dir_rows = (cd->n_dirs) ? ((cd->n_dirs - 1) / ms->n_cols) + 1 : 0;

    /* first row containing files */
    ms->fi_init_row = dir_rows * ms->n_cols;

    /* build from init posit, not first dir posit */
    ms->v_lim = ms->fi_init_row + (cd->n_children - cd->n_dirs);
}


/* initiation & teardown of editor (mutable and not) */
// void pretty_runner(Config *config) {
// void pretty_runner(const char path[], int mutable) {
// void pretty_runner(Config *config, int mutable) {
void pretty_runner(const char path[], int mutable, const char *indent_chars, const char *dedent_chars) {
    Buffer *b = NULL;
    RunTime *rt = NULL;
    Cursor *curs = NULL;

    // b = buffer_load(path);
    b = buffer_load(path);
    if (!b) {
        print_err(main_src, "failed to initiate the buffer", 5);
        goto teardown;
    }

    curs = initialize_cursor(indent_chars, dedent_chars);
    if (!curs) {
        goto teardown;
    }

    rt = init_rt_vars(b);
    if (rt == NULL) { goto teardown; }

    curs_set(1);

    /* run the editor */
    if (alter_file(b, rt, curs, mutable)) {
        print_err(main_src, "alter file returned an error", 4);
    }

teardown:
    teardown_editor(b, rt, curs); /* wipe slate */
}


// Cursor *initialize_cursor(void) {
// Cursor *initialize_cursor(Config *config) {
Cursor *initialize_cursor(const char *indent_chars, const char *dedent_chars) {
    Cursor *curs = calloc(1, sizeof(Cursor));
    if (curs == NULL) {
        print_err(main_src, "failed to initiate the cursor", 5);
        return NULL;
    }

    curs->row = 0;
    curs->col = 0;

    // curs->dedent = "}]";
    curs->dedent = dedent_chars;
    // curs->indent = "{[";
    curs->indent = indent_chars;
    curs->indent_l = 0;

    curs->smsg = NULL;
    curs->sprint = 0;
    
    curs->wo = 0;
    curs->action = 0;

    return curs;
}


RunTime *init_rt_vars(Buffer *b) {
    RunTime *rt = calloc(1, sizeof(RunTime));
    if (rt == NULL) {
        print_err(main_src, "failed to allocate memory for the runtime vars", 5);
        return NULL;
    }

    rt->pad = NULL;

    /* intialize the pad positions */
    rt->pad_row = 0;
    rt->pad_col = 0;

    rt->max_line = 0;
    for (int i = 0; i < b->n_lines; i++) {
        if (b->lines[i].len > rt->max_line) {
            rt->max_line = b->lines[i].len;
        }
    }

    /* find & adjust the given dimensions */
    getmaxyx(stdscr, rt->screen_h, rt->screen_w);
    rt->pad_w = (rt->max_line > rt->screen_w) ? rt->max_line + 1 : rt->screen_w;

    return rt;
}


void teardown_editor(Buffer *b, RunTime *rt, Cursor *curs) {
    if (b) { free_buff(b); }
    if (rt) { free(rt); }
    if (curs) { free(curs); }

    curs_set(0);
    clear(); /* wipe slate before returning to menu */
    refresh();
    touchwin(stdscr); /* force full repaint on next refresh */
}
