#ifndef FSIO_H
#define FSIO_H

#include "types.h"


/* recursive filesystem index-r, records findings in FSNode's */
int fls_recurse(FSNode* cd, char *tbuff);

/* returns type as symlink, file, or dir */
int df_type(const char *dir);
long fl_blocks(char *dir);

/* build the full path from a given FSNode instance */
// void untraverse(FSNode* cd, char buff[]);
int untraverse(FSNode *cd, char buff[]);
int traverse_to(FSNode* cd);

/* orders dirs before files (recursively) */
void order_rfs(FSNode* cd);
void order_fs(FSNode*  cd);

/* find the largest recorded block size */
int max_rblocks(FSNode* cd);
long max_blocks(FSNode* cd);

/* free allocated memory */
void free_rfs(FSNode*  cd);


#endif