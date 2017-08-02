#include <netinet/ip.h>
#include "socket.h"

int socket_create(int type, int proto)
{
    int sock;

    sock = RETRY(socket(AF_INET, type, proto));
    if (sock < 0)
        die(__LINE__, "socket()");

    return sock;
}

void socket_close(int sock)
{
    if (RETRY(close(sock)) < 0)
        die(__LINE__, "close()");
}

void socket_bind(int sock, struct sockaddr_in *addr)
{
    socklen_t len;

    len = sizeof(*addr);

    if (bind(sock, (struct sockaddr *) addr, len) < 0) {
        die(__LINE__, "bind()\n");
    }
}

void iphdr_create(struct iphdr *ip, char *daddr, int tot_len)
{
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(tot_len);
    ip->id = htons(0);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_UDP;
    ip->saddr = htonl(0);
    ip->daddr = inet_addr(daddr);
    ip->check = 0;
    ip->check = checksum((u_short *)ip, ip->tot_len);
}

void udphdr_create(struct udphdr *udp, u_short dport, u_short sport, int length)
{
    udp->dest = htons(dport);
    udp->source = htons(sport);
    udp->len = htons(sizeof(struct udphdr) + length);
    udp->check = 0;
}
