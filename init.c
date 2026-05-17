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

    /* set writeout to FALSE (0) */
    rt->wo = 0;

    /* initialize action to 0 */
    rt->act_code = 0;

    /* set the message & content to empty */
    rt->sprint = 0;
    rt->smsg = NULL;

    return rt;
}


Cursor *init_cursor(void) {
    Cursor *curs = malloc(sizeof(Cursor));
    if (curs == NULL) { return NULL; }
    curs->row = 0;
    curs->col = 0;
    curs->dedent = "}]";
    curs->indent = "{[";
    curs->indent_l = 0;
    return curs;
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



RTSpecs *init_RTS(FSNode *cd, RTSpecs *rts, int SHOW_SIZES) {
    rts->unkn_action = 0;
    rts->choice = 0;

    int xMax = getmaxx(stdscr);

    int xstrlen;
    rts->max_lenfn = 0;
    for (int x = 0; x < cd->n_children; x++) {
        xstrlen = strlen(cd->children[x]->name);
        if (rts->max_lenfn < xstrlen) { rts->max_lenfn = xstrlen; }
    }

    rts->padding = (SHOW_SIZES) ? 2 : 17;
    // rts->padding = 0;
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




FVWSpecs *initFVWS(FVWSpecs *fvw) {
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
