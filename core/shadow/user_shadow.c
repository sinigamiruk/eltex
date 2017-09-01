#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#define BUFFER_LEN 64

int main() {
	const char *file_name_proc = "/proc/mshadow";
	const char *file_name_shadow = "/etc/shadow";
	
	int fd_proc = 0, fd_shadow = 0;
	int ret_val = 0, pid = 0, sign = 0;
	sigset_t set;
	char *buffer = NULL, *buffer_shadow = NULL;

	fd_proc = open(file_name_proc, O_WRONLY);
	if (fd_proc < 0) {
		perror("open()");
		exit(EXIT_FAILURE);
	}

	buffer = malloc(sizeof(int));
	buffer_shadow = malloc(sizeof(char) * BUFFER_LEN);
	pid = getpid();
	sprintf(buffer, "%d", pid);
	
	ret_val = write(fd_proc, buffer, sizeof(pid));
	if (ret_val < 0) {
		perror("write()");
		exit(EXIT_FAILURE);
	}

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, NULL);
	
	printf("Wait signal SIGUSR1\n");
	sigwait(&set, &sign);
	do {
		switch(sign) {
			case 10:
				fd_shadow = open(file_name_shadow, O_RDONLY);
				while (read(fd_shadow, buffer_shadow, BUFFER_LEN)) {
					printf("%s", buffer_shadow);
				}
				break;
			}
	} while (sign != SIGUSR1);

	close(fd_proc);
	close(fd_shadow);
	free(buffer);
	free(buffer_shadow);
	exit(EXIT_SUCCESS);
}
