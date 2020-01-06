#ifndef __ZXY_SOCK_H__
#define __ZXY_SOCK_H__

#include "cstring.h"

#define SOCK_MAX_HOSTNAME	    (128)


extern const uint16_t SOCK_AF_UNSPEC;
extern const uint16_t SOCK_AF_UNIX;
extern const uint16_t SOCK_AF_INET;
extern const uint16_t SOCK_AF_INET6;
extern const uint16_t SOCK_AF_PACKET;
extern const uint16_t SOCK_AF_IRDA;

extern const uint16_t SOCK_TYPE_STREAM;
extern const uint16_t SOCK_TYPE_DGRAM;
extern const uint16_t SOCK_TYPE_RAW;
extern const uint16_t SOCK_TYPE_RDM;

extern const uint16_t SOCK_SOL_SOCKET;
extern const uint16_t SOCK_SOL_IP;
extern const uint16_t SOCK_SOL_TCP;
extern const uint16_t SOCK_SOL_UDP;
extern const uint16_t SOCK_SOL_IPV6;

extern const uint16_t SOCK_IP_TOS;
extern const uint16_t SOCK_IPTOS_LOWDELAY;
extern const uint16_t SOCK_IPTOS_THROUGHPUT;
extern const uint16_t SOCK_IPTOS_RELIABILITY;
extern const uint16_t SOCK_IPTOS_MINCOST;

extern const uint16_t SOCK_IPV6_TCLASS;

extern const uint16_t SOCK_SO_TYPE;
extern const uint16_t SOCK_SO_RCVBUF;
extern const uint16_t SOCK_SO_SNDBUF;
extern const uint16_t SOCK_TCP_NODELAY;
extern const uint16_t SOCK_SO_REUSEADDR;
extern const uint16_t SOCK_SO_NOSIGPIPE;
extern const uint16_t SOCK_SO_PRIORITY;
extern const uint16_t SOCK_IP_MULTICAST_IF;
extern const uint16_t SOCK_IP_MULTICAST_TTL;
extern const uint16_t SOCK_IP_MULTICAST_LOOP;
extern const uint16_t SOCK_IP_ADD_MEMBERSHIP;
extern const uint16_t SOCK_IP_DROP_MEMBERSHIP;

extern const int SOCK_MSG_OOB;
extern const int SOCK_MSG_PEEK;
extern const int SOCK_MSG_DONTROUTE;

/* socket_sd_type */
enum 
{
    SOCK_SD_RECEIVE		= 0,    /**< No more receive.	    */
    SOCK_SHUT_RD	    = 0,    /**< Alias for SD_RECEIVE.	    */
    SOCK_SD_SEND	    = 1,    /**< No more sending.	    */
    SOCK_SHUT_WR	    = 1,    /**< Alias for SD_SEND.	    */
    SOCK_SD_BOTH	    = 2,    /**< No more send and receive.  */
    SOCK_SHUT_RDWR		= 2     /**< Alias for SD_BOTH.	    */
};

#define SOCK_INADDR_ANY				((uint32_t)0)
#define SOCK_INADDR_BROADCAST		((uint32_t)0xffffffff)
#define SOCK_INADDR_NONE			((uint32_t)0xffffffff)
#define SOCK_INVALID_FD				(-1)
#define SOCK_CONN_MAX				(5)
#define SOCK_INET_ADDRSTRLEN		(16)
#define SOCK_INET6_ADDRSTRLEN		(46)
#define SOCK_ADDR_IN_SIN_ZERO_LEN	(8)

typedef void T_SockAddr;		//T_SockAddr = T_CSockAddr

#undef s_addr
typedef struct _tagInAddr
{
    uint32_t	s_addr;		/**< The 32bit IP address.	    */
}T_InAddr;

typedef struct _tagSockAddrIn
{
    uint16_t	sin_family;	/**< Address family.		    */
    uint16_t	sin_port;	/**< Transport layer port number.   */
    T_InAddr	sin_addr;	/**< IP address.		    */
    char	sin_zero[SOCK_ADDR_IN_SIN_ZERO_LEN]; /**< Padding.*/
}T_SockAddrIn;

#undef s6_addr
typedef union _tagIn6Addr
{
    uint8_t		s6_addr[16];   /**< 8-bit array */
    uint32_t	u6_addr32[4];
}T_In6Addr;

#define SOCK_IN6ADDR_ANY_INIT			{ { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#define SOCK_IN6ADDR_LOOPBACK_INIT		{ { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }

typedef struct _tagSockAddrIn6
{
    uint16_t	sin6_family;	    /**< Address family		    */

    uint16_t	sin6_port;	    /**< Transport layer port number. */
    uint32_t	sin6_flowinfo;	    /**< IPv6 flow information	    */
    T_In6Addr sin6_addr;	    /**< IPv6 address.		    */
    uint32_t sin6_scope_id;	    /**< Set of interfaces for a scope	*/
}T_SockAddrIn6;

typedef struct _tagAddrHdr
{
    uint16_t	sa_family;	/**< Common data: address family.   */
}T_AddrHdr;

typedef union _tagSockAddr
{
    T_AddrHdr		addr;	/**< Generic transport address.	    */
    T_SockAddrIn	ipv4;	/**< IPv4 transport address.	    */
    T_SockAddrIn6	ipv6;	/**< IPv6 transport address.	    */
}T_CSockAddr;

typedef struct _tagIpMreq
{
    T_InAddr imr_multiaddr;	/**< IP multicast address of group. */
    T_InAddr imr_interface;	/**< local IP address of interface. */
}T_IpMreq;

#define SOCK_MAX_SOCKOPT_PARAMS 4

typedef struct _tagSockoptParams
{
    unsigned cnt;
    struct {
		int level;
		int optname;
		void *optval;
		int optlen;
    }options[SOCK_MAX_SOCKOPT_PARAMS];
}T_SockoptParams;

/*****************************************************************************
 *
 * SOCKET ADDRESS MANIPULATION.
 *
 *****************************************************************************/
/*
	ntohs	//net uint16_t to host uint16_t
	htons	//host uint16_t to net uint16_t
	
	ntohl	//net uint32_t to host uint32_t
	htonl	//host uint32_t to net uint32_t
	
	inet_aton/inet_addr	//将字符串形式的IP地址 -> 网络字节顺序  的整型值
	inet_ntoa 	//网络字节顺序的整型值 -> 字符串形式的IP地址
	
	inet_pton	//ipv6和ipv4，inet_addr的扩展
	inet_ntop	//ipv6和ipv4，inet_ntoa的扩展
	
	
*/

API_DECL(char*)	INET_CNTOA(T_InAddr inaddr);
API_DECL(int)	INET_CATON(const str_t *cp, T_InAddr *inp);
API_DECL(int)	INET_CPTON(int af, const str_t *src, void *dst);
API_DECL(int)	INET_CNTOP(int af, const void *src, char *dst, int size);
API_DECL(char*) INET_CNTOP2(int af, const void *src, char *dst, int size);
API_DECL(T_InAddr) INET_CADDR(const str_t *cp);
API_DECL(T_InAddr) INET_CADDR2(const char *cp);

API_DECL(char*)	SOCK_AddrPrint(const T_SockAddr *addr, char *buf, int size, unsigned flags);
API_DECL(int)	SOCK_InitAddrIn(T_SockAddrIn *addr, const str_t *cp, uint16_t port);
API_DECL(int)	SOCK_InitCSockAddr(int af, T_CSockAddr *addr, const str_t *cp, uint16_t port);
API_DECL(int)	SOCK_AddrCompare(const T_SockAddr *addr1, const T_SockAddr *addr2);
API_DECL(void*)	SOCK_AddrGetAddrP32(const T_SockAddr *addr);
API_DECL(char)	SOCK_AddrIsNAddrAny(const T_SockAddr *addr);
API_DECL(unsigned) SOCK_AddrGetAddrLen(const T_SockAddr *addr);
API_DECL(unsigned) SOCK_AddrGetLen(const T_SockAddr *addr);
API_DECL(void)	SOCK_CSockAddrCopy(T_CSockAddr *dst, const T_CSockAddr *src);
API_DECL(void) 	SOCK_AddrCopy(T_SockAddr *dst, const T_SockAddr *src);
API_DECL(int)	SOCK_AddrSynthesize(int dst_af, T_SockAddr *dst, const T_SockAddr *src);
API_DECL(T_InAddr) SOCK_AddrInGetAddrI32(const T_SockAddrIn *addr);
API_DECL(void)	SOCK_I32SetAddrIn(T_SockAddrIn *addr, uint32_t hostaddr);
API_DECL(int)	SOCK_SetStrToAddrIn(T_SockAddrIn *addr, const str_t *cp);
API_DECL(int)	SOCK_SetStrToCSockAddr(int af, T_CSockAddr *addr,const str_t *cp);
API_DECL(uint16_t) SOCK_AddrGetPort(const T_SockAddr *addr);
API_DECL(uint16_t) SOCK_ADDR_AddrInGetPort(const T_SockAddrIn *addr);
API_DECL(int)	SOCK_CSockAddrSetPort(T_CSockAddr *addr,  uint16_t hostport);
API_DECL(void)	SOCK_AddrInSetPort(T_SockAddrIn *addr,  uint16_t hostport);

API_DECL(int) SOCK_CSockAddrParse(int af, unsigned options, const str_t *str, T_CSockAddr *addr);
/*****************************************************************************
 *
 * HOST NAME AND ADDRESS.
 *
 *****************************************************************************/
API_DECL(const str_t*)	SOCK_GetHostname(void);
API_DECL(T_InAddr)		SOCK_ADDR_GetHostAddr32(void);
/*****************************************************************************
 *
 * SOCKET API.
 *
 *****************************************************************************/
API_DECL(int) SOCK_Socket(int family,  int type, int protocol, int *sock);
API_DECL(int) SOCK_Close(int sockfd);
API_DECL(int) SOCK_Bind(int sockfd,  const T_SockAddr *my_addr, int addrlen);
API_DECL(int) SOCK_BindAddrIn(int sockfd,  uint32_t addr, uint16_t port);
API_DECL(int) SOCK_BindRandom(int sockfd, const T_SockAddr *addr, uint16_t port_range, uint16_t max_try);
API_DECL(int) SOCK_Listen(int sockfd,  int backlog );
API_DECL(int) SOCK_Accept(int serverfd, int *newsock, T_SockAddr *addr, int *addrlen);
API_DECL(int) SOCK_Connect(int sockfd, const T_SockAddr *serv_addr, int addrlen);
API_DECL(int) SOCK_GetPeerName(int sockfd, T_SockAddr *addr, int *namelen);
API_DECL(int) SOCK_GetSockName(int sockfd, T_SockAddr *addr,int *namelen);
API_DECL(int) SOCK_GetSockopt(int sockfd, uint16_t level, uint16_t optname, void *optval, int *optlen);
API_DECL(int) SOCK_SetSockopt(int sockfd, uint16_t level, uint16_t optname, const void *optval, int optlen);
API_DECL(int) SOCK_SetSockoptParams(int sockfd, const T_SockoptParams *params);					       
API_DECL(int) SOCK_SetSockoptSobuf(int sockfd, uint16_t optname, char auto_retry, unsigned *buf_size);
API_DECL(int) SOCK_Recv(int sockfd, void *buf, ssize_t *len,  unsigned flags);
API_DECL(int) SOCK_Recvfrom(int sockfd, void *buf, ssize_t *len, unsigned flags, T_SockAddr *from, int *fromlen);
API_DECL(int) SOCK_Send(int sockfd, const void *buf, ssize_t *len, unsigned flags);
API_DECL(int) SOCK_Sendto(int sockfd, const void *buf, ssize_t *len, unsigned flags, const T_SockAddr *to, int tolen);
API_DECL(int) SOCK_Shutdown(int sockfd,int how);



typedef struct _tagHostent
{
    char	*h_name;		/**< The official name of the host. */
    char	**h_aliases;		/**< Aliases list. */
    int		h_addrtype;	/**< Host address type. */
    int		h_length;		/**< Length of address. */
    char	**h_addr_list;	/**< List of addresses. */
}T_Hostent;

/** Shortcut to h_addr_list[0] */
#define h_addr h_addr_list[0]

typedef struct _tagAddrInfo
{
    char			ai_canonname[SOCK_MAX_HOSTNAME]; 	/**< Canonical name for host*/
    T_CSockAddr		ai_addr;							/**< Binary address.	    */
}T_CAddrInfo;


typedef union _tagIpRouteEntry
{
    /** IP routing entry for IP version 4 routing */
    struct
    {
		T_InAddr	if_addr;    /**< Local interface IP address.	*/
		T_InAddr	dst_addr;   /**< Destination IP address.	*/
		T_InAddr	mask;	    /**< Destination mask.		*/
    } ipv4;
}T_IPRouteEntry;


API_DECL(int) SOCK_IP_EnumInterface(int af, unsigned *count, T_CSockAddr ifs[]);
API_DECL(int) SOCK_IP_EnumRoute(unsigned *count, T_IPRouteEntry routes[]);

API_DECL(int) SOCK_GetHostByName(const str_t *name, T_Hostent *he);
API_DECL(int) SOCK_GetHostIp(int af, T_CSockAddr *addr);
API_DECL(int) SOCK_GetIpInterface(int af, const str_t *dst, T_CSockAddr *itf_addr, char allow_resolve, T_CSockAddr *p_dst_addr);
API_DECL(int) SOCK_GetDefaultIpInterface(int af, T_CSockAddr *addr);
API_DECL(int) SOCK_GetCSockAddrInfo(int af, const str_t *name, unsigned *count, T_CAddrInfo ai[]);





#endif	/* __ZXY_SOCK_H__ */

