#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fsio.h"
#include "config.h"
#include "error.h"


const char *DEFAULTS[DF_COUNT] = {
    [mutable_df]    = "1", /* mutable state : 1 mutable, 0 immutable*/
    [hide_size_df]  = "1", /* hide size : 1 no sizes, 0 show sizes */
    [verbosity_df]  = "1", /* verbosity : 1 = DEBUG, 5 = CRITICAL */
    [indent_len_df] = "4", /* length of a indent instance (usually 4 or 5) */
};


void initialize_config(Config *config) {
    config->root = NULL;
    config->root = calloc(1, MAX_FILENAME);
    if (!(config->root)) {
        print_err(util_src, "failed to allocate memory for the root path", 4);
        return;
    }

    config->indent_chars = DEFAULTS[indent_chars_df];
    config->dedent_chars = DEFAULTS[dedent_chars_df];

    config->mutable = atoi(DEFAULTS[mutable_df]);
    config->hide_size = atoi(DEFAULTS[hide_size_df]);

    config->colors_loaded = 0;
}
