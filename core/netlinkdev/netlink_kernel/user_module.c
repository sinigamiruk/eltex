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
#define NETLINK_CUSTOM_PROTOCOL 24

int main() {
	struct sockaddr_nl dest_addr, src_addr;
	struct nlmsghdr nlh;
	struct msghdr msg;
	struct iovec iov;

  int sd = 0, ret_val = 0;
  
  sd = socket(AF_NETLINK, SOCK_RAW, NETLINK_CUSTOM_PROTOCOL);
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
	strcpy(NLMSG_DATA(&nlh), "Hello");

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
	
	recvmsg(sd, &msg, 0);
	printf("Recv message: %s\n", (char *) NLMSG_DATA(&nlh));
	close(sd);
	exit(EXIT_SUCCESS);
}
