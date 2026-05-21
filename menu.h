#ifndef MENU_H
#define MENU_H

#include "types.h"


/* menu of indexed filesystem entries */
void menu(MGMT *mgmt);

/* allows calling view file from FSNode instance */
int read_from(FSNode* ff);

/* function to read the contents of a file */
int view_file(char *path);


#endif