#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int ret;
    int fd[2];
    char buffer[256];

    bzero(&buffer, 256);

    ret = pipe(fd);
    if (ret == -1) {
        fprintf(stderr, "%s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    switch (fork()) {
    case -1:
        fprintf(stderr, "%s", strerror(errno));
        exit(EXIT_FAILURE);
    case 0:
        close(fd[1]);
        read(fd[0], buffer, 256);
        printf("Recv - %s", buffer);
        close(fd[0]);
        exit(EXIT_SUCCESS);
    default:
        close(fd[0]);
        strcpy(buffer, "hello world\n");
        printf("Send - %s\n", buffer);
        write(fd[1], buffer, 256);
        close(fd[1]);
        pid = wait(&ret);
        exit(EXIT_SUCCESS);
    }

    return 0;
}
