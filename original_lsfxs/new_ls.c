#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const int MAX_FILENAME = 200;

int main(void)
{
    // char *buff = malloc(sizeof(char) * MAX_FILENAME);
    char buff[MAX_FILENAME];
    // if (buff == NULL)
    // {
    //     printf("failed to get memory for cwd");
    //     return 1;
    // }
    int t_sz = sizeof(buff);

    if (getcwd(buff, t_sz) != 0)
    {
        printf("cwd: %s\n", buff);
        return 0;
    }
    // free(buff);
    return 1;
}
