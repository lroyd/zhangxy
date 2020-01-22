#include <stdio.h>   
#include <stdlib.h>
#include <arpa/inet.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>

static int print_link(struct nl_msg * msg, void * arg)
{
    struct nlmsghdr * h = nlmsg_hdr(msg);
    struct ifinfomsg * iface = NLMSG_DATA(h);
    struct rtattr * attr = IFLA_RTA(iface);
    int remaining = RTM_PAYLOAD(h);

    for (; RTA_OK(attr, remaining); attr = RTA_NEXT(attr, remaining))
    {
        switch (attr->rta_type)
        {
			case IFLA_IFNAME:
				printf("Interface %d : %s\n", iface->ifi_index, (char *)RTA_DATA(attr));
				break;
				
			case IFLA_ADDRESS:
				printf("xxxxxx %d : %d\n", iface->ifi_index, nla_get_u32((char *)RTA_DATA(attr)));
				break;				
				
			default:
				break;
        }
    }

    return NL_OK;
}


static int print_addr(struct nl_msg * msg, void * arg)
{
    struct nlmsghdr * h = nlmsg_hdr(msg);
    struct ifaddrmsg * addr = NLMSG_DATA(h);
    struct rtattr * attr = IFLA_RTA(addr);
    int remaining = RTM_PAYLOAD(h);

    for (; RTA_OK(attr, remaining); attr = RTA_NEXT(attr, remaining))
    {
        switch (attr->rta_type)
        {
			case IFA_LABEL:
				printf("Interface  : %s\n", (char *)RTA_DATA(attr));
				break;
			case IFA_LOCAL:
			{
				int ip = *(int*)RTA_DATA(attr);	//ip地址是int型
				unsigned char bytes[4];
				bytes[0] = ip & 0xFF;
				bytes[1] = (ip >> 8) & 0xFF;
				bytes[2] = (ip >> 16) & 0xFF;
				bytes[3] = (ip >> 24) & 0xFF;
				printf("IP Address : %d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);
				break;
			}
			default:
				break;
        }
    }

    return NL_OK;
}

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
				
			case RTA_METRICS:	
				printf("11111111  : %s\n", (char *)RTA_DATA(attr));
				break;
				
			case RTA_OIF:	
				printf("222222222  : %s\n", (char *)RTA_DATA(attr));
				break;	
				
			default:
				break;
        }
    }

    printf("%s/%d gateway %s\n", dest, rte->rtm_dst_len, gway);
    return NL_OK;
}


void die(char * s)
{
    perror(s);
    exit(1);
}

int main1(void)
{
    struct nl_sock * s = nl_socket_alloc();
    if (s == NULL) {
        die("nl_socket_alloc");
    }

    if (nl_connect(s, NETLINK_ROUTE) < 0) {
        nl_socket_free(s);
        die("nl_connet");
    }

    struct rtgenmsg rt_hdr = { .rtgen_family = AF_NETLINK, };
	//RTM_GETLINK, RTM_GETADDR, RTM_GETROUTE, 
    if (nl_send_simple(s, RTM_GETADDR, NLM_F_REQUEST|NLM_F_DUMP, &rt_hdr, sizeof(rt_hdr)) < 0) {
        nl_socket_free(s);
        die("nl_send_simple");
    }

    //Retrieve the kernel's answer.
    nl_socket_modify_cb(s, NL_CB_VALID, NL_CB_CUSTOM, print_addr, NULL);
    nl_recvmsgs_default(s);

    nl_socket_free(s);
    return 0;
}

#include <netinet/ether.h>
#include <net/if.h>
#include <netlink/route/link.h>
#include <netlink/route/link/macvlan.h>
int main(void)
{
	struct nl_sock *sk;
	struct rtnl_link *link, *change;
	struct nl_cache *cache;
	int err = 0, master_index;

	sk = nl_socket_alloc();
	if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0) {
		nl_perror(err, "Unable to connect socket");
		return err;
	}

	if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &cache)) < 0) {
		nl_perror(err, "Unable to allocate cache");
		goto out;
	}

	if (!(link = rtnl_link_get_by_name(cache, "eth0"))) {
		fprintf(stderr, "Interface not found\n");
		err = 1;
		goto out;
	}
	
	printf("00000000 name = %s\n", rtnl_link_get_name(link));

	/* exit if the loopback interface is already deactivated */
	err = rtnl_link_get_flags(link);
	if (!(err & IFF_UP)) {
		err = 0;
		printf("lo interface has stop\n");
		goto out;
	}
	
#if 0
	//修改mac地址需要上下网卡
	change = rtnl_link_alloc();
	rtnl_link_unset_flags(change, IFF_UP);	//清除IFF_UP标志(down)

	if ((err = rtnl_link_change(sk, link, change, 0)) < 0) {
		nl_perror(err, "Unable to deactivate lo");
		goto out;
	}
	
	//设定mac地址		//30:4A:26:14:5A:A5
	struct nl_addr* addr ;
	char host[256];
	
	addr = nl_addr_build(AF_LLC, ether_aton("00:11:22:33:44:55"), ETH_ALEN);
	rtnl_link_set_addr(change, addr);
	nl_addr_put(addr);	
	
	rtnl_link_set_flags(change, IFF_UP);
	if ((err = rtnl_link_change(sk, link, change, 0)) < 0) {
		nl_perror(err, "Unable to activate lo");
		goto out;
	}	
	
	//读取mac地址
	addr = rtnl_link_get_addr(change);
	if (addr){
		printf("mac address = %s\n", nl_addr2str(addr, host, sizeof(host)));		
	}else{
		printf("4444444444\n");
	}

#endif	

#if 1	
	//设置ip地址
	struct rtnl_addr *local = rtnl_addr_alloc();
	rtnl_addr_set_label(local, "eth0");
	
	if ((err = rtnl_addr_alloc_cache(sk, &cache)) < 0) {
		nl_perror(err, "Unable to allocate cache");
		goto out;
	}
	
	local = rtnl_addr_get(cache, 1, local);
	if (local){
		
	}else{
		printf("not found eth0\n");
	}
	
	
#endif




#if 0
	//构造RTM_DELLINK
	//相当于 ifdown eth0
	change = rtnl_link_alloc();
	rtnl_link_unset_flags(change, IFF_UP);	//清除IFF_UP标志(down)

	if ((err = rtnl_link_change(sk, link, change, 0)) < 0) {
		nl_perror(err, "Unable to deactivate lo");
		goto out;
	}
	
	printf("1111111111111\n");
	//构造RTM_NEWLINK
	//相当于 ifup eth0
	rtnl_link_set_flags(change, IFF_UP);
	if ((err = rtnl_link_change(sk, link, change, 0)) < 0) {
		nl_perror(err, "Unable to activate lo");
		goto out;
	}

#endif
	err = 0;

out:
	nl_socket_free(sk);
	return err;	
}


