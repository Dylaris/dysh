#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "err.h"

CmdList *new_cmd_list(void)
{
    CmdList *cmd_list = malloc(sizeof(CmdList));
    ERR_EXIT_IFF(cmd_list, "new cmd list");

    cmd_list->count = 0;
    cmd_list->args = malloc(sizeof(CmdArg));
    ERR_EXIT_IFF(cmd_list->args, "new cmd list");
    cmd_list->args[0] = NULL;
    cmd_list->next_cmd = NULL;

    return cmd_list;
}

void free_cmd_list(CmdList *cmd_list)
{
    CmdList *p = cmd_list;
    while (p) {
        for (size_t i = 0; i < p->count; i++)
            free(p->args[i]);
        free(p->args);
        free(p);
        p = p->next_cmd;
    }
}

void append_cmd_arg(CmdList *cmd_list, CmdArg arg)
{
    /* Expand */
    cmd_list->count++;
    CmdArg *args = realloc(cmd_list->args, sizeof(CmdArg) * (cmd_list->count + 1));
    ERR_PRINT_IFF(args, "out of memory");

    /* Append new arg to args list (end with NULL) */
    cmd_list->args = args;
    cmd_list->args[cmd_list->count - 1] = strdup(arg);
    ERR_PRINT_IFF(cmd_list->args[cmd_list->count - 1], "out of memory");
    cmd_list->args[cmd_list->count] = NULL;
}

void print_cmd_args(CmdArg *args)
{
    CmdArg *p = args;
    while (*p) {
        printf("ARG: %s\n", *p);
        p++;
    }
    fflush(stdout);
}
