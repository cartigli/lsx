#include <ncurses.h>
#include <stdio.h>

#include "blank.h"
#include "c_color.h"
#include "py_color.h"
#include "types.h"
#include "highlight.h"
#include "error.h"

/* externed global regex rules */
SyntaxDemands *GLOBAL_DEMANDS = NULL;
unsigned int N_GLOBAL_DEMANDS = 0;

const char *GLOBAL_INDENTABLES = NULL;
const char *GLOBAL_DEDENTABLES = NULL;
int GLOBAL_INDENT_LEN = 0;

SyntaxTwins *GLOBAL_MULTILINE_PAIRS = NULL;
unsigned int N_GLOBAL_MULTILINE_DEMAND_PAIRS = 0;


/* RGB codes for color scheme (hexadecimal format) */

const ColorCode COLOR_CODES[] = {
    { "f6", "aa", "d3", }, /* pink      */
    // { 245, 170, 211, }, /* pink */
    // { 961, 667, 828, },
    { "23", "ad", "61", }, /* green     */
    // { 35, 173, 97, }, /* green */
    // { 137, 679, 380, },
    { "c3", "91", "ed", }, /* purple    */
    // { 195, 145, 237, }, /* purple */
    // { 765, 569, 930, },
    { "87", "ce", "eb", }, /* cyan      */
    // { 135, 206, 235, }, /* cyan */
    // { 530, 808, 922, },
    { "b3", "b3", "b3", }, /* gray      */
    // { 179, 179, 179, }, /* gray */
    // { 702, 702, 702, },
    { "fa", "f1", "87", }, /* yellow    */
    // { 250, 241, 135, }, /* yellow */
    // { 981, 945, 530, },
    { "e6", "67", "6b", }, /* reddish   */
    // { 230, 103, 107, }, /* reddish */
    // { 902, 404, 420, },
    { "27", "f5", "be", }, /* teal      */
    // { 39, 245, 190, }, /* teal */
    // { 153, 961, 745, },
    { "e9", "97", "3f", }, /* orange    */
    // { 233, 151, 63, }, /* orange */
    // { 914, 592, 247, },
    { "53", "df", "dd", }, /* py_cyan   */
    // { 83, 223, 221, }, /* py cyan */
    // { 326, 875, 868, },
    { "a8", "82", "ff", }, /* py_purple */
    // { 168, 130, 255, }, /* py_purple */
    // { 659, 510, 1000, },
    { "44", "cf", "6e", }, /* py_green  */
    // { 68, 207, 110, }, /* py_green  */
    // { 267, 812, 431, },
    { "b3", "b3", "b3", }, // duplicate for color distinction
};


static inline int hex_compr(const char hex[]) {
    int ccode;
    sscanf(hex, "%x", &ccode);
    return (int)((ccode * 1000) / 255.0 );
}


int load_colors(void) {
    if (!has_colors()) {
        print_err(hlte_src, "terminal does not support colors", 3);
        return 1;
    }
    start_color();

    /* args: (int: pair_no, fg color, bg color) */
    short keys_npres = 14; /* pink */
    short functions  = 15; /* green */
    short ints_ndecs = 16; /* purple */
    short declr_vars = 17; /* cyan */
    short comments   = 18; /* light gray */
    short strings    = 19; /* yellow */
    short operands   = 20; /* reddish */
    short testing    = 21; /* teal */
    short orange     = 22; /* orange */
    short py_cyan    = 23; /* python cyan */
    short py_purple  = 24; /* python purple */
    short py_green   = 25; /* python green */
    short ml_gray    = 26; // duplicate gray
    
    // init_color(keys_npres, 961, 667, 828
    init_color(
        keys_npres, // COLOR_CODES[0].r, COLOR_CODES[0].g, COLOR_CODES[0].b
        hex_compr(COLOR_CODES[0].r),
        hex_compr(COLOR_CODES[0].g),
        hex_compr(COLOR_CODES[0].b)
    );
    // init_color(functions, 137, 679, 380
    init_color(
        functions, // COLOR_CODES[1].r, COLOR_CODES[1].g, COLOR_CODES[1].b
        hex_compr(COLOR_CODES[1].r),
        hex_compr(COLOR_CODES[1].g),
        hex_compr(COLOR_CODES[1].b)
    );
    // init_color(ints_ndecs, 765, 569, 930
    init_color(
        ints_ndecs, // COLOR_CODES[2].r, COLOR_CODES[2].g, COLOR_CODES[2].b
        hex_compr(COLOR_CODES[2].r),
        hex_compr(COLOR_CODES[2].g),
        hex_compr(COLOR_CODES[2].b)
    );
    // init_color(declr_vars, 530, 808, 922
    init_color(
        declr_vars, // COLOR_CODES[3].r, COLOR_CODES[3].g, COLOR_CODES[3].b
        hex_compr(COLOR_CODES[3].r),
        hex_compr(COLOR_CODES[3].g),
        hex_compr(COLOR_CODES[3].b)
    );
    // init_color(comments, 702, 702, 702
    init_color(comments, // COLOR_CODES[4].r, COLOR_CODES[4].g, COLOR_CODES[4].b
        hex_compr(COLOR_CODES[4].r),
        hex_compr(COLOR_CODES[4].g),
        hex_compr(COLOR_CODES[4].b)
    );
    // init_color(strings, 981, 945, 530
    init_color(strings, // COLOR_CODES[5].r, COLOR_CODES[5].g, COLOR_CODES[5].b
        hex_compr(COLOR_CODES[5].r),
        hex_compr(COLOR_CODES[5].g),
        hex_compr(COLOR_CODES[5].b)
    );
    // init_color(operands, 902, 404, 420
    init_color(operands, // COLOR_CODES[6].r, COLOR_CODES[6].g, COLOR_CODES[6].b
        hex_compr(COLOR_CODES[6].r),
        hex_compr(COLOR_CODES[6].g),
        hex_compr(COLOR_CODES[6].b)
    );
    // init_color(testing, 153, 961, 745
    init_color(testing, // COLOR_CODES[7].r, COLOR_CODES[7].g, COLOR_CODES[7].b
        hex_compr(COLOR_CODES[7].r),
        hex_compr(COLOR_CODES[7].g),
        hex_compr(COLOR_CODES[7].b)
    );
    // init_color(orange, 914, 592, 247
    init_color(orange, // COLOR_CODES[8].r, COLOR_CODES[8].g, COLOR_CODES[8].b
        hex_compr(COLOR_CODES[8].r),
        hex_compr(COLOR_CODES[8].g),
        hex_compr(COLOR_CODES[8].b)
    );
    // init_color(py_cyan, 326, 875, 868
    init_color(py_cyan, // COLOR_CODES[9].r, COLOR_CODES[9].g, COLOR_CODES[9].b
        hex_compr(COLOR_CODES[9].r),
        hex_compr(COLOR_CODES[9].g),
        hex_compr(COLOR_CODES[9].b)
    );
    // init_color(py_purple, 659, 510, 1000
    init_color(py_purple, // COLOR_CODES[10].r, COLOR_CODES[10].g, COLOR_CODES[10].b
        hex_compr(COLOR_CODES[10].r),
        hex_compr(COLOR_CODES[10].g),
        hex_compr(COLOR_CODES[10].b)
    );
    // init_color(py_green, 267, 812, 431
    init_color(py_green, // COLOR_CODES[11].r, COLOR_CODES[11].g, COLOR_CODES[11].b
        hex_compr(COLOR_CODES[11].r),
        hex_compr(COLOR_CODES[11].g),
        hex_compr(COLOR_CODES[11].b)
    );
    init_color(ml_gray,
        hex_compr(COLOR_CODES[12].r),
        hex_compr(COLOR_CODES[12].g),
        hex_compr(COLOR_CODES[12].b)
    );

    init_pair(1,  keys_npres, COLOR_BLACK);
    init_pair(2,  functions,  COLOR_BLACK);
    init_pair(3,  ints_ndecs, COLOR_BLACK);
    init_pair(4,  declr_vars, COLOR_BLACK);
    init_pair(5,  comments,   COLOR_BLACK);
    init_pair(6,  strings,    COLOR_BLACK);
    init_pair(7,  operands,   COLOR_BLACK);
    init_pair(8,  testing,    COLOR_BLACK);
    init_pair(9,  orange,     COLOR_BLACK);
    init_pair(10, py_cyan,    COLOR_BLACK);
    init_pair(11, py_purple,  COLOR_BLACK);
    init_pair(12, py_green,   COLOR_BLACK);
    init_pair(13, ml_gray,   COLOR_BLACK);

    return 0;
}


int compile_regex(language lang) {
    // for the given lang, cache each regex expression after compiling it
    // additionally, quit if the regex expressions' are misordered
    SyntaxDemands *active_demands = NULL;
    unsigned int n_demands = 0;
    const char *active_indentables = NULL;
    const char *active_dedentables = NULL;
    SyntaxTwins *active_multi_pairs = NULL;
    int n_active_multi_pairs = 0;
    int active_indent_len = 0;

    switch(lang) {
        case c:
            active_demands = C_DEMANDS;
            n_demands = C_N_DEMANDS;

            active_indentables = C_INDENTABLES;
            active_dedentables = C_DEDENTABLES;
            active_indent_len = C_INDENT_LENGTH;

            active_multi_pairs = C_MULTI_PAIR_DEMANDS;
            n_active_multi_pairs = N_C_MULTI_PAIR_DEMANDS;
            break;

        case py:
            active_demands = PY_DEMANDS;
            n_demands = PY_N_DEMANDS;

            active_indentables = PY_INDENTABLES;
            active_dedentables = PY_DEDENTABLES;
            active_indent_len = PY_INDENT_LENGTH;

            active_multi_pairs = PY_MXX_TWINTERMS;
            n_active_multi_pairs = N_PY_MXX_TWINTERMS;
            break;

        case blank:
        default:
            active_demands = BLANK_DEMANDS;
            n_demands = BLANK_N_DEMANDS;

            active_indentables = BLANK_INDENTABLES;
            active_dedentables = BLANK_DEDENTABLES;
            active_indent_len = BLANK_INDENT_LENGTH;
            break;
    }

    if (!active_demands) { return 1; }

    GLOBAL_DEMANDS = active_demands;
    N_GLOBAL_DEMANDS = n_demands;

    GLOBAL_INDENTABLES = active_indentables;
    GLOBAL_DEDENTABLES = active_dedentables;
    GLOBAL_INDENT_LEN = active_indent_len;

    GLOBAL_MULTILINE_PAIRS = active_multi_pairs;
    N_GLOBAL_MULTILINE_DEMAND_PAIRS = n_active_multi_pairs;

    CmpOrder previous = NUMERICALS;
    for (unsigned int x = 0; x < n_demands; x++) {
        /* hard stop if mis-orderd expressions */
        if (previous > active_demands[x].type) {
            print_err(hlte_src, "failed to compile regex_expressions;"
                        " their order is corrupt", 3);
            return 1;
        }
        previous = active_demands[x].type;
        active_demands[x].compiled = (regcomp(&active_demands[x].cmp_expression,
                    active_demands[x].expression, REG_EXTENDED) == 0) ? 1 : 0;
    }
    for (unsigned int i = 0; i < 1; i++) {
        active_multi_pairs->ix[i].compiled = (regcomp(&active_multi_pairs->ix[i].cmp_expression,
                    active_multi_pairs->ix[i].expression, REG_EXTENDED) == 0) ? 1 : 0;
        active_multi_pairs->kx[i].compiled = (regcomp(&active_multi_pairs->kx[i].cmp_expression,
                    active_multi_pairs->kx[i].expression, REG_EXTENDED) == 0) ? 1 : 0;
    }
    return 0;
}


void free_reg(void) {
    if (!GLOBAL_DEMANDS) { return; }
    for (unsigned int f = 0; f < N_GLOBAL_DEMANDS; f++) {
        if (GLOBAL_DEMANDS[f].compiled) {
            regfree(&GLOBAL_DEMANDS[f].cmp_expression);
        }
    }
    N_GLOBAL_DEMANDS = 0;
    GLOBAL_DEMANDS = NULL;
    GLOBAL_INDENTABLES = NULL;
    GLOBAL_DEDENTABLES = NULL;
}
