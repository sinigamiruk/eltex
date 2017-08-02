#ifndef RAW_SOCKETS_SOCKET_H
#define RAW_SOCKETS_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include "utils.h"

struct pseudohdr
{
    u_int32_t source;
    u_int32_t dest;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};

int socket_create(int type, int proto);
void socket_close(int sock);
void socket_bind(int sock, struct sockaddr_in *addr);

void iphdr_create(struct iphdr *ip, char *daddr, int tot_len);
void udphdr_create(struct udphdr *udp, u_short dport, u_short sport, int length);


#endif
