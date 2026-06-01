#ifndef ERROR_H
#define ERROR_H

#include "types.h"

typedef enum { red, ora, yel, gre, blu, def, ESC_COUNT } color_esc;

// Note: the logging levels enum'd below have 5 levels; 0, 1, 2, 3, 4.
// however, they are indexed by integers 1 - 5 when calling the print_err
// function. this makes the calling much clearer, and allows the silent
// flag to completely silence logging, since it will require an enum'd
// level 5, which does not exist.
typedef enum { dbug, info, warn, erro, crit, LVL_COUNT } err_lvl;

typedef enum {
    buff_err,
    memm_err,
    wind_err,
    view_err,
    init_err,
    fsys_err,
    hlit_err,
    blue_err,
    io_m_err,
    regx_err,
    ileg_err,
    test_err,
    NAME_COUNT
} err_name;

typedef enum {
    buff_src,
    fsio_src,
    edit_src,
    hlte_src,
    main_src,
    menu_src,
    util_src,
    erro_src,
    SRC_COUNT
} err_src;

typedef enum {
    init_msg,
    mall_msg,
    open_msg,
    read_msg,
    edit_msg,
    diro_msg,
    dirs_msg,
    fdir_msg,
    oobi_msg,
    path_msg,
    scat_msg,
    comp_msg,
    null_msg,
    base_msg,
    rule_msg,
    perm_msg,
    test_msg,
    MSG_COUNT
} err_msg;

extern const char *const ERR_NAME[NAME_COUNT];

extern const char *const ERR_SRC[SRC_COUNT];

extern const char *const ERR_MSG[MSG_COUNT];

/* redirect stderr output to a .log file */
void init_logging(void);

// flush the recorded messages to the terminal
// **after** ncurses windows are closed & deleted
void flush_logs(err_lvl lvl);

// writes the current time
void clockk(char *buff, size_t m);

void print_err(err_src src, const char *const subject, int lvl);

void print_inf(err_src src, const char *const subject);

void precurse(int lvl, char buff[]);

#endif
