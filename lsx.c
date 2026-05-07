#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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


// globals vars
// size counter
long long sx = 0;
// list entries (default)
int list = 1;
// byte or block ticker
int blocks = 1;
// recursive ls indicator
int flat = 1;


// prototypes

// find/check cwd
int init_fs(void);
// checks if path in buff exist
int exists(void);
// checks if path @ pointer exists
int lli_exists(char *fi[]);

// sym link, file, dir, or other
int ftype(char s[]);
// recursive scan + record
// void ls_c(char s[]);
// non-recursive scan + rec
void ls_f(char s[]);

// count each file's bytes 
// no account for dir.entries atm
void summ(void); // just grab globals
// alt. block_count * block_sz
// long long block_summ(void); // ditto here
void block_summ(void);
// workers
void work_summ(void);
void work_block(void);

// pretty print summ's result
void pretty_print(void); // & here

// free the linked list
void xfree(void);




int main(int argc, char *argv[])
{
    if (argc > 4)
    {
        printf("too many args\n");
        return 1;
    }

    buff = malloc(MAX_FILENAME);
    if (buff == NULL)
    {
        return 1;
    }

    if (!(init_fs()))
    {
        // if NOT a succesful fs init:
        printf("failed to find cwd\n");
        return 1;
    }

    // arg assesment & validation
    if (argc > 1) // 2-4 args
    {
        // disk usage route
        if (strcmp(argv[1], "du") == 0)
        {
            list = 0;
            if (argc == 3)
            // lx du /path/to
            {
                if (strcmp(argv[2], "bytes") == 0)
                {
                    blocks = 0;
                }
                else
                {
                    strcat(buff, argv[2]);
                    // if (stat(buff, &st) != 0) // AB
                    // {
                    //     printf("err, does not exist: %s\n", buff);
                    //     return 1;
                    // }
                    if (!(exists()))
                    {
                        return 1;
                    }
                }
            }
            else if (argc == 4)
            {
                // blocks or bytes
                if (strcmp(argv[2], "bytes") == 0)
                {
                    // [ CMD ] lx du bytes /path
                    blocks = 0;
                }
                else if (strcmp(argv[2], "blocks") != 0)
                {
                    // only other acceptable (albeit useless)
                    // option/flag is blocks, but its the default
                    printf("incorrect use of arguments\n");
                    return 1;
                }
                strcat(buff, argv[3]);
                // if (stat(buff, &st) != 0) // AB
                // {
                //     printf("err, does not exist: %s\n", buff);
                //     return 1;
                // }
                if (!(exists()))
                {
                    return 1;
                }
                // alt. is default
            }
        }
        else
        {
            // list route
            if (argc == 3)
            {
                if (strlen(argv[1]) == 1 && argv[1][0] == 'r')
                {
                    printf("three args provided incorrectly for list\n");
                    return 1;
                }
                // recursive : true
                flat = 0;
                // concatenate the new targ
                strcat(buff, argv[2]);
                // check if it exists
                // if (stat(buff, &st) != 0) // AB
                // {
                //     printf("err, does not exist: %s\n", buff);
                //     return 1;
                // }
                if (!(exists()))
                {
                    return 1;
                }
            }
            else if (argc == 2)
            {
                // othewise, if argcv[1] is r
                if (strlen(argv[1]) == 1 && argv[1][0] == 'r')
                {
                    // enable recursive
                    flat = 0;
                }
                // if argv[1] isn't 'r', try the path
                else
                {
                    strcat(buff, argv[1]);
                    // if (stat(buff, &st) != 0) // AB
                    // {
                    //     printf("err, does not exist: %s\n", buff);
                    //     return 1;
                    // }
                    if (!(exists()))
                    {
                        return 1;
                    }
                }
            }
            else if (argc == 4)
            {
                free(buff);
                printf("no list functions w.four args\n");
                return 1;
            }
        }
    }
    // else, use cwd

    // process
    // HERE till EO(block) moved to
    // void cwd_processor(void)
    if (ftype(buff) == 1)
    {
        if (list)
        {
            // ls route
            ls_f(buff);
            // if (flat)
            // {
            //     // non-recursive
            //     ls_f(buff);
            // }
            // else
            // {
            //     // recursive
            //     ls_c(buff);
            // }
            // make a copy of the pointer
            dir_entry *entries = dir_entries;
            while (entries != NULL)
            {
                printf("%s\n", entries->f);
                entries = entries->next;
            }
        }
        else
        {
            // du route
            flat = 0;
            ls_f(buff); // recursive always
            if (blocks)
            {
                // mimic du's sizing strat
                block_summ();
            }
            else
            {
                // else, bytes
                summ();
            }
            pretty_print();
        }
        // either way, always xfree()
        // xfree();
    }
    else if (ftype(buff) == 2)
    {
        printf("cwd: %s (type: file)\n", buff);
        dir_entry *single = malloc(MAX_FILENAME);
        if (single == NULL)
        {
            printf("%s\n", MALLOC_ERROR);
            return 1;
        }
        // single item linked list lol
        single->f = buff;
        single->type = 2;
        single->next= dir_entries;
        dir_entries = single;
        if (blocks)
        {
            block_summ();
        }
        else
        {
            summ();
        }
        pretty_print();
    }
    else
    {
        printf("cwd: %s (type: NaN)\n", buff);
    }
    xfree();
    return 0;
}


// void ls_c(char s[])
// {
//     char *s_tmp = malloc(sizeof(char) * MAX_FILENAME);
//     if (s_tmp == NULL) {
//         printf("%s\n", MALLOC_ERROR);
//         return;
//     }
//     struct dirent *ent;
//     strcpy(s_tmp, s);

//     DIR *dir = opendir (s_tmp);
//     char *fdupe;

//     if (dir != NULL) {
//         while ((ent = readdir (dir)) != NULL) {
//             char *new_dupe = malloc(sizeof(char) * (strlen(s) + MAX_FILENAME));
//             if (new_dupe == NULL) {
//                 printf("%s\n", MALLOC_ERROR);
//                 return;
//             }

//             char *pdir = strcpy(new_dupe, s_tmp);
//             // if it doesn't end with an '/':
//             // append one ONCE before the loop
//             if (strcmp(&pdir[strlen(pdir) - 1], "/") != 0) {
//                 strcat(pdir, "/");
//             }

//             fdupe = strdup(ent->d_name);
//             if (fdupe != NULL) {
//                 if (strcmp(fdupe, ".") != 0 && strcmp(fdupe, "..") != 0) {
//                     dir_entry *entry = malloc(sizeof(dir_entry));
//                     if (entry == NULL) {
//                         printf("%s\n", MALLOC_ERROR);
//                         return;
//                     }

//                     // concatenate the dir.entry & parent
//                     strcat(pdir, fdupe);
//                     // check the type (drop symlinks)
//                     int ft = ftype(pdir);
//                     if (ft == 20) {
//                         // + free fdupe when symlink
//                         free(fdupe);
//                         continue;
//                     }

//                     entry->f = pdir;
//                     entry->type = ft;
//                     entry->next = dir_entries;

//                     dir_entries = entry;
//                     if (ft == 1) {
//                         ls_c(pdir);
//                     }
//                 }
//             }
//             else {
//                 printf("%s\n", MALLOC_ERROR);
//                 return;
//             }
//         }
//         (void) closedir (dir);
//     }
//     else {
//         printf("couldn't open (ls_c) %s\n", s);
//         return;
//     }
//     free(s_tmp);
// }


int init_fs (void)
{
    if (getcwd(buff, MAX_FILENAME) == NULL)
    {
        printf("could not find/open the cwd");
        return 0; // false
    }
    strcat(buff, "/");
    return 1; // true
}


int exists(void)
{
    if (stat(buff, &st) != 0)
    {
        printf("err, does not exist: %s\n", buff);
        return 0; // false
    }
    return 1; // true
}


int lli_exists(char *fi[])
{
    if (stat(*fi, &st) != 0)
    {
        printf("err, does not exist: %s\n", *fi);
        return 0;
    }
    return 1;
}


void ls_f(char s[])
{
    char *s_tmp = malloc(sizeof(char) * MAX_FILENAME);
    if (s_tmp == NULL) {
        printf("%s\n", MALLOC_ERROR);
        free(buff);
        return;
    }
    struct dirent *ent;
    strcpy(s_tmp, s);

    DIR *dir = opendir (s_tmp);
    char *fdupe;

    if (dir != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            char *pdir = malloc(sizeof(char) * (strlen(s) + MAX_FILENAME));
            if (pdir == NULL) {
                printf("%s\n", MALLOC_ERROR);
                return;
            }
            // char *pdir = strcpy(new_dupe, s_tmp);
            pdir = strcpy(pdir, s_tmp);

            // if it doesn't end with an '/':
            // append one ONCE before the loop
            if (strcmp(&pdir[strlen(pdir) - 1], "/") != 0) {
                strcat(pdir, "/");
            }

            fdupe = strdup(ent->d_name);
            if (fdupe != NULL) {
                if (strcmp(fdupe, ".") != 0 && strcmp(fdupe, "..") != 0) {
                    dir_entry *entry = malloc(sizeof(dir_entry));
                    if (entry == NULL) {
                        free(pdir);
                        free(fdupe);
                        printf("%s\n", MALLOC_ERROR);
                        return;
                    }

                    // concatenate the dir.entry & parent
                    strcat(pdir, fdupe);
                    // check the type (drop symlinks)
                    int ft = ftype(pdir);
                    if (ft == 20) {
                        // + free fdupe when sym
                        free(entry);
                        free(fdupe);
                        free(pdir);
                        continue;
                    }

                    entry->f = pdir;
                    entry->type = ft;
                    entry->next = dir_entries;

                    dir_entries = entry;
                    if (!(flat))
                    {
                        if (ft == 1)
                        {
                            ls_f(pdir);
                            // free(pdir);?
                        }
                    }
                    free(fdupe);
                }
                else
                {
                    free(pdir);
                    free(fdupe);
                }
            }
            else
            {
                printf("%s\n", MALLOC_ERROR);
                return;
            }
            // free(pdir);
            // free(fdupe); // don't think that's it
        }
        (void) closedir (dir);
    }
    else {
        printf("couldn't open (ls_f) %s\n", s);
        return;
    }
    free(s_tmp);
}


int ftype(char s[])
{
    if (lstat(s, &st) == -1) {
        // can't open | doesn't exist | permissions
        return 0;
    }
    if (S_ISLNK(st.st_mode)) {
        // symlink
        return 20;
    }
    if (S_ISREG(st.st_mode)) {
        // 'reg' file
        return 2;
    }
    if (S_ISDIR(st.st_mode)) {
        // directory
        return 1;
    }
    return 0;
}


void summ(void)
{
    // don't loose the pointer !
    dir_entry *sz_tmp = dir_entries;

    while (sz_tmp != NULL)
    {
        if (!(lli_exists(&(sz_tmp->f))))
        {
            return;
        }

        if (sz_tmp->type == 2)
        {
            work_summ();
        }

        // }
        // if (stat(sz_tmp->f, &st) == 0) // AB
        // {
        // else
        // {
        //     printf("(block summ) could not open %s\n", sz_tmp->f);
        // }
        sz_tmp = sz_tmp->next;
    }
}


void work_summ(void)
{
    sx = sx + (long long)st.st_size;
}



// long long block_summ(void)
void block_summ(void)
{
    // don't loose the pointer !
    dir_entry *sz_tmp = dir_entries;

    while (sz_tmp != NULL)
    {
        if (!(lli_exists(&(sz_tmp->f))))
        {
            return;
        }

        if (sz_tmp->type == 2)
        {
            work_block();
        }

        // }
        // if (stat(sz_tmp->f, &st) == 0) // AB
        // {
        // else
        // {
        //     printf("(block summ) could not open %s\n", sz_tmp->f);
        // }
        sz_tmp = sz_tmp->next;
    }
    // return sx * POSIX_BLOCK_SIZE;
    sx = sx * POSIX_BLOCK_SIZE;
}


void work_block(void)
{
    sx = sx + (long long)st.st_blocks;
}


void pretty_print(void)
{
    printf("raw sx: %lld\n", sx);
    int i = 0;
    long long lim = 1024;
    while (sx > lim)
    {
        sx = sx / 1024;
        i++;
    }
    char *vt;

    if (i == 1)
    {
        vt = "kb";
    }
    else if (i == 2)
    {
        vt = "mb";
    }
    else if (i == 3)
    {
        vt = "gb";
    }
    else if (i == 4)
    {
        vt = "tb";
    }
    else
    {
        if (blocks)
        {
            vt = "blocks";
        }
        else
        {
            vt = "bytes";
        }
    }
    printf("  size on disk: %lld %s\n", sx, vt);
}


void xfree(void)
{
    while (dir_entries != NULL)
    {
        dir_entry *temp = dir_entries;
        dir_entries = dir_entries->next;
        free(temp->f);
        free(temp);
    }
    free(dir_entries);
    free(buff);
}
