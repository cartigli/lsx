#ifndef BLANK_H
#define BLANK_H

#include "highlight.h"

extern const SyntaxDemand BLANK_DEMANDS[];
extern RGXE BLANK_RT_DEMANDS[];

extern const char *BLANK_INDENTABLES;
extern const char *BLANK_DEDENTABLES;
extern const unsigned int BLANK_INDENT_LEN;

extern const unsigned int N_BLANK_DEMANDS;

extern const SyntaxSpan BLANK_TWINTERMS[];
extern const unsigned int N_BLANK_TWINTERMS;
extern RGXE BLANK_RT_MXI_DEMANDS[];
extern RGXE BLANK_RT_MXK_DEMANDS[];

#endif
