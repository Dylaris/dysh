#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUF_SIZE 1024

typedef struct Cmd {
    int fin;  // file stdin
    int fout; // file stdout

    // Dynamic array for args
    size_t count;
    size_t capacity;
    char **args;

    struct Cmd *next;
    int builtin_idx;
} Cmd;
static Cmd *head = NULL;
static Cmd *tail = NULL;

static void __hello(void)
{
    printf("Hello, World\n");
}

typedef void (*BuiltinFunc)(void);
typedef struct BuiltinCmd {
    char *name;
    BuiltinFunc func;
} BuiltinCmd;

static BuiltinCmd builtin_cmds[] = {
    // name, builtin_func
    {"hello", __hello}
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
    // Parse the command and create a list of piped commands
    char *cmd_token = strtok(input, "|");
    while (cmd_token) {
        Cmd *cmd = new_cmd();
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
        func();
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

    Cmd *cur = head;
    int prev_pipe[2] = {-1, -1};
    int next_pipe[2] = {-1, -1};

    while (cur) {
        // Not the last command
        if (cur != tail) {
            if (pipe(next_pipe) == -1)  {
                perror("pipe");
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        } else if (pid == 0) {
            if (head != tail) pipe_redirect(cur, prev_pipe, next_pipe);
            file_redirect(cur);
            execute_cmd(cur);
        } else {
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
    while (wait(NULL) > 0);
}

static void loop(void)
{
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
