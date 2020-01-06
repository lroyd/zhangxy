#include <stdio.h>
#include <string.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <sys/socket.h>

#include "sock.h"

#define USE_MSG_NOSIGNAL	(1)		//默认使用

#define CHECK_STACK()		//不使用

const uint16_t SOCK_AF_UNSPEC	= AF_UNSPEC;
const uint16_t SOCK_AF_UNIX	= AF_UNIX;
const uint16_t SOCK_AF_INET	= AF_INET;
const uint16_t SOCK_AF_INET6	= AF_INET6;
const uint16_t SOCK_AF_PACKET	= AF_PACKET;
const uint16_t SOCK_AF_IRDA	= AF_IRDA;

const uint16_t SOCK_TYPE_STREAM		= SOCK_STREAM;
const uint16_t SOCK_TYPE_DGRAM		= SOCK_DGRAM;
const uint16_t SOCK_TYPE_RAW		= SOCK_RAW;
const uint16_t SOCK_TYPE_RDM		= SOCK_RDM;


const uint16_t SOCK_SOL_SOCKET	= SOL_SOCKET;

const uint16_t SOCK_SOL_IP	= 0;	
const uint16_t SOCK_SOL_TCP	= 6;
const uint16_t SOCK_SOL_UDP	= 17;
const uint16_t SOCK_SOL_IPV6	= 41;
const uint16_t SOCK_IP_TOS	= 1;

const uint16_t SOCK_IPTOS_LOWDELAY	= 0x10;
const uint16_t SOCK_IPTOS_THROUGHPUT	= 0x08;
const uint16_t SOCK_IPTOS_RELIABILITY	= 0x04;
const uint16_t SOCK_IPTOS_MINCOST	= 0x02;

const uint16_t SOCK_IPV6_TCLASS = 0xFFFF;


const uint16_t SOCK_SO_TYPE    = SO_TYPE;
const uint16_t SOCK_SO_RCVBUF  = SO_RCVBUF;
const uint16_t SOCK_SO_SNDBUF  = SO_SNDBUF;
const uint16_t SOCK_TCP_NODELAY= TCP_NODELAY;
const uint16_t SOCK_SO_REUSEADDR= SO_REUSEADDR;
const uint16_t SOCK_SO_NOSIGPIPE = 0xFFFF;
const uint16_t SOCK_SO_PRIORITY = 12;


const uint16_t SOCK_IP_MULTICAST_IF    = 0xFFFF;
const uint16_t SOCK_IP_MULTICAST_TTL   = 0xFFFF;
const uint16_t SOCK_IP_MULTICAST_LOOP  = 0xFFFF;
const uint16_t SOCK_IP_ADD_MEMBERSHIP  = 0xFFFF;
const uint16_t SOCK_IP_DROP_MEMBERSHIP = 0xFFFF;


/* recv() and send() flags */
const int SOCK_MSG_OOB		= MSG_OOB;
const int SOCK_MSG_PEEK		= MSG_PEEK;
const int SOCK_MSG_DONTROUTE	= MSG_DONTROUTE;


const str_t* SOCK_GetHostname(void)
{
    static char pBuf[SOCK_MAX_HOSTNAME];
    static str_t hostname;
    CHECK_STACK();
    if (hostname.ptr == NULL){
		hostname.ptr = pBuf;
		if (gethostname(pBuf, sizeof(pBuf)) != 0){
			hostname.ptr[0] = '\0';
			hostname.slen = 0;
		}else{
			hostname.slen = strlen(pBuf);
		}
    }
    return &hostname;
}

int SOCK_Socket(int af, int type, int proto, int *_pOut)
{
    CHECK_STACK();
    ASSERT_RETURN(_pOut != NULL, EO_INVAL);
    
    *_pOut = socket(af, type, proto);
    if (*_pOut == SOCK_INVALID_FD)
		return EO_FAILED;
    else{
		int32_t val = 1;
		if (type == SOCK_TYPE_STREAM){
			SOCK_SetSockopt(*_pOut, SOCK_SOL_SOCKET, SOCK_SO_NOSIGPIPE, &val, sizeof(val));
		}

#if defined(ZXY_HAS_SOCK_IPV6_V6ONLY) && ZXY_HAS_SOCK_IPV6_V6ONLY != 0
		if (af == SOCK_AF_INET6){
			SOCK_SetSockopt(*_pOut, SOCK_SOL_IPV6, IPV6_V6ONLY, &val, sizeof(val));
		}
#endif
		return EO_SUCCESS;
    }
}

int SOCK_Bind(int _u32Sock, const T_SockAddr *_pAddr, int _u32Len)
{
    CHECK_STACK();
    ASSERT_RETURN(_pAddr && _u32Len >= (int)sizeof(struct sockaddr_in), EO_INVAL);
    if (bind(_u32Sock, (struct sockaddr*)_pAddr, _u32Len) != 0)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

//使用主机字节序

int SOCK_BindAddrIn(int _u32Sock, uint32_t _u32HostIp, uint16_t _u16HostPort)
{
    T_SockAddrIn pAddr;
    CHECK_STACK();
    pAddr.sin_family = SOCK_AF_INET;
    memset(pAddr.sin_zero, 0, sizeof(pAddr.sin_zero));
    pAddr.sin_addr.s_addr	= htonl(_u32HostIp);
    pAddr.sin_port			= htons(_u16HostPort);
    return SOCK_Bind(_u32Sock, &pAddr, sizeof(T_SockAddrIn));
}

int SOCK_Close(int _u32Sock)
{
    CHECK_STACK();
    if (close(_u32Sock))
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_GetPeerName(int _u32Sock, T_SockAddr *_pAddr, int *_u32NameLen)
{
    CHECK_STACK();
    if (getpeername(_u32Sock, (struct sockaddr*)_pAddr, (socklen_t*)_u32NameLen) != 0)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_GetSockName(int _u32Sock, T_SockAddr *_pAddr, int *_u32NameLen)
{
    CHECK_STACK();
    if (getsockname(_u32Sock, (struct sockaddr*)_pAddr, (socklen_t*)_u32NameLen) != 0)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_Send(int _u32Sock, const void *_pBuf, ssize_t *_pLen, unsigned flags)
{
    CHECK_STACK();
    ASSERT_RETURN(_pLen, EO_INVAL);
#ifdef USE_MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;  //linux上无效SO_NOSIGPIPE
#endif
    *_pLen = send(_u32Sock, (const char*)_pBuf, (int)(*_pLen), flags);
    if (*_pLen < 0)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_Sendto(int _u32Sock, const void *_pBuf, ssize_t *_pLen, unsigned flags, const T_SockAddr *_pTo, int _u32ToLen)
{
    CHECK_STACK();
    ASSERT_RETURN(_pLen, EO_INVAL);
    *_pLen = sendto(_u32Sock, (const char*)_pBuf, (int)(*_pLen), flags, (const struct sockaddr*)_pTo, _u32ToLen);
    if (*_pLen < 0) 
		return EO_FAILED;
    else 
		return EO_SUCCESS;
}

int SOCK_Recv(int _u32Sock, void *_pBuf, ssize_t *_pLen, unsigned flags)
{
    CHECK_STACK();
    ASSERT_RETURN(_pBuf && _pLen, EO_INVAL);
    *_pLen = recv(_u32Sock, (char*)_pBuf, (int)(*_pLen), flags);
    if (*_pLen < 0) 
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_Recvfrom(int _u32Sock, void *_pBuf, ssize_t *_pLen, unsigned flags, T_SockAddr *_pFrom, int *_pFromLen)
{
    CHECK_STACK();
    ASSERT_RETURN(_pBuf && _pLen, EO_INVAL);
    *_pLen = recvfrom(_u32Sock, (char*)_pBuf, (int)(*_pLen), flags, (struct sockaddr*)_pFrom, (socklen_t*)_pFromLen);
    if (*_pLen < 0) 
		return EO_FAILED;
    else 
		return EO_SUCCESS;
}

int SOCK_GetSockopt(int _u32Sock, uint16_t _u16Level, uint16_t _u16OptName, void *_pOptValue, int *_pOptLen)
{
    CHECK_STACK();
    ASSERT_RETURN(_pOptValue && _pOptLen, EO_INVAL);
    if (getsockopt(_u32Sock, _u16Level, _u16OptName, (char*)_pOptValue, (socklen_t*)_pOptLen)!=0)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_SetSockopt(int _u32Sock, uint16_t _u16Level, uint16_t _u16OptName, const void *_pOptValue, int _u32OptLen)
{
    CHECK_STACK();
    if (setsockopt(_u32Sock, _u16Level, _u16OptName, (const char*)_pOptValue, _u32OptLen))
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_SetSockoptParams(int _u32Sock, const T_SockoptParams *_ptParams)
{
    unsigned int i = 0;
    int retval = EO_SUCCESS;
    CHECK_STACK();
    ASSERT_RETURN(_ptParams, EO_INVAL);
    for (; i < _ptParams->cnt && i < SOCK_MAX_SOCKOPT_PARAMS; ++i){
		int status = SOCK_SetSockopt(_u32Sock, (uint16_t)_ptParams->options[i].level, (uint16_t)_ptParams->options[i].optname, _ptParams->options[i].optval, _ptParams->options[i].optlen);
		if (status != EO_SUCCESS) {
			retval = status;
			printf("warning: error applying sock opt %d\r\n", _ptParams->options[i].optname);
		}
    }

    return retval;
}

int SOCK_Connect(int _u32Sock, const T_SockAddr *_pAddr, int _u32NameLen)
{
    CHECK_STACK();
    if (connect(_u32Sock, (struct sockaddr*)_pAddr, _u32NameLen) != 0)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_Shutdown(int _u32Sock, int how)
{
    CHECK_STACK();
    if (shutdown(_u32Sock, how) != 0)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_Listen(int _u32Sock, int _u32block)
{
    CHECK_STACK();
    if (listen(_u32Sock, _u32block) != 0)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}

int SOCK_Accept(int _u32SSock, int *_u32Sock, T_SockAddr *_pAddr, int *_pAddrLen)
{
    CHECK_STACK();
    ASSERT_RETURN(_u32Sock != NULL, EO_INVAL);
    *_u32Sock = accept(_u32SSock, (struct sockaddr*)_pAddr, (socklen_t*)_pAddrLen);
    if (*_u32Sock == SOCK_INVALID_FD)
		return EO_FAILED;
    else
		return EO_SUCCESS;
}



