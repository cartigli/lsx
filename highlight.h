#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <regex.h>

#include "types.h"


extern const ColorCode COLOR_CODES[];

extern unsigned int N_GLOBAL_DEMANDS;
extern SyntaxDemands *GLOBAL_DEMANDS;
extern const char *GLOBAL_INDENTABLES;
extern const char *GLOBAL_DEDENTABLES;
extern int GLOBAL_INDENT_LEN;

int load_colors(void);

// static inline int hex_compr(const char hex[]);

int compile_regex(language lang);

void free_reg(void);


#endif