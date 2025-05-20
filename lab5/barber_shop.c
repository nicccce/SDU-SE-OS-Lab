#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_BARBER 3
#define NUM_ROOM 20
#define NUM_SOFA 4
#define SIZE_NAME 20

#define MSQ_ROOM 1
#define MSQ_SOFA 2

int msqid;
int semid;

// 信号量操作辅助宏
#define SEM_OP(id, op)                           \
    do {                                         \
        struct sembuf sb = {id, (op), SEM_UNDO}; \
        if (semop(semid, &sb, 1) == -1) {       \
            perror("semop");                     \
            exit(EXIT_FAILURE);                  \
        }                                        \
    } while (0)

#define SEM_ROOM 0
#define SEM_SOFA 1
#define SEM_ACCOUNTBOOK 2

struct msgbuf {
    long mtype;
    char mtext[SIZE_NAME];
};

int keep_running = 1;
pid_t children[NUM_BARBER + 2];  // 3理发师 + sofa进程 + 主进程分支
int child_count = 0;

void cleanup() {
    msgctl(msqid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
}

void sigint_handler(int sig) {
    keep_running = 0;
    // 清理流程
    for (int i = 0; i < child_count; i++) {
        printf("killing %d\n",children[i]);
        kill(children[i], 9);
        waitpid(children[i], NULL, 0);
    }
    cleanup();
    exit(0);
}

int barber(char *name) {
    struct msgbuf customer;
    while (1) {
        if (msgrcv(msqid, &customer, sizeof(customer.mtext), MSQ_SOFA, IPC_NOWAIT) == -1) {
            printf("%s is sleeping.\n", name);
            msgrcv(msqid, &customer, sizeof(customer.mtext), MSQ_SOFA, 0);
        }
        SEM_OP(SEM_SOFA, 1);
        printf("%s is giving %s a haircut.\n", name, customer.mtext);
        sleep(10);
        SEM_OP(SEM_ACCOUNTBOOK, -1);
        printf("%s is using accountbook.\n", name);
        sleep(1);
        SEM_OP(SEM_ACCOUNTBOOK, 1);
    }
}

int main() {
    printf("pid:%d\n",getpid());
 
    // 创建IPC资源
    msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);

    semctl(semid, SEM_ROOM, SETVAL, NUM_ROOM);
    semctl(semid, SEM_SOFA, SETVAL, NUM_SOFA);
    SEM_OP(SEM_ACCOUNTBOOK, 1);

    // 创建理发师进程
    for (int i = 0; i < NUM_BARBER; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char name[] = { 'T','o','n','y', '1'+i, 0};
            barber(name);
            exit(EXIT_SUCCESS);
        }
        children[child_count++] = pid;
    }

    // 创建沙发管理进程
    pid_t sofa_pid = fork();
    if (sofa_pid == 0) {
        struct msgbuf customer;
        while (1) {
            msgrcv(msqid, &customer, sizeof(customer.mtext), MSQ_ROOM, 0);
            SEM_OP(SEM_SOFA, -1);
            SEM_OP(SEM_ROOM, 1);
            customer.mtype = MSQ_SOFA;
            printf("%s sits on the sofa.\n", customer.mtext);
            msgsnd(msqid, &customer, sizeof(customer.mtext), 0);
        }
    } else {
        children[child_count++] = sofa_pid;
    }

    // 使用 signal() 设置信号处理
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    // 主进程处理客户输入
    struct msgbuf customer;
    struct sembuf sb = {SEM_ROOM, -1, IPC_NOWAIT};
    while (keep_running) {
        scanf("%s", customer.mtext);
        if (semop(semid, &sb, 1) != -1) {
            customer.mtype = MSQ_ROOM;
            printf("%s is in the waiting room.\n", customer.mtext);
            msgsnd(msqid, &customer, sizeof(customer.mtext), 0);
        } else {
            printf("Waiting room is full, %s leaves.\n", customer.mtext);
        }
    }

    return 0;
}
