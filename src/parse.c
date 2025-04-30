#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "parse.h"
#include "err.h"

/* strdup() does not allow NULL as parameter, so I define this macro */
#define STR_COPY(ptr) \
    ({ \
        char *res = NULL; \
        if (ptr) res = strdup(ptr); \
        res; \
     })

/**
 * @brief Create a cmd
 * @return A cmd
 * @note Use NULL to mark the end of args
 * and next points to next struct Command
 */
struct Command *new_cmd(void)
{
    struct Command *cmd = malloc(sizeof(struct Command));
    EXIT_IF(cmd == NULL, "out of memory");

    cmd->count = 0;
    cmd->args = malloc(sizeof(CommandArg));
    EXIT_IF(cmd->args == NULL, "out of memory");
    cmd->args[0] = NULL;
    cmd->next = NULL;
    cmd->redirect_fd[0] = -1;
    cmd->redirect_fd[1] = -1;
    cmd->redirect_fd[2] = -1;
    cmd->flag = RUN_IN_FG;

    return cmd;
}

/**
 * @brief Free all the struct Commands in cmd_list
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
void free_cmd(struct Command *cmd)
{
    for (size_t i = 0; i < cmd->count; i++) {
        free(cmd->args[i]);
        for (int i = 0; i < 3; i++) {
            if (cmd->redirect_fd[i] != -1) {
                close(cmd->redirect_fd[i]);
                cmd->redirect_fd[i] = -1;
            }
        }
    }
    free(cmd->args);
    free(cmd);
}

/**
 * @brief Append the new cmd to the cmd list
 * @param cmd Current cmd holding this new_cmd
 * @param new_cmd The new cmd list
 * @note Need a extra NULL to mark the end of cmd list
 */
void append_cmd(struct Command *cmd)
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
void append_cmd_arg(struct Command *cmd, CommandArg arg)
{
    /* Expand */
    cmd->count++;
    CommandArg *args = realloc(cmd->args, sizeof(CommandArg) * (cmd->count + 1));
    EXIT_IF(args == NULL, "out of memory");

    /* Append new arg to args list (end with NULL) */
    cmd->args = args;
    cmd->args[cmd->count - 1] = STR_COPY(arg);
    EXIT_IF(cmd->args[cmd->count - 1] == NULL, "out of memory");
    cmd->args[cmd->count] = NULL;
}

/**
 * @brief Remove the new arg of the current cmd at the position 'idx'
 * @param cmd Current cmd holding this arg
 * @param idx Remove index
 */
void remove_cmd_arg(struct Command *cmd, size_t idx)
{
    if (idx > cmd->count) return;

    size_t old_args_count = cmd->count;
    CommandArg *old_args = cmd->args;

    CommandArg *new_args = malloc(sizeof(CommandArg));
    EXIT_IF(cmd->args == NULL, "out of memory");
    new_args[0] = NULL;

    cmd->args = new_args;
    cmd->count = 0;

    for (size_t i = 0; i < old_args_count; i++) {
        if (i == idx) {
            free(old_args[i]);
            continue;
        }
        append_cmd_arg(cmd, old_args[i]);
        free(old_args[i]);
    }
    free(old_args);
}

/**
 * @brief Print the args for debuging
 * @param cmd Target
 */
void print_cmd_args(struct Command *cmd)
{
    CommandArg *p = cmd->args;
    while (*p) {
        printf("ARG: %s\n", *p);
        p++;
    }
    fflush(stdout);
}

/**
 * @brief Split the input by semicolon
 * @param input struct Command line input
 */
void split_by_semicolon(char *input)
{
    char *cmd_str = strtok(input, CMD_DELIM);
    while (cmd_str) {
        struct Command *cmd = new_cmd();
        append_cmd(cmd);
        append_cmd_arg(cmd, cmd_str);
        cmd_str = strtok(NULL, CMD_DELIM);
    }
}

/**
 * @brief Just free the old args, so that I can reassign args
 * @param cmd Target need to be freed
 */
static void free_cmd_args(struct Command *cmd)
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
 * @brief Helper function for split_by_space
 * @param cmd The processed one
 */
static void _split_by_space(struct Command *cmd)
{
    char *cmd_str = STR_COPY(cmd->args[0]);
    free_cmd_args(cmd);
    CommandArg arg = strtok(cmd_str, ARG_DELIM);
    while (arg) {
        if (strcmp(arg, "&") == 0) cmd->flag = RUN_IN_BG;
        else append_cmd_arg(cmd, arg);
        arg = strtok(NULL, ARG_DELIM);
    }
    free(cmd_str);
}

/**
 * @brief Split each cmd's args (processed by pipe) by space to become a really arg
 * and we need to process 'next' cmd (if pipe) and cmd in cmd_list
 */
void split_by_space(void)
{
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        struct Command *cmd = cmd_list[i];
        if (!cmd) continue;

        /* Process piping if has */
        while (cmd) {
            _split_by_space(cmd);
            cmd = cmd->next;
        }
    }
}

/**
 * @brief Split each cmd by pipe '|'
 */
void split_by_pipe(void)
{
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        struct Command *cmd = cmd_list[i];
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
                cmd->next = new_cmd();  /* This is sub struct Command of piping, do not append to cmd_list */
                cmd = cmd->next;
            }
        }
        free(old_str);
    }
}

/**
 * @brief Helper function of process_redirect()
 * @param cmd The processed one
 * @return Zeor if success
 */
static int _process_redirect(struct Command *cmd)
{
    if (!(cmd->args[0])) return 1;

    for (size_t i = 0; i < cmd->count; i++) {
        CommandArg p = cmd->args[i];
        int open_flags = -1;
        int redirect_fd_nr = -1;

        if (!p) return 1;

        if (strcmp(p, ">") == 0) {
            if (i >= cmd->count - 1 || !(cmd->args[i + 1])) { 
                fprintf(stderr, "redirect error\n");
                return 1;
            }
            open_flags = O_CREAT | O_WRONLY | O_TRUNC;
            redirect_fd_nr = 1;
        } else if (strcmp(p, ">>") == 0) {
            if (i >= cmd->count - 1 || !(cmd->args[i + 1])) { 
                fprintf(stderr, "redirect error\n");
                return 1;
            }
            open_flags = O_CREAT | O_WRONLY | O_APPEND;
            redirect_fd_nr = 1;
        } else if (strcmp(p, "<") == 0) {
            if (i >= cmd->count - 1 || !(cmd->args[i + 1])) { 
                fprintf(stderr, "redirect error\n");
                return 1;
            }
            open_flags = O_RDONLY;
            redirect_fd_nr = 0;
        }

        p = cmd->args[i + 1];
        if (p && redirect_fd_nr != -1) {
            int fd = open(p, open_flags, 0644);
            if (fd < 0) {
                fprintf(stderr, "open error\n");
                return 1;
            }
            cmd->redirect_fd[redirect_fd_nr] = fd;

            remove_cmd_arg(cmd, i); /* remove redirect operator */
            remove_cmd_arg(cmd, i); /* remove redirect file (file position should be minus 1) */

            break;
        }
    }
    return 0;
}

/**
 * @brief Traversal the cmd args and put the redirect things to 
 * cmd's rdirect_fd[3]
 * @return Zero if success
 */
int process_redirect(void)
{
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        struct Command *cmd = cmd_list[i];

        while (cmd) {
            int status = _process_redirect(cmd);
            if (status != 0) return 1;
            cmd = cmd->next;
        }
    }
    return 0;
}
