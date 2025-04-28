#ifndef _DYSH_JOBS_H_
#define _DYSH_JOBS_H_

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "err.h"

#define MAX_JOBS        10
#define INVALID_JOBID   0

extern pid_t shell_pgrp;
extern pid_t jobs[MAX_JOBS];

static inline void set_signal_handler(int sig, void (*handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, NULL);
}

static inline void bring_process_to_foregound(pid_t pid)
{
    EXIT_IF(tcsetpgrp(STDIN_FILENO, pid) == -1, "bring process to foreground failed");
}

static inline void restore_terminal_to_shell()
{
    EXIT_IF(tcsetpgrp(STDIN_FILENO, shell_pgrp) == -1, "restore terminal to shell failed");
}

static inline void move_to_new_pgrp()
{
    EXIT_IF(setpgid(0, 0) == -1, "move to new pgrp failed");
}

static inline int is_job_alive(pid_t pid)
{
    return (kill(pid, 0) == 0) ? TRUE : FALSE;
}

static inline void append_job(pid_t pid)
{
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i] != INVALID_JOBID) continue;
        jobs[i] = pid;
        break;
    }
}

static inline void list_jobs(void)
{
    int count = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i] == INVALID_JOBID) continue;
        if (is_job_alive(jobs[i]) == FALSE) {
            jobs[i] = INVALID_JOBID;
            continue;
        }
        printf("job %d <pid: %u> <state: %c>\n",  count++, jobs[i], 'T');
    }
}

#endif /* _DYSH_JOBS_H_ */
