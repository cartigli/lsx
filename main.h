#ifndef MAIN_H
#define MAIN_H

#include "config.h"
#include "editor.h"
#include "menu.h"
#include "types.h"


enum args {
    nsl,
    nss,
    mtl,
    mts,
    ver,
    sil,
    ARG_COUNT
};


const char *const ARGS[ARG_COUNT] = {
    [nsl] = "hide_size",
    [nss] = "-sz",
    [mtl] = "immutable",
    [mts] = "-im",
    [ver] = "verbose",
    [sil] = "silent",
};


int main(int argc, char *argv[]);

void run(Config *config);

int spinup_window(void);


void menu_runner(Config *config);

MGMT *menu_init(Config *config);

MGMT *new_management(void);

Mstate *new_MS(void);
void populate_MS(Mstate *ms, FSNode *cd, int hide_size);


/* main editor managing function for editing files */
void pretty_runner(const char path[], int mutable, const char *indent_chars, const char *dedent_chars);

Cursor *initialize_cursor(const char *indent_chars, const char *dedent_chars);
RunTime *init_rt_vars(Buffer *b);

/* frees all allocated components + delets & erases window */
void teardown_editor(Buffer *b, RunTime *rt, Cursor *curs);


#endif