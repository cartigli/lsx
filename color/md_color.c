#include "md_color.h"

const SyntaxSpan MD_TWINTERMS[] = {
    {
        .init_rgx = "\\*",
        .kill_rgx = "\\*",
        .attr     = A_ITALIC,
        .color    = OFFWHITE,
    },
    {
        .init_rgx = "\\*\\*",
        .kill_rgx = "\\*\\*",
        .attr     = A_BOLD,
        .color    = OFFWHITE,
    },
};

const SyntaxDemand MD_DEMANDS[] = {
    {
        .rgx   = "",
        .color = OFFWHITE,
        .attr  = A_NORMAL,
    },
};

const char *MD_INDENTABLES       = "([";
const char *MD_DEDENTABLES       = ")]";
const unsigned int MD_INDENT_LEN = 4;

const unsigned int N_MD_DEMANDS = sizeof(MD_DEMANDS) / sizeof(MD_DEMANDS[0]);
const unsigned int N_MD_TWINTERMS =
    sizeof(MD_TWINTERMS) / sizeof(MD_TWINTERMS[0]);

RGXE MD_RT_DEMANDS[sizeof(MD_DEMANDS) / sizeof(MD_DEMANDS[0])];
RGXE MD_RT_MXI_DEMANDS[sizeof(MD_TWINTERMS) / sizeof(MD_TWINTERMS[0])];
RGXE MD_RT_MXK_DEMANDS[sizeof(MD_TWINTERMS) / sizeof(MD_TWINTERMS[0])];
