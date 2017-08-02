#include "utils.h"

u_short checksum(u_short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    u_short *w = addr;
    u_short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(u_char *) (&answer) = *(u_char *) w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    answer = (u_short)~sum;
    return (answer);
}

void die(int line, const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    fprintf(stderr, "%d: ", line);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, "%s", strerror(errno));
    va_end(vargs);
    exit(EXIT_FAILURE);
}
