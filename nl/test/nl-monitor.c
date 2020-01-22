#include <stdio.h>   
#include <stdlib.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netlink/route/link.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>

static int print_route(struct nl_msg * msg, void * arg)
{
    struct nlmsghdr * h = nlmsg_hdr(msg);
    struct rtmsg * rte = NLMSG_DATA(h);
    struct rtattr * attr = RTM_RTA(rte);
    int remaining = RTM_PAYLOAD(h);
    char dest[64] = {0};
    char gway[64] = {"unspecified"};

    if (rte->rtm_table != RT_TABLE_MAIN) {
        return NL_OK;
    }

    for (; RTA_OK(attr, remaining); attr = RTA_NEXT(attr, remaining))
    {
        switch (attr->rta_type)
        {
			case RTA_DST:
				if (rte->rtm_dst_len < 32)
					inet_ntop(AF_INET, RTA_DATA(attr), dest, sizeof(dest));
				else
					inet_ntop(AF_INET6, RTA_DATA(attr), dest, sizeof(dest));
				break;
			case RTA_GATEWAY:
				if (rte->rtm_dst_len < 32)
					inet_ntop(AF_INET, RTA_DATA(attr), gway, sizeof(gway));
				else
					inet_ntop(AF_INET6, RTA_DATA(attr), gway, sizeof(gway));
				break;
			default:
				break;
        }
    }

    if (h->nlmsg_type == RTM_NEWROUTE) {
        printf("add ");
    }
    else if (h->nlmsg_type == RTM_DELROUTE) {
        printf("del ");
    }
    else {
        printf("nlmsg_type=%d ", h->nlmsg_type);
    }
    printf("%s/%d gateway %s\n", dest, rte->rtm_dst_len, gway);
    return NL_OK;
}


static int print_link(struct nl_msg * msg, void * arg)
{
    struct nlmsghdr * h = nlmsg_hdr(msg);
    struct ifinfomsg * iface = NLMSG_DATA(h);
    struct rtattr * attr = IFLA_RTA(iface);
    int remaining = RTM_PAYLOAD(h);
	char ifname[32] = {0};
    for (; RTA_OK(attr, remaining); attr = RTA_NEXT(attr, remaining))
    {
        switch (attr->rta_type)
        {
			case IFA_LABEL:
				//printf("interface %d : %s\n", iface->ifi_index, (char *)RTA_DATA(attr));
				strcpy(ifname, (char *)RTA_DATA(attr));
				break;			
			default:
				break;
        }
    }
	printf("%s[%s]: %s \n", (h->nlmsg_type == RTM_NEWLINK) ? "NEWLINK" : "DELLINK", ifname, (iface->ifi_flags & IFF_UP) ? "up" : "down");	


    return NL_OK;
}

static int print_addr(struct nl_msg * msg, void * arg)
{
    struct nlmsghdr * h = nlmsg_hdr(msg);
    struct ifaddrmsg  * ifaddr = NLMSG_DATA(h);
    struct rtattr * attr = IFLA_RTA(ifaddr);
    int remaining = RTM_PAYLOAD(h);
	char ifname[32] = {0};
	unsigned char bytes[4] = {0};
    for (; RTA_OK(attr, remaining); attr = RTA_NEXT(attr, remaining))
    {
        switch (attr->rta_type)
        {
			case IFA_LABEL:
				//printf("interface %d : %s\n", iface->ifi_index, (char *)RTA_DATA(attr));
				strcpy(ifname, (char *)RTA_DATA(attr));
				break;
			case IFA_LOCAL:
			{
				int ip = *(int*)RTA_DATA(attr);	//ip地址是int型
				
				bytes[0] = ip & 0xFF;
				bytes[1] = (ip >> 8) & 0xFF;
				bytes[2] = (ip >> 16) & 0xFF;
				bytes[3] = (ip >> 24) & 0xFF;
				//printf("IP Address : %d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);
				break;
			}				
			default:
				break;
        }
    }
	printf("%s[%s]: %d.%d.%d.%d \n", (h->nlmsg_type == RTM_NEWADDR) ? "NEWADDR":"DELADDR", ifname, bytes[0], bytes[1], bytes[2], bytes[3]);	


    return NL_OK;
}

void die(char * s)
{
    perror(s);
    exit(1);
}

int main(void)
{
    struct nl_sock * s = nl_socket_alloc();
    if (s == NULL) {
        die("nl_socket_alloc");
    }

    nl_socket_disable_seq_check(s);
    nl_socket_modify_cb(s, NL_CB_VALID, NL_CB_CUSTOM, print_link, NULL);

    if (nl_connect(s, NETLINK_ROUTE) < 0) {
        nl_socket_free(s);
        die("nl_connet");
    }

    nl_socket_add_memberships(s, RTNLGRP_LINK, 0);		//RTNLGRP_LINK | RTNLGRP_IPV4_IFADDR | RTNLGRP_IPV4_ROUTE | RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE;

    while(1)
    {
        nl_recvmsgs_default(s);
    }

    nl_socket_free(s);
    return 0;
}