#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include "parse.h"

#define CMD_LENGTH  256
// #define DEBUG_PRINT_ARGS

/**
 * @brief Read input and store it to buffer
 * @param buf Buffer storing input
 * @param size Size of buffer
 */
static void read_input(char *buf, size_t size)
{
    assert(fgets(buf, size, stdin));
    buf[strcspn(buf, "\n")] = '\0';
}

/**
 * @brief Process input to correct form
 * @param input Command line input
 * @return A cmd list storing the tokens
 */
static void process_input(char *input)
{
    /* Need to check if input is valid */

    split_by_semicon(input);
    split_by_pipe();
}

/**
 * @brief A clean up function, executed after call exit
 */
static void clean_up(void)
{
}

/**
 * @brief Execute the command
 * @param cmd Store the needed args
 * @todo pipe and other combination command
 */
static void execute_cmd(Command *cmd)
{
    if (cmd->count == 0) return;
    if (strcmp(cmd->args[0], "exit") == 0)
        exit(0);

    char path[CMD_LENGTH];
    sprintf(path, "src/builtin/%s", cmd->args[0]);
    pid_t pid = fork();
    if (pid == 0) {
        execv(path, cmd->args);
        /* exit(0) is important now, if the path is not exist, execv() will fail
           so the child process will continue. What does it mean? It means we create 
           another dysh (child process), and child process will take the charge of terminal,
           because parent process is blocked at wait(NULL). So we need to exit twice */
        exit(0);
    }

    wait(NULL);
}

static void execute_cmd_list(void)
{

}

/**
 * @brief A main loop of dysh 
 * 1. Read input
 * 2. Process input
 * 3. Execute input
 */
static void loop(void) 
{
    char input[CMD_LENGTH];
    while (1) {
        printf("dysh> ");
        fflush(stdout);
        read_input(input, sizeof(input));
        process_input(input);
#ifdef DEBUG_PRINT_ARGS
        print_cmd_args(cmd->args);
#endif /* DEBUG_PRINT_ARGS */
        execute_cmd_list();

        memset(input, 0, sizeof(input));
        free_cmd_list();
    }
}

int main(void)
{
    /* Initialize cmd_list */
    Command *cmd_list[MAX_CMD_CNT];
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        cmd_list[i] = NULL;
    }

    atexit(clean_up);
    loop();
    return 0;
}