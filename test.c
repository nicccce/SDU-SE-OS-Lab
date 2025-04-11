#include <unistd.h>  

int main() {  
    pid_t pid = fork();  
    if (pid == 0) {  
        printf("Child PID: %d\n", getpid());  // 子进程  
    } else {  
        printf("Parent PID: %d\n", getpid()); // 父进程  
    }
    return 0;
}  