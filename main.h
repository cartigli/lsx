#ifndef MAIN_H
#define MAIN_H

#include "config.h"
#include "editor.h"
#include "menu.h"
#include "types.h"


static int arg_parse(int argc, char *argv[], Config *config);

static int arg_parse_path(char *argv, Config *config);

int main(int argc, char *argv[]);

void run_win(Config *config);


void menu_runner(Config *config);

void menu_init(MGMT *mgmt, Mstate *ms, Config *config);

void new_management(MGMT *mgmt);

void new_MS(Mstate *ms);

void populate_MS(Mstate *ms, FSNode *cd, int hide_size, int block_size);


/* main editor managing function for editing files */
void pretty_runner(Config *config, const char path[], int mutable);

void initialize_cursor(Cursor *curs, Config *config);
void init_rt_vars(RunTime *rt, Buffer *b);

language detect_lang(const char path[]);


#endif