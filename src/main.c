#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include "parse.h"

#define CMD_LENGTH       256
#define DEBUG_PRINT_ARGS 0
#define READ_END         0 /* pipe read end */
#define WRITE_END        1 /* pipe write end */
#define CMD_FOLER        "src/bin/"

/* Exit if exp is true */
#define EXIT_IF(exp, fmt, ...) do { \
        if (exp) { \
            fprintf(stderr, fmt, ##__VA_ARGS__); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

Command *cmd_list[MAX_CMD_CNT];

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
    split_by_semicolon(input);
    split_by_pipe();
    split_by_space();
}

/**
 * @brief This is helper function to piping cmd through recursively call
 * @param cmd Current cmd need to be executed
 * @param read_fd The read fd (previous pipe read end), we need to redirect it to STDIN
 * @note Also, we need to redirect the new pipe we created in this function to STDOUT,
 * and pass its read_end to next cmd
 */
static void execute_cmd_with_pipe(Command *cmd, int read_fd)
{
    int pfd[2];
    EXIT_IF(pipe(pfd) < 0, "failed pipe\n");

    pid_t pid = fork();
    if (pid == 0) {
        close(pfd[READ_END]);

        /* Redirect read_fd to STDIN and close duplicate read_fd */
        EXIT_IF(dup2(read_fd, STDIN_FILENO) < 0, "failed dup2\n");
        close(read_fd);

        /* Redirect pfd[WRITE_END] to STDOUT and close duplicate pfd[WRITE_END] if there is next piping cmd */
        if (cmd->next)
            EXIT_IF(dup2(pfd[WRITE_END], STDOUT_FILENO) < 0, "failed dup2\n");
        close(pfd[WRITE_END]);

        /* Output of current cmd is passed to next cmd through pipe */
        char path[CMD_LENGTH];
        sprintf(path, CMD_FOLER"%s", cmd->args[0]);
        execv(path, cmd->args);
        exit(EXIT_FAILURE); /* We arrive here when execv failed */
    }

    close(read_fd);
    close(pfd[WRITE_END]);
    waitpid(pid, NULL, 0); /* Block until child process is over execution */
    if (cmd->next)
        execute_cmd_with_pipe(cmd->next, pfd[READ_END]);
    close(pfd[READ_END]);
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
        exit(EXIT_SUCCESS);

    char path[CMD_LENGTH];
    sprintf(path, CMD_FOLER"%s", cmd->args[0]);

    if (!(cmd->next)) {
        /* No piping */

        pid_t pid = fork();
        if (pid == 0) {
            execv(path, cmd->args);
            exit(EXIT_FAILURE); /* We arrive here when execv failed */
        }
        wait(NULL);
    } else {
        /* Piping */

        int pfd[2];
        EXIT_IF(pipe(pfd) < 0, "failed pipe\n");

        pid_t pid = fork();
        if (pid == 0) {
            close(pfd[READ_END]);

            /* Redirect pfd[WRITE_END] to STDOUT and close duplicate pfd[WRITE_END] */
            EXIT_IF(dup2(pfd[WRITE_END], STDOUT_FILENO) < 0, "failed dup2\n");
            close(pfd[WRITE_END]);

            /* Output of current cmd is passed to next cmd through pipe */
            execv(path, cmd->args);
            exit(EXIT_FAILURE); /* We arrive here when execv failed */
        }

        close(pfd[WRITE_END]);
        waitpid(pid, NULL, 0); /* Block until child process is over execution */
        execute_cmd_with_pipe(cmd->next, pfd[READ_END]);
        close(pfd[READ_END]);
    }
}

/**
 * @brief Execute all command in cmd_list
 */
static void execute_cmd_list(void)
{
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        if (!cmd_list[i]) continue;
        execute_cmd(cmd_list[i]);
    }
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
#if DEBUG_PRINT_ARGS
        for (int i = 0; i < MAX_CMD_CNT; i++) {
            Command *cmd = cmd_list[i];
            if (!cmd) continue;
            printf("< command %d\n", i);
            int count = 0;
            while (cmd) {
                printf("<< comand pipe %d\n", count++);
                print_cmd_args(cmd);
                cmd = cmd->next;
            }
        }
#endif /* DEBUG_PRINT_ARGS */
        execute_cmd_list();

        memset(input, 0, sizeof(input));
        free_cmd_list();
    }
}

int main(void)
{
    /* Initialize cmd_list */
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        cmd_list[i] = NULL;
    }

    atexit(free_cmd_list);
    loop();
    return 0;
}
