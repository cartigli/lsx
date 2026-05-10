#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_FILENAME 1024

typedef struct FileSystemNode {
    char name[MAX_FILENAME];
    int is_dir;
    long blocks;
    int n_children;
    struct FileSystemNode** children;
} FileSystemNode;

FileSystemNode* cwd;

struct dirent *ent;
struct stat st;

int de_type(char di[]);
char *ft_decode(FileSystemNode* dx);

int main() {
    /* make the cwd's entry */
    cwd = malloc(sizeof(FileSystemNode));
    if (cwd == NULL) { return 1; }

    if (getcwd(cwd->name, sizeof(FileSystemNode)) == NULL) { return 1; }

    DIR *cwdD = opendir(cwd->name);
    if (cwdD == NULL) { return 1; }

    int children = 0;
    while ((ent = readdir(cwdD)) != NULL) { children++; }

    cwd->n_children = children;
    cwd->children = malloc(sizeof(FileSystemNode*) * children);

    int ie = 0;
    DIR *cwdI = opendir(cwd->name);
    while ((ent = readdir(cwdI)) != NULL) {
        FileSystemNode* entry = malloc(sizeof(FileSystemNode));
        strcpy(entry->name, ent->d_name);
        entry->is_dir = de_type(ent->d_name);
        cwd->children[ie] = entry;
        ie++;
    }

    printf("indexed: %s\n", cwd->name);

    for (int i = 0, n = cwd->n_children; i < n; i++) {
        // if (cwd->children[i]->is_dir) { printf("[ %s ]", "dir"); }
        // else if ((cwd->children[i]->is_dir) == 0) { printf("[ %s ]", "file"); }
        printf("[ %s ] ", ft_decode(cwd->children[i]));
        printf("entry %i: %s\n", i, cwd->children[i]->name);
    }

    free(cwd);
    return 0;
}

int de_type(char di[]) {
    char *s = malloc(MAX_FILENAME);
    strcpy(s, cwd->name);
    strcat(s, "/");
    strcat(s, di);
    if (lstat(s, &st) ==-1) { return -2; }
    if (S_ISLNK(st.st_mode)) { return -1; }
    if (S_ISREG(st.st_mode)) { return 0; }
    if (S_ISDIR(st.st_mode)) { return 1; }
    return -3; /* god forbid */
}

char *ft_decode(FileSystemNode* dx) {
    int is_dir = dx->is_dir;
    if (is_dir) { return "dir "; }
    else if (is_dir == 0) { return "file"; }
    return "NaN";
}