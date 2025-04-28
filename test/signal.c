#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

static void set_signal_handler(int sig, void (*handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, NULL);
}

static void signal_handler(int sig)
{
    switch (sig) {
    case SIGUSR1:   /* same to SIGSTOP */
        printf("Process <%u> received SIGUSR1!\n", getpid());
        kill(getpid(), SIGSTOP);
        break;
    case SIGUSR2:   /* same to SIGCONT */
        printf("Process <%u> received SIGUSR2!\n", getpid());
        kill(getpid(), SIGCONT);
        break;
    case SIGINT:
        printf("Process <%u> received SIGINT!\n", getpid());
        break;
    default: 
        break;
    }
}

int main(void)
{
    printf("parent process: %u\n", getpid());
    set_signal_handler(SIGINT, signal_handler);

    pid_t pid = fork();
    if (pid == 0) {
        printf("child process: %u\n", getpid());
        set_signal_handler(SIGINT, signal_handler);
        sleep(10);
        // if (errno == EINTR) printf("system call interrupt\n");
        exit(0);
    }
    wait(NULL);
    sleep(10);
    if (errno == EINTR) printf("system call interrupt\n");
    for (int i = 0; i < 10; i++) printf("%d\n", i);

    return 0;
}
