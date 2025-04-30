#include <stdio.h>
#include <unistd.h>
#include "color.h"

static void exec_option_op(void)
{
    printf("\n\t%s----- OPERATOR HELP -----%s\n", CYAN, RESET);
    printf("\t%s|%s     Pipe\n", YELLOW, RESET);
    printf("\t%s;%s     Command Separator\n", YELLOW, RESET);
    printf("\t%s&%s     Background Execution\n", YELLOW, RESET);
    printf("\t%s>%s     Output Redirection\n", YELLOW, RESET);
    printf("\t%s>>%s    Append Output Redirection\n", YELLOW, RESET);
    printf("\t%s<%s     Input Redirection\n", YELLOW, RESET);
}

static void exec_option_cmd(void)
{
    printf("\n\t%s----- COMMAND HELP -----%s\n", CYAN, RESET);
	printf("\t%scat:%s  Print the content of input file\n", YELLOW, RESET);
	printf("\t%secho:%s Print the input\n", YELLOW, RESET);
	printf("\t%sexit:%s Exit dysh\n", YELLOW, RESET);
	printf("\t%shelp:%s Help for dysh command\n", YELLOW, RESET);
	printf("\t%sjobs:%s List the jobs running in background\n", YELLOW, RESET);
	printf("\t%skill:%s Send signal to specific process\n", YELLOW, RESET);
	printf("\t%stee:%s  Read from stdin and write it to stdout and specific file\n", YELLOW, RESET);
	printf("\t%swc:%s   Calculate the total byte count and line count\n", YELLOW, RESET);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: help -[op|cmd]\n");
        return 1;
    }

    const char *optfmt = "oc";
    int opt;
    opterr = 0;     // disable the error from getopt() itself

    while ((opt = getopt(argc, argv, optfmt)) != -1) {
        switch (opt) {
        case 'o': exec_option_op();  break;
        case 'c': exec_option_cmd(); break;
        case '?': break;
        default:
            fprintf(stderr, "Usage: help -[op|cmd]\n");
            return 1;
        }
    }

    return 0;
}
