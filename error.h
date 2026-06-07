#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#define MAX_MSG_LEN 512
#define MAX_LOG_LEN 1024

typedef enum { POSIX, MACOS, WIN, NaN, OS_COUNT } os_tp;

typedef enum { blu = 0, gre, yel, ora, red, ESC_COUNT } color_esc;

typedef enum { dbug = 0, info, warn, erro, crit, LVL_COUNT } log_level_t;

// macro wrapper
#define LOG_DEBUG(...) inter_log(dbug, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  inter_log(info, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  inter_log(warn, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_ERRO(...)  inter_log(erro, __FILE__, __func__, __LINE__, __VA_ARGS__)
#define LOG_CRIT(...)  inter_log(crit, __FILE__, __func__, __LINE__, __VA_ARGS__)

// macro-wrapped error logger
void inter_log(log_level_t lvl, const char *file, const char *function, int line, const char *fmt, ...);

// extern const char *const ERR_SRC[SRC_COUNT];

// redirect stderr output to a .log file
int init_logging(char log_dest[]);

// build path for log file from file's current directory
int build_osp(char log_dest[]);
os_tp os_f(void);

// flush the recorded messages to the terminal
// **after** ncurses windows are closed & deleted
void flush_logs(char *log_dest, log_level_t lvl);

void clockk(char *buff, size_t m);

#endif
