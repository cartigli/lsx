#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "iofs.h"

struct dirent *ent;

void fls_recursion(FSNode* cd, char *tbuff) {
    cd->n_children = 0;

    int ix = 0;
    DIR *cd_d = opendir(tbuff);
    if (cd_d == NULL) { 
        printf("couldn't open dir\n");
        return;
    }
    while ((ent = readdir(cd_d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }
        cd->n_children++;
    }

    /* count 'parent' str to index & truncate later */
    int xterm_pl = strlen(tbuff);

    cd->children = malloc(cd->n_children * sizeof(FSNode*));

    rewinddir(cd_d);
    if (cd_d == NULL) { return; }
    while ((ent = readdir(cd_d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) { continue; }

        FSNode* entry = malloc(sizeof(FSNode));
        if (entry == NULL) { return; }

        entry->parent = cd; /* point it at the parent */

        int dtype;
        if (ent->d_type == DT_DIR) {
            dtype = 1;
            if (!(cd->n_dirs)) { cd->n_dirs = 1; }
            else { cd->n_dirs++; }
        }
        else if (ent->d_type == DT_REG) {
            dtype = 0;
        } else { dtype = -1; }

        /* make the full path with / + the new item's name */
        strcat(tbuff, "/");
        strcat(tbuff, ent->d_name);

        strcpy(entry->name, ent->d_name);
        entry->is_dir = dtype;

        if (dtype == 0) {
            long blocks = fl_blocks(tbuff);
            entry->blocks = blocks;
        } else if (dtype == 1) {
            fls_recursion(entry, tbuff);
        }

        tbuff[xterm_pl] = '\0'; /* reset buffer to parent's path */
        cd->children[ix] = entry;
        ix++;
    }
}


int df_type(char *dir) {
    struct stat st;
    if (lstat(dir, &st) ==-1) { return -2; }
    if (S_ISLNK(st.st_mode)) { return -1; }
    if (S_ISREG(st.st_mode)) { return 0; }
    if (S_ISDIR(st.st_mode)) { return 1; }
    return -3; /* god forbid */
}

long fl_blocks(char *dir) {
    struct stat st;
    if (stat(dir, &st) == -1) { return -2; }
    return (long)st.st_blocks;
}



void order_rfs(FSNode* cd) {
    if (cd == NULL) { return; }
    int n = cd->n_children;
    for (int i = 0; i < n; i++) { order_fs(cd->children[i]); }
    order_fs(cd);
}

void order_fs(FSNode* cd) {
    if (cd == NULL) { return; }
    if (cd->n_children) {
        int ndirs = 0;
        int n = cd->n_children;
        FSNode** tmp = malloc(sizeof(FSNode*) * n);
        for (int i = 0; i < n; i++) {
            if (cd->children[i]->is_dir) {
                tmp[ndirs] = cd->children[i];
                ndirs++;
            }
        }
        for (int l = 0; l < n; l++) {
            if (!(cd->children[l]->is_dir)) {
                tmp[ndirs] = cd->children[l];
                ndirs++;
            }
        }
        cd->children = tmp;
    }
}

void free_rfs(FSNode* cd) {
    if (cd == NULL) { return; }
    int n = cd->n_children;
    for (int i = 0; i < n; i++) {
        free_rfs(cd->children[i]);
    }
    free_fs(cd);
}

void free_fs(FSNode* cd) {
    if (cd == NULL) { return; }
    if (cd->n_children) { free(cd->children); }
    free(cd);
}

void untraverse(FSNode* cd, char* buff) {
    if (cd->parent != NULL) {
        untraverse(cd->parent, buff);
        strcat(buff, "/");
    }
    strcat(buff, cd->name);
}

int traverse_to(FSNode* cd) {
    if (!cd->is_dir) { return 1; }
    char *path = malloc(MAX_FILENAME);
    path[0] = '\0';

    untraverse(cd, path);
    printf("path: %s\n", path);

    printf("full path: %s\n", path);

    if (chdir(path) == -1) { return 1; }
    free(path);
    return 0;
}