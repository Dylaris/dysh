#ifndef _DYSH_PARSER_H_
#define _DYSH_PARSER_H_

#include <stddef.h>

#define ARG_DELIM   " \t"
#define PIPE_DELIM  "|"
#define CMD_DELIM   ";"
#define MAX_CMD_CNT 5

typedef char *CmdArg;

typedef struct Command {
    size_t count;           /* count of arg */
    CmdArg *args;           /* args list */
    struct Command *next;   /* used for piping */
} Command;

extern Command *cmd_list[MAX_CMD_CNT];

Command *new_cmd(void);
void free_cmd(Command *cmd);
void free_cmd_list(void);
void append_cmd_arg(Command *cmd, CmdArg arg);
void print_cmd_args(Command *cmd);
void split_by_pipe(void);
void split_by_semicon(char *input);

#endif /* _DYSH_PARSER_H_ */