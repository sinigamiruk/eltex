#include <linux/module.h>
#include <linux/kernel.h>

#include <asm/uaccess.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>

#include <net/checksum.h>
#include <net/net_namespace.h>
#include <net/flow.h>
#include <net/ip.h>

#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/icmp.h>

#define IFNAME "ens3"
#define SADDR "192.168.5.2"
#define DADDR "8.8.8.8"

static int __init m_init_module(void);
static void __exit m_cleanup_module(void);

static int create_packet(struct sk_buff *skb);
static unsigned int inet_addr(char *str);
