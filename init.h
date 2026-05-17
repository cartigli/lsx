#ifndef INIT_H
#define INIT_H

#include "buff.h"
#include "editor.h"
#include "menu.h"
#include "fsio.h"

RunTime *init_rt_vars(Buffer *b);

Cursor *init_cursor(int MUTABLE);

/* intializes screen dimensions, attributes, & elements */
int init_scr(int curs_vis);

/* computes color codes to RGB from hexadecimal */
int compile_regex(void);

int hex_compr(const char c[]);

Mstates *init_MS(FSNode *cd, Mstates *ms, int SHOW_SIZES);
// FVWSpecs *initFVWS(FVWSpecs *fvw);


#endif