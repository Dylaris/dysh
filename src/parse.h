#ifndef _DYSH_PARSER_H_
#define _DYSH_PARSER_H_

#include <stddef.h>

typedef char *CmdArg;

typedef struct CmdList {
    size_t count;
    CmdArg *args;
    struct CmdList *next_cmd;
} CmdList;

CmdList *new_cmd_list(void);
void free_cmd_list(CmdList *cmd_list);
void append_cmd_arg(CmdList *cmd_list, CmdArg arg);
void print_cmd_args(CmdArg *args);

#endif /* _DYSH_PARSER_H_ */