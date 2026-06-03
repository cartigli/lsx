#include <ncurses.h>
#include <stdio.h>

#include "blank.h"
#include "c_color.h"
#include "error.h"
#include "highlight.h"
#include "py_color.h"
#include "types.h"

const lang_names LANG_NAMES[] = {
    {
        .lang = c,
        .l    = "c",
    },
    {
        .lang = py,
        .l    = "py",
    },
    {
        .lang = blank,
        .l    = "none",
    },
};

// globally available compiled syntax expressions
SyntaxDemands *GLOBAL_DEMANDS = NULL;
unsigned int N_GLOBAL_DEMANDS = 0;

// globally available chars to alter the indent state
const char *GLOBAL_INDENTABLES = NULL;
const char *GLOBAL_DEDENTABLES = NULL;
int GLOBAL_INDENT_LEN          = 0;

// pairs of start | end for multi-line expressions
SyntaxTwins *GLOBAL_MULTILINE_PAIRS          = NULL;
unsigned int N_GLOBAL_MULTILINE_DEMAND_PAIRS = 0;

// RGB codes for color scheme (hexadecimal values)

const ColorCode COLOR_CODES[] = {
    {
        "f6",
        "aa",
        "d3",
    }, // pink
    {
        "23",
        "ad",
        "61",
    }, // green
    {
        "c3",
        "91",
        "ed",
    }, // purple
    {
        "87",
        "ce",
        "eb",
    }, // cyan
    {
        "b3",
        "b3",
        "b3",
    }, // gray
    {
        "fa",
        "f1",
        "87",
    }, // yellow
    {
        "e6",
        "67",
        "6b",
    }, // reddish
    {
        "27",
        "f5",
        "be",
    }, // teal
    {
        "e9",
        "97",
        "3f",
    }, // py_orange
    {
        "53",
        "df",
        "dd",
    }, // py_cyan
    {
        "a8",
        "82",
        "ff",
    }, // py_purple
    {
        "44",
        "cf",
        "6e",
    }, // py_green
    {
        "b3",
        "b3",
        "b3",
    }, // gray duplicate
};

static inline int hex_compr(const char hex[])
{
    int ccode;
    sscanf(hex, "%x", &ccode);
    return (int)((ccode * 1000) / 255.0);
}

int load_colors(void)
{
    if (!has_colors()) {
        print_err(hlte_src, "terminal does not support colors", 3);
        return 1;
    }
    start_color();

    // args: (int: pair_no, fg color, bg color)
    short keys_npres = 14; // pink
    short functions  = 15; // green
    short ints_ndecs = 16; // purple
    short declr_vars = 17; // cyan
    short comments   = 18; // light gray
    short strings    = 19; // yellow
    short operands   = 20; // reddish
    short testing    = 21; // teal
    short py_orange  = 22; // py_orange
    short py_cyan    = 23; // python cyan
    short py_purple  = 24; // python purple
    short py_green   = 25; // python green
    short ml_gray    = 26; // duplicate gray

    init_color(keys_npres, hex_compr(COLOR_CODES[0].r),
        hex_compr(COLOR_CODES[0].g), hex_compr(COLOR_CODES[0].b));
    init_color(functions, hex_compr(COLOR_CODES[1].r),
        hex_compr(COLOR_CODES[1].g), hex_compr(COLOR_CODES[1].b));
    init_color(ints_ndecs, hex_compr(COLOR_CODES[2].r),
        hex_compr(COLOR_CODES[2].g), hex_compr(COLOR_CODES[2].b));
    init_color(declr_vars, hex_compr(COLOR_CODES[3].r),
        hex_compr(COLOR_CODES[3].g), hex_compr(COLOR_CODES[3].b));
    init_color(comments, hex_compr(COLOR_CODES[4].r),
        hex_compr(COLOR_CODES[4].g), hex_compr(COLOR_CODES[4].b));
    init_color(strings, hex_compr(COLOR_CODES[5].r),
        hex_compr(COLOR_CODES[5].g), hex_compr(COLOR_CODES[5].b));
    init_color(operands, hex_compr(COLOR_CODES[6].r),
        hex_compr(COLOR_CODES[6].g), hex_compr(COLOR_CODES[6].b));
    init_color(testing, hex_compr(COLOR_CODES[7].r),
        hex_compr(COLOR_CODES[7].g), hex_compr(COLOR_CODES[7].b));
    init_color(py_orange, hex_compr(COLOR_CODES[8].r),
        hex_compr(COLOR_CODES[8].g), hex_compr(COLOR_CODES[8].b));
    init_color(py_cyan, hex_compr(COLOR_CODES[9].r),
        hex_compr(COLOR_CODES[9].g), hex_compr(COLOR_CODES[9].b));
    init_color(py_purple, hex_compr(COLOR_CODES[10].r),
        hex_compr(COLOR_CODES[10].g), hex_compr(COLOR_CODES[10].b));
    init_color(py_green, hex_compr(COLOR_CODES[11].r),
        hex_compr(COLOR_CODES[11].g), hex_compr(COLOR_CODES[11].b));
    init_color(ml_gray, hex_compr(COLOR_CODES[12].r),
        hex_compr(COLOR_CODES[12].g), hex_compr(COLOR_CODES[12].b));

    init_pair(1, keys_npres, COLOR_BLACK);
    init_pair(2, functions, COLOR_BLACK);
    init_pair(3, ints_ndecs, COLOR_BLACK);
    init_pair(4, declr_vars, COLOR_BLACK);
    init_pair(5, comments, COLOR_BLACK);
    init_pair(6, strings, COLOR_BLACK);
    init_pair(7, operands, COLOR_BLACK);
    init_pair(8, testing, COLOR_BLACK);
    init_pair(9, py_orange, COLOR_BLACK);
    init_pair(10, py_cyan, COLOR_BLACK);
    init_pair(11, py_purple, COLOR_BLACK);
    init_pair(12, py_green, COLOR_BLACK);
    init_pair(13, ml_gray, COLOR_BLACK);

    return 0;
}

int compile_regex(language lang)
{
    /* for the given lang, cache each *
     * regex expression after compiling it *
     * quit if the regex expressions' are misordered */

    switch (lang) {
        case c:
            GLOBAL_DEMANDS   = C_DEMANDS;
            N_GLOBAL_DEMANDS = N_C_DEMANDS;

            GLOBAL_INDENTABLES = C_INDENTABLES;
            GLOBAL_DEDENTABLES = C_DEDENTABLES;
            GLOBAL_INDENT_LEN  = C_INDENT_LEN;

            GLOBAL_MULTILINE_PAIRS          = C_TWINTERMS;
            N_GLOBAL_MULTILINE_DEMAND_PAIRS = N_C_MULTI_PAIR_DEMANDS;
            break;

        case py:
            GLOBAL_DEMANDS   = PY_DEMANDS;
            N_GLOBAL_DEMANDS = N_PY_DEMANDS;

            GLOBAL_INDENTABLES = PY_INDENTABLES;
            GLOBAL_DEDENTABLES = PY_DEDENTABLES;
            GLOBAL_INDENT_LEN  = PY_INDENT_LEN;

            GLOBAL_MULTILINE_PAIRS          = PY_TWINTERMS;
            N_GLOBAL_MULTILINE_DEMAND_PAIRS = N_PY_TWINTERMS;
            break;

        case blank:
        default:
            GLOBAL_DEMANDS   = BLANK_DEMANDS;
            N_GLOBAL_DEMANDS = N_BLANK_DEMANDS;

            GLOBAL_INDENTABLES = BLANK_INDENTABLES;
            GLOBAL_DEDENTABLES = BLANK_DEDENTABLES;
            GLOBAL_INDENT_LEN  = BLANK_INDENT_LEN;

            GLOBAL_MULTILINE_PAIRS          = BLANK_TWINTERMS;
            N_GLOBAL_MULTILINE_DEMAND_PAIRS = N_BLANK_TWINTERMS;
            break;
    }

    if (!GLOBAL_DEMANDS) return 1;

    CmpOrder previous = NUMERICALS;
    // for each (single line) expression, compile and record status
    for (unsigned int x = 0; x < N_GLOBAL_DEMANDS; x++) {
        // hard stop if mis-orderd expressions
        if (previous > GLOBAL_DEMANDS[x].type) {
            print_err(hlte_src,
                "failed to compile regex_expressions;"
                " their order is corrupt",
                3);
            return 1;
        }
        previous = GLOBAL_DEMANDS[x].type;
        GLOBAL_DEMANDS[x].compiled =
            (regcomp(&GLOBAL_DEMANDS[x].cmp_expression,
                 GLOBAL_DEMANDS[x].expression, REG_EXTENDED) == 0)
            ? 1
            : 0;
    }

    // for each pair of twin expressions,
    // compile each of their init & kill sequences
    for (int p = 0; p < (int)N_GLOBAL_MULTILINE_DEMAND_PAIRS; p++) {
        GLOBAL_MULTILINE_PAIRS[p].ix.compiled =
            (regcomp(&GLOBAL_MULTILINE_PAIRS[p].ix.cmp_expression,
                 GLOBAL_MULTILINE_PAIRS[p].ix.expression, REG_EXTENDED) == 0)
            ? 1
            : 0;
        GLOBAL_MULTILINE_PAIRS[p].kx.compiled =
            (regcomp(&GLOBAL_MULTILINE_PAIRS[p].kx.cmp_expression,
                 GLOBAL_MULTILINE_PAIRS[p].kx.expression, REG_EXTENDED) == 0)
            ? 1
            : 0;
    }
    return 0;
}

void free_reg(void)
{
    if (!GLOBAL_DEMANDS) return;
    for (unsigned int f = 0; f < N_GLOBAL_DEMANDS; f++) {
        if (GLOBAL_DEMANDS[f].compiled) {
            regfree(&GLOBAL_DEMANDS[f].cmp_expression);
        }
    }
    N_GLOBAL_DEMANDS   = 0;
    GLOBAL_DEMANDS     = NULL;
    GLOBAL_INDENTABLES = NULL;
    GLOBAL_DEDENTABLES = NULL;
    for (unsigned int i = 0, n = N_GLOBAL_MULTILINE_DEMAND_PAIRS; i < n; i++) {
        if (GLOBAL_MULTILINE_PAIRS[i].ix.compiled) {
            regfree(&GLOBAL_MULTILINE_PAIRS[i].ix.cmp_expression);
        }
        if (GLOBAL_MULTILINE_PAIRS[i].kx.compiled) {
            regfree(&GLOBAL_MULTILINE_PAIRS[i].kx.cmp_expression);
        }
    }
    GLOBAL_MULTILINE_PAIRS          = NULL;
    N_GLOBAL_MULTILINE_DEMAND_PAIRS = 0;
}
