# 操作系统算法实验笔记



## 实验一：进程控制

---



### 核心概念与系统调用

---

#### 1. 进程创建与执行

- **`fork()`**

  - 功能：创建子进程，子进程复制父进程的代码段、数据段和堆栈。

  - 返回值：父进程返回子进程 PID，子进程返回 0，失败返回 -1。

  - 特点：父子进程并发执行，共享文件描述符等资源。

    ```c++
    #include <unistd.h>  
    int main() {  
        pid_t pid = fork();  
        if (pid == 0) {  
            printf("Child PID: %d\n", getpid());  // 子进程  
        } else {  
            printf("Parent PID: %d\n", getpid()); // 父进程  
        }  
    }  
    ```

    

- **`execve()`**

  - 功能：加载新程序替换当前进程映像，执行新程序。

  - 参数：`path`（程序路径）、`argv[]`（命令行参数）、`envp[]`（环境变量）。

  - 特点：调用成功后**不返回**原进程，失败返回 -1。

    ```c++
    #include <unistd.h>  
    int main() {  
        char *args[] = {"/bin/ls", "-l", NULL};  
        execve(args[0], args, NULL);  // 替换为 ls -l  
        // 若执行失败才会执行下一行  
        perror("execve failed");  
    }  
    ```

    

- **`waitpid()`**

  - 功能：父进程等待指定子进程结束，并回收其资源。

  - 参数：

    - `pid > 0`：等待指定 PID 的子进程。

    - `pid = -1`：等待任意子进程。

    - `option`：控制等待行为。

      a.`WNOHANG`：非阻塞等待（立即返回未结束的子进程状态）。

      b.`WUNTRACED`：报告被暂停的子进程。

    ```c++
    #include <sys/wait.h>  
    int main() {  
        pid_t pid = fork();  
        if (pid == 0) {  
            sleep(2);  
            _exit(123);  // 子进程退出码123  
        } else {  
            int status;  
            waitpid(pid, &status, 0);  
            printf("Child exit code: %d\n", WEXITSTATUS(status)); // 输出123  
        }  
    }  
    ```

  > ###  状态检查宏
  >
  > |          宏           |                    用途                     |
  > | :-------------------: | :-----------------------------------------: |
  > |  `WIFEXITED(status)`  |           判断子进程是否正常退出            |
  > | `WEXITSTATUS(status)` | 获取子进程退出码（仅当WIFEXITED为真时有效） |
  > | `WIFSIGNALED(status)` |          判断子进程是否被信号终止           |
  > |  `WTERMSIG(status)`   |          获取终止子进程的信号编号           |
  > | `WIFSTOPPED(status)`  |            判断子进程是否被暂停             |
  > |  `WSTOPSIG(status)`   |         获取使子进程暂停的信号编号          |

  

  - **`wait()`**

    - 功能：等待任意子进程结束（简化版waitpid）（不一定是所有子进程）

      ```c
      #include <sys/wait.h>
      int main() {
          if (fork() == 0) {
              sleep(1);
              _exit(0);
          }
          wait(NULL); // 等待任意子进程结束
          printf("Child exited\n");
      }
      ```

      

  - **`getpid()`/`getppid()`**

    - 功能：获取当前进程/父进程PID

      ```c
      #include <unistd.h>
      int main() {
          printf("My PID:%d, Parent PID:%d\n", getpid(), getppid());
      }
      ```

      

#### 2. 进程控制与信号

- **信号处理机制**

  - 信号来源：键盘中断（`SIGINT`）、定时器（`SIGALRM`）、系统调用（`SIGKILL`）。
  - 处理方式：忽略（`SIG_IGN`）、默认处理（如终止进程）、自定义处理函数。

  

- **关键系统调用**

  - **`kill(pid, sig)`**：向指定进程发送信号。

    - 可以通过 `kill -l`查看系统当前的信号集合
  
    ```cpp
    #include <signal.h>  
    int main() {  
        pid_t pid = fork();  
        if (pid == 0) {  
            pause();  // 子进程暂停  
        } else {  
            sleep(1);  
            kill(pid, SIGINT);  // 发送中断信号  
        }  
    }  
    ```

    
  
  - **`pause()`**：挂起进程，直到收到信号。
  
    - `pause()` **总是返回 `-1`**，并设置 `errno = EINTR`（表示被信号中断）。
  
    ```c++
    #include <stdio.h>
    #include <unistd.h>
    #include <signal.h>
    
    void handler(int sig) {
        printf("Caught signal %d\n", sig);
    }
    
    int main() {
        signal(SIGINT, handler); 
        printf("Pausing... (Press Ctrl+C to interrupt)\n");
        pause();
        printf("pause() returned (errno = EINTR)\n");
        return 0;
    }
    ```
  
    

  - **`signal(sig, handler)`**：注册信号处理函数（如自定义的 `sigcat`）。

    ```cpp
    #include <signal.h>  
    void handler(int sig) {  
        printf("Caught signal %d\n", sig);  
    }  
    int main() {  
        signal(SIGINT, handler);  // 捕获 Ctrl+C  
        while(1);  // 死循环，测试信号处理  
       	/*
       	输入 Ctrl+C ： 
    		Caught signal 2 
        */
    }  
    ```
  
    
  
  - **`sleep()`**：使进程休眠指定秒数
  
    ```c
    #include <unistd.h>
    int main() {
        printf("Sleeping...\n");
        sleep(3); // 休眠3秒
        printf("Awake!\n");
    }
    ```
  
    
  
  - **`alarm()`**：设置定时器信号
  
    ```c
    #include <unistd.h>
    #include <signal.h>
    void handler(int sig) {
        printf("Alarm!\n");
    }
    int main() {
        signal(SIGALRM, handler);
        alarm(3); // 3秒后发送SIGALRM
        pause();
    }
    ```
  
  

### **示例实验解析**

---



#### **代码结构分析**

**1. 头文件 `pctl.h`**

```c
#include <sys/types.h> 
#include <wait.h> 
#include <unistd.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 

// 自定义信号处理函数类型（兼容性定义）
typedef void (*sighandler_t) (int); 

// SIGINT 信号处理函数
void sigcat() { 
    printf("%d Process continue\n", getpid());  // 打印进程继续信息
}
```
• **作用**：定义信号处理函数 `sigcat`，当进程收到 `SIGINT`（如 `Ctrl+C`）时，打印进程继续信息。



**2. 主程序 `pctl.c`**（核心逻辑）

```c
#include "pctl.h" 

int main(int argc, char *argv[]) { 
    int i; 
    int pid;          // 子进程 PID
    int status;       // 子进程退出状态
    char *args[] = {"/bin/ls", "-a", NULL};  // 子进程默认执行的命令

    // 注册 SIGINT 信号处理函数
    signal(SIGINT, (sighandler_t)sigcat); 

    // 创建子进程
    pid = fork(); 

    // 处理 fork 失败
    if (pid < 0) { 
        printf("Create Process fail!\n"); 
        exit(EXIT_FAILURE); 
    } 

    // 子进程逻辑
    if (pid == 0) { 
        // 打印父子进程信息
        printf("I am Child process %d\nMy father is %d\n", getpid(), getppid()); 

        // 挂起子进程，等待信号唤醒
        pause(); 

        // 子进程被唤醒后继续执行
        printf("%d child will Running: ", getpid()); 

        // 根据命令行参数决定执行命令
        if (argv[1] != NULL) { 
            // 打印用户输入的命令参数
            for (i = 1; argv[i] != NULL; i++) 
                printf("%s ", argv[i]); 
            printf("\n"); 

            // 执行用户指定的命令（如 `/bin/ls -l`）
            status = execve(argv[1], &argv[1], NULL); 
        } else { 
            // 执行默认命令 `/bin/ls -a`
            for (i = 0; args[i] != NULL; i++) 
                printf("%s ", args[i]); 
            printf("\n"); 
            status = execve(args[0], args, NULL); 
        } 
    } 
    // 父进程逻辑
    else { 
        printf("\nI am Parent process %d\n", getpid()); 

        // 如果命令行有参数，等待子进程结束
        if (argv[1] != NULL) { 
            waitpid(pid, &status, 0);  // 阻塞等待子进程退出
            printf("\nMy child exit! status = %d\n\n", status); 
        } 
        // 如果无参数，唤醒子进程后直接退出（不等待）
        else { 
            sleep(3);  // 模拟耗时操作（不可靠的同步方式）
            if (kill(pid, SIGINT) >= 0)  // 发送 SIGINT 唤醒子进程
                printf("%d Wakeup %d child.\n", getpid(), pid); 
            printf("%d don't Wait for child done.\n\n", getpid()); 
        } 
    } 
    return EXIT_SUCCESS; 
}
```



**Makefile**

```makefile
# 定义变量
head = pctl.h
srcs = pctl.c
objs = pctl.o
opts = -g -c

# 默认目标：编译生成可执行文件 pctl
all: pctl

# 生成可执行文件 pctl，依赖目标文件 pctl.o
pctl: $(objs)
    gcc $(objs) -o pctl

# 生成目标文件 pctl.o，依赖源文件 pctl.c 和头文件 pctl.h
pctl.o: $(srcs) $(head)
    gcc $(opts) $(srcs)

# 清理编译生成的文件
clean:
    rm pctl *.o
```



#### **关键代码解析**

**`argv` **

`argv` 是 C 语言中 `main` 函数的参数之一，表示**命令行参数**（Command-Line Arguments）：

- **定义**：`char *argv[]` 是一个字符串数组，存储用户在运行程序时输入的参数。
- 索引规则
  - `argv[0]`：程序自身的名称（如 `./pctl`）。
  - `argv[1]`、`argv[2]`...：用户输入的参数。
  - `argv[argc]`：固定为 `NULL`，表示参数结束。

**示例**：

当用户执行以下命令：

```bash
$ ./pctl /bin/ls -l
```

程序中的 `argv` 数组内容为：

```cpp
argv[0] = "./pctl"   // 程序名
argv[1] = "/bin/ls"  // 用户输入的第一个参数
argv[2] = "-l"       // 用户输入的第二个参数
argv[3] = NULL       // 参数结束标志
```

 

 **`execve` 参数分析**

在示例代码中，`execve` 的用法有两种场景：

**场景 1：用户输入子进程命令**

```c
status = execve(argv[1], &argv[1], NULL);
```

如果用户输入 `./pctl /bin/ls -l`，则：

- `argv[1] = "/bin/ls"`（命令路径）
- `&argv[1]` 指向 `{"/bin/ls", "-l", NULL}`（参数数组）

此时，`execve` 执行的命令为：

```
/bin/ls -l
```

- **关键点**：`argv` 数组的**第一个元素必须是程序自身路径**（`/bin/ls`），后续元素才是参数（`-l`）。

------

**场景 2：默认执行命令**

```c
status = execve(args[0], args, NULL);
```

其中 `args` 的定义为：

```c
char *args[] = {"/bin/ls", "-a", NULL}; // 默认命令
```

- `pathname = args[0] = "/bin/ls"`（程序路径）
- `argv[] = args`：参数数组为 `{"/bin/ls", "-a", NULL}`

此时，`execve` 执行的命令为：

```
/bin/ls -a
```



- 看似“重复” `/bin/ls`，但这是 Unix 的约定：

  参数数组的**第一个元素**必须是**被执行程序的路径**（即使不传递参数也要保留）。

  即当我输入`/bin/ls -l`，第一个参数（即路径`"/bin/ls"`）也是被当做参数列表第0位传入的，`argv[1]`才是`"-l"`



#### **实验现象与原理**

**1. 不带参数执行（默认行为）**

```bash
$ ./pctl
```
• **输出**：
  ```
  I am Parent process 4112 
  I am Child process 4113 
  My father is 4112 
  4112 Wakeup 4113 child.
  4112 don't Wait for child done.
  4113 Process continue 
  4113 child will Running: /bin/ls -a 
  . .. Makefile pctl pctl.c pctl.h pctl.o 
  ```
• **原理**：
  1. 父进程创建子进程后，休眠 3 秒，发送 `SIGINT` 唤醒子进程。
  2. 父进程直接退出，子进程成为**孤儿进程**，由 `init` 进程接管。
  3. 子进程执行默认命令 `/bin/ls -a` 后退出。

**2. 带参数执行（同步等待）**

```bash
$ ./pctl /bin/ls -l
```
• **输出**：
  ```
  I am Child process 4223 
  My father is 4222 
  I am Parent process 4222 
  4222 Waiting for child done.
  ^C4222 Process continue 
  4223 Process continue 
  4223 child will Running: /bin/ls -l 
  total 1708 
  -rw-r--r-- 1 root root 176 May 8 11:11 Makefile 
  -rwxr-xr-x 1 root root 8095 May 8 14:08 pctl 
  ...
  My child exit! status = 0 
  ```
• **原理**：
  1. 父进程创建子进程后，调用 `waitpid` 阻塞等待子进程退出。
  2. 用户按下 `Ctrl+C`，父子进程同时收到 `SIGINT`，唤醒子进程。
  3. 子进程执行 `/bin/ls -l` 后退出，父进程回收其状态。

**3. 进程状态与操作**

• **`Ctrl+Z`** 发送 `SIGTSTP`，暂停进程并放入后台。
• **`ps -l`** 显示进程状态 `T`（暂停）。
• **`fg`** 将进程恢复到前台，继续执行。



------

### 四、独立实验任务

**目标**：编写父子协作程序，子进程每隔 3 秒显示当前目录文件名。

#### 实现思路

1. 

   父进程逻辑

   ：

   - 创建子进程后，通过循环和 `sleep(3)` 控制定时发送信号（如 `SIGALRM`）。

2. 

   子进程逻辑

   ：

   - 注册信号处理函数，捕获父进程发送的信号。
   - 在信号处理函数中调用 `execve("/bin/ls", {"ls", "-a", NULL}, NULL)`。

#### 代码片段示例

```c
// 子进程信号处理函数  
void list_dir(int sig) {  
    execve("/bin/ls", (char *[]){"ls", "-a", NULL}, NULL);  
}  

// 父进程定时发送信号  
while (1) {  
    sleep(3);  
    kill(child_pid, SIGUSR1);  // 自定义信号  
}  
```

------

### 五、实验思考与总结

#### 1. 进程特性与操作系统实现

- **并发性**：通过 `fork()` 实现父子进程并发执行，CPU 时间片轮转体现并发性。
- **动态性**：进程状态随系统调用动态变化（如运行→暂停→就绪）。
- **独立性**：父子进程拥有独立地址空间（写时复制机制）。

#### 2. 信号机制的本质

- **软中断**：信号是用户态与内核态间的异步通信机制，通过中断处理函数响应事件。
- **控制权转移**：进程收到信号后，若未忽略或阻塞，则暂停当前代码执行，跳转至信号处理函数。

#### 3. 实验启示

- **进程创建与替换**：`fork()` + `execve()` 是 UNIX 执行新程序的经典模式（如 Shell 执行命令）。
- **孤儿进程处理**：父进程提前终止时，子进程由 init 进程接管，避免僵尸进程。
- **信号的应用场景**：定时任务、进程间同步、异常处理（如 `SIGSEGV` 处理段错误）。

------

**附注**：实验中需注意编译命令 `gcc -g` 保留调试信息，以及通过 `strace` 工具跟踪系统调用执行流程，深入理解操作系统底层行为。