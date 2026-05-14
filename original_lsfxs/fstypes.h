#ifndef FS_TYPES_H
#define FS_TYPES_H

#define MAX_FILENAME 1024

typedef struct FileSystemNode {
    char name[MAX_FILENAME];
    int is_dir;
    long blocks;
    int n_children;
    int n_dirs;
    struct FileSystemNode** children;
    struct FileSystemNode* parent;
} FileSystemNode;

FileSystemNode* cwd;

struct dirent *ent;
struct stat st;

/* LX Menu Prototypes */
void fls_recursion(FileSystemNode* di, char *tbuff);
void menu(FileSystemNode* cdi);

int de_type(char di[]);
int df_type(char di[]);
char *ft_decode(FileSystemNode* dx);
long fl_blocks(char *fd);

void untraverse(FileSystemNode* dd, char* buff);
int traverse_to(FileSystemNode* cd);

int max_strlen(FileSystemNode *cdi, int n_choices);

void order_rfs(FileSystemNode* ld);
void order_fs(FileSystemNode* dd);
void free_rfs(FileSystemNode* xd);
void free_fs(FileSystemNode* fd);

/* File View Prototypes */
int read_from(FileSystemNode* ff);
int view_file(char *path);

#endif