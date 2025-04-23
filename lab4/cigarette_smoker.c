#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>

#define NUM_SMOKERS 3
#define SEM_PERM 0666

// 全局变量用于信号处理
pid_t child_pids[NUM_SMOKERS];
int tobacco_sem, paper_sem, match_sem, smoke_sem;

// 信号量操作辅助宏
#define SEM_OP(semid, op) do { \
    struct sembuf sb = {0, (op), SEM_UNDO}; \
    if(semop(semid, &sb, 1) == -1) { \
        perror("semop"); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

// 清理函数
void cleanup() {
    semctl(tobacco_sem, 0, IPC_RMID);
    semctl(paper_sem, 0, IPC_RMID);
    semctl(match_sem, 0, IPC_RMID);
    semctl(smoke_sem, 0, IPC_RMID);
}

// 信号处理函数
void sig_handler(int sig) {
    // 终止所有子进程
    for(int i = 0; i < NUM_SMOKERS; i++) {
        if(child_pids[i] > 0) kill(child_pids[i], SIGTERM);
    }
    cleanup();
    exit(EXIT_SUCCESS);
}

void producer() {
    const char* materials[] = {"tobacco", "paper", "matches"};
    int sem_ids[] = {tobacco_sem, paper_sem, match_sem};

    while(1) {
        for(int i = rand() % 3;1; i = rand() % 3) { // 随机选择材料
            // 等待生产者放置材料
            printf("\nProducer placing %s and %s\n", materials[(i+1)%3], materials[(i+2)%3]);
            
            // 提供材料
            SEM_OP(sem_ids[i], 1);
            
            // 等待吸烟完成
            SEM_OP(smoke_sem, -1);
        }
    }
}

void smoker(int sem_id, const char* name) {
    while(1) {
        // 等待材料
        SEM_OP(sem_id, -1);
        
        printf("%s is smoking\n", name);
        sleep(2);
        printf("%s finishes smoking\n", name);

        // 通知完成
        SEM_OP(smoke_sem, 1);
    }
}

int main() {
    // 创建信号量集
    tobacco_sem = semget(IPC_PRIVATE, 1, IPC_CREAT | SEM_PERM);
    paper_sem = semget(IPC_PRIVATE, 1, IPC_CREAT | SEM_PERM);
    match_sem = semget(IPC_PRIVATE, 1, IPC_CREAT | SEM_PERM);
    smoke_sem = semget(IPC_PRIVATE, 1, IPC_CREAT | SEM_PERM);

    // 初始化信号量值
    semctl(tobacco_sem, 0, SETVAL, 0);
    semctl(paper_sem, 0, SETVAL, 0);
    semctl(match_sem, 0, SETVAL, 0);
    semctl(smoke_sem, 0, SETVAL, 0);

    // 注册信号处理
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // 创建吸烟者进程
    const char* names[] = {"TobaccoSmoker", "PaperSmoker", "MatchSmoker"};
    int sem_ids[] = {tobacco_sem, paper_sem, match_sem};

    for(int i = 0; i < NUM_SMOKERS; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            smoker(sem_ids[i], names[i]);
            exit(EXIT_SUCCESS);
        } else if(pid > 0) {
            child_pids[i] = pid;
        } else {
            perror("fork");
            cleanup();
            exit(EXIT_FAILURE);
        }
    }

    // 父进程作为生产者
    producer();

    // 永远不会执行到这里，保持代码完整性
    cleanup();
    return 0;
}