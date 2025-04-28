#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "parse.h"
#include "jobs.h"
#include "err.h"

#define ARG_DEBUG        0
#define SIG_DEBUG        1

#define CMD_LENGTH       256
#define READ_END         0      /* pipe read end */
#define WRITE_END        1      /* pipe write end */
#define CMD_FOLER        "src/bin/"

#define REDIRECT(cmd, old_fd) \
    do { \
        if (cmd->redirect_fd[old_fd] != -1) \
            EXIT_IF(dup2(cmd->redirect_fd[old_fd], old_fd) < 0, "failed dup2\n"); \
    } while (0)

struct Command *cmd_list[MAX_CMD_CNT];
pid_t shell_pgrp;
pid_t jobs[MAX_JOBS];

/**
 * @brief Clear up after exit
 */
static void clear_up(void)
{
    free_cmd_list();
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i] == INVALID_JOBID) continue;
        kill(jobs[i], SIGCONT);
        waitpid(jobs[i], NULL, 0);
    }
}

/**
 * @brief Signal handler for child
 * @param sig Handling signal
 */
static void signal_handler(int sig)
{
    switch (sig) {
    case SIGUSR1:   /* same to SIGSTOP */
        printf("Process <%u> received SIGUSR1!\n", getpid());
        EXIT_IF(kill(getpid(), SIGSTOP) == -1, "kill failed");
        break;
    case SIGUSR2:   /* same to SIGCONT */
        printf("Process <%u> received SIGUSR2!\n", getpid());
        EXIT_IF(kill(getpid(), SIGCONT) == -1, "kill failed");
        break;
    default: 
        break;
    }
}

/**
 * @brief Some thing need to do in child process before 
 * execute the struct Command
 */
static void ready_for_execution(struct Command *cmd)
{
    REDIRECT(cmd, STDIN_FILENO);
    REDIRECT(cmd, STDOUT_FILENO);
    REDIRECT(cmd, STDERR_FILENO);
#if DEBUG
    printf("redirect fd: %d %d %d\n", cmd->redirect_fd[0],
                                      cmd->redirect_fd[1],
                                      cmd->redirect_fd[2]);
#endif /* DEBUG */
    set_signal_handler(SIGUSR1, signal_handler);
    set_signal_handler(SIGUSR2, signal_handler);
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
 * @param input struct Command line input
 * @return Zero is success
 */
static int process_input(char *input)
{
    split_by_semicolon(input);
    split_by_pipe();
    split_by_space();
    return process_redirect();
}

/**
 * @brief A function to wait child. And it is non-block when child process
 * is stopped, and then it will append the job to jobs array
 * @param pid The target
 * @param status The reference of execution status
 * @note When we fork a child process, jobs array has been copied. So don't
 * be confused with them
 */
static void wait_child(pid_t pid, int *status)
{
    while (1) {
        pid_t result = waitpid(pid, status, WNOHANG | WUNTRACED);
        if (result != 0) {  /* not running */
            *status = 0;    /* stop is a normal state */
#ifdef SIG_DEBUG
            if (is_job_alive(pid) == FALSE) printf("job is not alive\n");
            else printf("job is alive\n");
#endif /* SIG_DEBUG */
            if (is_job_alive(pid) == TRUE) append_job(pid);
            break;     
        }
    }
}

/**
 * @brief This is helper function to piping cmd through recursively call
 * @param cmd Current cmd need to be executed
 * @param read_fd The read fd (previous pipe read end), we need to redirect it to STDIN
 * @return Zero if successfully execute
 * @note Also, we need to redirect the new pipe we created in this function to STDOUT,
 * and pass its read_end to next cmd
 */
static int execute_cmd_with_pipe(struct Command *cmd, int read_fd)
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
 * @brief Execute the struct Command
 * @param cmd Store the needed args
 * @return Zero if successfully execute
 */
static int execute_cmd(struct Command *cmd)
{
    if (cmd->count == 0) return 0;

    /* Special command */
    if (strcmp(cmd->args[0], "exit") == 0) exit(0);
    if (strcmp(cmd->args[0], "jobs") == 0) { list_jobs(); return 0; }

    char path[CMD_LENGTH];
    sprintf(path, CMD_FOLER"%s", cmd->args[0]);

    int status = 0;

    if (!(cmd->next)) {
        /* No piping */

        pid_t pid = fork();
        if (pid == 0) {
            ready_for_execution(cmd);

#ifdef SIG_DEBUG
            printf("child process <%u>\n", getpid());
            if (strcmp(cmd->args[0], "cat") == 0)
                raise(SIGUSR1);
#endif /* SIG_DEBUG */

            execv(path, cmd->args);
            exit(1); /* We arrive here when execv failed */
        }
        /* Interesting behavior: When using wait(NULL) to wait for a child 
           process that has been interrupted (e.g., by Ctrl+C), a strange 
           issue occurs where the prompt gets mixed with the struct Command output. 
           This doesn't happen when using waitpid(pid, NULL, 0) to wait for 
           the child process.
           
           Example:
           - Run: `tee test_file` in a child process.
           - Interrupt it using Ctrl+C.
           - After interrupting, run `cat Makefile`, and you'll notice that the output from `cat` and the prompt
             get mixed together due to the way the shell handles the interrupted child process. */

        wait_child(pid, &status);
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
 * @brief Execute all struct Command in cmd_list
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
    int status = 0;

    while (1) {
        printf("dysh> ");
        fflush(stdout);
        if (read_input(input, sizeof(input))) continue;
        status = process_input(input);
#if ARG_DEBUG
        for (int i = 0; i < MAX_CMD_CNT; i++) {
            struct Command *cmd = cmd_list[i];
            if (!cmd) continue;
            printf("< command %d\n", i);
            int count = 0;
            while (cmd) {
                printf("<< sub comand %d\n", count++);
                print_cmd_args(cmd);
                cmd = cmd->next;
            }
        }
#endif /* ARG_DEBUG */
        if (status == 0) /* Execute cmd after we process the input successfully */
            status = execute_cmd_list();
        if (status == 0) 
            printf("\n<<< Exit Code >>>: " YELLOW "%d" RESET " -> " GREEN "success" RESET "\n\n", status);
        else
            printf("\n<<< Exit Code >>>: " YELLOW "%d" RESET " -> " RED "failure" RESET "\n\n", status);

        memset(input, 0, sizeof(input));
        free_cmd_list();
        status = 0;
    }
}

int main(void)
{
    /* Initialize */
    shell_pgrp = getpgrp();
    for (int i = 0; i < MAX_JOBS; i++)
        jobs[i] = INVALID_JOBID;

    for (int i = 0; i < MAX_CMD_CNT; i++)
        cmd_list[i] = NULL;

    /* Set signal handler */
#ifdef SIG_DEBUG
    printf("parent process <%u>\n", getpid());
#endif /* SIG_DEBUG */
    set_signal_handler(SIGINT,  SIG_IGN);
    set_signal_handler(SIGUSR1, SIG_IGN);
    set_signal_handler(SIGUSR2, SIG_IGN);

    atexit(clear_up);

    loop();

    return 0;
}
