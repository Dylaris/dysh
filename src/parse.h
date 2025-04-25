#ifndef _DYSH_PARSER_H_
#define _DYSH_PARSER_H_

#include <stddef.h>

#define ARG_DELIM   " \t"
#define PIPE_DELIM  "|"
#define CMD_DELIM   ";"
#define MAX_CMD_CNT 5

typedef char *dysh_cmd_arg;

struct dysh_cmd {
    size_t count;           /* count of arg */
    dysh_cmd_arg *args;     /* args list */
    struct dysh_cmd *next;  /* used for piping */
    int redirect_fd[3];     /* used for redirecting */
};

extern struct dysh_cmd *cmd_list[MAX_CMD_CNT];

struct dysh_cmd *new_cmd(void);
void free_cmd(struct dysh_cmd *cmd);
void free_cmd_list(void);
void append_cmd_arg(struct dysh_cmd *cmd, dysh_cmd_arg arg);
void remove_cmd_arg(struct dysh_cmd *cmd, size_t idx);
void print_cmd_args(struct dysh_cmd *cmd);
void split_by_pipe(void);
void split_by_space(void);
void split_by_semicolon(char *input);
void process_redirect(void);

#endif /* _DYSH_PARSER_H_ */
