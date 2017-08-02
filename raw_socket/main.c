#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/ether.h>

#include "utils.h"
#include "socket.h"

#define DEBUG

#ifdef DEBUG
#define ERROR(msg)                                                      \
    fprintf(stderr, ":: file - %s, error - %s, errmsg - %s, line - %d\n", \
            __FILE__, strerror(errno), msg, __LINE__);
#else
#define ERROR(msg)
#endif

#define UDP_MSG_SIZE 256

void server_udp(struct sockaddr_in saddr)
{
    char buf[UDP_MSG_SIZE];
    int sock, nread;
    struct sockaddr_in caddr;
    socklen_t clen;

    sock = socket_create(SOCK_DGRAM, 0);
    socket_bind(sock, &saddr);

    clen = sizeof(caddr);

    while (1) {
        nread = recvfrom(sock, buf, UDP_MSG_SIZE, 0, (struct sockaddr *) &caddr, &clen);
        if (nread < 0) {
            die(__LINE__, "recvfrom()\n");
        }
        buf[nread] = '\0';
        printf("recieve from client - %s\n", buf);
    }
}

void client_udp(struct sockaddr_in addr)
{
    char buf[4096];
    char *data;
    int sock;
    socklen_t len;
    struct udphdr *udp_header;
    struct iphdr *ip_header;
    struct pseudohdr *pseudo_header;

    sock = socket_create(SOCK_RAW, IPPROTO_RAW);

    len = sizeof(addr);

    ip_header = (struct iphdr *)buf;

    udp_header = (struct udphdr *)(buf + sizeof(struct iphdr));

    data = buf + sizeof(struct iphdr) + sizeof(struct udphdr);

    strcpy(data, "hello world\n");

    ip_header->ihl = 5;
    ip_header->version = 4;
    ip_header->id = htons(12345);
    ip_header->daddr = 0;
    ip_header->ttl = 20;
    ip_header->tos = 0;
    ip_header->frag_off = 0;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->saddr = htonl(0);
    ip_header->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data);
    ip_header->check = 0;
    ip_header->daddr = addr.sin_addr.s_addr;

    ip_header->check = checksum((u_short *)buf, ip_header->tot_len);

    udp_header->source = 0;
    udp_header->dest = addr.sin_port;
    udp_header->len = htons(sizeof(struct udphdr) + strlen(data));
    udp_header->check = 0;

    while (1) {
        sleep(1);
        if (sendto(sock, buf, ip_header->tot_len,  0, (struct sockaddr *) &addr, len) < 0) {
            die(__LINE__, "sendto()\n");
        }
    }
}

int main(int argc, char *argv[])
{
    enum { SERVER, CLIENT };
    struct sockaddr_in addr;
    int opt;
    int choice;

    bzero(&addr, sizeof(addr));
    addr.sin_port = htons(8000);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    choice = SERVER;

    while ((opt = getopt(argc, argv, "sc:i:p:")) != -1) {
        switch (opt) {
            case 's':
                choice = SERVER;
                break;
            case 'c':
                choice = CLIENT;
                break;
            case 'p':
                addr.sin_port = htons(atoi(optarg));
                break;
            case 'i':
                addr.sin_addr.s_addr = inet_addr(optarg);
                break;
            default:
                break;
        }
    }

    printf("%s\n", choice == SERVER ? "SERVER" : "CLIENT");
    printf("IP - %s\n", inet_ntoa(addr.sin_addr));
    printf("PORT - %d\n", ntohs(addr.sin_port));

    if (choice == SERVER) {
        server_udp(addr);
    } else if (choice == CLIENT) {
        client_udp(addr);
    }
    return 0;
}
