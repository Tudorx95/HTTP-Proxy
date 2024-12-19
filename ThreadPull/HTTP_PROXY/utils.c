#include "utils.h"

int determine_lines(char *message)
{
    int i = 1;
    char *line = strtok(message, "\n\0");
    for (; (line = strtok(NULL, "\n\0")); i++)
        if (!strcmp(line, "\n"))
            break;
    return i;
}