#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define BUF_LEN 8192 

struct route_info {
	u_int dst_addr;
	u_int src_addr;
	u_int gateway;
	char if_name[IF_NAMESIZE];
};

int reply_msg(int sd, struct sockaddr_nl dest_addr);
void print_route(struct nlmsghdr *nhl);

char *ntoa(int addr) {
	static char buffer[18];
	sprintf(buffer, "%d.%d.%d.%d",
					(addr & 0x000000FF)      ,
					(addr & 0x0000FF00) >>  8,
					(addr & 0x00FF0000) >> 16,
					(addr & 0xFF000000) >> 24);
	return buffer;
}

int reply_msg(int sd, struct sockaddr_nl dest_addr) {
  struct msghdr rtnl_reply;	
  struct iovec io_reply;
  struct nlmsghdr *msg_ptr;
  
  int len = 0;
  char message[BUF_LEN];
  
  memset(&io_reply, 0, sizeof(io_reply));
  memset(&rtnl_reply, 0, sizeof(rtnl_reply));
  
  io_reply.iov_base = message;
  io_reply.iov_len = BUF_LEN;
  rtnl_reply.msg_iov = &io_reply;
  rtnl_reply.msg_iovlen = 1;
  rtnl_reply.msg_name = &dest_addr;
  rtnl_reply.msg_namelen = sizeof(dest_addr);

  len = recvmsg(sd, &rtnl_reply, 0);
  if (len < 0) {
    perror("recvmsg()");
    return -1;
  }
  
  msg_ptr = (struct nlmsghdr *) message;
  for (; NLMSG_OK(msg_ptr, len); msg_ptr = NLMSG_NEXT(msg_ptr, len)) {
      switch(msg_ptr->nlmsg_type) {
          case 3:	
            break;
          case 16:
            //rtnl_print_link(msg_ptr);
            break;
          case 24:
            	print_route(msg_ptr);
            break;
          default:
            printf("message type %d, length %d\n", 
                    msg_ptr->nlmsg_type, msg_ptr->nlmsg_len);
            break;
        }
  }

  return 0;
}

void print_route(struct nlmsghdr *nlh) {
  struct rtmsg *rt_entry;
  struct rtattr *rt_attr;
  struct route_info rt_info;
  int rt_len = 0;
  unsigned char rt_mask = 0;

  rt_entry = (struct rtmsg *) NLMSG_DATA(nlh);
  rt_attr = (struct rtattr *) RTM_RTA(rt_entry);
  rt_mask = rt_entry->rtm_dst_len; 
  
  if (rt_entry->rtm_table != RT_TABLE_MAIN)
    return;

  rt_len = RTM_PAYLOAD(nlh);

  for (; RTA_OK(rt_attr, rt_len); rt_attr = RTA_NEXT(rt_attr, rt_len)) {
 		   switch(rt_attr->rta_type) {
					case RTA_OIF:
						if_indextoname(*(int *)RTA_DATA(rt_attr), rt_info.if_name);
						break;
					case RTA_GATEWAY:
						rt_info.gateway = *(u_int *) RTA_DATA(rt_attr);
						break;
					case RTA_PREFSRC:
						rt_info.src_addr = *(u_int *) RTA_DATA(rt_attr);
						break;
					case RTA_DST:
						rt_info.dst_addr = *(u_int *) RTA_DATA(rt_attr);
						break;
			} 
	}
	
	printf("%s\t", rt_info.dst_addr ? ntoa(rt_info.dst_addr) : "0.0.0.0  ");
	printf("%s\t", rt_info.gateway ? ntoa(rt_info.gateway) : "*.*.*.*");
	printf("%s\t", rt_info.if_name);
	printf("%s\n", rt_info.src_addr ? ntoa(rt_info.src_addr) : "*.*.*.*");
}

int main() {
	struct sockaddr_nl dest_addr, src_addr;
	struct nlmsghdr nlh;
	struct rtgenmsg gen;
	struct msghdr msg;
	struct iovec iov;

  int sd = 0, ret_val = 0;
  
  sd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sd < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  memset(&dest_addr, 0, sizeof(dest_addr));
  memset(&src_addr, 0, sizeof(src_addr));
	memset(&nlh, 0, sizeof(NLMSG_SPACE(BUF_LEN)));
  memset(&msg, 0, sizeof(msg));
  memset(&iov, 0, sizeof(iov));
	
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid();
  src_addr.nl_groups = 0;

  dest_addr.nl_family = AF_NETLINK;

  nlh.nlmsg_len = NLMSG_SPACE(BUF_LEN);
  nlh.nlmsg_type = RTM_GETROUTE;
  nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  nlh.nlmsg_pid = getpid();
  nlh.nlmsg_seq = 1;
  gen.rtgen_family = AF_INET;

  iov.iov_base = (void *) &nlh;
	iov.iov_len = nlh.nlmsg_len;

	msg.msg_name = (void *) &(dest_addr);
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	ret_val = sendmsg(sd, &msg, 0);
  if (ret_val < 0) {
    perror("sendmsg()");
    exit(EXIT_FAILURE);
  }
	
  ret_val = reply_msg(sd, dest_addr);

	close(sd);
	exit(EXIT_SUCCESS);
}
