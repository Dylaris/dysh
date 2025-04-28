#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#define SIGNAL_CONVERT(sig_str, res) do { \
             if (strcmp((sig_str), "SIGCONT") == 0) *(res) = SIGCONT; \
        else if (strcmp((sig_str), "SIGSTOP") == 0) *(res) = SIGSTOP; \
        else if (strcmp((sig_str), "SIGKILL") == 0) *(res) = SIGKILL; \
        else { \
            fprintf(stderr, "not yet implement it\n"); \
            exit(1); \
        } \
    } while (0)

int main(int argc, char **argv)
{
    if (argc < 3 || !atoi(argv[2])) {
        fprintf(stderr, "Usage: kill -SIG*** pid1 pid2 ...\n");
        return 1;
    }

    int sig = -1;
    char *sig_str = (char *) argv[1] + 1;   // skip '-'
    if (isdigit(sig_str[0])) sig = atoi(sig_str);
    else SIGNAL_CONVERT(sig_str, &sig);

    for (int i = 2; i < argc; i++) {
        int pid = atoi(argv[i]);
        kill(pid, sig);
    }

    return 0;
}
