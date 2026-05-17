#ifndef INIT_H
#define INIT_H

#include "buff.h"
#include "editor.h"
#include "menu.h"
#include "fsio.h"

RunTime *init_rt_vars(Buffer *b);

Cursor *init_cursor(void);

/* intializes screen dimensions, attributes, & elements */
int init_scr(int curs_vis);

/* computes color codes to RGB from hexadecimal */
int compile_regex(void);

int hex_compr(const char c[]);

RTSpecs *init_RTS(FSNode *cd, RTSpecs *rts, int SHOW_SIZES);
FVWSpecs *initFVWS(FVWSpecs *fvw);


#endif