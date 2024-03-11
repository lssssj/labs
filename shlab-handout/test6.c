#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

static void sigchld(int unused) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    printf("Child %d exited with status %04x\n", pid, status);
    }
}
int main(void) {
    int*p = (int*)malloc(1);
    printf("%p\n", p);
    p = (int*)malloc(1);
    printf("%p\n", p);
    p = (int*)malloc(1);
    printf("%p\n", p);
    p = (int*)malloc(1);
    printf("%p\n", p);
    p = (int*)malloc(1);
    printf("%p\n", p);
    p = (int*)malloc(1);
    printf("%p\n", p);
    p = (int*)malloc(1);
    printf("%p\n", p);
    p = (int*)malloc(1);
    printf("%p\n", p);
    p = (int*)malloc(1);
    printf("%p\n", p);
}