#ifndef _DYSH_PARSER_H_
#define _DYSH_PARSER_H_

#include <stddef.h>

#define ARG_DELIM   " \t"
#define PIPE_DELIM  "|"
#define CMD_DELIM   ";"
#define MAX_CMD_CNT 5

#define RUN_IN_BG (1 << 0)
#define RUN_IN_FG (1 << 1)

typedef char *CommandArg;

struct Command {
    size_t count;           /* count of arg */
    CommandArg *args;       /* args list */
    struct Command *next;   /* used for piping */
    int redirect_fd[3];     /* used for redirecting */
    int flag;               /* some command flags */
};

extern struct Command *cmd_list[MAX_CMD_CNT];

struct Command *new_cmd(void);
void free_cmd(struct Command *cmd);
void free_cmd_list(void);
void append_cmd_arg(struct Command *cmd, CommandArg arg);
void remove_cmd_arg(struct Command *cmd, size_t idx);
void print_cmd_args(struct Command *cmd);
void split_by_pipe(void);
void split_by_space(void);
void split_by_semicolon(char *input);
int process_redirect(void);

#endif /* _DYSH_PARSER_H_ */
