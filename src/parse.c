#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parse.h"

/* strdup() does not allow NULL as parameter, so I define this macro */
#define STR_COPY(ptr) ({ \
        char *res = NULL; \
        if (ptr) res = strdup(ptr); \
        res; \
    })

/**
 * @brief Create a cmd
 * @return A cmd
 * @note Use NULL to mark the end of args
 * and next points to next command
 */
Command *new_cmd(void)
{
    Command *cmd = malloc(sizeof(Command));
    assert(cmd);

    cmd->count = 0;
    cmd->args = malloc(sizeof(CmdArg));
    assert(cmd->args);
    cmd->args[0] = NULL;
    cmd->next = NULL;

    return cmd;
}

/**
 * @brief Free all the commands in cmd_list
 */
void free_cmd_list(void)
{
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        if (cmd_list[i]) {
            free_cmd(cmd_list[i]);
            cmd_list[i] = NULL;
        }
    }
}

/**
 * @brief Free the cmd
 * @param cmd Target
 * @note The objects we need to free
 * 1. String pointed by args list (generated through strdup)
 * 2. Args list itself
 * 3. Cmd itself
 */
void free_cmd(Command *cmd)
{
    for (size_t i = 0; i < cmd->count; i++)
        free(cmd->args[i]);
    free(cmd->args);
    free(cmd);
}

/**
 * @brief Append the new cmd to the cmd list
 * @param cmd Current cmd holding this new_cmd
 * @param new_cmd The new cmd list
 * @note Need a extra NULL to mark the end of cmd list
 */
void append_cmd(Command *cmd)
{
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        if (!cmd_list[i]) {
            cmd_list[i] = cmd;
            return;
        }
    }

    fprintf(stderr, "out of the capacity of cmd list\n");
    exit(1);
}

/**
 * @brief Append the new arg to the current cmd
 * @param cmd Current cmd holding this arg
 * @param arg The new arg
 * @note Need a extra NULL to mark the end of args list
 */
void append_cmd_arg(Command *cmd, CmdArg arg)
{
    /* Expand */
    cmd->count++;
    CmdArg *args = realloc(cmd->args, sizeof(CmdArg) * (cmd->count + 1));
    assert(args);

    /* Append new arg to args list (end with NULL) */
    cmd->args = args;
    cmd->args[cmd->count - 1] = STR_COPY(arg);
    assert(cmd->args[cmd->count - 1]);
    cmd->args[cmd->count] = NULL;
}

/**
 * @brief Print the args for debuging
 * @param cmd Target
 */
void print_cmd_args(Command *cmd)
{
    CmdArg *p = cmd->args;
    while (*p) {
        printf("ARG: %s\n", *p);
        p++;
    }
    fflush(stdout);
}

/**
 * @brief Split the input by semicon
 * @param input Command line input
 */
void split_by_semicon(char *input)
{
    char *cmd_str = strtok(input, CMD_DELIM);
    while (cmd_str) {
        Command *cmd = new_cmd();
        append_cmd(cmd);
        append_cmd_arg(cmd, cmd_str);
        cmd_str = strtok(NULL, CMD_DELIM);
    }
}

/**
 * @brief Just free the old args, so that I can reassign args
 * @param cmd Target need to be freed
 */
static void free_cmd_args(Command *cmd)
{
    for (size_t i = 0; i < cmd->count; i++) {
        if (cmd->args[i]) {
            free(cmd->args[i]);
            cmd->args[i] = NULL;
        }
    }
    cmd->count = 0;
}

/**
 * @brief Split each cmd list by pipe '|'
 */
void split_by_pipe(void)
{
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        Command *cmd = cmd_list[i];
        if (!cmd) continue;

        char *old_str = STR_COPY(cmd->args[0]);
        char *pipe_cmd_str = strtok(old_str, PIPE_DELIM);
        while (pipe_cmd_str) {
            /* pipe_cmd_str is a pointer to cmd->args[0], if we donot use
               strdup, free_cmd_args(cmd) also affect pipe_cmd_str */
            free_cmd_args(cmd);
            append_cmd_arg(cmd, pipe_cmd_str);
            pipe_cmd_str = strtok(NULL, PIPE_DELIM);
            if (pipe_cmd_str) {
                cmd->next = new_cmd();  /* This is sub command of piping, do not append to cmd_list */
                cmd = cmd->next;
            }
        }
        free(old_str);
    }
}