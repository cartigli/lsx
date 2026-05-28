#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <regex.h>

#include "types.h"


// enum'd color codes mapped to RGB values
extern const ColorCode COLOR_CODES[];

// per-line demands
extern unsigned int N_GLOBAL_DEMANDS;
extern SyntaxDemands *GLOBAL_DEMANDS;

extern const char *GLOBAL_INDENTABLES;
extern const char *GLOBAL_DEDENTABLES;
extern int GLOBAL_INDENT_LEN;

// multi-line demands; initiators & terminators
extern SyntaxTwins *GLOBAL_MULTILINE_PAIRS;
extern unsigned int N_GLOBAL_MULTILINE_DEMAND_PAIRS;

// calculate & compile ncurses color indeces
int load_colors(void);

// compile & cache regex expressions
int compile_regex(language lang);

// free compiled regex
void free_reg(void);


#endif