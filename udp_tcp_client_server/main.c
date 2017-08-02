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

#define UDP_MSG_SIZE 256
#define TCP_MSG_SIZE 256

static void die(int line, const char * format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    fprintf(stderr, "%d: ", line);
    vfprintf(stderr, format, vargs);
    fprintf(stderr, "%s", strerror(errno));
    va_end(vargs);
    exit(EXIT_FAILURE);
}

void server_udp(unsigned short port)
{
    char buf[UDP_MSG_SIZE];
    int sock, nread;
    struct sockaddr_in saddr, caddr;
    socklen_t slen, clen;

    bzero(buf, sizeof(buf));

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        die(__LINE__, "socket()\n");
    }

    bzero(&saddr, sizeof(saddr));
    saddr.sin_port = htons(port);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    slen = sizeof(saddr);
    clen = sizeof(caddr);

    if (bind(sock, (struct sockaddr *) &saddr, slen) < 0) {
        die (__LINE__, "bind()\n");
    }

    while (1) {
        nread = recvfrom(sock, buf, UDP_MSG_SIZE, 0, (struct sockaddr *) &caddr, &clen);
        if (nread < 0) {
            die(__LINE__, "recvfrom()\n");
        }
        buf[nread] = '\0';
        printf("recieve from client - %s\n", buf);
        if (sendto(sock, buf, UDP_MSG_SIZE, 0, (struct sockaddr *) &caddr, clen) < 0) {
            die(__LINE__, "sendto()\n");
        }
    }

}

void client_udp(unsigned short port)
{
    char buf[UDP_MSG_SIZE];
    int sock, nread;
    struct sockaddr_in addr;
    socklen_t len;

    strcpy(buf, "hello world!");

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        die(__LINE__, "socket()\n");
    }

    bzero(&addr, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    len = sizeof(addr);

    while (1) {
        sleep(1);
        if (sendto(sock, buf, UDP_MSG_SIZE, 0, (struct sockaddr *) &addr, len) < 0) {
            die(__LINE__, "sendto()\n");
        }
        nread = recvfrom(sock, buf, UDP_MSG_SIZE, 0, (struct sockaddr *) &addr, &len);
        if (nread < 0) {
            die(__LINE__, "recvfrom()\n");
        }
        buf[nread] = '\0';
        printf("recieve from server - %s\n", buf);
    }

}

void server_tcp(unsigned short port)
{
    char buf[TCP_MSG_SIZE];
    int ssock, csock, nread;
    struct sockaddr_in saddr, caddr;
    socklen_t slen, clen;

    bzero(buf, sizeof(buf));

    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock < 0) {
        die(__LINE__, "socket()\n");
    }

    bzero(&saddr, sizeof(saddr));
    saddr.sin_port = htons(port);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    slen = sizeof(saddr);
    clen = sizeof(caddr);

    if (bind(ssock, (struct sockaddr *) &saddr, slen) < 0) {
        die(__LINE__, "bind()\n");
    }

    if (listen(ssock, 5) < 0) {
        die(__LINE__, "listen()\n");
    }

    csock = accept(ssock,  (struct sockaddr *)&caddr,  &clen);
    while (1) {
        nread = recv(csock, buf, TCP_MSG_SIZE, 0);
        if (nread < 0) {
            die(__LINE__, "recv()\n");
        }
        buf[nread] = '\0';
        printf("recieve from client - %s\n", buf);

        if (send(csock, buf, nread, 0) < 0) {
            die(__LINE__, "send()\n");
        }
    }
    close(ssock);
}

void client_tcp(unsigned short port)
{
    char buf[TCP_MSG_SIZE];
    int sock, nread;
    struct sockaddr_in addr;
    socklen_t len;

    strcpy(buf, "hello world!");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        die(__LINE__, "socket()\n");
    }

    bzero(&addr, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    len = sizeof(addr);

    if (connect(sock, (struct sockaddr *)&addr, len) < 0) {
        die(__LINE__, "connect()\n");
    }

    while (1) {
        sleep(1);
        if (send(sock, buf, TCP_MSG_SIZE, 0) < 0) {
            die(__LINE__, "send()\n");
        }
        /*
        nread = recv(sock, buf, TCP_MSG_SIZE, 0);
        if (nread < 0) {
            die(__LINE__, "recv()\n");
        }
        buf[nread] = '\0';
        printf("recieve from server - %s\n", buf);
        */
    }
}

int main(void)
{
    /*server_udp(8001);*/
    client_udp(8000);
    /*server_tcp(8002);*/
    /*client_tcp(8001);*/
    return 0;
}
