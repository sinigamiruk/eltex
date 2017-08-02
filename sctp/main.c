#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <zconf.h>

char buf[256];

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

int socket_create(int type, int protocol)
{
    int sock;

    sock = socket(AF_INET, type, protocol);
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

void socket_close(int sock)
{
    if (close(sock) < 0) {
        die(__LINE__, "close()");
    }
}

void server(unsigned short port)
{
    struct sockaddr_in serverAddr, clientAddr;
    struct sctp_initmsg initmsg;
    struct sctp_sndrcvinfo sndrcv;
    int serverSock, clientSock;
    int opt, nbytes;
    socklen_t clientLen;

    opt = 1;
    nbytes = 0;

    bzero(&buf, 256);
    bzero(&initmsg, sizeof(initmsg));
    bzero(&serverAddr, sizeof(serverAddr));
    bzero(&clientAddr, sizeof(clientAddr));

    serverSock = socket_create(SOCK_STREAM, IPPROTO_SCTP);

    socket_addrl(&serverAddr, INADDR_ANY, port);
    socket_bind(serverSock, &serverAddr);

    initmsg.sinit_max_instreams = 2;
    initmsg.sinit_num_ostreams = 2;
    initmsg.sinit_max_attempts = 2;

    socket_setsockopt(serverSock, IPPROTO_SCTP, SCTP_INITMSG, &opt);

    socket_listen(serverSock);

    clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientSock < 0) {
        die(__LINE__, "accept()");
    }
    printf("new client\n");

    while (1) {
        nbytes = sctp_recvmsg(clientSock, buf, 256, (struct sockaddr *)&clientAddr, &clientLen, &sndrcv, 0);
        if (nbytes < 0) {
            die(__LINE__, "sctp_recvmsg()");
        } else if (nbytes == 0) {
            printf("client disconnect\n");
            socket_close(clientSock);
            break;
        } else {
            buf[nbytes] = '\0';
            printf("sctp recieve %s\n", buf);
        }
    }
    socket_close(serverSock);
}

void client(unsigned short port)
{
    struct sockaddr_in serverAddr;
    int serverSock;
    int opt, nbytes;
    socklen_t serverLen;

    opt = 1;
    nbytes = 0;

    bzero(&buf, 256);
    strcpy(buf, "hello world\n");

    serverSock = socket_create(SOCK_STREAM, IPPROTO_SCTP);

    socket_addrc(&serverAddr, "127.0.0.1", port);
    serverLen = sizeof(serverAddr);

    if (connect(serverSock, (struct sockaddr *)&serverAddr, serverLen) < 0) {
        die(__LINE__, "connect()");
    }

    while (1) {
        sleep(1);
        nbytes = sctp_sendmsg(serverSock, buf, 256, NULL, 0, 0, 0, 0, 0, 0);
        if (nbytes < 0) {
            die(__LINE__, "csctp_sendmsg");
        }
    }
    socket_close(serverSock);
}

int main(void)
{
    /* server(8000); */
    client(8000);
    return 0;
}
