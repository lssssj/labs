#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

void signal_handler(int signo) {
    printf("Received signal: %d in process %d\n", signo, getpid());
}

int main() {
    printf("main start\n");
    signal(SIGUSR1, signal_handler);

    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Fork error");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            signal(SIGUSR1, signal_handler);
            printf("child %d start\n", getpid());
            sleep(30);
            exit(i);
        } 
    }
    for (int i = 0; i < 3; ++i) {
      int status;
        pid_t terminated_pid = wait(&status);

        if (terminated_pid > 0) {
            if (WIFEXITED(status)) {
                printf("Child process %d terminated with status %d\n", terminated_pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Child process %d terminated by signal %d\n", terminated_pid, WTERMSIG(status));
            }
        }
    }

    printf("All child processes have terminated. Exiting...\n");

    return 0;
}