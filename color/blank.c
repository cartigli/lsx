#include "blank.h"


SyntaxTwins BLANK_TWINTERMS[] = {
    {
        .ix = { .expression = "", .color_code = LTGRAY, .type = NUMERICALS, },
        .kx = { .expression = "", .color_code = LTGRAY, .type = NUMERICALS, },
    },
};


SyntaxDemands BLANK_DEMANDS[] = {
    /*  FAKES */
    {
        .expression = "",
        .color_code = PURPLE, 
        .type = NUMERICALS
    }
};

const char *BLANK_INDENTABLES = { "{[(" };
const char *BLANK_DEDENTABLES = { "}])" };
int BLANK_INDENT_LEN = 4;

const unsigned int N_BLANK_DEMANDS = 1;

const unsigned int N_BLANK_TWINTERMS = 1;