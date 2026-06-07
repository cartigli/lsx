#ifndef MARKDOWN_H
#define MARKDOWN_H

#include "highlight.h"

extern const SyntaxDemand MD_DEMANDS[];
extern const unsigned int N_MD_DEMANDS;
extern RGXE MD_RT_DEMANDS[];

extern const char *MD_INDENTABLES;
extern const char *MD_DEDENTABLES;
extern const unsigned int MD_INDENT_LEN;

extern const SyntaxSpan MD_TWINTERMS[];
extern const unsigned int N_MD_TWINTERMS;
extern RGXE MD_RT_MXI_DEMANDS[];
extern RGXE MD_RT_MXK_DEMANDS[];

#endif
