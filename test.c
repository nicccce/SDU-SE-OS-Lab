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