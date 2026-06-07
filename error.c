#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "error.h"
#include "fsio.h"
#include "utils.h"

// error message handler

char log_local[MAX_FILENAME] = {0};

static const char *const ERR_LVL[LVL_COUNT] = {
    [dbug] = "DEBUG",
    [info] = "INFO",
    [warn] = "WARNING",
    [erro] = "ERROR",
    [crit] = "CRITICAL",
};

int init_logging(char *log_dest)
{
    /* sets the stderr stream to point to a LOG file, which is *
     * flushed on exit. The file is deleted & remade each run. */

    if (build_osp(log_dest)) return 1;

    unlink(log_dest);
    if (freopen(log_dest, "a", stderr) == NULL) return 1;

    setvbuf(stderr, NULL, _IOLBF, BUFSIZ);

    return 0;
}

int build_osp(char log_dest[])
{
    // build path by os

    char file_local[MAX_FILENAME];

#if defined(_WIN32)
return 1;

#elif defined(__unix__) || defined(__linux__)
ssize_t len =
    readlink("/proc/self/exe", file_local, sizeof(file_local) - 1);
if (len == -1) return 1;

#elif defined(__APPLE__) && defined(__MACH__)
uint32_t size = sizeof(file_local);
if (_NSGetExecutablePath(file_local, &size) != 0) {
    return 1;
}

#else
    return 1;
#endif

    char *dir = dirname(file_local);
    if (dir == NULL) fprintf(stdout, "failed to find dirname");

    strcpy(log_dest, dir);
    sf_strcat(log_dest, "/lx.log", MAX_FILENAME);

    return 0;
}

void flush_logs(char log_dest[], log_level_t lvl)
{
    if (lvl < 0 || lvl >= LVL_COUNT) return;

    FILE *f = fopen(log_dest, "r");
    if (!f) {
        fprintf(stdout,
            "failed to open %s to"
            " read recorded logs\n",
            log_dest);
        return;
    }

    char *line = NULL;
    size_t eol = 0;
    ssize_t n;

    while ((n = getline(&line, &eol, f)) != -1) {
        for (log_level_t level = lvl; level < LVL_COUNT; level++) {
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

static const char *level_str(log_level_t lvl)
{
    switch (lvl) {
        case dbug: return "\033[34mDEBUG\033[0m";
        case info: return "\033[32mINFO\033[0m";
        case warn: return "\033[33mWARNING\033[0m";
        case erro: return "\033[31;1mERROR\033[0m";
        case crit: return "\033[31mCRITICAL\033[0m";
        default: return "?";
    }
}

void inter_log(log_level_t lvl, const char *file, const char *function, int line, const char *fmt, ...)
{
    if (lvl < 0 || lvl >= LVL_COUNT) return;

    char when[16];
    clockk(when, sizeof when);

    char err_msg[MAX_MSG_LEN];
    va_list ap;        // initiate
    va_start(ap, fmt); // last named arg
    vsnprintf(err_msg, sizeof err_msg, fmt, ap);
    va_end(ap); // cleanup

    char err_log[MAX_LOG_LEN];
    snprintf(err_log, sizeof err_log, "%s %s [%s:%d in %s()]: %s\n",
        when, level_str(lvl), file, line, function, err_msg);

    fprintf(stderr, "%s", err_log);
}
