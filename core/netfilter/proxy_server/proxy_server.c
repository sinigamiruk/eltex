#include "proxy_server.h"

static uint16_t dns_port;

static int __init m_init_module(void) {
	nf_register_hook(&n_ops_in);
	nf_register_hook(&n_ops_out);

	dns_port = htons(53);
	
	pr_info("Module (proxy_server) is loaded\n");
	return 0;
}

static void __exit m_cleanup_module(void) {
	nf_unregister_hook(&n_ops_in);
	nf_unregister_hook(&n_ops_out);
	
	pr_info("Module (proxy_server) is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(m_init_module);
module_exit(m_cleanup_module);

static uint32_t 
hook_in_packets(void *priv, struct sk_buff *skb, 
							const struct nf_hook_state *state) {
	struct iphdr *iph = NULL;
	struct udphdr *udph = NULL;
	struct dnshdr *dnsh = NULL;
	struct dns_node *node = NULL;
	struct r_data *r_data = NULL;
	unsigned char *hname = NULL;
	int offset = 0, name_len = 0;

	iph = ip_hdr(skb);
	hname = kmalloc(HOSTNAME_LEN * sizeof(char), GFP_KERNEL);

	if (iph->protocol != IPPROTO_UDP)
		goto out;

	udph = udp_hdr(skb);

	if (udph->source != dns_port)	
		goto out;

	dnsh = (struct dnshdr*) (udph + 1);
	
	strncpy(hname,
			((unsigned char *) (dnsh + 1)), 
			HOSTNAME_LEN);

	name_len = strlen(hname);
	pr_info("[IN] name_len: %d", name_len);

	node = search(&dns_tree, hname);
	if (node == NULL) {
		node = kmalloc(sizeof(struct dns_node), GFP_KERNEL);
		offset = sizeof(struct dnshdr) + name_len + 5;
		r_data = (struct r_data *) ((unsigned char *) dnsh + offset);
		
		if (ntohs(r_data->type) != 1 && ntohs(r_data->data_len) != 4)
			goto out;

		node->addr = r_data->addr;
		node->ttl = r_data->ttl;
		strncpy(node->host_name, hname, HOSTNAME_LEN);

		if (insert(&dns_tree, node) > 0) {
			pr_info("[IN] Insert is successful");
		}

		pr_info("[IN] dns: id = 0x%x name = %s", ntohs(dnsh->id), node->host_name);
		pr_info("[IN] type: %hu ttl: %d addr: %pI4", 
			ntohs(r_data->type), ntohl(r_data->ttl), &node->addr);
		pr_info("[IN] skb_len: %d", skb->len);
	}

out:
	kfree(hname);
	return NF_ACCEPT;
}

static uint32_t 
hook_out_packets(void *priv, struct sk_buff *skb,
				const struct nf_hook_state *state) {
	struct iphdr *iph = NULL;
	struct dnshdr *dnsh = NULL;
	struct udphdr *udph = NULL;
	struct dns_node *node = NULL;

	unsigned char *hname = NULL;
	int name_len = 0;

	hname = kmalloc(HOSTNAME_LEN * sizeof(char), GFP_KERNEL);

	iph = ip_hdr(skb);
	if (iph->protocol != IPPROTO_UDP)
		goto out_accept;

	udph = udp_hdr(skb);
	if (udph->dest != dns_port)	
		goto out_accept;

	dnsh = (struct dnshdr*) (udph + 1);
	strncpy(hname,
			((unsigned char *) (dnsh + 1)), 
			HOSTNAME_LEN);
	name_len = strlen(hname);

	//pr_info("[OUT] name: %s", hname);

	node = search(&dns_tree, hname);
	if (node != NULL) {
		pr_info("[OUT] Host name is finded: %s", node->host_name);

		if (create_packet(skb, node) == 0)
			goto out_drop;
	}

out_accept:
	kfree(hname);
	return NF_ACCEPT;

out_drop:
	kfree(hname);
	return NF_DROP;
}

static struct dns_node *search(struct rb_root *root, char *string) {
  struct rb_node *node = root->rb_node;

	while (node) {              
		struct dns_node *data = container_of(node, struct dns_node, node);
		int result;

		result = strcmp(string, data->host_name);

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return data;
	}
	return NULL;
}

static int insert(struct rb_root *root, struct dns_node *data) {
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	while (*new) {
		struct dns_node *this = container_of(*new, struct dns_node, node);
		int result = strcmp(data->host_name, this->host_name);

		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return 0;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, root);

	return 1;
}

static int create_packet(struct sk_buff *skb_old, struct dns_node *node) {
	struct iphdr *iph = NULL;
	struct udphdr *udph = NULL;
	struct r_data *req_data = NULL;
	struct sk_buff *skb = NULL;
	struct dnshdr *dnsh = NULL; 
	struct rtable *rt = NULL;

	int req_offset = 0, ret_val = 0;
	uint32_t temp = 0;

	skb = skb_copy_expand(
					skb_old,
					0, 
					sizeof(struct r_data),
					GFP_ATOMIC);
	if (skb == NULL) {
		pr_alert("skb_copy_expand()");
		return -1;
	}
	
	skb->len += sizeof(struct r_data);
	skb->tail += sizeof(struct r_data);
	/*skb->end += sizeof(struct r_data);*/
	/*memset(skb->head, 0, skb->mac_len);*/

	req_offset = sizeof(struct udphdr) + 
					sizeof(struct dnshdr) +
					strlen(node->host_name) + 5;

	iph = ip_hdr(skb);
	iph->tot_len = htons(skb->len);

	temp = iph->saddr;
	iph->saddr = iph->daddr;
	iph->daddr = temp;

	udph = udp_hdr(skb);

	temp = udph->dest;
	udph->dest = udph->source;
	udph->source = temp;
	udph->len = htons(ntohs(udph->len) + sizeof(struct r_data));

	dnsh = (struct dnshdr *) (udph + 1);

    dnsh->qr = 1;
    dnsh->rd = 1;
    dnsh->ra = 1;
    dnsh->ad = 0;
    dnsh->q_count = htons(1);
    dnsh->ans_count = htons(1);
    dnsh->auth_count = htons(0);
    dnsh->add_count = htons(0);

	/*fl4.saddr = iph->saddr;
	fl4.daddr = iph->daddr;
	fl4.flowi4_oif = skb_old->dev->ifindex;*/

	rt = skb_rtable(skb_old);

	//rt = ip_route_output_key(&init_net, &fl4);
	pr_info("[OUT] name from rtable: %s", rt->dst.dev->name);
	pr_info("[OUT] name: %s", skb_old->dev->name);
	skb_dst_set(skb, &rt->dst);

	req_data = (struct r_data *) ((unsigned char *) udph + req_offset);

	req_data->name = htons(0xc00c);
	req_data->type = htons(1);
	req_data->_class = htons(1);
	req_data->ttl = node->ttl;
	req_data->data_len = htons(4);
	req_data->addr = node->addr;

	if(IS_ERR_OR_NULL(skb)) {
		pr_info("[OUT] skb with errors!!!");
	}

	pr_info("[OUT] UDP_SRC: %d UDP_DST: %d", ntohs(udph->source), ntohs(udph->dest));

	pr_info("[OUT] IP_src: %pI4", &iph->saddr);
	pr_info("[OUT] IP_dest: %pI4", &iph->daddr);
	pr_info("[OUT] IP_addr: %pI4", &req_data->addr);
	pr_info("[OUT] type: %hu", ntohs(req_data->type));
	pr_info("[OUT] skb_len: %d", skb->len);
	pr_info("[OUT] ip_len: %d", ntohs(iph->tot_len));

	ret_val = ip_local_out(&init_net, NULL, skb);
	return ret_val;
}
