#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

volatile pid_t pid;
void handler(int signo) {
    printf("here\n");
    pid = waitpid(-1, NULL, 0);
    printf("in hanlder %d\n", pid);
}

int main() {
    sigset_t mask, prev;
    signal(SIGCHLD, handler);

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    // 阻塞 SIGCHLD 信号
    sigprocmask(SIG_BLOCK, &mask, &prev);

    if (fork() == 0) {
        // 在子进程中，立即退出
        printf("child %d\n", getpid());
        sleep(20);
        exit(0);
    }

    // 在父进程中，等待接收到 SIGCHLD 信号
    pid = 0;
    while (!pid) {
        // 挂起进程，等待接收到 SIGCHLD 信号
        sigsuspend(&prev);
    }

    // 此处表示接收到了 SIGCHLD 信号
    printf("Received SIGCHLD signal\n");

    // 恢复之前的信号掩码
    sigprocmask(SIG_SETMASK, &prev, NULL);

    return 0;
}
