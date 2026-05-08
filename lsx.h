#ifndef LSX_H
#define LSX_H


// constants
#define MAX_FILENAME 1000
#define POSIX_BLOCK_SIZE 512
#define MALLOC_ERROR "malloc failed to return a valid array"


// global struct for scan results
typedef struct dir_entry
{
    char *f; // path
    int type; // folder/file/symlink/other
    struct dir_entry *next; // next node ^
} dir_entry;

// original pointer :: don't loose it!
dir_entry *dir_entries = NULL;

// struct for dir.entries
struct stat st;

// cwd 
char *buff;


// prototypes

// find/check cwd
int init_fs(void);
// checks if path in buff exist
int exists(void);
// checks if path @ pointer exists
int lli_exists(char *fi[]);

// sym link, file, dir, or other
int ftype(char s[]);
// non-recursive scan + rec
void ls_f(char s[]);

// count each file's bytes 
// (no account for dir.entries atm)
void summ(void);
// alt. block_count * block_sz
void block_summ(void);
// workers
void work_summ(void);
void work_block(void);

// pretty print summ's result
void pretty_print(void); // & here

// free the linked list
void xfree(void);

// manual memory-safe strcat fx
void sf_strcat(char *a, char *o, int bufflen);


#endif
