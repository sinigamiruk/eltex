#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/rbtree.h>
#include <asm/uaccess.h>
#include <net/net_namespace.h>
#include <net/flow.h>
#include <net/ip.h>

#include <linux/ip.h>
#include <linux/udp.h>

#define HOSTNAME_LEN 255

static struct rb_root dns_tree = RB_ROOT;

static int __init m_init_module(void);
static void __exit m_cleanup_module(void);

static uint32_t 
hook_in_packets(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);
static uint32_t 
hook_out_packets(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

static struct 
dns_node *search(struct rb_root *root, char *host_name);
static int 
insert(struct rb_root *root, struct dns_node *data);

static int create_packet(struct sk_buff *skb, struct dns_node *node);

struct nf_hook_ops n_ops_in = {
	.hook = hook_in_packets,
	.pf = PF_INET,
	.hooknum = NF_INET_PRE_ROUTING,
	.priority = NF_IP_PRI_FIRST,
};

struct nf_hook_ops n_ops_out = {
	.hook = hook_out_packets,
	.pf = PF_INET,
	.hooknum = NF_INET_POST_ROUTING,
	.priority = NF_IP_PRI_FIRST,
};

struct dnshdr {
 	unsigned short id; // identification number
 
    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag
 
    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available
 
    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};

struct question {
    unsigned short qtype;
    unsigned short qclass;
};

struct query {
    unsigned char *name;
    struct question *ques;
};

struct dns_node {
	struct rb_node node;
    char host_name[HOSTNAME_LEN];
	uint32_t addr;
    uint32_t ttl;
};

struct r_data {
    unsigned short name;
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
    unsigned int addr;
} __attribute__((__packed__));
