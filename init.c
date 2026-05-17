#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#include "highlight.h"
#include "init.h"

int compile_regex(void) {
    /* for every demand, compile the RegEx expression & cache the result *
     * additionally, enforce the order with a check before proceeding */
    CmpOrder previous = NUMERICALS;
    for (unsigned int x = 0; x < N_DEMANDS; x++) {
        if (previous > DEMANDS[x].type) {
            printf("DEMAND [%u] OUT OF ORDER | QUITTING\n", x);
            return 1;
        }
        previous = DEMANDS[x].type;

        DEMANDS[x].compiled = (regcomp(&DEMANDS[x].cmp_expression,
                                        DEMANDS[x].expression,
                                        REG_EXTENDED) == 0) ? 1 : 0;
    }
    return 0;
}


Cursor *init_cursor(int MUTABLE) {
    Cursor *curs = malloc(sizeof(Cursor));
    if (curs == NULL) { return NULL; }
    curs->row = 0;
    curs->col = 0;

    curs->dedent = "}]";
    curs->indent = "{[";
    curs->indent_l = 0;

    curs->wo = 0;
    curs->smsg = 0;

    curs->sprint = 0;
    curs->act_code = 0;

    curs->Mutable = MUTABLE;
    return curs;
}


RunTime *init_rt_vars(Buffer *b) {
    RunTime *rt = malloc(sizeof(RunTime));
    if (rt == NULL) { return NULL; }
     rt->max_line = 0;
    for (int i = 0; i < b->n_lines; i++) {
        if (b->lines[i].len > rt->max_line) {
            rt->max_line = b->lines[i].len;
        }
    }

    rt->pad = NULL;

    /* intialize the pad positions */
    rt->pad_row = 0;
    rt->pad_col = 0;

    /* find & adjust the given dimensions */
    getmaxyx(stdscr, rt->screen_h, rt->screen_w);
    rt->pad_w = (rt->max_line > rt->screen_w) ? rt->max_line + 1 : rt->screen_w;
    rt->view_h = rt->screen_h - 1;
    rt->view_w = rt->screen_w;

    // /* set writeout to FALSE (0) */
    // rt->wo = 0;

    // /* initialize action to 0 */
    // rt->act_code = 0;

    // /* set the message & content to empty */
    // rt->sprint = 0;
    // rt->smsg = NULL;

    return rt;
}


int init_scr(int curs_vis) {
    initscr();
    if (has_colors()) {
        start_color();
        /* args: (int: pair_no, fg color, bg color) */
        short keys_npres = 14;
        short functions = 15;
        short ints_ndecs = 16;
        short declr_vars = 17;
        short comments = 18;
        short strings = 19;
        short operands = 20;
        short testing = 21;

        init_color(keys_npres, 
            hex_compr(COLOR_CODES[0].r),
            hex_compr(COLOR_CODES[0].g),
            hex_compr(COLOR_CODES[0].b)
        );
        init_color(functions,
            hex_compr(COLOR_CODES[1].r),
            hex_compr(COLOR_CODES[1].g),
            hex_compr(COLOR_CODES[1].b)
        );
        init_color(ints_ndecs,
            hex_compr(COLOR_CODES[2].r),
            hex_compr(COLOR_CODES[2].g),
            hex_compr(COLOR_CODES[2].b)
        );
        init_color(declr_vars,
            hex_compr(COLOR_CODES[3].r),
            hex_compr(COLOR_CODES[3].g),
            hex_compr(COLOR_CODES[3].b)
        );
        init_color(comments,
            hex_compr(COLOR_CODES[4].r),
            hex_compr(COLOR_CODES[4].g),
            hex_compr(COLOR_CODES[4].b)
        );
        init_color(strings,
            hex_compr(COLOR_CODES[5].r),
            hex_compr(COLOR_CODES[5].g),
            hex_compr(COLOR_CODES[5].b)
        );
        init_color(operands,
            hex_compr(COLOR_CODES[6].r),
            hex_compr(COLOR_CODES[6].g),
            hex_compr(COLOR_CODES[6].b)
        );
        init_color(testing,
            hex_compr(COLOR_CODES[7].r),
            hex_compr(COLOR_CODES[7].g),
            hex_compr(COLOR_CODES[7].b)
        );

        init_pair(1, keys_npres, COLOR_BLACK);
        init_pair(2,  functions, COLOR_BLACK);
        init_pair(3, ints_ndecs, COLOR_BLACK);
        init_pair(4, declr_vars, COLOR_BLACK);
        init_pair(5,   comments, COLOR_BLACK);
        init_pair(6,    strings, COLOR_BLACK);
        init_pair(7,   operands, COLOR_BLACK);
        init_pair(8,    testing, COLOR_BLACK);

    } else {
        return 1;
    }
    raw();                 /* catch ctrl + c as well as ctrl + o & other commons */
    noecho();
    keypad(stdscr, TRUE);  /* we do want to capture key strokes */
    curs_set(curs_vis);           /* initialize the cursor */
    leaveok(stdscr, TRUE); /* physical cursor doesn't need to appear on screen */
    return 0;
}

int hex_compr(const char c[]) {
    int ccode;
    sscanf(c, "%x", &ccode);
    return (int)((ccode * 1000) / 255.0 );
}



Mstates *init_MS(FSNode *cd, Mstates *ms, int SHOW_SIZES) {
    ms->unkn_action = 0;
    ms->choice = 0;

    int xMax = getmaxx(stdscr);

    int xstrlen;
    ms->max_lenfn = 0;
    for (int x = 0; x < cd->n_children; x++) {
        xstrlen = strlen(cd->children[x]->name);
        if (ms->max_lenfn < xstrlen) { ms->max_lenfn = xstrlen; }
    }

    ms->padding = (SHOW_SIZES) ? 2 : 17;
    ms->col_width = ms->max_lenfn + ms->padding;
    ms->n_cols = xMax / ms->col_width;

    if (ms->n_cols < 1)              { ms->n_cols = 1; }
    if (ms->n_cols > cd->n_children) { ms->n_cols = cd->n_children; }

    /* calculate virtual grid of n_choices given n_dirs */
    int dir_rows = (cd->n_dirs) ? ((cd->n_dirs - 1) / ms->n_cols) + 1 : 0;

    ms->fi_init_row = dir_rows * ms->n_cols;            /* first row containing files */
    ms->v_lim = ms->fi_init_row + (cd->n_children - cd->n_dirs); /* build from init posit, not first dir posit */

    ms->cd_selected = 0;
    ms->rf_selected = 0;
    ms->pd_selected = 0;
    return ms;
}




// FVWSpecs *initFVWS(FVWSpecs *fvw) {
//     getmaxyx(stdscr, fvw->screen_h, fvw->screen_w);

//     /* make pad atleast size of window incase file doesn't fill */
//     /* condition ? expression if true : expression if false */
//     fvw->pad_w = (fvw->max_line > fvw->screen_w) ? fvw->max_line + 1 : fvw->screen_w;

//     /* scrolling state : which row/column is in the top-left of the viewport */
//     fvw->pad_row = 0;
//     fvw->pad_col = 0;
//     fvw->view_h = fvw->screen_h - 1; /* room for status bar at the bottom */
//     fvw->view_w = fvw->screen_w;

//     return fvw;
// }
