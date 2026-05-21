#include <ncurses.h>
#include <stdio.h>

#include "highlight.h"
#include "error.h"


/* RGB codes for color scheme (hexadecimal format) */

const ColorCode COLOR_CODES[] = {
    { "f6", "aa", "d3" }, /* pink    */
    { "23", "ad", "61" }, /* green   */
    { "c3", "91", "ed" }, /* purple  */
    { "87", "ce", "eb" }, /* cyan    */
    { "b3", "b3", "b3" }, /* gray    */
    { "fa", "f1", "87" }, /* yellow  */
    { "e6", "67", "6b" }, /* reddish */
    { "27", "f5", "be" }  /* teal    */
};


/* not const anymore because needs to gain & hold compiled regex expressions */

SyntaxDemands DEMANDS[] = {
    /*  NUMERICALS */
    {
        // .expression = "[[:digit:]]*",
        .expression = "[[:digit:]]+",
        .color_code = PURPLE, 
        .type = NUMERICALS
    },
    {
        // .expression = "[[:digit:]]*\\.[[:digit:]]*",
        .expression = "[[:digit:]]+\\.[[:digit:]]+",
        .color_code = PURPLE,
        .type = NUMERICALS
    },

    /* VARIABLES */
    {
        .expression = "(int|float|double|long|short|char|"
                    "void)[[:space:]]+([[:alnum:]_]+)[;,)]",
        .color_code = CYAN,
        .type = VARIABLES
    },
    {
        .expression = "[[:alnum:]_*]*[[:space:]]{0,}[={1,2}"
                    "|<=|<|>=|>][[:space:]]{0,}[[:alnum:]_*\"]*",
        .color_code = CYAN,
        .type = VARIABLES
    },
    {
        .expression = "[[:alnum:]_]*[->][[:alnum:]_]*",
        .color_code = CYAN,
        .type = VARIABLES
    },
    {
        .expression = "(\\[[[:alnum:]]\\]\\.)([[:alnum:]_]*)",
        .color_code = CYAN,
        .type = VARIABLES
    },

    /* FUNCTIONS */
    {
        .expression = "([^[:space:]()]+)\\(",
        .color_code = GREEN,
        .type = FUNCTIONS
    },

    /* OPERANDS */
    {
        .expression = "[&*/\\<>%=^+-]",
        .color_code = REDDSH,
        .type = OPERANDS
    },

    /* PREPROCESSORS & HEADERS */
    {
        .expression = "#[a-zA-Z_]+",
        .color_code = PINK,
        .type = PREPROCS
    },

    {
        .expression = "<[^>][[:alnum:]./]*>",
        .color_code = PINK,
        .type = HEADERS
    },

    /* KEYWORDS */
    {
        .expression = "(^|[^a-zA-Z_])(int|float|double|"
                    "unsigned|const|long|short|char|NULL|void)"
                    "([^a-zA-Z_]|$)",
        .color_code = PINK,
        .type = KEYWORDS
    },
    {
        .expression = "(^|[^a-zA-Z_])(return|if|else|while|"
                    "for|typedef|struct)([^a-zA-Z_]|$)",
        .color_code = PINK,
        .type = KEYWORDS
    },

    /* PUNCTUATION */
    {
        .expression = "[].,!?:;'[{}()]",
        .color_code = LTGRAY,
        .type = PUNCTUATION
    },

    /* STRINGS */
    {
        .expression = "\"([^\"]*)\"",
        .color_code = YELLOW,
        .type = STRINGS
    },
    {
        .expression = "\'([^\']*)\'",
        .color_code = YELLOW,
        .type = STRINGS
    },
    /* SUBSTITUITIONS */
    {
        .expression = "%[s|i|l|p]",
        .color_code = PURPLE,
        .type = SUBSTITUTES
    },
    
    /* COMMENTS */
    {
        .expression = "/\\*[^\n]*[\\*|\\*/]",
        .color_code = LTGRAY,
        .type = COMMENTS
    },
    {
        .expression = "\\*[[:space:]][^\n]*\\*/",
        .color_code = LTGRAY,
        .type = COMMENTS
    },
    {
        .expression = "^(\\*|[[:space:]]*\\*).+\\*$",
        .color_code = LTGRAY,
        .type = COMMENTS
    },
    {
        .expression = "//[^\n]*",
        .color_code = LTGRAY,
        .type = COMMENTS
    }

};


const unsigned int N_DEMANDS = sizeof(DEMANDS) / sizeof(DEMANDS[0]);



int load_colors(void) {
    if (!has_colors()) {
        print_err(hlte_src, "terminal does not support colors", 3);
        return 1;
    }
    start_color();

    /* args: (int: pair_no, fg color, bg color) */
    short keys_npres = 14;
    short functions = 15;
    short ints_ndecs = 16;
    short declr_vars = 17;
    short comments = 18;
    short strings = 19;
    short operands = 20;
    short testing = 21;

    init_color(
        keys_npres, 
        hex_compr(COLOR_CODES[0].r),
        hex_compr(COLOR_CODES[0].g),
        hex_compr(COLOR_CODES[0].b)
    );
    init_color(
        functions,
        hex_compr(COLOR_CODES[1].r),
        hex_compr(COLOR_CODES[1].g),
        hex_compr(COLOR_CODES[1].b)
    );
    init_color(
        ints_ndecs,
        hex_compr(COLOR_CODES[2].r),
        hex_compr(COLOR_CODES[2].g),
        hex_compr(COLOR_CODES[2].b)
    );
    init_color(
        declr_vars,
        hex_compr(COLOR_CODES[3].r),
        hex_compr(COLOR_CODES[3].g),
        hex_compr(COLOR_CODES[3].b)
    );
    init_color(comments,
        hex_compr(COLOR_CODES[4].r),
        hex_compr(COLOR_CODES[4].g),
        hex_compr(COLOR_CODES[4].b)
    );
    init_color(strings,
        hex_compr(COLOR_CODES[5].r),
        hex_compr(COLOR_CODES[5].g),
        hex_compr(COLOR_CODES[5].b)
    );
    init_color(operands,
        hex_compr(COLOR_CODES[6].r),
        hex_compr(COLOR_CODES[6].g),
        hex_compr(COLOR_CODES[6].b)
    );
    init_color(testing,
        hex_compr(COLOR_CODES[7].r),
        hex_compr(COLOR_CODES[7].g),
        hex_compr(COLOR_CODES[7].b)
    );
    init_pair(1, keys_npres, COLOR_BLACK);
    init_pair(2,  functions, COLOR_BLACK);
    init_pair(3, ints_ndecs, COLOR_BLACK);
    init_pair(4, declr_vars, COLOR_BLACK);
    init_pair(5,   comments, COLOR_BLACK);
    init_pair(6,    strings, COLOR_BLACK);
    init_pair(7,   operands, COLOR_BLACK);
    init_pair(8,    testing, COLOR_BLACK);

    return 0;
}


int hex_compr(const char hex[]) {
    int ccode;
    sscanf(hex, "%x", &ccode);
    return (int)((ccode * 1000) / 255.0 );
}


int compile_regex(void) {
    /* for every demand, compile the RegEx expression & cache the result *
     * additionally, enforce the order with a check before proceeding */
    CmpOrder previous = NUMERICALS;
    for (unsigned int x = 0; x < N_DEMANDS; x++) {
        /* hard stop if mis-orderd expressions */
        if (previous > DEMANDS[x].type) {
            print_err(hlte_src, "failed to compile regex_expressions;"
                        " their order is likely incorrect", 3);
            return 1;
        }
        previous = DEMANDS[x].type;
        DEMANDS[x].compiled = (regcomp(&DEMANDS[x].cmp_expression,
                    DEMANDS[x].expression, REG_EXTENDED) == 0) ? 1 : 0;
    }
    return 0;
}


void free_reg(void) {
    for (unsigned int f = 0; f < N_DEMANDS; f++) {
        if (DEMANDS[f].compiled) {
            regfree(&DEMANDS[f].cmp_expression);
        }
    }
}
