#include "c_color.h"


SyntaxTwins PY_TWINTERMS[] = {
    {
        .ix = {
            .expression = "\"\"\"",
            .color_code = YELLOW,
            .type = NUMERICALS,
        },
        .kx = {
            .expression = "\"\"\"",
            .color_code = YELLOW,
            .type = NUMERICALS,
        },
    },
    {
        .ix = {
            .expression = "\'\'\'",
            .color_code = YELLOW,
            .type = NUMERICALS,
        },
        .kx = {
            .expression = "\'\'\'",
            .color_code = YELLOW,
            .type = NUMERICALS,
        },
    },
};


SyntaxDemands PY_DEMANDS[] = {
    /*  NUMERICALS */
    {
        .expression = "[[:digit:]]+",
        .color_code = PY_PURPLE, 
        .type = NUMERICALS
    },
    {
        .expression = "[[:digit:]]+\\.[[:digit:]]+",
        .color_code = PY_PURPLE,
        .type = NUMERICALS
    },

    /* VARIABLES */
    /* comparisons; variables evaluated against one another */
    {
        .expression = "[[:alnum:]_*]*[[:space:]]{0,}[={1,2}"
                    "|<=|<|>=|>][[:space:]]{0,}[[:alnum:]_*\"]*",
        .color_code = PY_CYAN,
        .type = VARIABLES
    },

    /* FUNCTIONS
     * (keyword style) capture the whole word 'def' + one or more
     *characters that are not whitespace ending when ( is found */
    {
        .expression = "(^|[^a-zA-Z_]])def[^a-zA-Z_]([^[:space:]()]+)\\(",
        .color_code = PY_GREEN,
        .type = FUNCTIONS
    },

    /* OPERANDS */
    {
        .expression = "*/\\<>%=^+-",
        .color_code = REDDSH,
        .type = OPERANDS
    },

    /* IMPORTS */
    {
        .expression = "(^|[^a-zA-Z_])(import[^a-zA-Z_]+[[:alnum:]]+|$)",
        .color_code = ORANGE,
        .type = PREPROCS
    },

    /* KEYWORDS */
    {
        .expression = "(^|[^a-zA-Z_])("
                    "False|True|None|"
                    "and|is|in|not|or" //|pass|return|yield|"
                    // "break|continue"
                    ")([^a-zA-Z_]|$)",
        .color_code = PINK,
        .type = KEYWORDS
    },
    /* CONTROL FLOW */
    {
        .expression = "(^|[^a-zA-Z_])("
                    "pass|return|yield|break|continue|"
                    "from|for|if|elif|else|while|with|"
                    "try|except|finally|raise"
                    ")([^a-zA-Z_]|$)",
        .color_code = PINK,
        .type = KEYWORDS
    },
    /* SYSTEM & STATE */
    {
        .expression = "(^|[^a-zA-Z_])("
                    "import|global|nonlocal|"
                    "def|class|lambda|case|"
                    "assert|async|wait"
                    ")([^a-zA-Z_]|$)",
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
    { /* double quote */
        .expression = "\"[^\"]*\"",
        .color_code = YELLOW,
        .type = STRINGS
    },
    { /* single quote */
        .expression = "\'[^\']*\'",
        .color_code = YELLOW,
        .type = STRINGS
    },

    { /* triple-double quote */
        // .expression = "(\"\"\"[^\"\"\"]*\"\"\")",
        .expression = "\"{3}[a-zA-Z_][^\"{3}]*\"{3}",
        .color_code = YELLOW,
        .type = STRINGS
    },


    /* SUBSTITUITIONS */
    /* a double quote followed by 0 or more of anything but a double quote *'
     * (capture group) a open curly bracket followed by one or more of anything *
     * but a closed curly bracket until a closed curly bracket */
    {
        .expression = "(\"[^\"]*({[^}]+})[^\"]*\")",
        .color_code = PY_PURPLE,
        .type = SUBSTITUTES
    },
    
    /* COMMENTS *
     * anywhere a hashtag is until the EOL */
    {
        .expression = "#[^\n]*",
        .color_code = LTGRAY,
        .type = COMMENTS
    },
};


const char *PY_INDENTABLES = { "{[(:" };
const char *PY_DEDENTABLES = { "}])" };
int PY_INDENT_LEN = 4;


const unsigned int N_PY_DEMANDS = sizeof(PY_DEMANDS) / sizeof(PY_DEMANDS[0]);

const unsigned int N_PY_TWINTERMS =
            sizeof(PY_TWINTERMS) / sizeof(PY_TWINTERMS[0]);