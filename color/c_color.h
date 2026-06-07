#ifndef C_COLOR_H
#define C_COLOR_H

#include "highlight.h"

extern const SyntaxDemand C_DEMANDS[];
extern const unsigned int N_C_DEMANDS;
extern RGXE C_RT_DEMANDS[];

extern const char *C_INDENTABLES;
extern const char *C_DEDENTABLES;
extern const unsigned int C_INDENT_LEN;

extern const SyntaxSpan C_TWINTERMS[];
extern const unsigned int N_C_TWINTERMS;
extern RGXE C_RT_MXI_DEMANDS[];
extern RGXE C_RT_MXK_DEMANDS[];

#endif
