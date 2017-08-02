#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <zconf.h>
#include <pcap.h>

#define DEBUG

#ifdef DEBUG
#define ERROR(msg)                                                      \
    fprintf(stderr, ":: file - %s, error - %s, errmsg - %s, line - %d\n", \
            __FILE__, strerror(errno), msg, __LINE__);
#else
#define ERROR(msg)
#endif

#define IP_VER(ip) (((ip)->ver_ihl) >> 4)
#define IP_IHL(ip) (((ip)->ver_ihl) & 0x0f)

typedef struct {
    u_char dmac[6];
    u_char smac[6];
    u_short type;
} ethernet_header;

typedef struct {
    u_int saddr;
    u_int daddr;
    u_char zero;
    u_char protocol;
    u_short udp_length;
} pseudo_header;

typedef struct {
    u_char b1;
    u_char b2;
    u_char b3;
    u_char b4;
} ipv4_address;

typedef struct {
    u_char ver_ihl;
    u_char tos;
    u_short tlen;
    u_short ident;
    u_short offset;
    u_char ttl;
    u_char protocol;
    u_short sum;
    ipv4_address saddr;
    ipv4_address daddr;
} ipv4_header;

typedef struct {
    u_short sport;
    u_short dport;
    u_short length;
    u_short sum;
} udp_header;

char errbuf[PCAP_ERRBUF_SIZE];

void print_devices();
void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

int main(int argc, char *argv[])
{
    int opt;
    pcap_t *handle;
    char *dev;
    bpf_u_int32 network;
    bpf_u_int32 mask;
    struct bpf_program bpf;
    char filter[] = "ip";

    while ((opt = getopt(argc, argv, "pl")) != -1) {
        switch (opt) {
            case 'p': {
                          print_devices();
                          break;
                      }
            case 'l': {
                          dev = pcap_lookupdev(errbuf);
                          if (dev == NULL)
                              ERROR(errbuf);

                          if (pcap_lookupnet(dev, &network, &mask, errbuf) == -1)
                              ERROR(errbuf);

                          handle = pcap_open_live(dev, 1500, 1, 0, errbuf);
                          if (handle == NULL)
                              ERROR(errbuf);

                          if (pcap_compile(handle, &bpf, filter, 0, network) == -1)
                              ERROR("pcap_compile()");

                          if (pcap_setfilter(handle, &bpf) == -1)
                              ERROR("pcap_setfilter()");

                          pcap_loop(handle, 100000, callback, NULL);
                          pcap_close(handle);
                          break;
                      }
            default:
                      break;
        }
    }
    return 0;
}

u_short check_sum(const void *buf, int nbytes)
{
    u_int sum;
    const u_short *part;
    u_short last_byte;

    last_byte = 0;
    part = buf;

    while (nbytes > 1) {
        sum += *part;
        nbytes -= 2;
        part++;
    }

    if (nbytes == 1) {
        *((u_char*)&last_byte) = *(u_char*)buf;
        sum += last_byte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return ((u_short)~sum);
}

void print_devices()
{
    pcap_if_t *devices, *dev;
    pcap_addr_t *addr;

    if (pcap_findalldevs(&devices, errbuf) == -1) {
        ERROR("pcap_findalldevs()");
    }

    printf("All devices:\n");
    for (dev = devices; dev != NULL; dev = dev->next) {
        printf("-> %s\n", dev->name);
    }
    printf("\n");

    pcap_freealldevs(devices);
}

void callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    int ip_size;
    ethernet_header *eth;
    ipv4_header *ipv4;
    udp_header *udp;
    pseudo_header pseudo_h;

    eth = (ethernet_header *)packet;

    printf("Ethernet header - ");
    switch (ntohs(eth->type)) {
        case ETHERTYPE_ARP:
            printf("ARP\n");
            return;
        case ETHERTYPE_IP:
            printf("IP\n");
            break;
        case ETHERTYPE_REVARP:
            printf("RARP\n");
            return;
        default:
            printf("unknown\n");
            return;
    }

    printf("Source mac - %x.%x.%x.%x.%x.%x\n", 
        eth->smac[0],
        eth->smac[1],
        eth->smac[2],
        eth->smac[3],
        eth->smac[4],
        eth->smac[5]
        );

    printf("Destination mac - %x.%x.%x.%x.%x.%x\n\n", 
        eth->dmac[0],
        eth->dmac[1],
        eth->dmac[2],
        eth->dmac[3],
        eth->dmac[4],
        eth->dmac[5]
        );

    printf("-----------------------------------------------------");

    ipv4 = (ipv4_header *)(packet + sizeof(ethernet_header));
    ip_size = IP_IHL(ipv4) * 4;
    if (ip_size < 20) {
        printf("Wrong package size: %d bytes\n", ip_size);
        return;
    }

    if (check_sum(ipv4, ip_size) == 0) {
        printf("Wrong checksum\n");
        return;
    }

    printf("\n");
    printf("ver_ihl  - %d\n", ipv4->ver_ihl);
    printf("tos      - %d\n", ipv4->tos);
    printf("tlen     - %d\n", ipv4->tlen);
    printf("ident    - %d\n", ipv4->ident);
    printf("offset   - %d\n", ipv4->offset);
    printf("ttl      - %d\n", ipv4->ttl);
    printf("protocol - %d\n", ipv4->protocol);
    printf("checksum - %d\n\n", ipv4->sum);

    
    printf("-----------------------------------------------------");
    printf("\n");

    printf("Protocol - ");
    switch (ipv4->protocol) {
        case IPPROTO_TCP:
            printf("TCP\n");
            return;
        case IPPROTO_UDP:
            printf("UDP\n");
            break;
        case IPPROTO_ICMP:
            printf("ICMP\n");
            return;
        case IPPROTO_IP:
            printf("IP\n");
            return;
        default:
            printf("unknown\n");
            return;
    }

    udp = (udp_header *)ipv4;

    printf("source port      - %d\n", ntohs(udp->sport));
    printf("destination port - %d\n", ntohs(udp->dport));
    printf("length           - %d\n", ntohs(udp->length));
    printf("checksum         - %d\n", ntohs(udp->sum));

    pseudo_h.zero = 0;
    pseudo_h.protocol = IPPROTO_UDP;
    pseudo_h.udp_length = 0;
}
