#include <stdio.h>

#include "error.h"
#include "utils.h"

int sf_strcat(char *a, const char *o, int bufflen) {
    if (!a || !o) {
        print_err(util_src, "unexpected NULL buffer[s] passed to safe strcat", 3);
        return 1;
    }
    int c = 0;
    int lim = bufflen - 1;

    /* skip past the used values of *a */
    while (c < bufflen && *(a + c)) { c++; }

    if (c >= lim) {
        if (*o != '\0') { return 0; }
        print_err(util_src, "out of bounds index in safe strcat; bufflen too small", 3);
        return 1;
    }

    while (*o) {
        if ( c >= lim - 1) {
            /* bad strcat, kill it to save crash & return error */
            *(a + c) = '\0';
            print_err(util_src, "failed to safely concatenate strings; corrupted buffer", 4);
            return 1;
        }
        *(a + c) = *o; /* copy o to a's free values one byte at a time */
        c++;
        o++;
    }
    *(a + c) = '\0'; /* add a terminator (room from bufflen - 1) */
    return 0;
}


void usage(void) {
    char *use = "lx (ls+) usage:\n"
    "Usage:\n"
    "lx                    run menu on current directory\n"
    "lx /path/to/dir       run menu on directory at /path\n"
    "lx /path/to/file.txt  run editor on file at /path (no menu)\n"
    "\n"
    "Options:\n"
    "no_size, -ns          don't show the file's sizes\n"
    "no_edit, -ne          don't allow files to be edited (read mode only)\n"
    "verbose, silent       set the logging to max or none, respectively\n"
    "-v<level>             set the logging verbosity to an int from...\n"
    "                                     ...1 [DEBUG] to 5 [CRITICAL]\n"
    "\n"
    "Notes:\n"
    "A: lx /path/to/dir & lx /path/to/file are the same command;\n"
    "the action to do next is determined by the type of the\n"
    "resulting st_type from the path (i.e., its automatic).\n"
    "B: if passing a path as an argument, it must be the last\n"
    "argument. All other args' or flags' ordering is unimportant.\n"
    "\n"
    "max args: [ 4 + a path ]\n"
    "ex: lx -ns -ne verbose /path/to/dir (or file)\n";
    fprintf(stderr, "%s", use);
}

// typedef struct {
//     char indent_chars[32];
//     char dedent_chars[32];
//     int mutable;
//     int hide_size;
//     int verbosity;
// } BaseConfig;


// BaseConfig *initialize_config(void) {
//     BaseConfig *bs = NULL;
//     bs = calloc(1, sizeof(BaseConfig));
//     if (!bs) {
//         print_err(util_Src, "failed to allocate memory for the BaseConfig", 4);
//         return NULL;
//     }

//     bs->indent_chars = NULL;
//     bs->dedent_chars = NULL;

//     bs->mutable = 0;
//     bs->hide_size = 0;
//     bs->verbosity = 0;

//     return bs;
// }


// // void load_base_config(Config *config, const char *path) {
// BaseConfig *load_config(const char *path, BaseConfig bs) {
//     FILE *f = fopen(path);
//     if (!f) {
//         print_err(util_src, "failed to open config file", 4);
//         return;
//     }

//     BaseConfig *bs;
//     bs = initialize_config();
//     if (!bs) { return NULL; }

//     char line[256];
//     while (fgets(line), sizeof(line)) {
//         /* skip empty or commented lines */
//         if (line[0] == '#' || line[0] == '\n') { continue; }

//         char key[32], val[32];
//         /* read up to 31 chars into key until a '=', then read *
//          * up to 31 chars into val until a new line char is found */
//         if (sscanf(line, "%31[^=]=%31[^\n]", key, val) == 2) {
//             if (strcmp(key, "indent_chars") == 0) {
//                 bs->indent_chars = val;
//             } else if (strcmp(key, "dedent_chars") == 0) {
//                 bs->dedent_chars = val;
//             } else if (strcmp(key, "mutable") == 0) {
//                 bs->mutable = val;
//             } else if (strcmp(key, "hide_size") == 0) {
//                 bs->hide_size = val;
//             } else if (strcmp(keu, "verbosity") == 0) {
//                 bs->verbosity = val;
//             }
//         }
//     }