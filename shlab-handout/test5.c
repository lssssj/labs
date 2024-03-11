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
    
    pid_t pid = fork();
    if (pid == 0) {
        // 在子进程中，立即退出
        printf("child %d\n", getpid());
        sleep(20);
        exit(0);
    }
    printf("wait\n");
    pid_t x = waitpid(pid, NULL, 0);
    printf("child exit %d\n", x);

    return 0;
}
