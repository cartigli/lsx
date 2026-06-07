#include "c_color.h"

const SyntaxSpan C_TWINTERMS[] = {
    // /* comments */ && "some string \$
    {
        .init_rgx = "[[:space:]]*/\\*[[:space:]]*",
        .kill_rgx = "[[:space:]]*\\*/[[:space:]]*",
        .attr     = A_NORMAL,
        .color    = LTGRAY,
    },
    {
        .init_rgx = "\"[[:space:][:digit:][:print:]]*\\\\$",
        .kill_rgx = "^[[:space:][:digit:][:print:]]*\";$",
        .attr     = A_NORMAL,
        .color    = YELLOW,
    },
};

// not const anymore because needs to
// gain & hold compiled regex expressions
const SyntaxDemand C_DEMANDS[] = {
    //  NUMERICALS
    {
        .rgx   = "[[:digit:]]+",
        .color = PURPLE,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "[[:digit:]]+\\.[[:digit:]]+",
        .color = PURPLE,
        .attr  = A_NORMAL,
    },

    // VARIABLES
    {
        .rgx   = "(int|float|double|long|short|char|"
                 "void)[[:space:]]+([[:alnum:]_]+)[;,)]",
        .color = CYAN,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "[[:alnum:]_*]*[[:space:]]{0,}[={1,2}"
                 "|<=|<|>=|>][[:space:]]{0,}[[:alnum:]_*\"]*",
        .color = CYAN,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "[[:alnum:]_]*[->][[:alnum:]_]*",
        .color = CYAN,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "(\\[[[:alnum:]]\\]\\.)([[:alnum:]_]*)",
        .color = CYAN,
        .attr  = A_NORMAL,
    },

    // FUNCTIONS
    {
        .rgx   = "([^[:space:]()]+)\\(",
        .color = GREEN,
        .attr  = A_NORMAL,
    },

    // OPERANDS
    {
        .rgx   = "[&*/\\<>%=^+-]",
        .color = REDDSH,
        .attr  = A_NORMAL,
    },

    // PREPROCESSORS & HEADERS
    {
        .rgx   = "#[a-zA-Z_]+",
        .color = PINK,
        .attr  = A_NORMAL,
    },

    {
        .rgx   = "<[^>][[:alnum:]./]*>",
        .color = PINK,
        .attr  = A_NORMAL,
    },

    // KEYWORDS
    {
        .rgx   = "(^|[^a-zA-Z_])("
                 "bool|int|float|double|long|short|"
                 "signed|unsigned|char|void"
                 ")([^a-zA-Z_]|$)",
        .color = PINK,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "(^|[^a-zA-Z_])("
                 "return|if|else|while|"
                 "for|typedef|struct"
                 ")([^a-zA-Z_]|$)",
        .color = PINK,
        .attr  = A_NORMAL,
    },

    // PUNCTUATION
    {
        .rgx   = "[].,!?:;'[{}()]",
        .color = LTGRAY,
        .attr  = A_NORMAL,
    },

    // STRINGS
    {
        .rgx   = "\"([^\"]*)\"",
        .color = YELLOW,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "\'([^\']*)\'",
        .color = YELLOW,
        .attr  = A_NORMAL,
    },
    // SUBSTITUITIONS
    {
        .rgx   = "%[s|i|l|p]",
        .color = PURPLE,
        .attr  = A_NORMAL,
    },

    // COMMENTS
    {
        .rgx   = "/\\*[^\n]*[\\*|\\*/]",
        .color = LTGRAY,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "\\*[[:space:]][^\n]*\\*/",
        .color = LTGRAY,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "^(\\*|[[:space:]]*\\*).+\\*$",
        .color = LTGRAY,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "//[^\n]*",
        .color = LTGRAY,
        .attr  = A_NORMAL,
    },

};

const char *C_INDENTABLES       = "{[(";
const char *C_DEDENTABLES       = "}])";
const unsigned int C_INDENT_LEN = 4;

const unsigned int N_C_DEMANDS   = sizeof(C_DEMANDS) / sizeof(C_DEMANDS[0]);
const unsigned int N_C_TWINTERMS = sizeof(C_TWINTERMS) / sizeof(C_TWINTERMS[0]);

RGXE C_RT_DEMANDS[sizeof(C_DEMANDS) / sizeof(C_DEMANDS[0])];
RGXE C_RT_MXI_DEMANDS[sizeof(C_TWINTERMS) / sizeof(C_TWINTERMS[0])];
RGXE C_RT_MXK_DEMANDS[sizeof(C_TWINTERMS) / sizeof(C_TWINTERMS[0])];
