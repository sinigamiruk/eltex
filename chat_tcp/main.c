#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <zconf.h>
#include <signal.h>

#define EPOLL_MAX_EVENTS 20
#define DEBUG

#ifdef DEBUG
#define ERROR(msg)                                                      \
    fprintf(stderr, ":: file - %s, error - %s, errmsg - %s, line - %d\n", \
            __FILE__, strerror(errno), msg, __LINE__);
#else
#define ERROR(msg)
#endif

enum {
    PACKET_DISCONNECT,
    PACKET_CONNECT,
    PACKET_OK
};

struct msg_packet {
    int type;
    int count;
    char name[20];
    char buf[232];
};

/* PACKET_AUTH 
   20bytes;
*/

bool running;
int epoll_fd;

void error_handler(int signo)
{
    switch (signo) {
    case SIGINT:
    case SIGTERM:
        break;

    default:
        break;
    }

    exit(EXIT_SUCCESS);
}

ssize_t recv_all(int sock, struct msg_packet *packet, int size)
{
    int nread;
    int len = size;
    char *ptr = (char *)packet;

    while (len > 0) {
        nread = read(sock, ptr, sizeof(int) * 2);
        if (nread <= 0)
            return nbytes;

    }


  
    return nread;
}

ssize_t send_all(int sock, void *buf, int size)
{
    return size;
}

void server(const char *address, unsigned short port)
{
    int i, fd, opt, server_sock, client_sock, res, n_sockets;
    struct sockaddr_in server_addr, client_addr;
    struct epoll_event *events, new_event;
    struct msg_packet packet;
    socklen_t server_addrlen, client_addrlen;
    ssize_t nbytes;

    opt = 1;
    bzero(&buf, 256);
    bzero(&packet, sizeof(packet));

    server_addrlen = sizeof(server_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
        ERROR("socket()");

    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    res = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (res == -1)
        ERROR("setsockopt()");

    res = bind(server_sock, (struct sockaddr *)&server_addr, server_addrlen);
    if (res == -1)
        ERROR("bind()");

    res = listen(server_sock, 10);
    if (res == -1)
        ERROR("listen()");

    epoll_fd = epoll_create(EPOLL_MAX_EVENTS);
    if (epoll_fd == -1)
        ERROR("epoll_create()");

    new_event.data.fd = server_sock;
    new_event.events = EPOLLIN;

    res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &new_event);
    if (res == -1)
        ERROR("epoll_ctl()");

    running = true;
  
    while (running) {
        n_sockets = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, -1);
        if (n_sockets == -1)
            ERROR("epoll_ctl()");
        for (i = 0; i < n_sockets; ++i) {
            if (events[i].events & EPOLLIN) {
                if (events[i].data.fd == server_sock) {
                    new_event.data.fd = accept(server_sock, 
                                               (struct sockaddr *)&client_addr, &client_addrlen);
                    if (new_event.data.fd == -1)
                        ERROR("accept()");

                    nbytes = recv_all(new_event.data.fd, (char *)&packet, 256);
                    if (nbytes <= 0) {
                        close(new_event.data.fd);
                        continue;
                    }

                    if (packet.type != PACKET_CONNECT) {
                        close(new_event.data.fd);
                        continue;
                    } else {
            
                    }

          

                    packet.type = PACKET_OK;

                    nbytes = send_all(new_event.data.fd, (char *)&packet, 256);
                    if (nbytes <= 0) {
                        close(new_event.data.fd);
                        continue;
                    }

                    new_event.events = EPOLLIN;

                    res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 
                                    new_event.data.fd, &new_event);
                    if (res == -1)
                        ERROR("epoll_ctl()");					
                } else {
                    nbytes = recv_all(events[i].data.fd, &buf, 256);
                    if (nbytes <= 0) {
                        res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, 
                                        events[i].data.fd, &events[i]);
                        if (res == -1)
                            ERROR("epoll_ctl()");

                        close(events[i].data.fd);
                    } else {
                        for (fd = 0; fd < n_sockets; ++fd) {
                            if (events[fd].events & EPOLLIN) {
                                if (events[fd].data.fd == server_sock 
                                    || events[fd].data.fd == events[i].data.fd) {
                                    nbytes = send_all(events[fd].data.fd, buf, 256);
                                    if (nbytes <= 0) {
                                        res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, 
                                                        events[fd].data.fd, &events[fd]);
                                        if (res == -1)
                                            ERROR("epoll_ctl()");

                                        close(events[fd].data.fd);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    close(server_sock);
}

void client(const char *address, unsigned short port)
{
    
}

int main(void)
{
    server("127.0.0.1", 8000);
    return 0;
}
