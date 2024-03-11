#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void signal_handler(int signo) {
    printf("Received signal: %d\n", signo);
}

int main() {
    // 设置信号处理函数
    signal(SIGUSR1, signal_handler);

    // 阻塞信号SIGUSR1
    sigset_t blocked_set, old_set;
    sigemptyset(&blocked_set);
    sigaddset(&blocked_set, SIGUSR1);

    // 将blocked_set中的信号添加到进程的阻塞信号集中，并将原来的阻塞信号集保存在old_set中
    sigprocmask(SIG_BLOCK, &blocked_set, &old_set);

    printf("Signal SIGUSR1 is blocked\n");

    // 休眠一段时间
    sleep(20);

    // 解除对SIGUSR1的阻塞
    sigprocmask(SIG_UNBLOCK, &blocked_set, NULL);

    printf("Signal SIGUSR1 is unblocked\n");

    // 继续休眠，此时如果有SIGUSR1信号在休眠期间发生，将被记录在未决信号集中
    sleep(20);

    return 0;
}
