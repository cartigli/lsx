#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fsio.h"
#include "config.h"
#include "error.h"


const char *DEFAULTS[DF_COUNT] = {
    [mutable_df]   = "1", /* mutable state : 1 mutable, 0 immutable*/
    [hide_size_df] = "1", /* hide size : 1 no sizes, 0 show sizes */
    [verbosity_df] = "1", /* verbosity : 1 = DEBUG, 5 = CRITICAL */

    [indent_chars_df] = "[{(", /* chars that cause an indent at EOL */
    [dedent_chars_df] = "]})", /* chars that cause a dedent at EOL */
};


Config *initialize_config(void) {
    Config *config = NULL;
    config = calloc(1, sizeof(Config));
    if (!config) {
        print_err(util_src, "failed to allocate memory for the Config", 4);
        return NULL;
    }
    config->root = NULL;
    config->root = calloc(1, MAX_FILENAME);
    if (!(config->root)) {
        print_err(util_src, "failed to allocate memory for the root path", 4);
        free(config);
        return NULL;
    }

    config->indent_chars = DEFAULTS[indent_chars_df];
    config->dedent_chars = DEFAULTS[dedent_chars_df];

    config->mutable = atoi(DEFAULTS[mutable_df]);
    config->hide_size = atoi(DEFAULTS[hide_size_df]);
    config->verbosity = atoi(DEFAULTS[verbosity_df]);

    return config;
}


// Config *load_config(const char *path) {
//     FILE *f = fopen(path, "r");
//     if (!f) {
//         print_err(util_src, "failed to open config file", 4);
//         return NULL;
//     }

//     Config *config;
//     config = initialize_config();
//     if (!config) { return NULL; }

//     char line[256];
//     while (fgets(line, sizeof(line), f)) {
//         /* skip empty or commented lines */
//         if (line[0] == '#' || line[0] == '\n') { continue; }

//         char key[32], val[32];
//         /* read up to 31 chars into key until a '=', then read *
//          * up to 31 chars into val until a new line char is found */
//         if (sscanf(line, "%31[^=]=%31[^\n]", key, val) == 2) {
//             if (strcmp(key, "indent_chars") == 0) {
//                 config->indent_chars = val;
//             } else if (strcmp(key, "dedent_chars") == 0) {
//                 config->dedent_chars = val;
//             } else if (strcmp(key, "mutable") == 0) {
//                 config->mutable = atoi(&val[0]);
//             } else if (strcmp(key, "hide_size") == 0) {
//                 config->hide_size = atoi(&val[0]);
//             } else if (strcmp(key, "verbosity") == 0) {
//                 config->verbosity = atoi(&val[0]);
//             }
//         }
//     }
//     return config;
// }
