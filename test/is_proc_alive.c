#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define TRUE  1
#define FALSE 0

static inline int is_job_alive(pid_t pid)
{
    return (kill(pid, 0) == 0 && errno != ESRCH) ? TRUE : FALSE;
}

/* In this section, we determine how to verify whether a process is alive.
   We know that kill(pid, 0) sends a signal to the target process. If the
   process is not alive, it returns -1 and sets errno to ESRCH.
   It's important to note that if we send signals such as SIGCONT,
   SIGINT, or SIGKILL, the target process will terminate its execution
   and eventually become a zombie. It remains alive until we use wait()
   to clean it up. */

int main(void)
{
    printf("parent process: %u\n", getpid());

    pid_t pid = fork();
    if (pid == 0) {
        printf("child process: %u\n", getpid());
        raise(SIGSTOP);
        exit(0);
    }

    sleep(2);   // give the child a chance to stop

    printf("before kill\n");
    if (is_job_alive(pid) == TRUE) printf("alive\n");
    else printf("not alive\n");

#if 0
    printf("after kill\n");
    kill(pid, SIGCONT);
    sleep(1);   // give the child a chance to execute
    wait(NULL);
    if (is_job_alive(pid) == TRUE) printf("alive\n");
    else printf("not alive\n");
#else
    printf("after kill\n");
    kill(pid, SIGKILL);
    // kill(pid, SIGINT);
    sleep(1);   // give the child a chance to execute
    wait(NULL);
    if (is_job_alive(pid) == TRUE) printf("alive\n");
    else printf("not alive\n");
#endif

    return 0;
}
