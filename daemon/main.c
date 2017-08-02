#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

static void die (int line, const char * format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    fprintf(stderr, "%d: ", line);
    vfprintf(stderr, format, vargs);
    va_end(vargs);
    exit(EXIT_FAILURE);
}

void daemonize()
{
    pid_t pid;
    pid_t sid;

    pid = fork();

    if (pid < 0)
        die(__LINE__, "pid < 0\n");

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);

    if (chdir("/") < 0) {
        die(__LINE__, "chdir() error\n");
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("mydaemon", LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "message from daemon.");
    closelog();
}

int main(void)
{
    daemonize();
    return 0;
}
