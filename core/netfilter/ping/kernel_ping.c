#include "kernel_ping.h"

static int __init m_init_module(void) {
	struct sk_buff *skb = NULL;
	create_packet(skb);
	return 0;
}

static void __exit m_cleanup_module(void) {
	/*nf_unregister_hook(&nops);*/
}

MODULE_LICENSE("GPL");
module_init(m_init_module);
module_exit(m_cleanup_module);

static int create_packet(struct sk_buff *skb) {
	struct iphdr *iph = NULL;
	struct icmphdr *icmph = NULL;

	struct net_device *ndev = NULL;
	struct rtable *rt = NULL;
	struct flowi4 fl4;
	
	unsigned int size = 0;
	/*int i = 0;*/

	size = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct icmphdr);

	skb = alloc_skb(size, GFP_ATOMIC);
	if (skb == NULL) {
		pr_alert("alloc_skb()");
		return -1;
	}

	skb_reserve(skb, size);
	skb_push(skb, size - sizeof(struct ethhdr));
	skb_set_mac_header(skb, 0);
	skb_set_network_header(skb, 0);
	skb_set_transport_header(skb, sizeof(struct iphdr));

	ndev = dev_get_by_name(&init_net, IFNAME);

	fl4.saddr = inet_addr(SADDR);
	fl4.daddr = inet_addr(DADDR);
	fl4.flowi4_oif = ndev->ifindex;

	rt = ip_route_output_key(&init_net, &fl4);
	skb_dst_set(skb, &rt->dst);
	skb->dev = ndev;

	iph = ip_hdr(skb);
	iph->ihl = sizeof(struct iphdr) / 4;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) 
    	+ sizeof(struct icmphdr));
    iph->id = htons(12345);
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_ICMP;
    iph->check = 0;
	iph->saddr = fl4.saddr;
	iph->daddr = fl4.daddr;
	
	icmph = icmp_hdr(skb);
	icmph->type = ICMP_ECHO;
	icmph->code = 0;
 	icmph->un.echo.sequence = 1231;
 	icmph->un.echo.id = 0;
 	icmph->checksum = ip_compute_csum(icmph, 
 		sizeof(struct icmphdr));

	ip_local_out(&init_net, NULL, skb);
	return 0;
}

static unsigned int inet_addr(char *str) { 
	int a, b, c, d;
	char arr[4];
	sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d);
	arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d;
	return *(unsigned int*) arr;
}
