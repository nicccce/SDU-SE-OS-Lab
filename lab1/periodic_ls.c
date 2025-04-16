#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// 子进程信号处理函数
void child_handler(int sig) {
    if (sig == SIGUSR1) {
        pid_t pid = fork(); // 创建子进程的子进程执行 ls
        if (pid == 0) {
            execlp("/bin/ls", "ls", "-a", NULL); // 执行 ls -a
            exit(EXIT_FAILURE); // 如果 execlp 失败，退出子进程
        }
    }
}

void child_sigterm_handler(int sig) {
    printf("Child %d received SIGTERM, exiting...\n", getpid());
    _exit(0);
}

pid_t child_pid;  // 全局变量存储子进程PID

void cleanup_child(int sig) {
    printf("\nTerminating child process %d...\n", child_pid);
    kill(child_pid, SIGTERM);  // 先发送 SIGTERM
    sleep(1);                  // 等待子进程处理
    
    // 强制回收子进程
    int status;
    if (waitpid(child_pid, &status, 0) > 0) {
        printf("Child %d exited with status %d\n", child_pid, WEXITSTATUS(status));
    } else {
        perror("waitpid failed");
    }
    _exit(0);
}

int main() {
    pid_t pid = fork(); // 创建子进程

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE); // 如果 fork 失败，退出
    } else if (pid == 0) { 
        // 子进程代码
        signal(SIGCHLD, SIG_IGN);  // 忽略孙子进程的终止信号
        signal(SIGUSR1, child_handler); // 注册 SIGUSR1 
        signal(SIGTERM, child_sigterm_handler); // 注册 SIGTERM
        printf("Child PID: %d\n", getpid());
        while (1) pause(); // 永久挂起，等待信号
    } else {
        // 父进程代码
        printf("Parent PID: %d\n", getpid());

        child_pid = pid; // 保存子进程PID
        // 父进程注册终止信号处理
        signal(SIGINT, cleanup_child);   // Ctrl+C

        // 父进程主循环
        while (1) {
            sleep(3);
            kill(child_pid, SIGUSR1);  // 每3秒唤醒子进程
        }
    }
    return 0;
}
