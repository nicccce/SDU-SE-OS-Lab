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

    - 如果不等待的话，会导致子进程的pid一直**不被回收**

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
          wait(NULL); // 等待任意子进程结束 （里面参数可以填一个int指针存子进程退出状态，null表示不关心）
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



#### 1.**代码结构分析**

**头文件 `pctl.h`**

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



**主程序 `pctl.c`**（核心逻辑）

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



#### 2.**关键代码解析**

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



#### **3.实验现象与原理**

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





---



## **实验二：进程通信**

---



### **核心概念与系统调用**

---

#### **1. 管道创建与基本操作**

• **`pipe()`**

  • **功能**：创建无名管道，返回两个文件描述符。

  • **参数**：`int pipefd[2]`，其中：
    ◦ `pipefd[0]`：管道的**读端**。
    ◦ `pipefd[1]`：管道的**写端**。

  • **返回值**：成功返回 `0`，失败返回 `-1`。

  • **特点**：管道是**半双工**的，需两个管道实现双向通信。

  ```c
  #include <unistd.h>
  int main() {
      int pipefd[2];
      if (pipe(pipefd) == -1) {
          perror("pipe");
          exit(EXIT_FAILURE);
      }
      // pipefd[0] 用于读，pipefd[1] 用于写
  }
  ```



**管道泄漏**

`lsof` 显示未关闭的描述符。

  ```bash
lsof -p <PID> | grep pipe  # 检查泄漏的描述符
  ```



#### **2. 管道读写**

- **`read()` **

```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t count);
```

**参数**：

- `fd`：管道读端文件描述符（`pipefd[0]`）。
- `buf`：存储读取数据的缓冲区地址。
- `count`：期望读取的最大字节数。

**返回值**：

- **成功**：返回实际读取的字节数（可能小于 `count`）。
- **返回 0**：表示到达文件末尾（所有写端已关闭，且缓冲区无数据）。
- **失败**：返回 `-1`，并设置 `errno`。

|          **场景**          |                           **行为**                           |
| :------------------------: | :----------------------------------------------------------: |
|         管道有数据         | 立即返回数据，读取字节数 ≤ `count`。<br />（即想读八字节但管道内只有四字节时候，只会把那四字节读完，然后返回4） |
|  管道为空，且有写端未关闭  |             **阻塞**，直到有数据写入或写端关闭。             |
| 管道为空，且所有写端已关闭 |                    返回 `0`（表示 EOF）。                    |
| 非阻塞模式（`O_NONBLOCK`） |        若管道为空，立即返回 `-1`，`errno = EAGAIN`。         |



- **`write()` **

```c
#include <unistd.h>
ssize_t write(int fd, const void *buf, size_t count);
```

**参数**：

- `fd`：管道写端文件描述符（`pipefd[1]`）。
- `buf`：待写入数据的缓冲区地址。
- `count`：期望写入的字节数。

**返回值**：

- **成功**：返回实际写入的字节数（可能小于 `count`，需循环写入剩余数据）。
- **失败**：返回 `-1`，并设置 `errno`。

**关键行为**：

|          **场景**          |                        **行为**                        |
| :------------------------: | :----------------------------------------------------: |
|      管道缓冲区有空间      |   立即写入数据，返回写入字节数（通常等于 `count`）。   |
|       管道缓冲区已满       |                 **阻塞**，直到有空间。                 |
|       所有读端已关闭       | 触发 `SIGPIPE` 信号（默认终止进程），`errno = EPIPE`。 |
| 非阻塞模式（`O_NONBLOCK`） |     若缓冲区满，立即返回 `-1`，`errno = EAGAIN`。      |



- **阻塞行为**：
  - **读端**：若管道为空，`read()` 阻塞，直到有数据或写端关闭。
  - **写端**：若管道缓冲区（默认64KB）满，`write()` 阻塞，直到有空间。



- **非阻塞模式**：通过 `fcntl()` 设置 `O_NONBLOCK`。

  ```c
  // 设置非阻塞读
  fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
  // 设置非阻塞写
  fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
  ```



- **`close()` **

```c
#include <unistd.h>
int close(int fd);
```

**参数**：

- `fd`：要关闭的文件描述符 如`close(pipefd[0])`表示关闭读端。

**返回值**：

- **成功**：返回 `0`。
- **失败**：返回 `-1`，并设置 `errno`。

**关键规则**：

可以理解为有一个信号量在计这个管道的某端的引用数，每次`fork`时候就×2，close的时候-1。当这个信号量为0时表示所有端已经关闭

|     **场景**     |                           **操作**                           |
| :--------------: | :----------------------------------------------------------: |
|      读进程      | 必须关闭写端（`close(pipefd[1])`），否则 `read()` 可能永久阻塞。 |
|      写进程      |      必须关闭读端（`close(pipefd[0])`），避免资源泄漏。      |
| 所有进程关闭写端 |             读进程的 `read()` 返回 `0`（EOF）。              |
| 所有进程关闭读端 |             写进程的 `write()` 触发 `SIGPIPE`。              |



### **示例实验解析**

---

#### **1. 实验目标**
父子进程通过两个管道（`pipe1` 和 `pipe2`）协作，将整数 `x` 从 1 累加到 10：

- **父进程**：通过 `pipe1` 发送数据，通过 `pipe2` 接收结果。
- **子进程**：通过 `pipe1` 接收数据，处理后通过 `pipe2` 返回。

---

#### **2. 代码结构分析**
```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    int pipe1[2], pipe2[2];
    int x;

    // 创建两个管道
    pipe(pipe1);
    pipe(pipe2);

    if (fork() == 0) {  // 子进程
        close(pipe1[1]);  // 关闭 pipe1 写端
        close(pipe2[0]);  // 关闭 pipe2 读端
        do {
            read(pipe1[0], &x, sizeof(int));  // 从 pipe1 读
            printf("Child %d read: %d\n", getpid(), x++);
            write(pipe2[1], &x, sizeof(int)); // 结果写入 pipe2
        } while (x <= 9);
        close(pipe1[0]);
        close(pipe2[1]);
    } else {            // 父进程
        close(pipe1[0]);  // 关闭 pipe1 读端
        close(pipe2[1]);  // 关闭 pipe2 写端
        x = 1;
        do {
            write(pipe1[1], &x, sizeof(int)); // 数据写入 pipe1
            read(pipe2[0], &x, sizeof(int));  // 从 pipe2 读结果
            printf("Parent %d read: %d\n", getpid(), x++);
        } while (x <= 9);
        close(pipe1[1]);
        close(pipe2[0]);
    }
    return EXIT_SUCCESS;
}
```



| **步骤** | **父进程动作**        | **子进程动作**        | **x 值变化**    |
| -------- | --------------------- | --------------------- | --------------- |
| 1        | 写 `pipe1`：x=1       | 读 `pipe1`：x=1 → x=2 | 父 x=1 → 子 x=2 |
| 2        | 读 `pipe2`：x=2 → x=3 | 写 `pipe2`：x=2       | 父 x=3 → 子 x=2 |
| 3        | 写 `pipe1`：x=3       | 读 `pipe1`：x=3 → x=4 | 父 x=3 → 子 x=4 |
| ...      | ...                   | ...                   | ...             |
| 10       | 读 `pipe2`：x=10      | 退出循环              | 终止            |

---

#### **3. 关键代码解析**

- **管道方向设计**：
  - **`pipe1`**：父进程写 → 子进程读。
  -  **`pipe2`**：子进程写 → 父进程读。

- **描述符关闭逻辑**：

  - **父进程**：

  ```c
      close(pipe1[0]);  // 不需要读 pipe1
      close(pipe2[1]);  // 不需要写 pipe2
  ```

  - **子进程**：

      ``` c
      close(pipe1[1]);  // 不需要写 pipe1
      close(pipe2[0]);  // 不需要读 pipe2
      ```

  

- **循环终止条件**：

  当父进程的 `x=9` 时，写入 `pipe1`，子进程处理后返回 `x=10`，父进程读取后终止。



---



------

## **实验三：进程调度算法**

------

### **核心概念与系统调用**

------

#### **1. 调度策略**

Linux 系统提供三种调度策略：

|     **策略**      | **值** |                           **特点**                           |
| :---------------: | :----: | :----------------------------------------------------------: |
| **`SCHED_OTHER`** |   0    | 默认分时调度策略，采用动态优先级（基于时间片和 `nice` 值）。 |
| **`SCHED_FIFO`**  |   1    | 先进先出实时调度策略，高优先级进程独占 CPU 直到主动释放；同级进程按队列顺序执行。 |
|  **`SCHED_RR`**   |   2    | 时间片轮转实时调度策略，同级进程轮流执行，时间片耗尽后重新排队。 |

------

#### **2. 优先级管理**

- Linux 调度器将进程分为两个优先级层级：

  - **实时进程（Real-Time）**：使用 `SCHED_FIFO` 或 `SCHED_RR`，优先级范围为 **1~99**（数值越大优先级越高）。
  - **普通进程（Normal）**：使用 `SCHED_OTHER`，优先级是0， `nice` 值决定相对占用时间长短（范围通常为 -20~19，数值越小优先级越高，但层级低于实时进程）。

  **调度顺序总原则**：

  **​实时进程（FIFO/RR）​**​ 总是优先于 ​**​普通进程（OTHER）​**​。

  |               **场景**               |                        **调度行为**                        |
  | :----------------------------------: | :--------------------------------------------------------: |
  |        同优先级 `SCHED_FIFO`         | 严格 FIFO，先到者独占 CPU 直到主动退出或被更高优先级抢占。 |
  |         同优先级 `SCHED_RR`          |        时间片轮转，每个进程运行固定时间后让出 CPU。        |
  | 同优先级混合 `SCHED_FIFO`+`SCHED_RR` | `SCHED_FIFO` 优先运行，仅当其不运行时 `SCHED_RR` 才轮转。  |

------

#### **3. 关键系统调用**

- **设置调度策略**：

  ```c
  #include <sched.h>
  int sched_setscheduler(pid_t pid, int policy, const struct sched_param *sp);
  ```

  - 参数：

    - `pid`：目标进程 PID（`0` 表示当前进程）。

    - `policy`：调度策略（`SCHED_OTHER`、`SCHED_FIFO`、`SCHED_RR`）。

    - `sp`：指向 `sched_param` 结构体的指针，包含静态优先级。

      - ```c
        struct sched_param {
            int sched_priority;  // 进程的调度优先级（仅对实时策略有效）,值越大优先级越高
        };
        ```

  - **权限**：设置实时策略（`SCHED_FIFO`/`SCHED_RR`）需要 `CAP_SYS_NICE` 能力（通常需 root 权限）。

- **设置动态优先级**：

  ```c
  #include <sys/resource.h>
  int setpriority(int which, int who, int prio);
  ```

  - 设置的是nice值，对`SCHED_OTHER`起作用。
  - 参数：
    - `which`：作用对象（`PRIO_PROCESS` 进程、`PRIO_PGRP` 进程组、`PRIO_USER` 用户）。
    - `who`：目标进程 PID、进程组 ID 或用户 UID。
    - `prio`：动态优先级值（`-20` 到 `19`）。

- **获取调度策略**：

  ```c
  int sched_getscheduler(pid_t pid);
  ```

  - 返回值：当前进程的调度策略（`0`、`1` 或 `2`）。



------

### **示例实验解析**

------

#### **1. 实验目标**

父进程创建 3 个子进程，为每个子进程设置不同的调度策略和优先级，观察其执行顺序：

- **策略与优先级来源**：通过命令行参数指定（优先级参数 1-3，策略参数 4-6）。
- **默认行为**：未指定参数时，优先级为 `10`，策略为 `SCHED_OTHER`。

------

#### **2. 代码结构分析**

```c
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char *argv[]) {
    int pid[3];                   // 子进程 PID 数组
    struct sched_param p[3];      // 调度参数结构体数组

    for (int i = 0; i < 3; i++) {
        if ((pid[i] = fork()) > 0) {  // 父进程逻辑
            // 设置子进程静态优先级（从命令行参数或默认值 10）
            p[i].sched_priority = (argv[i+1] != NULL) ? atoi(argv[i+1]) : 10;
            // 设置调度策略（从命令行参数或默认 SCHED_OTHER）
            sched_setscheduler(pid[i], (argv[i+4] != NULL) ? atoi(argv[i+4]) : SCHED_OTHER, &p[i]);
            // 设置动态优先级（同静态优先级）
            setpriority(PRIO_PROCESS, pid[i], (argv[i+1] != NULL) ? atoi(argv[i+1]) : 10);
        } else {                      // 子进程逻辑
            sleep(1);                 // 等待父进程完成设置
            for (int j = 0; j < 10; j++) {
                // 报告当前优先级
                printf("Child PID = %d priority = %d\n", getpid(), getpriority(PRIO_PROCESS, 0));
                sleep(1);
            }
            exit(EXIT_SUCCESS);
        }
    }

    // 父进程报告子进程策略
    printf("My child %d policy is %d\n", pid[0], sched_getscheduler(pid[0]));
    printf("My child %d policy is %d\n", pid[1], sched_getscheduler(pid[1]));
    printf("My child %d policy is %d\n", pid[2], sched_getscheduler(pid[2]));
    return EXIT_SUCCESS;
}
```

------

#### **3. 关键代码逻辑**

|    **步骤**    |                        **父进程操作**                        |                **子进程操作**                |
| :------------: | :----------------------------------------------------------: | :------------------------------------------: |
| **创建子进程** |             循环调用 `fork()` 创建 3 个子进程。              |              进入子进程代码块。              |
| **设置优先级** | 根据命令行参数设置静态优先级（`p[i].sched_priority`）和动态优先级（`setpriority`）。 |                   无操作。                   |
|  **设置策略**  |     根据命令行参数设置调度策略（`sched_setscheduler`）。     |                   无操作。                   |
| **子进程执行** |                           无操作。                           | 每隔 1 秒报告 PID 和动态优先级，持续 10 秒。 |
| **父进程报告** |                   打印各子进程的调度策略。                   |                   无操作。                   |

------

### **运行结果与原理分析**

------

#### **1. 实验一：同策略不同优先级**

**命令**：

```bash
$ ./psched 10 5 -10 0 0 0  # 优先级 10, 5, -10；策略均为 SCHED_OTHER
```

**输出**：

```
Child PID = 10773 priority = -10 
Child PID = 10772 priority = 5 
Child PID = 10771 priority = 10 
...
```

**分析**：

- 在 `SCHED_OTHER` 策略下，动态优先级（`nice` 值）生效，值越小优先级越高。
- 进程 `10773` 的 `nice=-10`（最高优先级）最先执行，`10772`（`nice=5`）次之，`10771`（`nice=10`）最后。

------

#### **2. 实验二：混合策略**

**命令**：

```bash
$ ./psched 10 5 18 0 0 1  # 优先级 10, 5, 18；策略为 SCHED_OTHER, SCHED_OTHER, SCHED_FIFO
```

**输出**：

```
Child PID = 11308 priority = 18  # SCHED_FIFO 策略
Child PID = 11307 priority = 5   # SCHED_OTHER 策略
Child PID = 11306 priority = 10  # SCHED_OTHER 策略
...
```

**分析**：

- 进程 `11308` 使用 `SCHED_FIFO`（实时策略），尽管其 `nice=18`（最低动态优先级），但仍优先于 `SCHED_OTHER` 进程。
- `SCHED_FIFO` 进程独占 CPU 直到主动释放，因此 `11308` 持续排在输出首位。



---

