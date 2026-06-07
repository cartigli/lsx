#ifndef PY_COLOR_H
#define PY_COLOR_H

#include "highlight.h"

extern const SyntaxDemand PY_DEMANDS[];
extern const unsigned int N_PY_DEMANDS;
extern RGXE PY_RT_DEMANDS[];

extern const char *PY_INDENTABLES;
extern const char *PY_DEDENTABLES;
extern const unsigned int PY_INDENT_LEN;

extern const SyntaxSpan PY_TWINTERMS[];
extern const unsigned int N_PY_TWINTERMS;
extern RGXE PY_RT_MXI_DEMANDS[];
extern RGXE PY_RT_MXK_DEMANDS[];

#endif
