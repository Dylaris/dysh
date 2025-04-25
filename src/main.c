#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "parse.h"
#include "err.h"

#define CMD_LENGTH       256
#define DEBUG            0
#define READ_END         0 /* pipe read end */
#define WRITE_END        1 /* pipe write end */
#define CMD_FOLER        "src/bin/"

#define REDIRECT(cmd, old_fd) \
    do { \
        if (cmd->redirect_fd[old_fd] != -1) \
            EXIT_IF(dup2(cmd->redirect_fd[old_fd], old_fd) < 0, "failed dup2\n"); \
    } while (0)

struct dysh_cmd *cmd_list[MAX_CMD_CNT];

/**
 * @brief Reset Signal handler
 */
static void reset_signal_handler(int sig)
{
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, NULL);
}

/**
 * @brief Some thing need to do in child process before 
 * execute the struct dysh_cmd
 */
static void ready_for_execution(struct dysh_cmd *cmd)
{
    REDIRECT(cmd, STDIN_FILENO);
    REDIRECT(cmd, STDOUT_FILENO);
    REDIRECT(cmd, STDERR_FILENO);
#if DEBUG
    printf("redirect fd: %d %d %d\n", cmd->redirect_fd[0],
                                      cmd->redirect_fd[1],
                                      cmd->redirect_fd[2]);
#endif /* DEBUG */
    reset_signal_handler(SIGINT);
}

/**
 * @brief Signal handler
 * @note For shell process, we do no need to do anything else, block some 
 * signal is ok. For child process, using to execute struct dysh_cmd, we need to 
 * change the signal handler. Basically, SIGINT should be valid to child process
 * instead of shell process.
 * @param sig Handling signal
 */
static void signal_handler(int sig)
{
    switch (sig) {
    case SIGINT:
        puts("You pressed CTRL+C");
        break;
    default: 
        break;
    }
}

/**
 * @brief Read input and store it to buffer
 * @param buf Buffer storing input
 * @param size Size of buffer
 * @return Zero if there has at least one printable character
 */
static int read_input(char *buf, size_t size)
{
    /* fgtes() will return NULL and set errno to EINTR when signal happened */
    EXIT_IF(fgets(buf, size, stdin) == NULL && errno != EINTR, "read input");
    buf[strcspn(buf, "\n")] = '\0';

    /* Check if the input is 'empty' */
    char *p = buf;
    while (*p) {
        if (isprint(*p)) return 0;
        p++;
    }
    return 1;
}

/**
 * @brief Process input to correct form
 * @param input struct dysh_cmd line input
 * @return A cmd list storing the tokens
 */
static void process_input(char *input)
{
    split_by_semicolon(input);
    split_by_pipe();
    split_by_space();
    process_redirect();
}

/**
 * @brief This is helper function to piping cmd through recursively call
 * @param cmd Current cmd need to be executed
 * @param read_fd The read fd (previous pipe read end), we need to redirect it to STDIN
 * @return Zero if successfully execute
 * @note Also, we need to redirect the new pipe we created in this function to STDOUT,
 * and pass its read_end to next cmd
 */
static int execute_cmd_with_pipe(struct dysh_cmd *cmd, int read_fd)
{
    int pfd[2];
    EXIT_IF(pipe(pfd) < 0, "failed pipe\n");

    pid_t pid = fork();
    if (pid == 0) {
        ready_for_execution(cmd);

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
        exit(1); /* We arrive here when execv failed */
    }

    close(read_fd);
    close(pfd[WRITE_END]);
    int status = 0;
    waitpid(pid, &status, 0); /* Block until child process is over execution */
    if (cmd->next)
        status += execute_cmd_with_pipe(cmd->next, pfd[READ_END]);
    close(pfd[READ_END]);

    return status;
}

/**
 * @brief Execute the struct dysh_cmd
 * @param cmd Store the needed args
 * @return Zero if successfully execute
 */
static int execute_cmd(struct dysh_cmd *cmd)
{
    if (cmd->count == 0) return 0;

    if (strcmp(cmd->args[0], "exit") == 0)
        exit(0);

    char path[CMD_LENGTH];
    sprintf(path, CMD_FOLER"%s", cmd->args[0]);

    int status = 0;

    if (!(cmd->next)) {
        /* No piping */

        pid_t pid = fork();
        if (pid == 0) {
            ready_for_execution(cmd);

            execv(path, cmd->args);
            exit(1); /* We arrive here when execv failed */
        }
        /* Interesting behavior: When using wait(NULL) to wait for a child 
           process that has been interrupted (e.g., by Ctrl+C), a strange 
           issue occurs where the prompt gets mixed with the struct dysh_cmd output. 
           This doesn't happen when using waitpid(pid, NULL, 0) to wait for 
           the child process.
           
           Example:
           - Run: `tee test_file` in a child process.
           - Interrupt it using Ctrl+C.
           - After interrupting, run `cat Makefile`, and you'll notice that the output from `cat` and the prompt
             get mixed together due to the way the shell handles the interrupted child process. */
        waitpid(pid, &status, 0);
    } else {
        /* Piping */

        int pfd[2];
        EXIT_IF(pipe(pfd) < 0, "failed pipe\n");

        pid_t pid = fork();
        if (pid == 0) {
            ready_for_execution(cmd);

            close(pfd[READ_END]);

            /* Redirect pfd[WRITE_END] to STDOUT and close duplicate pfd[WRITE_END] */
            EXIT_IF(dup2(pfd[WRITE_END], STDOUT_FILENO) < 0, "failed dup2\n");
            close(pfd[WRITE_END]);

            /* Output of current cmd is passed to next cmd through pipe */
            execv(path, cmd->args);
            exit(1); /* We arrive here when execv failed */
        }

        close(pfd[WRITE_END]);
        waitpid(pid, &status, 0); /* Block until child process is over execution */
        execute_cmd_with_pipe(cmd->next, pfd[READ_END]);
        close(pfd[READ_END]);
    }

    return status;
}

/**
 * @brief Execute all struct dysh_cmd in cmd_list
 * @return Zero if successfully execute
 */
static int execute_cmd_list(void)
{
    int status = 0;
    for (int i = 0; i < MAX_CMD_CNT; i++) {
        if (!cmd_list[i]) continue;
        status += execute_cmd(cmd_list[i]);
    }
    return status;
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
    memset(input, 0, sizeof(input));

    while (1) {
        printf("dysh> ");
        fflush(stdout);
        if (read_input(input, sizeof(input))) continue;
        process_input(input);
#if DEBUG
        for (int i = 0; i < MAX_CMD_CNT; i++) {
            struct dysh_cmd *cmd = cmd_list[i];
            if (!cmd) continue;
            printf("< command %d\n", i);
            int count = 0;
            while (cmd) {
                printf("<< sub comand %d\n", count++);
                print_cmd_args(cmd);
                cmd = cmd->next;
            }
        }
#endif /* DEBUG */
        int status = execute_cmd_list();
        if (!status) 
            printf("\n<<< Exit Code >>>: " YELLOW "%d" RESET " -> " GREEN "success" RESET "\n\n", status);
        else
            printf("\n<<< Exit Code >>>: " YELLOW "%d" RESET " -> " RED "failure" RESET "\n\n", status);

        memset(input, 0, sizeof(input));
        free_cmd_list();
    }
}

int main(void)
{
    /* Initialize cmd_list */
    for (int i = 0; i < MAX_CMD_CNT; i++)
        cmd_list[i] = NULL;

    /* Set signal handler */
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    atexit(free_cmd_list);
    loop();
    return 0;
}
