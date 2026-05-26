#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "error.h"
#include "fsio.h"
#include "utils.h"


/* if there is a failure inside fls_recurse, the previously        *
 * allocated children or FSNodes are leaked on return. on error,   *
 * it should recursively free the entry currently being built,     *
 * then set the number of children to the currently indexed count  *
 * before propogating up. That way, the next fls_recurse call only *
 * cleans up ix children from the parent node.                     */
int fls_recurse(FSNode* cd, char *buff) {
    struct dirent *ent;
    DIR *dir = opendir(buff);
    if (dir == NULL) { 
        print_err(fsio_src, "failed to open directory while recursing", 5);
        return 1;
    }

    cd->n_children = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 ||
                    strcmp(ent->d_name, "..") == 0) { continue; }
        cd->n_children++;
    }

    /* count 'parent' str to index & truncate later */
    int xterm_pl = strlen(buff);

    /* if the directory is empty, ensure nothing is processed */
    if (cd->n_children <= 0) { cd->children = NULL; }
    else {
        cd->children = calloc(cd->n_children, sizeof(FSNode*));
        if (!cd->children) {
            print_err(fsio_src, "failed to allocate memory"
                        " for a directories entries' child nodes", 1);
            closedir(dir);
            return 1;
        }
    }

    int ix = 0;
    rewinddir(dir);
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 ||
                    strcmp(ent->d_name, "..") == 0) { continue; }

        FSNode* entry = calloc(1, sizeof(FSNode));
        if (entry == NULL) {
            print_err(fsio_src, "failed to allocate memory"
                        " for a FSNode instance while recursing", 4);
            closedir(dir);
            cd->n_children = ix;
            return 1;
        }

        /* point it at the parent */
        entry->parent = cd;

        int dtype;
        if (ent->d_type == DT_DIR) {
            dtype = 1;
        } else if (ent->d_type == DT_REG) {
            dtype = 0;
        } else {
            free_rfs(entry);
            cd->n_children--;
            continue;
        }

        if (sf_strcat(buff, "/", MAX_FILENAME) ||
                        sf_strcat(buff, ent->d_name, MAX_FILENAME)) {
            print_err(fsio_src, "safe cat returned an error from"
                        " concatenating a path while recursing", 4);
            buff[xterm_pl] = '\0'; /* reset corrupted buffer */
            closedir(dir);
            free_rfs(entry);
            cd->n_children = ix;
            return 1;
        }

        strcpy(entry->name, ent->d_name);
        entry->is_dir = dtype;

        if (dtype == 0) {
            long blocks = fl_blocks(buff);
            if (blocks == -2) {
                print_err(fsio_src, "failed to find blocks used"
                            " for a file on disk while recursing", 2);
                buff[xterm_pl] = '\0';
                free_rfs(entry);
                cd->n_children--;
                continue;
            }
            entry->blocks = blocks;
        } else if (dtype == 1) { /* printing an error here would be + 1 *
            * prints for every recursion | propogate the error up instead */
            if (fls_recurse(entry, buff)) {
                closedir(dir);
                free_rfs(entry);
                cd->n_children = ix;
                return 1;
            }
        }
        
        if (dtype == 1) { cd->n_dirs++; }
        
        cd->children[ix] = entry;
        ix++;

        /* reset buffer to parent's path */
        buff[xterm_pl] = '\0';
    }
    if (closedir(dir) == -1) {
        print_err(fsio_src, "failed to close directory"
                    " safely while recursing", 3);
    }
    return 0;
}


int df_type(const char *path) {
    struct stat st;
    if (lstat(path, &st) == -1) { return -2; } /* permission denied / doesn't exist */
    if (S_ISLNK(st.st_mode))    { return -1; } /* symlink */
    if (S_ISREG(st.st_mode))    { return  0; }  /* file */
    if (S_ISDIR(st.st_mode))    { return  1; }  /* directory */
    print_err(fsio_src, "failed to check or get the types"
                " of an entry; unkown type", 3);
    return -3; /* god forbid - unkown type */ 
}


long fl_blocks(char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        print_err(fsio_src, "failed to get the disk usage"
                    " of a file; unkown disk usage", 3);
        return -2;
    }
    return (long)st.st_blocks;
}


void order_rfs(FSNode* cd) {
    if (cd == NULL) { return; }
    for (int i = 0; i < cd->n_children; i++) {
        order_fs(cd->children[i]);
    }
    order_fs(cd);
}


void order_fs(FSNode* cd) {
    if (cd == NULL) { return; }
    if (cd->n_children) {
        int idx = 0;
        FSNode** tmp = calloc(1, sizeof(FSNode*) * cd->n_children);
        if (!tmp) {
            print_err(fsio_src, "failed to allocate memory"
                        " for tempory sorting FSNode", 4);
            return;
        }
        for (int i = 0; i < cd->n_children; i++) {
            if (cd->children[i]->is_dir) {
                tmp[idx] = cd->children[i];
                idx++;
            }
        }
        for (int l = 0; l < cd->n_children; l++) {
            if (!(cd->children[l]->is_dir)) {
                tmp[idx] = cd->children[l];
                idx++;
            }
        }
        free(cd->children);
        cd->children = tmp;
    }
}


int max_rblocks(FSNode* cd) {
    if (cd == NULL) {
        print_err(fsio_src, "unexpected NULL *cd passed"
                    " to max_rblocks", 3);
        return 0;
    }
    long max = 0;
    long allmax = 0;
    for (int i = 0; i < cd->n_children; i++) {
        max = max_blocks(cd->children[i]);

        if (allmax < max) { allmax = max; }
        max = 0;
    }
    max = max_blocks(cd);
    if (allmax < max) { allmax = max; }

    if (allmax == 0) {
        print_err(fsio_src, "failed to find any valid"
                    " disk use (at all)", 3);
        return 0;
    }
    int cushion = 0;
    while (allmax != 0) {
        allmax /= 10;
        cushion++;
    }

    return cushion;
}


long max_blocks(FSNode* cd) {
    if (cd == NULL) { return 0; }
    long max = 0;
    long allmax = 0;
    if (cd->n_children) {
        for (int i = 0; i < cd->n_children; i++) {
            max = cd->children[i]->blocks * ST_BLOCK_SIZE;
            if (allmax < max) { allmax = max; }
            max = 0;
        }
    }
    return allmax;
}


void free_rfs(FSNode* cd) {
    if (cd == NULL) { return; }
    for (int i = 0; i < cd->n_children; i++) {
        free_rfs(cd->children[i]);
    }
    free(cd->children);
    free(cd);
}


// void untraverse(FSNode* cd, char buff[]) {
//     if (cd->parent != NULL) {
//         untraverse(cd->parent, buff);
//         sf_strcat(buff, "/", MAX_FILENAME);
//     }
//     sf_strcat(buff, cd->name, MAX_FILENAME);
// }


int untraverse(FSNode* cd, char buff[]) {
    if (cd->parent != NULL) {
        if (untraverse(cd->parent, buff)) { return 1; }
        if (sf_strcat(buff, "/", MAX_FILENAME)) { return 1; }
    }
    if (sf_strcat(buff, cd->name, MAX_FILENAME)) { return 1; }
    return 0;
}


// int untraverse(FSNode *cd, char buff[]) {
//     if (cd->parent == NULL) { return 0; }
//     if (untraverse(cd->parent, buff)) { return 1; }
//     if (sf_strcat(buff, "/", MAX_FILENAME)) { return 1; }
//     if (sf_strcat(buff, cd->name, MAX_FILENAME)) { return 1; }
//     return 0;
// }