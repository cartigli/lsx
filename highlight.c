#include "highlight.h"


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
        .expression = "[[:digit:]]*",
        .color_code = PURPLE, 
        .type = NUMERICALS
    },
    {
        .expression = "[[:digit:]]*\\.[[:digit:]]*",
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