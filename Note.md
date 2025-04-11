# 操作系统算法实验笔记

## 实验一：进程控制

------

### 核心概念与系统调用

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

    ```c++
    #include <unistd.h>  
    int main() {  
        printf("PID=%d paused. Try: kill -SIGCONT %d\n", getpid(), getpid());  
        pause();  // 挂起直到任意信号到达  
        printf("Resumed by signal\n");  
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

  

------

### 三、示例实验解析

#### 1. 实验程序结构

- 

  `pctrl.c`

  ```c
  int main(int argc, char *argv[]) {  
      signal(SIGINT, sigcat);  // 注册信号处理函数  
      pid = fork();  
      if (pid == 0) {  
          // 子进程：暂停并等待信号  
          printf("Child PID: %d\n", getpid());  
          pause();  
          // 根据参数执行新程序（如 /bin/ls -a）  
          execve(args[0], args, NULL);  
      } else {  
          // 父进程：唤醒子进程或等待其结束  
          sleep(3);  
          kill(pid, SIGINT);  
      }  
      return 0;  
  }  
  ```

- **`pctrl.h`** 头文件：定义信号处理函数 `sigcat`，打印进程继续执行的提示。

#### 2. 实验现象分析

- **场景1：不带参数执行 `./pctl`**
  - 父进程创建子进程后，通过 `kill(pid, SIGINT)` 唤醒子进程。
  - 子进程执行默认命令 `ls -a`，父进程不等待子进程结束，**子进程成为孤儿进程**，由 init 进程回收资源。
- **场景2：带参数执行 `./pctl /bin/ls -l`**
  - 父进程通过 `waitpid()` 等待子进程结束。
  - 子进程被唤醒后执行 `ls -l`，父进程在子进程结束后打印退出状态（`status = 0`）。

#### 3. 关键问题验证

- 

  进程状态查看

  ：

  bash

  Copy

  ```bash
  $ ps -l  
  # 输出显示父子进程均为 "T"（暂停状态），验证了 pause() 和信号唤醒机制。  
  ```

- 

  信号处理流程

  ：

  - 子进程调用 `pause()` 后挂起，等待信号。
  - 父进程发送 `SIGINT`，触发子进程的 `sigcat` 函数，打印提示并继续执行。

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

c

Copy

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