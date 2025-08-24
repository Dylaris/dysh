#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>

#define BUF_SIZE 1024

typedef enum JobState {
    RUNNING,
    STOPPED,
    DONE,
} JobState;

typedef struct Job {
    pid_t pgid;
    int job_id;
    char *cmd_str;
    JobState state;
    struct Job *next;
} Job;

static Job *job_list = NULL;
static int next_job_id = 1;

typedef struct Cmd {
    int fin;  // file stdin
    int fout; // file stdout
    struct Cmd *next;
    int builtin_idx;
    bool background;

    // Dynamic array for args
    size_t count;
    size_t capacity;
    char **args;
} Cmd;

static Cmd *head = NULL;
static Cmd *tail = NULL;

static void __hello(Cmd *cmd)
{
    (void)cmd;
    printf("Hello, World\n");
}

static void __jobs(Cmd *cmd)
{
    (void)cmd;
    Job *cur = job_list;
    while (cur) {
        const char *state_str;
        switch (cur->state) {
        case RUNNING: state_str = "RUNNING"; break;
        case STOPPED: state_str = "STOPPED"; break;
        case DONE:    state_str = "DONE";    break;
        default:      state_str = "UNKNOWN"; break;
        }
        printf("[%d] %s %s\n", cur->job_id, state_str, cur->cmd_str);
        cur = cur->next;
    }
}

static void __fg(Cmd *cmd)
{
    // 2 means cmd name and NULL
    if (cmd->count < 2 + 1) {
        fprintf(stderr, "Usage: fg <jobid>\n");
        return;
    }

    int job_id = atoi(cmd->args[1]);
    Job *job = job_list;
    while (job) {
        if (job->job_id == job_id) {
            // Move job to front
            tcsetpgrp(STDIN_FILENO, job->pgid);
            // Send SIGCONT signal to process group
            kill(-job->pgid, SIGCONT);
            // Wait util the job finishes
            int status;
            waitpid(job->pgid, &status, WUNTRACED);
            // Restore the terminal control to shell
            tcsetpgrp(STDIN_FILENO, getpgrp());
            // Update job state
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                job->state = DONE;
            } else if (WIFSTOPPED(status)) {
                job->state = STOPPED;
            }
            return;
        }
        job = job->next;
    }
    fprintf(stderr, "Job %d not found\n", job_id);
}

static void __bg(Cmd *cmd)
{
    // 2 means cmd name and NULL
    if (cmd->count < 2 + 1) {
        fprintf(stderr, "Usage: bg <jobid>\n");
        return;
    }

    int job_id = atoi(cmd->args[1]); 
    Job *job = job_list;
    while (job) {
        if (job->job_id == job_id) {
            // Send SIGCONT to process group (run at background)
            kill(-job->pgid, SIGCONT);
            job->state = RUNNING;
            printf("[%d] %s &\n", job->job_id, job->cmd_str);
            return;
        }
        job = job->next;
    }
    fprintf(stderr, "Job %d not found\n", job_id);
}

typedef void (*BuiltinFunc)(Cmd*);
typedef struct BuiltinCmd {
    char *name;
    BuiltinFunc func;
} BuiltinCmd;

static BuiltinCmd builtin_cmds[] = {
    // name, builtin_func
    {"hello", __hello},
    {"jobs",  __jobs },
    {"fg",    __fg   },
    {"bg",    __bg   }
};

static void append_arg(Cmd *cmd, char *arg)
{
    if (cmd->capacity < cmd->count + 1) {
        cmd->capacity = cmd->capacity < 8 ? 8 : 2*cmd->capacity;
        cmd->args = realloc(cmd->args, sizeof(char*)*cmd->capacity);
        if (!cmd->args) {
            perror("realloc");
            exit(1);
        }
    }
    cmd->args[cmd->count++] = arg ? strdup(arg) : NULL;
}

static Cmd *new_cmd(void)
{
    Cmd *cmd = malloc(sizeof(Cmd));
    if (!cmd) {
        perror("malloc");
        exit(1);
    }
    cmd->fin = -1;
    cmd->fout = -1;
    cmd->background = false;
    cmd->count = 0;
    cmd->capacity = 0;
    cmd->args = NULL;
    cmd->next = NULL;
    cmd->builtin_idx = -1;
    return cmd;
}

static void append_cmd(Cmd *cmd)
{
    if (!head) {
        head = cmd;
        tail = cmd;
    } else {
        tail->next = cmd;
        tail = cmd;
    }
}

static void clear_list(void)
{
    Cmd *cur = head;
    while (cur) {
        Cmd *tmp = cur->next;
        for (size_t i = 0; i < cur->count; i++) {
            if (cur->args[i]) free(cur->args[i]);
        }
        free(cur->args);
        free(cur);
        cur = tmp;
    }
    head = NULL;
    tail = NULL;
}

static void add_job(pid_t pgid, const char *cmd_str, bool background) 
{
    Job *job = malloc(sizeof(Job));
    job->pgid = pgid;
    job->job_id = next_job_id++;
    job->cmd_str = strdup(cmd_str);
    job->state = background ? RUNNING : DONE;
    job->next = job_list;
    job_list = job;
    if (background) {
        printf("[%d] %d\n", job->job_id, pgid);
    }
}

static void remove_job(pid_t pgid)
{
    Job *prev = NULL, *cur = job_list;
    while (cur) {
        if (cur->pgid == pgid) {
            if (prev) {
                prev->next = cur->next;
            } else {
                job_list = cur->next;
            }
            free(cur->cmd_str);
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

static void update_job_state(pid_t pgid, JobState state)
{
    Job *job = job_list;
    while (job) {
        if (job->pgid == pgid) {
            job->state = state;
            return;
        }
        job = job->next;
    }
}

static void sigchld_handler(int sig)
{
    (void)sig;

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            remove_job(pid);
        } else if (WIFSTOPPED(status)) {
            update_job_state(pid, STOPPED);
        } else if (WIFCONTINUED(status)) {
            update_job_state(pid, RUNNING);
        }
    }
}

static void read_input(char *input, size_t bufsize)
{
    if (!fgets(input, bufsize, stdin)) {
        if (feof(stdin) != 0) {
            printf("\nSuccessfully exit dysh!\n");
            exit(0);
        } else {
            perror("fgets");
            exit(1);
        }
    }
    input[strcspn(input, "\n")] = '\0';
}

static void process_input(char *input)
{    
    // Check wheter the command will run at background
    bool background = false;
    for (int i = (int)strlen(input) - 1; i >= 0; i--) {
        switch (input[i]) {
        case ' ': 
            break;
        case '&':
            background = true;
            input[i] = '\0';
            break;
        default:
            break;
        }
        if (background) break;
    }


    // Parse the command and create a list of piped commands
    char *cmd_token = strtok(input, "|");
    while (cmd_token) {
        Cmd *cmd = new_cmd();
        cmd->background = background;
        // Temporary storage
        cmd->args = (char**)strdup(cmd_token);
        if (!cmd->args) {
            perror("strdup");
            exit(1);
        }
        append_cmd(cmd);
        cmd_token = strtok(NULL, "|");
    }

    // Parse arguments for each command
    for (Cmd *cur = head; cur != NULL; cur = cur->next) {
        // Restore cmd->args to NULL
        char *arg_buf = (char*)cur->args;
        cur->args = NULL;

        char *arg_token = strtok(arg_buf, " ");
        while (arg_token) {
            // Redirect
            if (strstr(arg_token, ">>")) {
                char *filename = strtok(NULL, " ");
                int fd = open(filename, O_CREAT | O_APPEND | O_WRONLY, 0644);
                if (fd == -1) {
                    perror("open");
                } else {
                    cur->fout = fd;
                }
            } else if (strstr(arg_token, ">")) {
                char *filename = strtok(NULL, " ");
                int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
                if (fd == -1) {
                    perror("open");
                } else {
                    cur->fout = fd;
                }
            } else if (strstr(arg_token, "<")) {
                char *filename = strtok(NULL, " ");
                int fd = open(filename, O_RDONLY);
                if (fd == -1) {
                    perror("open");
                } else {
                    cur->fin = fd;
                }
            } else {
                append_arg(cur, arg_token);
            }
            arg_token = strtok(NULL, " ");
        }
        append_arg(cur, NULL);
        if (arg_buf) free(arg_buf);

        // Check if the command is builtin command
        for (size_t i = 0; i < sizeof(builtin_cmds)/sizeof(builtin_cmds[0]); i++) {
            if (strcmp(cur->args[0], builtin_cmds[i].name) == 0) cur->builtin_idx = i;
        }
    }
}

static void file_redirect(Cmd *cmd)
{
    if (cmd->fin != -1) {
        if (dup2(cmd->fin, STDIN_FILENO) == -1) perror("dup2");
        close(cmd->fin);
        cmd->fin = -1;
    }
    if (cmd->fout != -1) {
        if (dup2(cmd->fout, STDOUT_FILENO) == -1) perror("dup2");
        close(cmd->fout);
        cmd->fout = -1;
    }
}

static void pipe_redirect(Cmd *cmd, int prev_pipe[], int next_pipe[])
{
    // Redirect read end of pipe to stdin if it is not the first command
    if (cmd != head) {
        if (dup2(prev_pipe[0], STDIN_FILENO) == -1) {
            perror("dup2");
            return;
        }
    }
    
    // Redirect write end of pipe to stdout if it is not the last command
    if (cmd->next != NULL) {
        if (dup2(next_pipe[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return;
        }
    }
    
    // Close unnecessary pipe descriptors
    if (prev_pipe[0] != -1) close(prev_pipe[0]);
    if (next_pipe[1] != -1) close(next_pipe[1]);
}

static void execute_cmd(Cmd *cmd)
{
    if (cmd->builtin_idx != -1) {
        BuiltinFunc func = builtin_cmds[cmd->builtin_idx].func;
        func(cmd);
        exit(0);
    } else {
        execvp(cmd->args[0], cmd->args);
        // Unreachable
        perror("execvp");
        exit(1);
    }
}

static void execute_input(void)
{
    // No command
    if (!head) return;

    // Critical consideration for built-in command execution:
    // When a built-in command is forked into a child process, certain job control
    // operations may fail due to process group constraints. For example:
    //   sleep 30 & ; kill -SIGSTOP <pid> ; fg 1
    // In this scenario, the 'fg' command (executed in a child process) will attempt
    // to bring the background job to the foreground and transfer terminal control.
    // However, since the child process resides in a separate process group without
    // terminal ownership, it will receive a SIGTTOU signal and suspend execution.
    //
    // To prevent this issue, we directly execute built-in commands in the shell
    // process when all of the following conditions are met:
    // 1. The command is a built-in (not an external program)
    // 2. The command is not running in the background
    // 3. The command is not part of a pipeline
    if (head->builtin_idx != -1 && !head->background && head->next == NULL) {
        BuiltinFunc func = builtin_cmds[head->builtin_idx].func;
        func(head);
        return;
    }

    Cmd *cur = head;
    int prev_pipe[2] = {-1, -1};
    int next_pipe[2] = {-1, -1};
    pid_t pgid = 0;
    char cmd_str[BUF_SIZE] = {0};

    // Build command string for job recording
    for (Cmd *cur = head; cur != NULL; cur = cur->next) {
        // Last argument is NULL
        for (size_t i = 0; i < cur->count - 1; i++) {
            if (cur->args[i]) {
                strcat(cmd_str, cur->args[i]);
                strcat(cmd_str, " ");
            }
        }
        if (cur->next) {
            strcat(cmd_str, " | ");
        }
    }

    while (cur) {
        // Not the last command
        if (cur != tail) {
            if (pipe(next_pipe) == -1)  {
                perror("pipe");
                return;
            }
        }

        // Both the parent and child processes set their process group ID, 
        // which may seem redundant at first glance. However, this is actually 
        // done to handle race conditions related to the execution order of processes.

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        } else if (pid == 0) {
            if (pgid == 0) {
                // Set the first process as the process group leader
                setpgid(0, 0);
            } else {
                // Other processes join the group
                setpgid(0, pgid);
            }

            if (head != tail) pipe_redirect(cur, prev_pipe, next_pipe);
            file_redirect(cur);
            execute_cmd(cur);
        } else {
            if (pgid == 0) {
                // Set the first process as the process group leader
                pgid = pid;
                setpgid(pid, pgid);
                // Add to job list if it is the background job
                if (cur->background) add_job(pgid, cmd_str, true);
            } else {
                // Other processes join the group
                setpgid(pid, pgid);
            }

            // Close unnecessary pipe descriptors
            if (prev_pipe[0] != -1) close(prev_pipe[0]);
            if (prev_pipe[1] != -1) close(prev_pipe[1]);

            // Close unnecessary file descriptors
            if (cur->fin  != -1) close(cur->fin);
            if (cur->fout != -1) close(cur->fout);

            // Next command
            if (cur != tail) {
                prev_pipe[0] = next_pipe[0];
                prev_pipe[1] = next_pipe[1];
            }
            cur = cur->next;
        }
    }

    // Wait all processes if it is not background job
    if (!head->background) {
        int status;
        while(waitpid(-pgid, &status, WUNTRACED) > 0) {
            if (WIFSTOPPED(status)) {
                // Add to job list if process stopped
                add_job(pgid, cmd_str, false);
                update_job_state(pgid, STOPPED);
                printf("[%d] STOPPED %s\n", next_job_id - 1, cmd_str);
                break;
            }
        }
    }
}

static void loop(void)
{
    // Set signal handler (SIGCHLD)
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    // SA_RESTART: If a system call is interrupted by a signal handler, it will be automatically restarted.
    // SA_NOCLDSTOP: Prevents the kernel from sending SIGCHLD to the parent process when a child process stops.
    //               SIGCHLD will only be sent when the child process terminates.
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    char input[BUF_SIZE] = {0};
    while (1) {
        printf("dysh> ");
        fflush(stdout);
        read_input(input, sizeof(input));
        process_input(input);
        execute_input();
#if 0
        int count = 0;
        for (Cmd *cur = head; cur != NULL; cur = cur->next) {
            count++;
            printf("cmd: ");
            for (size_t i = 0; i < cur->count; i++) {
                printf("%s ", cur->args[i]);
            }
            printf("\n");
        }
        printf("count: %d\n", count);
#endif
        clear_list();
    }
}

int main(void)
{
    loop();
    return 0;
}
