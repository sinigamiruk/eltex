#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <zconf.h>

#define BUF_SIZE 256

char buf[BUF_SIZE];

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

int socket_create(int type)
{
    int sock;

    sock = socket(AF_INET, type, 0);
    if (sock < 0) {
        die(__LINE__, "socket()");
    }

    return sock;
}

void socket_bind(int sock, struct sockaddr_in *addr)
{
    socklen_t len;
    len = sizeof(*addr);
    if (bind(sock, (struct sockaddr *)addr, len) < 0) {
        die(__LINE__, "bind()");
    }
}

void socket_addrl(struct sockaddr_in *addr, long address, unsigned short port)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = htonl(address);
}

void socket_addrc(struct sockaddr_in *addr, const char *address, unsigned short port)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(address);
}

void socket_setsockopt(int sock, int level, int optname, void *optval)
{
    if (setsockopt(sock, level, optname, &optval, sizeof(optval)) < 0) {
        die(__LINE__, "setsockopt()");
    }
}

void socket_close(int sock)
{
    if (close(sock) < 0) {
        die(__LINE__, "close()");
    }
}

int socket_accept(int sock, struct sockaddr_in *addr, socklen_t *len)
{
    int client;

    client = accept(sock, (struct sockaddr *) addr, len);
    if (client < 0) {
        die(__LINE__, "accept()");
    }

    return client;
}

void socket_listen(int sock)
{
    if (listen(sock, 5) < 0) {
        die(__LINE__, "listen()");
    }
}

void broadcast_server(unsigned short port)
{
    struct sockaddr_in addr;
    int sock, opt;
    socklen_t len;
    opt = 1;

    strcpy(buf, "hello world!");

    bzero(&addr, sizeof(addr));
    sock = socket_create(SOCK_DGRAM);

    socket_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt);

    socket_addrl(&addr, INADDR_BROADCAST, port);

    socket_bind(sock, &addr);

    len = sizeof(addr);

    while (1) {
        sleep(1);
        sendto(sock, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, len);
        printf("broadcast send: %s\n", buf);
    }
    socket_close(sock);
}

void broadcast_client(unsigned short port)
{
    struct sockaddr_in addr, clientAddr;
    int sock, opt;
    socklen_t len, clientLen;
    opt = 1;

    bzero(&addr, sizeof(addr));
    bzero(&clientAddr, sizeof(clientAddr));

    sock = socket_create(SOCK_DGRAM);

    socket_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt);

    socket_addrl(&addr, INADDR_BROADCAST, port);

    socket_bind(sock, &addr);

    len = sizeof(addr);

    while (1) {
        recvfrom(sock, buf, BUF_SIZE, 0, (struct sockaddr *) &clientAddr, &clientLen);
        printf("broadcast recvfrom: %s\n", buf);
    }

    socket_close(sock);
}

void multicast_server(const char *addrgroup, unsigned short port)
{
    struct sockaddr_in addr;
    int sock, opt;
    socklen_t len;
    opt = 1;

    strcpy(buf, "hello world!");

    bzero(&addr, sizeof(addr));
    sock = socket_create(SOCK_DGRAM);

    socket_addrc(&addr, addrgroup, port);

    len = sizeof(addr);

    while (1) {
        sleep(1);
        sendto(sock, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, len);
        printf("multicast sendto: %s", buf);
    }

    socket_close(sock);
}

void multicast_client(const char *addrgroup, unsigned short port)
{
    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int sock, opt;
    socklen_t len;
    opt = 1;

    strcpy(buf, "hello world!");

    sock = socket_create(SOCK_DGRAM);

    socket_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt);

    bzero(&addr, sizeof(addr));
    socket_addrl(&addr, INADDR_ANY, port);

    mreq.imr_multiaddr.s_addr = inet_addr(addrgroup);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    socket_setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq);

    len = sizeof(addr);

    while (1) {
        recvfrom(sock, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &len);
        printf("multicast recvfrom: %s", buf);
    }

    socket_close(sock);
}

int main(void)
{
    bzero(&buf, BUF_SIZE);
    /*broadcast_server(8000);*/
    broadcast_client(8001);
    /* multicast_server("225.0.0.37", 8000); */
    /* multicast_client("225.0.0.37", 8000); */
    return 0;
}
