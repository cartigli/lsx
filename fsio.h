#ifndef FSIO_H
#define FSIO_H

// POSIZ byte size of a block on disk
#define ST_BLOCK_SIZE 512
// maximum chars in a filename's buffer
#define MAX_FILENAME 1024

/* self-referencing struct for *
 * recording all filesystem entries found *
 * note: the root's parents are NULL *
 * so traversal above it is bounded */
typedef struct FSNode {
    int is_dir;
    long blocks;
    int n_children;
    int n_dirs;
    char name[MAX_FILENAME];
    struct FSNode **children;
    struct FSNode *parent;
} FSNode;

// recursive filesystem indexing, records findings in FSNode's
int fls_recurse(FSNode *cd, char *tbuff);

// returns type as symlink, file, or dir
int df_type(const char *dir);
long fl_blocks(char *dir);

// build the full path from a given FSNode instance
int untraverse(FSNode *cd, char buff[]);
int traverse_to(FSNode *cd);

// orders dirs before files (recursively)
void order_rfs(FSNode *cd);
void order_fs(FSNode *cd);

// find the largest recorded block size
int max_rblocks(FSNode *cd);
long max_blocks(FSNode *cd);

// free allocated memory
void free_rfs(FSNode *cd);

#endif
