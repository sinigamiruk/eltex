#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PSELECT

#define MAX_EVENTS 10

#if defined(SELECT) || defined(PSELECT)
#include <sys/select.h>
#elif defined(POLL) || defined(PPOLL)
#include <sys/poll.h>
#define POLL_MAX 1024
#else
#include <sys/epoll.h>
#endif

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

int socket_create(struct sockaddr_in *addr, int type, unsigned short port)
{
    int sock;
    socklen_t len;

    sock = socket(AF_INET, type, 0);
    if (sock < 0) {
        die(__LINE__, "socket()");
    }

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = INADDR_ANY;

    len = sizeof(*addr);
    if (bind(sock, (struct sockaddr *)addr, len) < 0) {
        die(__LINE__, "bind()");
    }

    return sock;
};

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

void socket_setsockopt(int sock, int flag)
{
    int option;

    option = 1;

    if (setsockopt(sock, SOL_SOCKET, flag, (char*)&option, sizeof(option)) < 0) {
        die(__LINE__, "setsockopt()");
    }
}

#if defined(SELECT) || defined(PSELECT)

int max;

void set_max(int val)
{
    max = val > max ? val : max;
}

#endif

int main(void)
{
    struct sockaddr_in tcpaddr, udpaddr;
    struct sockaddr_in clientAddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    int clientSock;
    int tcpsock, udpsock, ret, i;
    ssize_t nbytes;
    bool running;
    char udpbuf[256], tcpbuf[256];

    memset(&udpbuf, 0, 256);
    memset(&tcpbuf, 0, 256);

    bzero(&clientAddr, sizeof(clientAddr));
    bzero(&tcpaddr, sizeof(tcpaddr));
    bzero(&udpaddr, sizeof(udpaddr));

    udpsock = socket_create(&udpaddr, SOCK_DGRAM, 8000);
    tcpsock = socket_create(&tcpaddr, SOCK_STREAM, 8001);

    socket_setsockopt(udpsock, SO_REUSEADDR);
    socket_setsockopt(tcpsock, SO_REUSEADDR);

    socket_listen(tcpsock);

    running = true;

#if defined(SELECT) || defined(PSELECT)
    fd_set rset, tset;

    max = -1;

    FD_ZERO(&rset);
    FD_ZERO(&tset);

    set_max(tcpsock > udpsock ? tcpsock : udpsock);

    FD_SET(tcpsock, &rset);
    FD_SET(udpsock, &rset);
#elif defined(POLL) || defined(PPOLL)
    struct pollfd pfds[POLL_MAX];
    nfds_t nfds;

    pfds[0].fd = tcpsock;
    pfds[0].events = POLLIN;
    pfds[1].fd = udpsock;
    pfds[1].events = POLLIN;

    nfds = 2;
#else
    struct epoll_event eset[2], *events;
    int epollfd;

    events = malloc(sizeof(struct epoll_event) * MAX_EVENTS);

    epollfd = epoll_create(MAX_EVENTS);
    if (epollfd < 0) {
        die(__LINE__, "epoll_create()");
    }

    eset[0].events = EPOLLIN;
    eset[0].data.fd = udpsock;
    eset[1].events = EPOLLIN;
    eset[1].data.fd = tcpsock;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, udpsock, &eset[0]) < 0) {
        die(__LINE__, "epoll_ctl()");
    }

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, tcpsock, &eset[1]) < 0) {
        die(__LINE__, "epoll_ctl()");
    }
#endif
    while (running) {
#if defined(SELECT)
        tset = rset;
        ret = select(max + 1, &tset, NULL, NULL, NULL);
#elif defined(PSELECT)
        tset = rset;
        ret = pselect(max + 1, &tset, NULL, NULL, NULL, NULL);
#elif defined(POLL)
        ret = poll(pfds, nfds, 0);
#elif defined(PPOLL)
        ret = ppoll(pfds, nfds, NULL, NULL);
#else
        ret = epoll_wait(epollfd, events, MAX_EVENTS, -1);
#endif
#if defined(SELECT) || defined(PSELECT)
        if (ret) {
            for (i = 0; i <= max; ++i) {
                if (FD_ISSET(i, &tset)) {
                    if (i == tcpsock) {
                        clientSock = accept(i, (struct sockaddr *)&clientAddr, &clientlen);
                        printf("new client\n");
                        FD_SET(clientSock, &rset);
                        set_max(clientSock);
                    } else if (i == udpsock) {
                        recvfrom(udpsock, &udpbuf, 256, 0, (struct sockaddr *)&clientAddr, &clientlen);
                        printf("udp recieve: %s\n", udpbuf);
                    } else {
                        nbytes = recv(i, &tcpbuf, 256, 0);
                        if (nbytes == 0) {
                            printf("disconnect\n");
                            close(i);
                            FD_CLR(i, &rset);
                            continue;
                        }
                        printf("tcp recieve: %s\n", tcpbuf);
                    }
                }
            }
        }
#elif defined(POLL) || defined(PPOLL)
        if (ret) {
            for (i = 0; i < nfds; ++i) {
                if (pfds[i].revents & POLLIN) {
                    if (pfds[i].fd == tcpsock) {
                        clientSock = accept(tcpsock, (struct sockaddr *) &clientAddr, &clientlen);
                        printf("new client\n");

                        pfds[nfds].fd = clientSock;
                        pfds[nfds].events = POLLIN;

                        nfds++;
                    } else if (pfds[i].fd == udpsock) {
                        recvfrom(udpsock, udpbuf, 256, 0, (struct sockaddr *)&clientAddr, &clientlen);
                        printf("udp recieve: %s\n", udpbuf);
                    } else {
                        nbytes = recv(pfds[i].fd, tcpbuf, 256, 0);
                        if (nbytes == 0) {
                            int s;
                            close(pfds[i].fd);
                            pfds[i].fd = -1;
                            pfds[i].events = 0;
                            printf("disconnect\n");

                            for (s = i; s < nfds; ++s) {
                                pfds[s] = pfds[s + 1];
                            }
                            nfds--;
                        } else {
                            printf("tcp recieve: %s\n", tcpbuf);
                        }
                    }
                }
            }
        }
#else
        for (i = 0; i < ret; ++i) {
            if (udpsock == events[i].data.fd) {
                recvfrom(udpsock, udpbuf, 256, 0, (struct sockaddr *)&clientAddr, &clientlen);
                printf("udp recieve: %s\n", udpbuf);
            } else if (tcpsock == events[i].data.fd) {
                struct epoll_event event;
                clientSock = accept(tcpsock, (struct sockaddr *)&clientAddr, &clientlen);

                printf("new client\n");

                event.events = EPOLLIN;
                event.data.fd = clientSock;

                epoll_ctl(epollfd, EPOLL_CTL_ADD, clientSock, &event);
            } else {
                nbytes = recv(events[i].data.fd, tcpbuf, 256, 0);
                if (nbytes == 0) {
                    printf("client disconnect\n");
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
                    close(events[i].data.fd);
                    continue;
                }
                printf("tcp recieve: %s\n", tcpbuf);
            }
        }
#endif
    }
    return 0;
}
