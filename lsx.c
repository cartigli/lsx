#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_FILENAME 1000
#define MALLOC_ERROR "malloc failed to return a valid array"
#define POSIX_BLOCK_SIZE 512
#define CHAR_BYTE_SIZE sizeof(char)


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
char cwd[MAX_FILENAME];

// buffer for d_name's
// const char *buff[MAX_FILENAME];
char *buff; // = malloc(MAX_FILENAME);

// globals vars
// size counter
long long s = 0;
// list entries (default)
int list = 1;
// target needed bool
int abuff = 1;


// prototypes
// sym link, file, dir, or other
int ftype(char s[]);
// recursive scan + record
void ls_c(char s[]);
// non-recursive scan + rec
void ls_f(char s[]);
// find/check cwd
int init_fs(void);
// count each file's bytes 
// no account for dir.entries atm
long long summ(void);
// alt. block_count * block_sz
long long block_summ(void);
// pretty print summ's result
void format_sz(long long x);
// free the linked list
void xfree(void);



int main(int argc, char *argv[])
{
    // argument handling
    if (argc > 3)
    {
        printf("[OPTIONS] du:/path; lx ~/path (default path: cwd)\n");
        return 1;
    }

    buff = malloc(MAX_FILENAME);
    if (buff == NULL)
    {
        return 1;
    }
    printf("buff allocated\n");

    if (!(init_fs()))
    {
        printf("failed to find cwd\n");
        return 1;
    }
    printf("found cwd\n");

    if (argc > 1)
    {
        printf("found two args\n");
        // du routes
        if (strcmp(argv[1], "du") != 0)
        {
            printf("chose a new target: %s\n", argv[1]);
            list = 0;
            if (argc > 2)
            {
                printf("concatenating buff & argv[2]...\n");
                printf("buff: %s, argv[2]: %s\n", buff, argv[2]);
                strcat(buff, argv[2]);
                printf("buff (after concat): %s\n", buff);
                if (stat(buff, &st) != 0)
                {
                    printf("err, does not exist: %s\n", buff);
                    return 1;
                }
                else
                {
                    printf("buff: %s exists, abuff = 0\n", buff);
                    abuff = 0;
                }
            }
            // else
            // {
            //     printf("du used; turning list off\n");
            //     list = 0;
            // }
        }
        // else just assume its a path & handle error / doesn't exist
        // also assume buff is valid
        else
        {
            printf("chose: du, buff: %s\n", buff);
            // strcat(buff, argv[2]);
            if (argc > 2)
            {
                strcat(buff, argv[2]);
            }
            if (stat(buff, &st) != 0)
            {
                // return if doesn't exist
                printf("err, does not exist: %s\n", buff);
                return 1;
            }
            else
            {
                // otherwise, turn target needed off
                abuff = 0;
                // and turn list off
                list = 0;
            }
        }
    }

    // // argument handling
    // if (argc > 3)
    // {
    //     printf("[OPTIONS] du:/path; lx ~/path (default path: cwd)\n");
    //     return 1;
    // }
    // // wait until args are verified
    // char buff[MAX_FILENAME];
    // if (argc > 1)
    // {
    //     if (strcmp(argv[1], "du") == 0)
    //     {
    //         list = 0;
    //     }
    //     else if (argv[1][0] == 'd' && argv[1][1] == 'u' && argv[1][2] == ':')
    //     {
    //         for (int i = 3, n = strlen(argv[1]); i < n; i++)
    //         {
    //             strcpy(&buff[i-3], &argv[1][i]);
    //         }
    //         printf("buff: %s\n", buff);
    //         list = 0;
    //         abuff = 0;
    //     }
    //     else
    //     {
    //         // if the ssecond arg doesn't have a leading /:
    //         if (argv[1][0] == '/')
    //         {
    //             // get the cwd
    //             if (getcwd(buff, sizeof(buff)) == NULL)
    //             {
    //                 return 1;
    //             }
    //             // append '/' and then append the second arg
    //             strcat(strcat(buff, "/"), argv[1]);
    //             // if it doesn't exist, inform & exit
    //             if (stat(buff, &st) != 0)
    //             {
    //                 printf("%s\n", "[OPTIONS]: du = disk use, /path/to = alt. dir\n");
    //                 return 1;
    //             }
    //             else
    //             {
    //                 // otherwise, set abuff to no target needed
    //                 abuff = 0;
    //             }
    //         }
    //         else
    //         {
    //             // struct stat path_strat;
    //             if (stat(buff, &st) != 0)
    //             {
    //                 printf("%s does not exist\n", buff);
    //                 return 1;
    //             }
    //             strcpy(buff, argv[1]);
    //             abuff = 0;
    //         }
    //     }
    // }
    // // end arg validation & handling

    if (ftype(buff) == 1)
    {
        if (list)
        {
            ls_f(buff); // non-recursive
            // make a copy of the pointer therein
            dir_entry *entries = dir_entries;
            while (entries != NULL)
            {
                printf("%s\n", entries->f);
                entries = entries->next;
            }
        }
        else
        {
            // if not list, then du
            ls_c(buff); // recursive
            long long total = block_summ();
            // float ptotal = (float)total;
            printf("bytes: %.1f\n", (float)total);
            format_sz(total);
        }
        xfree();
        return 0;
    }
    else if (ftype(buff) == 2)
    {
        printf("cwd: %s (type: file)\n", buff);
    }
    else 
    {
        printf("cwd: %s (type: NaN)\n", buff);
    }
    return 0;
}


void ls_c(char s[])
{
    char *s_tmp = malloc(sizeof(char) * MAX_FILENAME);
    if (s_tmp == NULL) {
        printf("%s\n", MALLOC_ERROR);
        return;
    }
    struct dirent *ent;
    strcpy(s_tmp, s);

    DIR *dir = opendir (s_tmp);
    char *fdupe;

    if (dir != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            char *new_dupe = malloc(sizeof(char) * (strlen(s) + MAX_FILENAME));
            if (new_dupe == NULL) {
                printf("%s\n", MALLOC_ERROR);
                return;
            }

            char *pdir = strcpy(new_dupe, s_tmp);
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
                        printf("%s\n", MALLOC_ERROR);
                        return;
                    }

                    // concatenate the dir.entry & parent
                    strcat(pdir, fdupe);
                    // check the type (drop symlinks)
                    int ft = ftype(pdir);
                    if (ft == 20) {
                        // + free fdupe when sym
                        free(fdupe);
                        continue;
                    }

                    entry->f = pdir;
                    entry->type = ft;
                    entry->next = dir_entries;

                    dir_entries = entry;
                    if (ft == 1) {
                        ls_c(pdir);
                    }
                }
            }
            else {
                printf("%s\n", MALLOC_ERROR);
                return;
            }
        }
        (void) closedir (dir);
    }
    else {
        printf("couldn't open (ls_c) %s\n", s);
        return;
    }
    free(s_tmp);
}


// check exists and write cwd to XVARX
int init_fs (void)
{
    if (getcwd(buff, MAX_FILENAME) == NULL)
    {
        printf("could not find/open the cwd");
        return 0;
        // return char *cwd = "00";
    }
    // cwd = strcat(buff, "/");
    strcat(buff, "/");
    // return cwd;
    return 1;
}


void ls_f(char s[])
{
    char *s_tmp = malloc(sizeof(char) * MAX_FILENAME);
    if (s_tmp == NULL) {
        printf("%s\n", MALLOC_ERROR);
        return;
    }
    struct dirent *ent;
    strcpy(s_tmp, s);

    DIR *dir = opendir (s_tmp);
    char *fdupe;

    if (dir != NULL) {
        while ((ent = readdir (dir)) != NULL) {
            char *new_dupe = malloc(sizeof(char) * (strlen(s) + MAX_FILENAME));
            if (new_dupe == NULL) {
                printf("%s\n", MALLOC_ERROR);
                return;
            }
            char *pdir = strcpy(new_dupe, s_tmp);

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
                        printf("%s\n", MALLOC_ERROR);
                        return;
                    }

                    // concatenate the dir.entry & parent
                    strcat(pdir, fdupe);
                    // check the type (drop symlinks)
                    int ft = ftype(pdir);
                    if (ft == 20) {
                        // + free fdupe when sym
                        free(fdupe);
                        continue;
                    }

                    entry->f = pdir;
                    entry->type = ft;
                    entry->next = dir_entries;

                    dir_entries = entry;
                }
            }
            else {
                printf("%s\n", MALLOC_ERROR);
                return;
            }
        }
        (void) closedir (dir);
    }
    else {
        printf("couldn't open (ls_c) %s\n", s);
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
        return 20;
    }

    if (S_ISREG(st.st_mode)) {
        return 2;
    }
    if (S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}


long long summ(void)
{
    // don't loose the pointer !
    dir_entry *sz_tmp = dir_entries;

    while (sz_tmp != NULL)
    {
        if (stat(sz_tmp->f, &st) == 0)
        {
            if (sz_tmp->type == 1)
            {
                s = s + 40;
            }
            else if (sz_tmp->type == 2)
            {
                s = s + (long long)st.st_size;
            }
        }
        else
        {
            printf("couldn't open (summ) %s\n", sz_tmp->f);
        }
        sz_tmp = sz_tmp->next;
    }
    return s;
}


long long block_summ(void)
{
    // don't loose the pointer !
    dir_entry *sz_tmp = dir_entries;

    while (sz_tmp != NULL)
    {
        if (stat(sz_tmp->f, &st) == 0)
        {
            if (sz_tmp->type == 1)
            {
                s = s + 40;
            }
            else if (sz_tmp->type == 2)
            {
                s = s + (long long)st.st_blocks;
            }
        }
        else
        {
            printf("couldn't open (summ) %s\n", sz_tmp->f);
        }
        sz_tmp = sz_tmp->next;
    }
    return s * POSIX_BLOCK_SIZE;
}


void format_sz(long long x)
{
    int i = 0;
    long long lim = 1024;
    while (x > lim)
    {
        x = x / 1024;
        i++;
    }
    
    // char *vt = malloc(sizeof(char) * 4);
    char *vt;

    if (i == 1)
    {
        // printf("total: %lld %s\n", x, "kb");
        vt = "kb";
    }
    else if (i == 2)
    {
        // printf("total: %lld %s\n", x, "mb");
        vt = "mb";
    }
    else if (i == 3)
    {
        // printf("total: %lld %s\n", x, "gb");
        vt = "gb";
    }
    else if (i == 4)
    {
        // printf("total: %lld %s\n", x, "tb");
        vt = "tb";
    }
    else
    {
        // printf("total: %lld bytes\n", x);
        vt = "XX";
    }
    printf("  size on disk: %.1lld %s\n", x, vt);
    // free(vt);
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
}
