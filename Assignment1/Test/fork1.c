#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = fork();
    int ppid = getppid();
    int cpid = getpid();
    printf("pid = %d \n ppid = %d \n cpid = %d\n", pid, ppid, cpid);
    printf("Hello World\n");
    return 0;
}
