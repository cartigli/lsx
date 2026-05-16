#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <regex.h>


/* so you can use: COLOR_CODES[PINK] *
 * to get the hex code for pink */
enum COLORS {
    PINK = 1,
    GREEN = 2,
    PURPLE = 3,
    CYAN = 4,
    LTGRAY = 5,
    YELLOW = 6,
    REDDSH = 7,
    TEAL = 8
};


/* enforce the order of expressions *
 * comprehended by the regex engine */
typedef enum {
    NUMERICALS,  /* 1, 23 */
    VARIABLES,   /* int i; */
    FUNCTIONS,   /* main() */
    OPERANDS,    /* 20 * 5 */
    PREPROCS,    /* #include */
    HEADERS,     /* regex.h */
    KEYWORDS,    /* return; */
    PUNCTUATION, /* , { ; */
    STRINGS,     /* "hello world" */
    SUBSTITUTES, /* "hello %s" */
    COMMENTS     /* // comment */
} CmpOrder;


typedef struct {
    const char *expression; /* RegEx expression (string) */
    regex_t cmp_expression; /* compiled RegEx expression */
    enum COLORS color_code; /* a code to RGB code in hex */
    CmpOrder          type; /* the type of expression */
    int           compiled; /* bool for a valid compile */
} SyntaxDemands;


typedef struct {
    const char r[4];
    const char g[4];
    const char b[4];
} ColorCode;


extern const ColorCode COLOR_CODES[];

extern SyntaxDemands DEMANDS[];

extern const unsigned int N_DEMANDS;


#endif