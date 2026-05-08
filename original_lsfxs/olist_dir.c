#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

// longest length of a directory / file path accepted
#define MAX_FILENAME 800

// struct for LL of file/folder data
typedef struct node
{
    char f[MAX_FILENAME];
    struct node *next;
} dir_entry;

dir_entry *dir_entries = NULL;

int ftype(char s[]);
void list_dir(char s[]);

int main(void)
{
    char buff[MAX_FILENAME];
    int t_sz = sizeof(buff);

    if (getcwd(buff, t_sz) != 0) {
        printf("cwd: %s\n", buff);
    }

    int stat = ftype(buff);
    if (stat > 0) {
        if (stat == 1) {
            printf("its a directory\n");
            list_dir(buff);
        }
        else {
            printf("its a file\n");
            return 0;
        }
    }
    else {
        printf("it doesn't exist\n");
        return 0;
    }

    while (dir_entries != NULL)
    {
        printf("entry: %s\n", dir_entries->f);
        dir_entries = dir_entries->next;
    }
}

int ftype(char s[])
{
    struct stat path_strat;

    // check if it exists
    if (stat(s, &path_strat) != 0) {
        return 0;
    }
    
    if (S_ISDIR(path_strat.st_mode)) {
        return 1;
    }
    
    else if (S_ISREG(path_strat.st_mode)) {
        return 2;
    }
    return 3;
}

char *type_wr(char s[])
{
    char *t = malloc(sizeof(char) * 4);
    if (ftype(s) == 1)
    {
        t = "dir";
    }
    else if (ftype(s) == 2)
    {
        t = "file";
    }
    else
    {
        t = "NaN";
    }
    return t;
}

void list_dir(char s[])
{
    DIR *dr = opendir (s);
    struct dirent *ent;
    
    //char *t;

    if (dr != NULL) {
        while ((ent = readdir (dr)) != NULL) {
            dir_entry *entry = malloc(sizeof(dir_entry));
            if (entry == NULL)
            {
                break;
            }
            strcpy(&entry->f[0], ent->d_name);
            // entry->f[0] = ent->d_name;
            // entry->next = NULL;
            entry->next = dir_entries;
            dir_entries = entry;
        }
        (void) closedir (dr);
    } else {
        printf("couldn't open %s\n", s);
    }
}













