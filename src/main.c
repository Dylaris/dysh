#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "err.h"

#define TOKEN_DELIM " \t\r"

static void read_line(char *buf, size_t size)
{
    ERR_EXIT_IFF(fgets(buf, size, stdin), "read a line"); 
    buf[strcspn(buf, "\n")] = '\0';
}

static CmdList *process_line(char *line)
{
    CmdList *cmd_list = new_cmd_list();
    CmdArg arg = strtok(line, TOKEN_DELIM);
    while (arg) {
        append_cmd_arg(cmd_list, arg);
        arg = strtok(NULL, TOKEN_DELIM);
    }

    return cmd_list;
}

static void loop(void) 
{
    char line[256];
    CmdList *cmd_list;
    while (1) {
        printf("dysh> ");
        fflush(stdout);
        read_line(line, sizeof(line));
        cmd_list = process_line(line);
        print_cmd_args(cmd_list->args);

        memset(line, 0, sizeof(line));
        free_cmd_list(cmd_list);
    }
}

int main(void)
{
    loop();
    return 0;
}