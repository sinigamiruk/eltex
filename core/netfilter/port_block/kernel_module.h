#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <asm/uaccess.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

static int __init m_init_module(void);
static void __exit m_cleanup_module(void);
static uint32_t m_filter_port(void *priv, struct sk_buff *skb,
						const struct nf_hook_state *state);

static struct nf_hook_ops n_ops = {
	.hook = m_filter_port,
	.pf = PF_INET,
	.hooknum = NF_INET_LOCAL_IN,
	.priority = NF_IP_PRI_FIRST,
};
