#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef long long ll;
int main(int argc, char *argv[]) {
    int pipe1[2];
    if(pipe(pipe1) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if(fork() == 0) {
        ll a;
        close(pipe1[1]); // 关闭写端
        printf("%ld\n",read(pipe1[0], &a, sizeof(a))); // 子进程读取管道
        printf("Child process read: %lld\n", a);
        close(pipe1[0]); // 关闭读端
    }else{
        int b=0;


        write(pipe1[1], &b, sizeof(b)); // 父进程写入管道
        printf("Parent process wrote: %d\n", b);

        close(pipe1[0]); // 关闭读端
        close(pipe1[1]); // 关闭写端

        wait(NULL); // 等待子进程结束
    }


    return 0;
}