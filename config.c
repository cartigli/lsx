#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "error.h"
#include "fsio.h"

const char *DEFAULTS[DF_COUNT] = {
    [verbosity_df]  = "0", // verbosity : 0 = DEBUG, 4 = CRITICAL
    [mutable_df]    = "1", // mutable : 1 mutable, 0 immutable
    [hide_size_df]  = "1", // sizes : 1 no sizes, 0 show sizes
    [indent_len_df] = "4", // length of a indent instance
};

void initialize_config(Config *config)
{
    config->root = NULL;
    config->root = calloc(1, MAX_FILENAME);
    if (!(config->root)) {
        print_err(util_src, "failed to allocate memory for the root path", 4);
        return;
    }

    config->indent_chars = DEFAULTS[indent_chars_df];
    config->dedent_chars = DEFAULTS[dedent_chars_df];

    config->mutable   = atoi(DEFAULTS[mutable_df]);
    config->hide_size = atoi(DEFAULTS[hide_size_df]);

    config->verbosity = atoi(DEFAULTS[verbosity_df]);

    config->colors_loaded = 0;
}
