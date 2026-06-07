#include <stdio.h>

#include "error.h"
#include "utils.h"

char *s = "this is a multi \
    line string";

int sf_strcat(char *a, const char *o, int bufflen)
{
    if (!a || !o) {
        LOG_ERRO("unexpected NULL buffer[s] passed");
        return 1;
    }
    int c   = 0;
    int lim = bufflen - 1;

    // skip past the used values of *a
    while (c < bufflen && *(a + c)) c++;

    if (c >= lim) {
        if (*o != '\0') return 0;
        LOG_ERRO("OUT_OF_BOUNDS index; bufflen too small");
        return 1;
    }

    while (*o) {
        // bad strcat, kill it to save crash & return error
        if (c >= lim - 1) {
            *(a + c) = '\0';
            LOG_ERRO("failed to safelt concatenate; buffer corrupted");
            return 1;
        }
        // copy o to a's free values one byte at a time
        *(a + c) = *o;
        c++;
        o++;
    }
    // add a terminator (room from bufflen - 1)
    *(a + c) = '\0';
    return 0;
}

void usage(void)
{
    char *use =
        "lx (ls+) usage:\n"
        "Usage:\n"
        "lx                    run menu on current directory\n"
        "lx /path/to/dir       run menu on directory at /path\n"
        "lx /path/to/file.txt  run editor on file at /path (no menu)\n"
        "\n"
        "Options:\n"
        "sizes, -sz            don't show the file's sizes\n"
        "immutable, -im        don't allow files to be edited (read mode "
        "only)\n"
        "verbose, silent       set the logging to max or none, respectively\n"
        "-v[n]                 set the logging verbosity to [n];"
        " 0 = DEBUG, 4 = CRITICAL\n"
        "\n"
        "notes:\n"
        "A: lx /path/to/dir & lx /path/to/file are the same command;\n"
        "the action to do next is determined by the type of the\n"
        "resulting st_type from the path (i.e., its automatic).\n"
        "B: if passing a path as an argument, it must be the last\n"
        "argument. All other args' or flags' ordering is unimportant.\n"
        "\n";
    fprintf(stdout, "%s", use);
}
