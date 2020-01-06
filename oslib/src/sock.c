#include <stdio.h>
#include <string.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <sys/socket.h>

#include "config_site.h"
#include "sock.h"
#include "ctype.h"

#define CHECK_STACK()		//不使用

char *INET_CNTOA(T_InAddr inaddr)
{
    struct in_addr addr;
    memcpy(&addr, &inaddr, sizeof(addr));
    return inet_ntoa(addr);
}

int INET_CATON(const str_t *cp, T_InAddr *inp)
{
    char tempaddr[SOCK_INET_ADDRSTRLEN];
    inp->s_addr = SOCK_INADDR_NONE;
    ASSERT_RETURN(cp && cp->slen && inp, 0);
    if (cp->slen >= SOCK_INET_ADDRSTRLEN) return 0;

    memcpy(tempaddr, cp->ptr, cp->slen);
    tempaddr[cp->slen] = '\0';

#if defined(ZXY_HAS_SOCK_INET_ATON) && ZXY_HAS_SOCK_INET_ATON != 0
    return inet_aton(tempaddr, (struct in_addr *)inp);
#else
    inp->s_addr = inet_addr(tempaddr);
    return inp->s_addr == SOCK_INADDR_NONE ? 0 : 1;
#endif
}

int INET_CPTON(int af, const str_t *src, void *dst)
{
    char tempaddr[SOCK_INET6_ADDRSTRLEN];
    ASSERT_RETURN(af == SOCK_AF_INET || af == SOCK_AF_INET6, EO_AFNOTSUP);
    ASSERT_RETURN(src && src->slen && dst, EO_INVAL);

    if (af == SOCK_AF_INET) ((T_InAddr*)dst)->s_addr = SOCK_INADDR_NONE;

    if (src->slen >= SOCK_INET6_ADDRSTRLEN) return EO_NAMETOOLONG;
    memcpy(tempaddr, src->ptr, src->slen);
    tempaddr[src->slen] = '\0';

#if defined(ZXY_HAS_SOCK_INET_PTON) && ZXY_HAS_SOCK_INET_PTON != 0
	//inet_pton 返回：若成功则为1,若输入不是有效的表达格式则为0,若出错则为-1
	int status = inet_pton(af, tempaddr, dst);
    if (status == 0)
		status = EO_INVAL;
	else if (status == 1)
		status = EO_SUCCESS;
	else
		status = EO_UNKNOWN;
    return status;
#elif !defined(ZXY_HAS_SOCK_IPV6) || ZXY_HAS_SOCK_IPV6==0
    return EO_IPV6NOTSUP;
#else
    assert(!"not supported");
    return EO_IPV6NOTSUP;
#endif
}

int INET_CNTOP(int af, const void *src, char *dst, int size)
{
    ASSERT_RETURN(src && dst && size, EO_INVAL);
	ASSERT_RETURN(af == SOCK_AF_INET || af == SOCK_AF_INET6, EO_AFNOTSUP);
    *dst = '\0';
#if defined(ZXY_HAS_SOCK_INET_PTON) && ZXY_HAS_SOCK_INET_PTON != 0
	//返回：若成功则为指向结果的指针， 若出错则为NULL
    if (inet_ntop(af, src, dst, size) == NULL) return EO_FAILED;
   
    return EO_SUCCESS;
#elif !defined(ZXY_HAS_SOCK_IPV6) || ZXY_HAS_SOCK_IPV6==0
    return EO_IPV6NOTSUP;
#else
    assert(!"not supported");
    return EO_IPV6NOTSUP;
#endif
}

char *INET_CNTOP2(int af, const void *src, char *dst, int size)
{
    int status;
    status = INET_CNTOP(af, src, dst, size);
    return (status == EO_SUCCESS)? dst : NULL;
}

T_InAddr INET_CADDR(const str_t *cp)
{
    T_InAddr addr;
    INET_CATON(cp, &addr);
    return addr;
}

T_InAddr INET_CADDR2(const char *cp)
{
    str_t str = c_str((char*)cp);
    return INET_CADDR(&str);
}

char *SOCK_AddrPrint(const T_SockAddr *addr, char *buf, int size, unsigned flags)
{
    enum {
		WITH_PORT = 1,
		WITH_BRACKETS = 2
    };

    char txt[SOCK_INET6_ADDRSTRLEN];
    char port[32];
    const T_AddrHdr *h = (const T_AddrHdr*)addr;
    char *bquote, *equote;
	CHECK_STACK();
    if (INET_CNTOP(h->sa_family, SOCK_AddrGetAddrP32(addr), txt, sizeof(txt))) 
		return "";
	
    if (h->sa_family != SOCK_AF_INET6 || (flags & WITH_BRACKETS)==0){
		bquote = ""; equote = "";
    } 
	else{
		bquote = "["; equote = "]";
    }

    if (flags & WITH_PORT){
		snprintf(port, sizeof(port), ":%d", SOCK_AddrGetPort(addr));
    } 
	else {
		port[0] = '\0';
    }
    snprintf(buf, size, "%s%s%s%s", bquote, txt, equote, port);
    return buf;
}

//将str_t str_addr设置到addr(T_SockAddrIn)中，默认ipv4
int SOCK_SetStrToAddrIn(T_SockAddrIn *addr, const str_t *str_addr)
{
    CHECK_STACK();
    ASSERT_RETURN(!str_addr || str_addr->slen < SOCK_MAX_HOSTNAME, (addr->sin_addr.s_addr = SOCK_INADDR_NONE, EO_INVAL));
    addr->sin_family = SOCK_AF_INET;
    memset(addr->sin_zero, 0, sizeof(addr->sin_zero));
    if (str_addr && str_addr->slen) {
		addr->sin_addr = INET_CADDR(str_addr);
		if (addr->sin_addr.s_addr == SOCK_INADDR_NONE) {
			T_CAddrInfo ai;
			unsigned count = 1;
			int status;
			status = SOCK_GetCSockAddrInfo(SOCK_AF_INET, str_addr, &count, &ai);
			if (status == EO_SUCCESS) {
				memcpy(&addr->sin_addr, &ai.ai_addr.ipv4.sin_addr, sizeof(addr->sin_addr));
			} 
			else {
				return status;
			}
		}

    } else {
		addr->sin_addr.s_addr = 0;
    }

    return EO_SUCCESS;
}

//将str_t str_addr设置到addr(T_CSockAddr)中
int SOCK_SetStrToCSockAddr(int af, T_CSockAddr *addr, const str_t *str_addr)
{
    int status;
    if (af == SOCK_AF_INET) {
		return SOCK_SetStrToAddrIn(&addr->ipv4, str_addr);
    }
	
    ASSERT_RETURN(af == SOCK_AF_INET6, EO_AFNOTSUP);
    /* IPv6 specific */
    addr->ipv6.sin6_family = SOCK_AF_INET6;
    if (str_addr && str_addr->slen){
		status = inet_pton(SOCK_AF_INET6, str_addr->ptr, &addr->ipv6.sin6_addr);
		if (status != EO_SUCCESS){
			T_CAddrInfo ai;
			unsigned count = 1;
			status = SOCK_GetCSockAddrInfo(SOCK_AF_INET6, str_addr, &count, &ai);
			if (status == EO_SUCCESS){
				memcpy(&addr->ipv6.sin6_addr, &ai.ai_addr.ipv6.sin6_addr, sizeof(addr->ipv6.sin6_addr));
				addr->ipv6.sin6_scope_id = ai.ai_addr.ipv6.sin6_scope_id;
			}
		}
    } 
	else {
		status = EO_SUCCESS;
    }
    return status;
}

//将地址和端口设置到addr(T_SockAddrIn)中，默认ipv4
int SOCK_InitAddrIn(T_SockAddrIn *addr, const str_t *str_addr, uint16_t port)
{
    ASSERT_RETURN(addr, (addr->sin_addr.s_addr = SOCK_INADDR_NONE, EO_INVAL));
    addr->sin_family = SOCK_AF_INET;
    memset(addr->sin_zero, 0, sizeof(addr->sin_zero));
    SOCK_AddrInSetPort(addr, port);
    return SOCK_SetStrToAddrIn(addr, str_addr);
}

//将地址和端口设置到addr(T_CSockAddr)中
int SOCK_InitCSockAddr(int af, T_CSockAddr *addr, const str_t *cp, uint16_t port)
{
    int status;
    if (af == SOCK_AF_INET) return SOCK_InitAddrIn(&addr->ipv4, cp, port);
	
    ASSERT_RETURN(af == SOCK_AF_INET6, EO_AFNOTSUP);
    memset(addr, 0, sizeof(T_SockAddrIn6));
    addr->addr.sa_family = SOCK_AF_INET6;
    status = SOCK_SetStrToCSockAddr(af, addr, cp);
	
    if (status != EO_SUCCESS) return status;
    addr->ipv6.sin6_port = htons(port);
    return EO_SUCCESS;
}

//比较两个地址是否一致(T_SockAddr),0相等
int SOCK_AddrCompare(const T_SockAddr *addr1, const T_SockAddr *addr2)
{
    const T_CSockAddr *a1 = (const T_CSockAddr*) addr1;
    const T_CSockAddr *a2 = (const T_CSockAddr*) addr2;
    int port1, port2;
    int result;
    if (a1->addr.sa_family < a2->addr.sa_family)
		return -1;
    else if (a1->addr.sa_family > a2->addr.sa_family)
		return 1;
    result = memcmp(SOCK_AddrGetAddrP32(a1), SOCK_AddrGetAddrP32(a2), SOCK_AddrGetAddrLen(a1));
    if (result != 0) return result;
    port1 = SOCK_AddrGetPort(a1);
    port2 = SOCK_AddrGetPort(a2);
    if (port1 < port2)
		return -1;
    else if (port1 > port2)
		return 1;
    return 0;
}

T_InAddr SOCK_ADDR_GetHostAddr32(void)
{
    T_SockAddrIn addr;
    const str_t *hostname = SOCK_GetHostname();
    SOCK_SetStrToAddrIn(&addr, hostname);
    return addr.sin_addr;
}

uint16_t SOCK_ADDR_AddrInGetPort(const T_SockAddrIn *addr)
{
    return ntohs(addr->sin_port);
}

//返回ip address 整形值的地址
void *SOCK_AddrGetAddrP32(const T_SockAddr *addr)
{
    const T_CSockAddr *a = (const T_CSockAddr*)addr;
    ASSERT_RETURN(a->addr.sa_family == SOCK_AF_INET || a->addr.sa_family == SOCK_AF_INET6, NULL);
    if (a->addr.sa_family == SOCK_AF_INET6)
		return (void*) &a->ipv6.sin6_addr;
    else
		return (void*) &a->ipv4.sin_addr;
}

//判断是否是非主机地址(SOCK_INADDR_ANY)
char SOCK_AddrIsNAddrAny(const T_SockAddr *addr)
{
    const T_CSockAddr *a = (const T_CSockAddr*)addr;
    if (a->addr.sa_family != SOCK_AF_INET && a->addr.sa_family != SOCK_AF_INET6)
		return 0;
	else if (a->addr.sa_family == SOCK_AF_INET6){
		uint8_t zero[24];
		memset(zero, 0, sizeof(zero));
		return memcmp(a->ipv6.sin6_addr.s6_addr, zero, sizeof(T_In6Addr)) != 0;
    } 
	else
		return a->ipv4.sin_addr.s_addr != SOCK_INADDR_ANY;
}

uint16_t SOCK_AddrGetPort(const T_SockAddr *addr)
{
    const T_CSockAddr *a = (const T_CSockAddr*) addr;
    ASSERT_RETURN(a->addr.sa_family == SOCK_AF_INET || a->addr.sa_family == SOCK_AF_INET6, (uint16_t)0xFFFF);
    return ntohs((uint16_t)(a->addr.sa_family == SOCK_AF_INET6 ? a->ipv6.sin6_port : a->ipv4.sin_port));
}

//获取地址部分的长度
unsigned SOCK_AddrGetAddrLen(const T_SockAddr *addr)
{
    const T_CSockAddr *a = (const T_CSockAddr*) addr;
    ASSERT_RETURN(a->addr.sa_family == SOCK_AF_INET || a->addr.sa_family == SOCK_AF_INET6, 0);
    return a->addr.sa_family == SOCK_AF_INET6 ? sizeof(T_In6Addr) : sizeof(T_InAddr);
}
//获取套接字地址长度
unsigned SOCK_AddrGetLen(const T_SockAddr *addr)
{
    const T_CSockAddr *a = (const T_CSockAddr*) addr;
    ASSERT_RETURN(a->addr.sa_family == SOCK_AF_INET || a->addr.sa_family == SOCK_AF_INET6, 0);
    return a->addr.sa_family == SOCK_AF_INET6 ? sizeof(T_SockAddrIn6) : sizeof(T_SockAddrIn);
}

void SOCK_CSockAddrCopy(T_CSockAddr *dst, const T_CSockAddr *src)
{
    const char *srcbuf = (char*)SOCK_AddrGetAddrP32(src);
    char *dstbuf = ((char*)dst) + (srcbuf - (char*)src);
    memcpy(dstbuf, srcbuf, SOCK_AddrGetAddrLen(src));
}

void SOCK_AddrCopy(T_SockAddr *dst, const T_SockAddr *src)
{
    memcpy(dst, src, SOCK_AddrGetLen(src));
}

int SOCK_AddrSynthesize(int dst_af, T_SockAddr *dst, const T_SockAddr *src)
{
    char ip_addr_buf[SOCK_INET6_ADDRSTRLEN];
    unsigned int count = 1;
    T_CAddrInfo ai[1];
    str_t ip_addr;
    int status;
    ASSERT_RETURN(src && dst, EO_INVAL);
    if (dst_af == ((const T_CSockAddr *)src)->addr.sa_family){
        SOCK_AddrCopy(dst, src);
        return EO_SUCCESS;
    }
    SOCK_AddrPrint(src, ip_addr_buf, sizeof(ip_addr_buf), 0);
    ip_addr = c_str(ip_addr_buf);
    status = SOCK_GetCSockAddrInfo(dst_af, &ip_addr, &count, ai); 
    if (status == EO_SUCCESS && count > 0){
    	SOCK_AddrCopy(dst, &ai[0].ai_addr);
    	SOCK_CSockAddrSetPort(dst, SOCK_AddrGetPort(src));
    }
    return status;
}

void SOCK_AddrInSetPort(T_SockAddrIn *addr, uint16_t hostport)
{
    addr->sin_port = htons(hostport);
}

int SOCK_CSockAddrSetPort(T_CSockAddr *addr, uint16_t hostport)
{
    int af = addr->addr.sa_family;
    ASSERT_RETURN(af==SOCK_AF_INET || af==SOCK_AF_INET6, EO_INVAL);
    if (af == SOCK_AF_INET6)
		addr->ipv6.sin6_port = htons(hostport);
    else
		addr->ipv4.sin_port = htons(hostport);
    return EO_SUCCESS;
}
//返回32位整形地址
T_InAddr SOCK_AddrInGetAddrI32(const T_SockAddrIn *addr)
{
    T_InAddr in_addr;
    in_addr.s_addr = ntohl(addr->sin_addr.s_addr);
    return in_addr;
}

//将32位整形地址设置到addr(T_SockAddrIn)中
void SOCK_I32SetAddrIn(T_SockAddrIn *addr, uint32_t hostaddr)
{
    addr->sin_addr.s_addr = htonl(hostaddr);
}

int sockaddr_parse2(int af, unsigned options, const str_t *str, str_t *p_hostpart, uint16_t *p_port, int *raf)
{
    const char *end = str->ptr + str->slen;
    const char *last_colon_pos = NULL;
    unsigned colon_cnt = 0;
    const char *p;

    ASSERT_RETURN((af == SOCK_AF_INET || af == SOCK_AF_INET6 || af == SOCK_AF_UNSPEC) && options==0 && str != NULL, EO_INVAL);
    if (str->slen == 0 || str->ptr == NULL){
		if (p_hostpart) p_hostpart->slen = 0;
		if (p_port) *p_port = 0;
		if (raf) *raf = SOCK_AF_INET;
		return EO_SUCCESS;
    }

    for (p=str->ptr; p!=end; ++p){
		if (*p == ':'){
			++colon_cnt;
			last_colon_pos = p;
		}
    }

    if (af == SOCK_AF_UNSPEC){
		if (colon_cnt > 1)
			af = SOCK_AF_INET6;
		else
			af = SOCK_AF_INET;
    } else if (af == SOCK_AF_INET && colon_cnt > 1)
		return EO_INVAL;

    if (raf)
		*raf = af;

    if (af == SOCK_AF_INET){
		/* Parse as IPv4. Supported formats:
		 *  - "10.0.0.1:80"
		 *  - "10.0.0.1"
		 *  - "10.0.0.1:"
		 *  - ":80"
		 *  - ":"
		 */
		str_t hostpart;
		unsigned long port;

		hostpart.ptr = (char*)str->ptr;

		if (last_colon_pos) {
			str_t port_part;
			int i;
			hostpart.slen = last_colon_pos - str->ptr;
			port_part.ptr = (char*)last_colon_pos + 1;
			port_part.slen = end - port_part.ptr;

			/* Make sure port number is valid */
			for (i=0; i<port_part.slen; ++i){
				if (!c_isdigit(port_part.ptr[i]))
					return EO_INVAL;
			}
			port = c_strtoul(&port_part);
			if (port > 65535)
				return EO_INVAL;
		} 
		else {
			hostpart.slen = str->slen;
			port = 0;
		}

		if (p_hostpart)
			*p_hostpart = hostpart;
		if (p_port)
			*p_port = (uint16_t)port;

		return EO_SUCCESS;

    } 
	else if (af == SOCK_AF_INET6){
		/* Parse as IPv6. Supported formats:
		 *  - "fe::01:80"  ==> note: port number is zero in this case, not 80!
		 *  - "[fe::01]:80"
		 *  - "fe::01"
		 *  - "fe::01:"
		 *  - "[fe::01]"
		 *  - "[fe::01]:"
		 *  - "[::]:80"
		 *  - ":::80"
		 *  - "[::]"
		 *  - "[::]:"
		 *  - ":::"
		 *  - "::"
		 */
		str_t hostpart, port_part;
		if (*str->ptr == '[') {
			char *end_bracket;
			int i;
			unsigned long port;

			if (last_colon_pos == NULL)
				return EO_INVAL;

			end_bracket = c_strchr(str, ']');
			if (end_bracket == NULL)
				return EO_INVAL;

			hostpart.ptr = (char*)str->ptr + 1;
			hostpart.slen = end_bracket - hostpart.ptr;

			if (last_colon_pos < end_bracket) {
				port_part.ptr = NULL;
				port_part.slen = 0;
			} 
			else {
				port_part.ptr = (char*)last_colon_pos + 1;
				port_part.slen = end - port_part.ptr;
			}

			/* Make sure port number is valid */
			for (i=0; i<port_part.slen; ++i) {
				if (!c_isdigit(port_part.ptr[i]))
					return EO_INVAL;
			}
			port = c_strtoul(&port_part);
			if (port > 65535)
				return EO_INVAL;

			if (p_hostpart)
				*p_hostpart = hostpart;
			if (p_port)
				*p_port = (uint16_t)port;
			return EO_SUCCESS;
		} 
		else {
			/* Treat everything as part of the IPv6 IP address */
			if (p_hostpart)
				*p_hostpart = *str;
			if (p_port)
				*p_port = 0;
			return EO_SUCCESS;
		}
    } 
	else {
		return EO_AFNOTSUP;
    }
}

int SOCK_CSockAddrParse(int af, unsigned options, const str_t *str, T_CSockAddr *addr)
{
    str_t hostpart;
    uint16_t port;
    int status;

    ASSERT_RETURN(addr, EO_INVAL);
    ASSERT_RETURN(af == SOCK_AF_UNSPEC || af == SOCK_AF_INET || af == SOCK_AF_INET6, EO_INVAL);
    ASSERT_RETURN(options == 0, EO_INVAL);

    status = sockaddr_parse2(af, options, str, &hostpart, &port, &af);
    if (status != EO_SUCCESS) return status;

#if !defined(ZXY_HAS_SOCK_IPV6) || !ZXY_HAS_SOCK_IPV6
    if (af == SOCK_AF_INET6) return EO_IPV6NOTSUP;
#endif
    status = SOCK_InitCSockAddr(af, addr, &hostpart, port);
#if defined(ZXY_HAS_SOCK_IPV6) && ZXY_HAS_SOCK_IPV6
    if (status != EO_SUCCESS && af == SOCK_AF_INET6) {
		const char *last_colon_pos=NULL, *p;
		const char *end = str->ptr + str->slen;
		unsigned long long_port;
		str_t port_part;
		int i;

		for (p=str->ptr; p!=end; ++p) {
			if (*p == ':')
				last_colon_pos = p;
		}

		if (last_colon_pos == NULL) return status;

		hostpart.ptr = (char*)str->ptr;
		hostpart.slen = last_colon_pos - str->ptr;

		port_part.ptr = (char*)last_colon_pos + 1;
		port_part.slen = end - port_part.ptr;

		for (i=0; i<port_part.slen; ++i) {
			if (!c_isdigit(port_part.ptr[i]))
				return status;
		}
		long_port = c_strtoul(&port_part);
		if (long_port > 65535)
			return status;

		port = (uint16_t)long_port;
		status = SOCK_InitCSockAddr(SOCK_AF_INET6, addr, &hostpart, port);
    }
#endif
    return status;
}

int SOCK_BindRandom(int sockfd, const T_SockAddr *addr, uint16_t port_range, uint16_t max_try)
{
    T_CSockAddr bind_addr;
    int addr_len;
    uint16_t base_port;
    int status = EO_SUCCESS;
    CHECK_STACK();
    ASSERT_RETURN(addr, EO_INVAL);

    SOCK_AddrCopy(&bind_addr, addr);
    addr_len = SOCK_AddrGetLen(addr);
    base_port = SOCK_AddrGetPort(addr);

    if (base_port == 0 || port_range == 0){
		return SOCK_Bind(sockfd, &bind_addr, addr_len);
    }

    for (; max_try; --max_try){
		uint16_t port;
		port = (uint16_t)(base_port + rand() % (port_range + 1));
		SOCK_CSockAddrSetPort(&bind_addr, port);
		status = SOCK_Bind(sockfd, &bind_addr, addr_len);
		if (status == EO_SUCCESS)
			break;
    }

    return status;
}

int SOCK_SetSockoptSobuf(int sockfd, uint16_t optname, char auto_retry, unsigned *buf_size)
{
    int status;
    int try_size, cur_size, i, step, size_len;
    enum {MAX_TRY = 20};
    CHECK_STACK();
    ASSERT_RETURN(sockfd != SOCK_INVALID_FD && buf_size && *buf_size > 0 && (optname == SOCK_SO_RCVBUF || optname == SOCK_SO_SNDBUF), EO_INVAL);

    size_len = sizeof(cur_size);
    status = SOCK_GetSockopt(sockfd, SOCK_SOL_SOCKET, optname, &cur_size, &size_len);
    if (status != EO_SUCCESS) return status;

    try_size = *buf_size;
    step = (try_size - cur_size) / MAX_TRY;
    if (step < 4096)
		step = 4096;

    for (i = 0; i < (MAX_TRY-1); ++i){
		if (try_size <= cur_size) {
			/* Done, return current size */
			*buf_size = cur_size;
			break;
		}

		status = SOCK_SetSockopt(sockfd, SOCK_SOL_SOCKET, optname, &try_size, sizeof(try_size));
		if (status == EO_SUCCESS){
			status = SOCK_GetSockopt(sockfd, SOCK_SOL_SOCKET, optname, &cur_size, &size_len);
			if (status != EO_SUCCESS){
				/* Ops! No info about current size, just return last try size
				 * and quit.
				 */
				*buf_size = try_size;
				break;
			}
		}
		if (!auto_retry) break;
		try_size -= step;
    }

    return status;
}

static int if_enum_by_af(int af, unsigned *p_cnt, T_CSockAddr ifs[])
{
    int status;
    ASSERT_RETURN(p_cnt && *p_cnt > 0 && ifs, EO_INVAL);
    memset(ifs, 0, sizeof(ifs[0]) * (*p_cnt));
    status = SOCK_GetDefaultIpInterface(af, &ifs[0]);
    if (status != EO_SUCCESS) return status;
    *p_cnt = 1;
    return EO_SUCCESS;
}

int SOCK_IP_EnumInterface(int af, unsigned *p_cnt, T_CSockAddr ifs[])
{
    unsigned start;
    int status;
    start = 0;
    if (af == SOCK_AF_INET6 || af == SOCK_AF_UNSPEC){
		unsigned max = *p_cnt;
		status = if_enum_by_af(SOCK_AF_INET6, &max, &ifs[start]);
		if (status == EO_SUCCESS){
			start += max;
			(*p_cnt) -= max;
		}
    }

    if (af == SOCK_AF_INET || af == SOCK_AF_UNSPEC){
		unsigned max = *p_cnt;
		status = if_enum_by_af(SOCK_AF_INET, &max, &ifs[start]);
		if (status == EO_SUCCESS){
			start += max;
			(*p_cnt) -= max;
		}
    }
    *p_cnt = start;
    return (*p_cnt != 0) ? EO_SUCCESS : EO_NOTFOUND;
}

int SOCK_IP_EnumRoute(unsigned *p_cnt, T_IPRouteEntry routes[])
{
    T_CSockAddr itf;
    int status;
    ASSERT_RETURN(p_cnt && *p_cnt > 0 && routes, EO_INVAL);
    memset(routes, 0, sizeof(routes[0]) * (*p_cnt));
    status = SOCK_GetDefaultIpInterface(SOCK_AF_INET, &itf);
    if (status != EO_SUCCESS) return status;
    routes[0].ipv4.if_addr.s_addr = itf.ipv4.sin_addr.s_addr;
    routes[0].ipv4.dst_addr.s_addr = 0;
    routes[0].ipv4.mask.s_addr = 0;
    *p_cnt = 1;
    return EO_SUCCESS;
}


int SOCK_GetHostIp(int af, T_CSockAddr *addr)
{
    unsigned i, count, cand_cnt;
    enum {
		CAND_CNT			= 8,
		/* Weighting to be applied to found addresses */
		WEIGHT_HOSTNAME		= 1,	/* hostname IP is not always valid! */
		WEIGHT_DEF_ROUTE	= 2,
		WEIGHT_INTERFACE	= 1,
		WEIGHT_LOOPBACK		= -5,
		WEIGHT_LINK_LOCAL	= -4,
		WEIGHT_DISABLED		= -50,
		MIN_WEIGHT			= WEIGHT_DISABLED + 1	/* minimum weight to use */
    };

    T_CSockAddr cand_addr[CAND_CNT];
    int cand_weight[CAND_CNT];
    int selected_cand;
    char strip[SOCK_INET6_ADDRSTRLEN + 10];
    /* Special IPv4 addresses. */
    struct spec_ipv4_t
    {
		uint32_t addr;
		uint32_t mask;
		int	    weight;
    }spec_ipv4[] =
    {
		/* 127.0.0.0/8, loopback addr will be used if there is no other
		 * addresses.
		 */
		{ 0x7f000000, 0xFF000000, WEIGHT_LOOPBACK },

		/* 0.0.0.0/8, special IP that doesn't seem to be practically useful */
		{ 0x00000000, 0xFF000000, WEIGHT_DISABLED },

		/* 169.254.0.0/16, a zeroconf/link-local address, which has higher
		 * priority than loopback and will be used if there is no other
		 * valid addresses.
		 */
		{ 0xa9fe0000, 0xFFFF0000, WEIGHT_LINK_LOCAL }
    };
    /* Special IPv6 addresses */
    struct spec_ipv6_t
    {
		uint8_t addr[16];
		uint8_t mask[16];
		int	   weight;
    }spec_ipv6[] =
    {
		/* Loopback address, ::1/128 */
		{ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		  {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
		   0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
		  WEIGHT_LOOPBACK
		},

		/* Link local, fe80::/10 */
		{ {0xfe,0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		  {0xff,0xc0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		  WEIGHT_LINK_LOCAL
		},

		/* Disabled, ::/128 */
		{ {0x0,0x0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{ 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
		  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
		  WEIGHT_DISABLED
		}
    };
    T_CAddrInfo ai;
    int status;

    cand_cnt = 0;
    memset(cand_addr, 0, sizeof(cand_addr));
    memset(cand_weight, 0, sizeof(cand_weight));
    for (i = 0; i < ARRAY_SIZE(cand_addr); ++i){
		cand_addr[i].addr.sa_family = (uint16_t)af;
    }

    addr->addr.sa_family = (uint16_t)af;

    /* Get default interface (interface for default route) */
    if (cand_cnt < ARRAY_SIZE(cand_addr)){
		status = SOCK_GetDefaultIpInterface(af, addr);
		if (status == EO_SUCCESS) {
			printf("default IP is %s\r\n", SOCK_AddrPrint(addr, strip, sizeof(strip), 0));
			SOCK_CSockAddrSetPort(addr, 0);
			for (i = 0; i < cand_cnt; ++i){
				if (SOCK_AddrCompare(&cand_addr[i], addr)==0)
					break;
			}
			cand_weight[i] += WEIGHT_DEF_ROUTE;
			if (i >= cand_cnt) {
				SOCK_CSockAddrCopy(&cand_addr[i], addr);
				++cand_cnt;
			}
		}
    }

    if (cand_cnt < ARRAY_SIZE(cand_addr)){
		unsigned start_if = cand_cnt;
		count = ARRAY_SIZE(cand_addr) - start_if;

		status = SOCK_IP_EnumInterface(af, &count, &cand_addr[start_if]);
		if (status == EO_SUCCESS && count){
			/* Clear the port number */
			for (i = 0; i < count; ++i)
				SOCK_CSockAddrSetPort(&cand_addr[start_if+i], 0);

			/* For each candidate that we found so far (that is the hostname
			 * address and default interface address, check if they're found
			 * in the interface list. If found, add the weight, and if not,
			 * decrease the weight.
			 */
			for (i = 0; i < cand_cnt; ++i){
				unsigned j;
				for (j=0; j<count; ++j){
					if (SOCK_AddrCompare(&cand_addr[i], &cand_addr[start_if+j])==0)
					break;
				}

				if (j == count) {
					/* Not found */
					cand_weight[i] -= WEIGHT_INTERFACE;
				} else {
					cand_weight[i] += WEIGHT_INTERFACE;
				}
			}

			/* Add remaining interface to candidate list. */
			for (i=0; i<count; ++i){
				unsigned j;
				for (j=0; j<cand_cnt; ++j){
					if (SOCK_AddrCompare(&cand_addr[start_if+i], &cand_addr[j])==0)
						break;
				}

				if (j == cand_cnt){
					SOCK_CSockAddrCopy(&cand_addr[cand_cnt], &cand_addr[start_if+i]);
					cand_weight[cand_cnt] += WEIGHT_INTERFACE;
					++cand_cnt;
				}
			}
		}
    }

    /* Apply weight adjustment for special IPv4/IPv6 addresses
     * See http://trac.pjsip.org/repos/ticket/1046
     */
    if (af == SOCK_AF_INET){
	for (i = 0; i < cand_cnt; ++i){
	    unsigned j;
	    for (j=0; j<ARRAY_SIZE(spec_ipv4); ++j){
		    uint32_t a = ntohl(cand_addr[i].ipv4.sin_addr.s_addr);
		    uint32_t pa = spec_ipv4[j].addr;
		    uint32_t pm = spec_ipv4[j].mask;
		    if ((a & pm) == pa){
				cand_weight[i] += spec_ipv4[j].weight;
				break;
		    }
	    }
	}
    } 
	else if (af == SOCK_AF_INET6){
		for (i = 0; i < ARRAY_SIZE(spec_ipv6); ++i){
			unsigned j;
			for (j = 0; j < cand_cnt; ++j){
				uint8_t *a = cand_addr[j].ipv6.sin6_addr.s6_addr;
				uint8_t am[16];
				uint8_t *pa = spec_ipv6[i].addr;
				uint8_t *pm = spec_ipv6[i].mask;
				unsigned k;

				for (k = 0; k < 16; ++k){
					am[k] = (uint8_t)((a[k] & pm[k]) & 0xFF);
				}

				if (memcmp(am, pa, 16)==0){
					cand_weight[j] += spec_ipv6[i].weight;
				}
			}
		}
    }
	else{
		return EO_AFNOTSUP;
    }

    /* Enumerate candidates to get the best IP address to choose */
    selected_cand = -1;
    for (i = 0; i < cand_cnt; ++i) {
		printf("Checking candidate IP %s, weight=%d\r\n", SOCK_AddrPrint(&cand_addr[i], strip, sizeof(strip), 0), cand_weight[i]);
		if (cand_weight[i] < MIN_WEIGHT) continue;
		if (selected_cand == -1)
			selected_cand = i;
		else if (cand_weight[i] > cand_weight[selected_cand])
			selected_cand = i;
    }

    /* If else fails, returns loopback interface as the last resort */
    if (selected_cand == -1) {
		if (af==SOCK_AF_INET) {
			addr->ipv4.sin_addr.s_addr = htonl (0x7f000001);
		} else {
			T_In6Addr *s6_addr;
			s6_addr = (T_In6Addr*) SOCK_AddrGetAddrP32(addr);
			memset(s6_addr, 0, sizeof(T_In6Addr));
			s6_addr->s6_addr[15] = 1;
		}
		printf("Loopback IP %s returned\r\n", SOCK_AddrPrint(addr, strip, sizeof(strip), 0));
    } 
	else{
		SOCK_CSockAddrCopy(addr, &cand_addr[selected_cand]);
		printf("Candidate %s selected\r\n", SOCK_AddrPrint(addr, strip, sizeof(strip), 0));
    }

    return EO_SUCCESS;
}

int SOCK_GetIpInterface(int af, const str_t *dst, T_CSockAddr *itf_addr, char allow_resolve, T_CSockAddr *p_dst_addr)
{
    T_CSockAddr dst_addr;
    int fd;
    int len;
    uint8_t zero[64];
    int status;

    SOCK_InitCSockAddr(af, &dst_addr, NULL, 53);
    status = INET_CPTON(af, dst, SOCK_AddrGetAddrP32(&dst_addr));
    if (status != EO_SUCCESS){
		/* "dst" is not an IP address. */
		if (allow_resolve){
			status = SOCK_InitCSockAddr(af, &dst_addr, dst, 53);
		}
		else{
			str_t cp;
			if (af == SOCK_AF_INET){
				cp = c_str("1.1.1.1");
			}
			else{
				cp = c_str("1::1");
			}
			status = SOCK_InitCSockAddr(af, &dst_addr, &cp, 53);
		}
		if (status != EO_SUCCESS) return status;
    }

    status = SOCK_Socket(af, SOCK_TYPE_DGRAM, 0, &fd);
    if (status != EO_SUCCESS) return status;
    status = SOCK_Connect(fd, &dst_addr, SOCK_AddrGetLen(&dst_addr));
    if (status != EO_SUCCESS) {SOCK_Close(fd); return status;}

    len = sizeof(*itf_addr);
    status = SOCK_GetSockName(fd, itf_addr, &len);
    if (status != EO_SUCCESS) {SOCK_Close(fd); return status;}

    SOCK_Close(fd);

    memset(zero, 0, sizeof(zero));
    if (memcmp(SOCK_AddrGetAddrP32(itf_addr), zero, SOCK_AddrGetAddrLen(itf_addr)) == 0) return EO_NOTFOUND;

    if (p_dst_addr) *p_dst_addr = dst_addr;

    return EO_SUCCESS;
}

int SOCK_GetDefaultIpInterface(int af, T_CSockAddr *addr)
{
    str_t cp;
    if (af == SOCK_AF_INET){
		cp = c_str("1.1.1.1");
    }
	else{
		cp = c_str("1::1");
    }

    return SOCK_GetIpInterface(af, &cp, addr, 0, NULL);
}

int SOCK_GetHostByName(const str_t *hostname, T_Hostent *phe)
{
    struct hostent *he;
    char copy[SOCK_MAX_HOSTNAME];
    assert(hostname && hostname ->slen < SOCK_MAX_HOSTNAME);
    if (hostname->slen >= SOCK_MAX_HOSTNAME) return EO_NAMETOOLONG;

    memcpy(copy, hostname->ptr, hostname->slen);
    copy[hostname->slen] = '\0';

    he = gethostbyname(copy);
    if (!he) return EO_RESOLVE;

    phe->h_name = he->h_name;
    phe->h_aliases = he->h_aliases;
    phe->h_addrtype = he->h_addrtype;
    phe->h_length = he->h_length;
    phe->h_addr_list = he->h_addr_list;

    return EO_SUCCESS;
}

int SOCK_GetCSockAddrInfo(int af, const str_t *nodename, unsigned *count, T_CAddrInfo ai[])
{
#if defined(ZXY_HAS_SOCK_GETADDRINFO) && ZXY_HAS_SOCK_GETADDRINFO!=0
    char nodecopy[SOCK_MAX_HOSTNAME];
    char has_addr = 0;
    unsigned i;
    int rc;
    struct addrinfo hint, *res, *orig_res;

    ASSERT_RETURN(nodename && count && *count && ai, EO_INVAL);
    ASSERT_RETURN(nodename->ptr && nodename->slen, EO_INVAL);
    ASSERT_RETURN(af == SOCK_AF_INET || af == SOCK_AF_INET6 || af == SOCK_AF_UNSPEC, EO_INVAL);

    if (nodename->slen >= SOCK_MAX_HOSTNAME) return EO_NAMETOOLONG;
    memcpy(nodecopy, nodename->ptr, nodename->slen);
    nodecopy[nodename->slen] = '\0';

	memset(&hint, 0, sizeof(hint));
    hint.ai_family = af;
    rc = getaddrinfo(nodecopy, NULL, &hint, &res);
    if (rc != 0) return EO_RESOLVE;
    orig_res = res;
    for (i = 0; i < *count && res; res = res->ai_next) {
		if (af! = SOCK_AF_UNSPEC && res->ai_family != af) continue;
		if (res->ai_canonname){
			strncpy(ai[i].ai_canonname, res->ai_canonname, sizeof(ai[i].ai_canonname));
			ai[i].ai_canonname[sizeof(ai[i].ai_canonname)-1] = '\0';
		}
		else {
			strcpy(ai[i].ai_canonname, nodecopy);
		}
		ASSERT_ON_FAIL(res->ai_addrlen <= sizeof(T_CSockAddr), continue);
		memcpy(&ai[i].ai_addr, res->ai_addr, res->ai_addrlen);
		++i;
    }
    *count = i;
    freeaddrinfo(orig_res);
    return (*count > 0? EO_SUCCESS : EO_RESOLVE);
#else
    if (af == SOCK_AF_INET || af == SOCK_AF_UNSPEC){
		T_Hostent he;
		unsigned i, max_count;
		int status;
		status = SOCK_GetHostByName(nodename, &he);
		if (status != EO_SUCCESS) return status;
		max_count = *count;
		*count = 0;
		memset(ai, 0, max_count * sizeof(T_CAddrInfo));
		for (i = 0; he.h_addr_list[i] && *count < max_count; ++i){
			strncpy(ai[*count].ai_canonname, he.h_name, sizeof(ai[*count].ai_canonname));
			ai[*count].ai_canonname[sizeof(ai[*count].ai_canonname) - 1] = '\0';
			ai[*count].ai_addr.ipv4.sin_family = SOCK_AF_INET;
			memcpy(&ai[*count].ai_addr.ipv4.sin_addr, he.h_addr_list[i], he.h_length);
			(*count)++;
		}
		return (*count > 0? EO_SUCCESS : EO_RESOLVE);
    }
	else{
		*count = 0;
		return EO_IPV6NOTSUP;
    }
#endif
}

