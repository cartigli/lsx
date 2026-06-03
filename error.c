#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "error.h"

// error message handler

static const char *const ANSI_ESC[ESC_COUNT] = {
    [red] = "\033[31m",
    [ora] = "\033[31;1m",
    [yel] = "\033[33m",
    [gre] = "\033[32m",
    [blu] = "\033[34m",
    [def] = "\033[0m",
};

static const char *const ERR_LVL[LVL_COUNT] = {
    [crit] = "CRITICAL",
    [erro] = "ERROR",
    [warn] = "WARNING",
    [info] = "INFO",
    [dbug] = "DEBUG",
};

// const char *const ERR_NAME[NAME_COUNT] = {
//     [buff_err] = "BUFFER",
//     [memm_err] = "MEMORY",
//     [wind_err] = "WINDOW",
//     [view_err] = "VIEW",
//     [init_err] = "INITIATION",
//     [fsys_err] = "FILESYSTEM",
//     [hlit_err] = "HIGHLIGHT",
//     [blue_err] = "UNFORESEEN",
//     [io_m_err] = "OPERATIONS",
//     [regx_err] = "REGEX",
//     [ileg_err] = "ILLEGAL",
//     [test_err] = "TESTING",
// };

const char *const ERR_SRC[SRC_COUNT] = {
    [buff_src] = "buff.c/.h",
    [fsio_src] = "fsio.c/.h",
    [edit_src] = "editor.c/.h",
    [hlte_src] = "highlight.c/.h",
    [main_src] = "main.c/.h",
    [menu_src] = "menu.c/.h",
    [util_src] = "utils.c/.h",
    [erro_src] = "error.c/.h",
};

// const char *const ERR_MSG[MSG_COUNT] = {
//     [init_msg] = "failed to initiate",
//     [mall_msg] = "failed to allocate new memory for",
//     [open_msg] = "failed to open file",
//     [read_msg] = "error reading file",
//     [edit_msg] = "error editing file",
//     [diro_msg] = "error opening directory",
//     [dirs_msg] = "error scanning directory",
//     [fdir_msg] = "directory error",
//     [oobi_msg] = "out of bounds index",
//     [path_msg] = "failed to get/make path for",
//     [scat_msg] = "safe strcat returned an err while",
//     [comp_msg] = "failed to compile",
//     [null_msg] = "(encountered) NULL/empty",
//     [base_msg] = "failed to",
//     [rule_msg] = "cannot",
//     [perm_msg] = "permission denied while attempting to",
//     [test_msg] = "give a warm welcome to",
// };

void init_logging(void)
{
    /* sets the stderr stream to point to a LOG file, which is *
     * flushed on exit. The file is deleted & remade each run. */

    unlink(LOG_DEST);
    if (freopen(LOG_DEST, "a", stderr) == NULL) return;

    // _IOBLF = line buffered
    // BUFSIZ = sys alloc memory
    setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
}

void flush_logs(err_lvl lvl)
{
    FILE *f = fopen(LOG_DEST, "r");
    if (!f) {
        fprintf(stdout,
            "failed to open %s to"
            " read recorded logs\n",
            LOG_DEST);
        return;
    }

    char *line = NULL;
    size_t eol = 0;
    ssize_t n; // signed so getline can return a negative error

    while ((n = getline(&line, &eol, f)) != -1) {
        for (err_lvl level = lvl; level < LVL_COUNT; level++) {
            if (strstr(line, ERR_LVL[level])) fputs(line, stdout);
        }
    }
    fclose(f);
    free(line);
}

void clockk(char buff[], size_t m)
{
    time_t now     = time(NULL);
    struct tm *utc = gmtime(&now);
    strftime(buff, m, "%H:%M:%S", utc);
}

void print_err(err_src src, const char *const subject, int lvl)
{
    if (src >= SRC_COUNT || src < 0 || lvl > 5 || lvl < 1) return;

    char errb[MAX_ERR_LEN];
    char buff[32];

    clockk(buff, 16);
    precurse(lvl, buff);
    snprintf(errb, sizeof errb, "[%s] %s \n", ERR_SRC[src], subject ? subject : "");
    fprintf(stderr, "%s", errb);
}

void print_inf(err_src src, const char *const subject)
{
    if (src >= SRC_COUNT || src < 0) return;

    char err_msg[MAX_ERR_LEN];
    char buff[32];

    clockk(buff, 16);
    precurse(1, buff);
    snprintf(err_msg, sizeof err_msg, "[%s] %s \n", ERR_SRC[src], subject ? subject : "");
    fprintf(stderr, "%s", err_msg);
}

void precurse(int lvl, char buff[])
{
    fprintf(stderr, " %s ", buff);
    /* assumes the normal terminal is back *
     * (ncurses windows closed & deleted) so generic escape *
     * codes for terminal colors work as per usual */

    switch (lvl) {
        case 1:
            fprintf(stderr, "%sDEBUG", ANSI_ESC[blu]);
            break;
        case 2:
            fprintf(stderr, "%sINFO", ANSI_ESC[gre]);
            break;
        case 3:
            fprintf(stderr, "%sWARNING", ANSI_ESC[yel]);
            break;
        case 4:
            fprintf(stderr, "%sERROR", ANSI_ESC[ora]);
            break;
        case 5:
            fprintf(stderr, "%sCRITICAL", ANSI_ESC[red]);
            break;
        default:
            fprintf(stderr, "UNKNOWN");
            break;
    }
    fprintf(stderr, " %s", ANSI_ESC[def]);
}
