#include "blank.h"

//  DUMMIES
const SyntaxSpan BLANK_TWINTERMS[] = {
    {
        .init_rgx = "",
        .kill_rgx = "",
        .attr     = A_NORMAL,
        .color    = OFFWHITE,
    },
};

const SyntaxDemand BLANK_DEMANDS[] = {
    {
        .rgx   = "",
        .color = OFFWHITE,
        .attr  = A_NORMAL,
    },
};

const char *BLANK_INDENTABLES       = "([";
const char *BLANK_DEDENTABLES       = ")]";
const unsigned int BLANK_INDENT_LEN = 4;

const unsigned int N_BLANK_DEMANDS =
    sizeof(BLANK_DEMANDS) / sizeof(BLANK_DEMANDS[0]);
const unsigned int N_BLANK_TWINTERMS =
    sizeof(BLANK_TWINTERMS) / sizeof(BLANK_TWINTERMS[0]);

RGXE BLANK_RT_DEMANDS[sizeof(BLANK_DEMANDS) / sizeof(BLANK_DEMANDS[0])];
RGXE BLANK_RT_MXI_DEMANDS[sizeof(BLANK_TWINTERMS) / sizeof(BLANK_TWINTERMS[0])];
RGXE BLANK_RT_MXK_DEMANDS[sizeof(BLANK_TWINTERMS) / sizeof(BLANK_TWINTERMS[0])];
