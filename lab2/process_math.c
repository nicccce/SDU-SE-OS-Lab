#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

// 定义关闭管道端的宏
#define CLOSE_PIPE_END(pipe, end) do { \
    if (close(pipe[end]) == -1) { \
        perror("close"); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

#define CLOSE_ALL_PIPES() do { \
    CLOSE_PIPE_END(x_pipe, 0); CLOSE_PIPE_END(x_pipe, 1); \
    CLOSE_PIPE_END(y_pipe, 0); CLOSE_PIPE_END(y_pipe, 1); \
} while(0)

// 全局管道
int x_pipe[2];  // x管道
int y_pipe[2];  // y管道
int res_pipe[2];// 结果管道(加法可以交换，所以不关注xy的顺序，可以复用结果管道)

// 阶乘
unsigned factorial(unsigned n) {
    unsigned result = 1;
    for (; n > 1; n--) {
        result *= n;
    }
    return result;
}

// 斐波那契
unsigned fibonacci(unsigned n) {
    if (n <= 2) return 1;
    unsigned a = 1, b = 1, temp;
    for (unsigned i = 3; i <= n; i++) {
        temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

// 子进程（实现函数复用）
void child_process(int read_pipe, int write_pipe, unsigned (*calc_func)(unsigned)) {
    // 关闭不需要的管道端
    if (read_pipe == x_pipe[0]) CLOSE_PIPE_END(y_pipe, 0);   // 关闭x_pipe的写端
    else CLOSE_PIPE_END(x_pipe, 0);  // 关闭y_pipe的写端
    CLOSE_PIPE_END(x_pipe, 1);
    CLOSE_PIPE_END(y_pipe, 1);
    CLOSE_PIPE_END(res_pipe, 0);  // 关闭res_pipe的读端

    unsigned input;
    while (1) {
        ssize_t bytes = read(read_pipe, &input, sizeof(input));
        if (bytes <= 0) {
            if (bytes == 0) break;  // 正常结束
            perror("read");
            exit(EXIT_FAILURE);
        }
        
        unsigned result = calc_func(input);
        if (write(write_pipe, &result, sizeof(result)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    
    // 安全关闭管道
    close(read_pipe);
    close(write_pipe);

    puts("done");
    exit(EXIT_SUCCESS);
}

// 主进程
int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        printf("Usage: %s <test_case_num>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int test_case_num = atoi(argv[1]);
    if (test_case_num < 0||test_case_num > 1) {
        printf("Test case number must be 0 or 1.\n");
        exit(EXIT_FAILURE);
    }

    // 初始化管道
    if (pipe(x_pipe) == -1 || pipe(y_pipe) == -1 || pipe(res_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 创建计算f(x)的子进程
    pid_t pid_x = fork();
    if (pid_x == 0) {
        child_process(x_pipe[0], res_pipe[1], factorial);
    } else if (pid_x == -1) {
        perror("fork x");
        exit(EXIT_FAILURE);
    }

    // 创建计算f(y)的子进程
    pid_t pid_y = fork();
    if (pid_y == 0) {
        child_process(y_pipe[0], res_pipe[1], fibonacci);
    } else if (pid_y == -1) {
        perror("fork y");
        exit(EXIT_FAILURE);
    }
    
    // 关闭不需要的管道端
    CLOSE_PIPE_END(x_pipe, 0);
    CLOSE_PIPE_END(y_pipe, 0);
    CLOSE_PIPE_END(res_pipe, 1);

    // 测试数据
    struct TestCase { unsigned x, y; } tests[] = {{1, 2}, {5, 5}};

    // 发送数据
    if (write(x_pipe[1], &tests[test_case_num].x, sizeof(tests[test_case_num].x)) == -1) {
        perror("write x");
        CLOSE_PIPE_END(x_pipe, 1);
        CLOSE_PIPE_END(y_pipe, 1);
        CLOSE_PIPE_END(res_pipe, 0);
        exit(EXIT_FAILURE);
    }
    puts("write x done");
    if (write(y_pipe[1], &tests[test_case_num].y, sizeof(tests[test_case_num].y)) == -1) {
        perror("write y");
        CLOSE_PIPE_END(x_pipe, 1);
        CLOSE_PIPE_END(y_pipe, 1);
        CLOSE_PIPE_END(res_pipe, 0);
        exit(EXIT_FAILURE);
    }
    puts("write y done");

    // 关闭写端触发EOF
    CLOSE_PIPE_END(x_pipe, 1);
    CLOSE_PIPE_END(y_pipe, 1);

    // 读取并汇总结果
    unsigned fx, fy;

    if (read(res_pipe[0], &fx, sizeof(fx)) == -1 || 
        read(res_pipe[0], &fy, sizeof(fy)) == -1) {
        perror("read result");
        CLOSE_PIPE_END(x_pipe, 1);
        CLOSE_PIPE_END(y_pipe, 1);
        CLOSE_PIPE_END(res_pipe, 0);
        exit(EXIT_FAILURE);
    }
    printf("f(%u,%u) = %u + %u = %u\n", tests[test_case_num].x, tests[test_case_num].y, fx, fy, fx + fy);
    
    CLOSE_PIPE_END(res_pipe, 0);
    // 清理资源
    waitpid(pid_x, NULL, 0);
    waitpid(pid_y, NULL, 0);
    return EXIT_SUCCESS;
}