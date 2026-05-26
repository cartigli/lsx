#include "blank.h"


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
int BLANK_INDENT_LENGTH = 4;


const unsigned int BLANK_N_DEMANDS = 1;
