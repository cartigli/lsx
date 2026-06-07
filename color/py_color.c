#include "py_color.h"

const SyntaxSpan PY_TWINTERMS[] = {
    // triple single & double quoted strings
    {
        .init_rgx = "\"\"\"",
        .kill_rgx = "\"\"\"",
        .attr     = A_NORMAL,
        .color    = YELLOW,
    },
    {
        .init_rgx = "\'\'\'",
        .kill_rgx = "\'\'\'",
        .attr     = A_NORMAL,
        .color    = YELLOW,
    },
};

const SyntaxDemand PY_DEMANDS[] = {
    //  NUMERICALS
    {
        .rgx   = "[[:digit:]]+",
        .color = PY_PURPLE,
        .attr  = A_NORMAL,
    },
    {
        .rgx   = "[[:digit:]]+\\.[[:digit:]]+",
        .color = PY_PURPLE,
        .attr  = A_NORMAL,
    },

    // VARIABLES
    // comparisons; variables evaluated against one another
    {
        .rgx   = "[[:alnum:]_*]*[[:space:]]{0,}[={1,2}"
                 "|<=|<|>=|>][[:space:]]{0,}[[:alnum:]_*\"]*",
        .color = PY_CYAN,
        .attr  = A_NORMAL,
    },

    // FUNCTIONS
    // (keyword style) capture the whole word 'def' + one or more
    // characters that are not whitespace ending when <(> is found
    {
        .rgx   = "(^|[^a-zA-Z_]])def[^a-zA-Z_]([^[:space:]()]+)\\(",
        .color = PY_GREEN,
        .attr  = A_NORMAL,
    },

    // OPERANDS
    {
        .rgx   = "*/\\<>%=^+-",
        .color = REDDSH,
        .attr  = A_NORMAL,
    },

    // IMPORTS
    {
        .rgx   = "(^|[^a-zA-Z_])(import[^a-zA-Z_]+[[:alnum:]]+|$)",
        .color = ORANGE,
        .attr  = A_NORMAL,
    },

    // KEYWORDS
    {
        .rgx   = "(^|[^a-zA-Z_])("
                 "False|True|None|"
                 "and|is|in|not|or"
                 ")([^a-zA-Z_]|$)",
        .color = PINK,
        .attr  = A_NORMAL,
    },
    // CONTROL FLOW
    {
        .rgx   = "(^|[^a-zA-Z_])("
                 "pass|return|yield|break|continue|"
                 "from|for|if|elif|else|while|with|"
                 "try|except|finally|raise"
                 ")([^a-zA-Z_]|$)",
        .color = PINK,
        .attr  = A_NORMAL,
    },
    // SYSTEM & STATE
    {
        .rgx   = "(^|[^a-zA-Z_])("
                 "import|global|nonlocal|"
                 "def|class|lambda|case|"
                 "assert|async|wait"
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
        // double quote
        .rgx   = "\"[^\"]*\"",
        .color = YELLOW,
        .attr  = A_NORMAL,
    },
    {
        // single quote
        .rgx   = "\'[^\']*\'",
        .color = YELLOW,
        .attr  = A_NORMAL,
    },

    {
        // triple-double quote
        .rgx   = "\"{3}[a-zA-Z_][^\"{3}]*\"{3}",
        .color = YELLOW,
        .attr  = A_NORMAL,
    },

    /* SUBSTITUITIONS
     * a double quote followed by 0 or more of anything but a double quote
     * (capture group) a open curly bracket followed by one or more of anything
     * but a closed curly bracket until a closed curly bracket */
    {
        .rgx   = "(\"[^\"]*({[^}]+})[^\"]*\")",
        .color = PY_PURPLE,
        .attr  = A_NORMAL,
    },

    // COMMENTS
    // anywhere a hashtag is until the EOL
    {
        .rgx   = "#[^\n]*",
        .color = LTGRAY,
        .attr  = A_NORMAL,
    },
};

const char *PY_INDENTABLES       = "{[(:";
const char *PY_DEDENTABLES       = "}]))";
const unsigned int PY_INDENT_LEN = 4;

const unsigned int N_PY_DEMANDS = sizeof(PY_DEMANDS) / sizeof(PY_DEMANDS[0]);
const unsigned int N_PY_TWINTERMS =
    sizeof(PY_TWINTERMS) / sizeof(PY_TWINTERMS[0]);

RGXE PY_RT_DEMANDS[sizeof(PY_DEMANDS) / sizeof(PY_DEMANDS[0])];
RGXE PY_RT_MXI_DEMANDS[sizeof(PY_TWINTERMS) / sizeof(PY_TWINTERMS[0])];
RGXE PY_RT_MXK_DEMANDS[sizeof(PY_TWINTERMS) / sizeof(PY_TWINTERMS[0])];
