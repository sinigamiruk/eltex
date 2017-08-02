#include <stdio.h>
#include <sys/socket.h>
#include <getopt.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#ifndef RETRY
#define RETRY(expression)                           \
    (__extension__                                  \
     ({ int __result;                               \
         do __result = (long int) (expression);     \
         while (__result == -1L && errno == EINTR); \
         __result; }))
#endif

#define PAYLOAD_SIZE   48
#define ICMP_SIZE      sizeof(struct icmphdr)
#define IP_SIZE        sizeof(struct iphdr)
#define EPOLL_MAXEVENT 20

int seq;
int total;

u_short checksum(u_short *ptr, int nbytes)
{
    register long sum;
    u_short oddbyte;
    register short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char*)&oddbyte) = *(u_char*)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short)~sum;

    return (answer);
}

static void die(int line, const char * format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    fprintf(stderr, "%d: ", line);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, " %s\n", strerror(errno));
    va_end(vargs);
    exit(EXIT_FAILURE);
}

void print_icmp(struct icmphdr *icmp)
{
    printf("*******icmp header*******\n");
    printf("* icmp->type     - %d\n", icmp->type);
    printf("* icmp->code     - %d\n", icmp->code);
    printf("* icmp->id       - %d\n", ntohs(icmp->un.echo.id));
    printf("* icmp->sequence - %d\n", ntohs(icmp->un.echo.sequence));
    printf("*************************\n");
}

void signal_handler(int signo)
{
    switch (signo) {
        case SIGINT:
        case SIGTERM:
            printf("%d packets transmitted, %d received.\n", seq, total);
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "unknown signal %d\n", signo);
            exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    struct epoll_event new_event, *events;
    struct icmphdr *icmp_send, *icmp_recv;
    struct iphdr *ip_recv;
    struct sockaddr_in daddr;
    struct timeval timeout, tmp_timeout;
    socklen_t dlen;
    fd_set set_fd, set_tmp;
    int sock, opt, n, i, max, ttl;
    pid_t pid;
    char *ip;
    char packet_send[ICMP_SIZE + PAYLOAD_SIZE];
    char packet_recv[IP_SIZE + ICMP_SIZE + PAYLOAD_SIZE];
    char source_ip[16];

    total = 0;
    seq = 0;
    ttl = 255;
    ip = "8.8.8.8";
    pid = getppid();
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while ((opt = getopt(argc, argv, "i:t:")) != -1) {
        switch (opt) {
            case 'i':
                ip = optarg;
                break;
            case 't':
                ttl = atoi(optarg);
                break;
            default:
                break;
        }
    }

    inet_pton(AF_INET, ip, &(daddr.sin_addr));
    daddr.sin_family = AF_INET;

    dlen = sizeof(daddr);

    sock = RETRY(socket(AF_INET, SOCK_RAW, IPPROTO_ICMP));
    if (sock == -1) {
        die(__LINE__, "socket()");
    }

    FD_ZERO(&set_fd);
    FD_SET(sock, &set_fd);

    max = sock;

    icmp_send = (struct icmphdr *)packet_send;

    for (i = 1; i < ttl; ++i) {
        sleep(1);
        seq = i;

        icmp_send->type = ICMP_ECHO;
        icmp_send->checksum = 0;
        icmp_send->code = 0;
        icmp_send->un.echo.id = htons((uint16_t) pid);
        icmp_send->un.echo.sequence = htons((uint16_t) seq);

        icmp_send->checksum = checksum((u_short *) &packet_send, sizeof(packet_send));

        if (setsockopt(sock, IPPROTO_IP, IP_TTL, &i, sizeof(i)) < 0) {
            die(__LINE__, "setsockopt()");
            exit(EXIT_FAILURE);
        }

        tmp_timeout = timeout;

        sendto(sock, &packet_send, sizeof(packet_send), 0, (struct sockaddr *) &daddr, dlen);

        while (1) {
            set_tmp = set_fd;
            n = select(max + 1, &set_tmp, NULL, NULL, &tmp_timeout);
            if (n < 0) {
                die(__LINE__, "select()");
                exit(EXIT_FAILURE);
            } else if (n == 0) {
                printf("timeout...\n");
                break;
            } else {
                if (FD_ISSET(sock, &set_tmp)) {
                    recvfrom(sock, &packet_recv, sizeof(packet_recv), 0, NULL, NULL);
                    ip_recv = (struct iphdr *) packet_recv;
                    icmp_recv = (struct icmphdr *) (packet_recv + sizeof(struct iphdr));

                    if (icmp_recv->type == 11) {
                        printf("%d: %s ", seq, inet_ntoa(*(struct in_addr *) &ip_recv->saddr));
                        printf(" time - %ld\n", ((timeout.tv_sec - tmp_timeout.tv_sec) * 1000000L
                                                 + timeout.tv_usec) - tmp_timeout.tv_usec);
                        break;
                    } else if (icmp_recv->type == 0) {
                        if (ntohs(icmp_recv->un.echo.id) != pid) {
                            continue;
                        } else {
                            printf("%d: %s ", seq, inet_ntoa(*(struct in_addr *) &ip_recv->saddr));
                            printf(" time - %ld\n", ((timeout.tv_sec - tmp_timeout.tv_sec) * 1000000L
                                                     + timeout.tv_usec) - tmp_timeout.tv_usec);
                            goto end;
                        }
                    } else {
                        continue;
                    }
                }
            }
        }
    }

end:
    return 0;
}
