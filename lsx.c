#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "lsx.h"


// globals trackers
// byte (or block) counter
long long sx = 0;
// list entries (default)
int list = 1;
// du by blocks (default)
int blocks = 1;
// no recursion (defualt)
int flat = 1;


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
                    sf_strcat(buff, argv[2], MAX_FILENAME);
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
                    printf("incorrect use of arguments\n");
                    return 1;
                }
                sf_strcat(buff, argv[3], MAX_FILENAME);
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
                sf_strcat(buff, argv[2], MAX_FILENAME);
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
                    sf_strcat(buff, argv[1], MAX_FILENAME);
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
    }// else, use cwd

    // process
    if (ftype(buff) == 1)
    {
        if (list)
        {
            // ls route
            ls_f(buff);
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
            pdir = strcpy(pdir, s_tmp);

            // if it doesn't end with an '/':
            // append one ONCE before the loop
            if (strcmp(&pdir[strlen(pdir) - 1], "/") != 0) {
                sf_strcat(pdir, "/", MAX_FILENAME);
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
                    sf_strcat(pdir, fdupe, MAX_FILENAME);
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
        }
        (void) closedir (dir);
    }
    else {
        printf("couldn't open (ls_f) %s\n", s);
        return;
    }
    free(s_tmp);
}


int init_fs (void)
{
    if (getcwd(buff, MAX_FILENAME) == NULL)
    {
        printf("could not find/open the cwd");
        return 0; // false
    }
    sf_strcat(buff, "/", MAX_FILENAME);
    return 1; // true
}


// C passes arguments by value
// the pointers don't need to be saved or protected
void sf_strcat(char *a, char *o, int bufflen)
{
    int lim = bufflen -1;

    int c = 0;
    while (*(a + c) && c < lim) 
    // while str a is not NULL value
    {
        // move + 1
        c++;
    }
    while (*o && c < lim)
    // while new string is not NULL
    {
        // copy the char
        *(a + c) = *o;
        // and move + 1
        c++;
        o++;
    }
    // set the final char to NULL
    *(a + c) = '\0';
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
        return 0; // false
    }
    return 1; // true
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
        if (lli_exists(&(sz_tmp->f)))
        {
            if (sz_tmp->type == 2)
            {
                work_summ();
            }
        }
        else
        {
            return;
        }
        sz_tmp = sz_tmp->next;
    }
}


void work_summ(void)
{
    sx = sx + (long long)st.st_size;
}


void block_summ(void)
{
    // don't loose the pointer !
    dir_entry *sz_tmp = dir_entries;

    while (sz_tmp != NULL)
    {
        if (lli_exists(&(sz_tmp->f)))
        {
            if (sz_tmp->type == 2)
            {
                work_block();
            }
        }
        else
        {
            return;
        }
        sz_tmp = sz_tmp->next;
    }
    sx = sx * POSIX_BLOCK_SIZE;
}


void work_block(void)
{
    sx = sx + (long long)st.st_blocks;
}


void pretty_print(void)
{
    char *vt;
    int i = 0;
    long long lim = 1024;
    while (sx > lim)
    {
        sx = sx / lim;
        i++;
    }

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
