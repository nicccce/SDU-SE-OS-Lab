#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <sched.h>
#include <sys/wait.h>

volatile sig_atomic_t stop = 0;

// 信号处理函数：调整优先级
void adjust_priority(int sig) {
    int old_prio = getpriority(PRIO_PROCESS, 0);
    int new_prio = old_prio;

    switch (sig) {
        case SIGINT:  // 优先级+1
            new_prio++;
            break;
        case SIGTSTP: // 优先级-1
            new_prio--;
            break;
    }

    // 限制优先级在合法范围 [-20, 19]
    if (new_prio < -20) new_prio = -20;
    if (new_prio > 19) new_prio = 19;

    if (setpriority(PRIO_PROCESS, 0, new_prio) == 0) {
        printf("Process %d: Priority adjusted from %d to %d\n", 
               getpid(), old_prio, new_prio);
    } else {
        perror("setpriority failed");
    }
}

// 打印进程信息
void print_info() {
    printf("PID: %d | Priority: %d | Policy: %s\n",
           getpid(),
           getpriority(PRIO_PROCESS, 0),
           (sched_getscheduler(0) == SCHED_OTHER) ? "SCHED_OTHER" : sched_getscheduler(0) == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_RR");
}


pid_t child_pid;
pid_t parent_pid;

void stop_handler(int sig) {
    puts("Stopping process...\n");
    if(child_pid != 0) {
        kill(parent_pid, 9);
    }else{
        kill(child_pid, 9);
    }
    exit(0);
}

int main() {
    // 注册信号处理函数
    signal(SIGINT, adjust_priority);
    signal(SIGTSTP, adjust_priority);
    signal(SIGTERM, stop_handler);

    parent_pid = getpid();

    child_pid = fork();
    if (child_pid == 0) {
        // 子进程：每秒打印信息
        while (!stop) {
            print_info();
            sleep(1);
        }
        exit(0);
    } else {
        // 父进程：每2秒打印信息
        while (!stop) {
            print_info();
            sleep(2);
        }
        wait(NULL);
    }
    return 0;
}