#include "kernel_module.h"

uint16_t port;

static int __init m_init_module(void) {
	port = htons(7777);
	nf_register_hook(&n_ops);

	return 0;
}

static void __exit m_cleanup_module(void) {
	nf_unregister_hook(&n_ops);
}

MODULE_LICENSE("GPL");
module_init(m_init_module);
module_exit(m_cleanup_module);

static uint32_t 
m_filter_port(void *priv, struct sk_buff *skb,
				const struct nf_hook_state *state) {
	struct iphdr *iph = NULL;
	struct udphdr *udph = NULL;
	struct tcphdr *tcph = NULL;

	iph = ip_hdr(skb);

	if (iph->protocol == IPPROTO_UDP) {
		udph = udp_hdr(skb);
		if (udph->dest == port || udph->source == port) {
			return NF_DROP;
		}
	} else if (iph->protocol == IPPROTO_TCP) {
		tcph = tcp_hdr(skb);
		if (tcph->dest == port || tcph->source == port) {
			return NF_DROP;
		}
	}

	return NF_ACCEPT;
}
