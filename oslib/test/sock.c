#include "sock.h"


#define UDP_PORT		(51234)
#define TCP_PORT        (UDP_PORT+10)
#define BIG_DATA_LEN	(8192)
#define LOCAL_ADDRESS	("127.0.0.1")

static char bigdata[BIG_DATA_LEN];
static char bigbuffer[BIG_DATA_LEN];



static int format_test(void)
{
    str_t s = c_str(LOCAL_ADDRESS);
    unsigned char *p;
    T_InAddr addr;		//u32整形
    char zero[64];
    T_SockAddrIn addr2;
    const str_t *hostname;
    const unsigned char A[] = {127, 0, 0, 1};

    printf("format_test()\r\n");
#if 0    
    if (INET_CATON(&s, &addr) != 1) return -10;
    p = (unsigned char*)&addr;
    if (p[0] != A[0] || p[1] != A[1] || p[2] != A[2] || p[3] != A[3]) {
		printf("error: mismatched address. p0=%d, p1=%d, p2=%d, p3=%d\r\n", p[0] & 0xFF, p[1] & 0xFF, p[2] & 0xFF, p[3] & 0xFF);
		return -15;
    }

    p = (unsigned char*)INET_CNTOA(addr);
    if (!p) return -20;

    if (c_strcmp2(&s, (char*)p) != 0) return -22;

#if 0
    /* pj_inet_pton() */
    /* pj_inet_ntop() */
    
	const str_t s_ipv4 = c_str("127.0.0.1");
	const str_t s_ipv6 = c_str("fe80::2ff:83ff:fe7c:8b42");
	char buf_ipv4[SOCK_INET_ADDRSTRLEN];
	char buf_ipv6[SOCK_INET6_ADDRSTRLEN];
	T_InAddr ipv4;
	T_In6Addr ipv6;

	if (pj_inet_pton(SOCK_AF_INET, &s_ipv4, &ipv4) != 0)
	    return -24;

	p = (unsigned char*)&ipv4;
	if (p[0]!=A[0] || p[1]!=A[1] || p[2]!=A[2] || p[3]!=A[3]) {
	    return -25;
	}

	if (pj_inet_pton(SOCK_AF_INET6, &s_ipv6, &ipv6) != 0)
	    return -26;

	p = (unsigned char*)&ipv6;
	if (p[0] != 0xfe || p[1] != 0x80 || p[2] != 0 || p[3] != 0 ||
	    p[4] != 0 || p[5] != 0 || p[6] != 0 || p[7] != 0 ||
	    p[8] != 0x02 || p[9] != 0xff || p[10] != 0x83 || p[11] != 0xff ||
	    p[12]!=0xfe || p[13]!=0x7c || p[14] != 0x8b || p[15]!=0x42)
	{
	    return -27;
	}

	if (pj_inet_ntop(SOCK_AF_INET, &ipv4, buf_ipv4, sizeof(buf_ipv4)) != 0)
	    return -28;
	if (pj_stricmp2(&s_ipv4, buf_ipv4) != 0)
	    return -29;

	if (pj_inet_ntop(SOCK_AF_INET6, &ipv6, buf_ipv6, sizeof(buf_ipv6)) != 0)
	    return -30;
	if (pj_stricmp2(&s_ipv6, buf_ipv6) != 0)
	    return -31;

#endif

    SOCK_InitAddrIn(&addr2, 0, 1000);
    memset(zero, 0, sizeof(zero));
    if (memcmp(addr2.sin_zero, zero, sizeof(addr2.sin_zero)) != 0) return -35;
#endif
    hostname = SOCK_GetHostname();
    if (!hostname || !hostname->ptr || !hostname->slen) return -40;

    printf("get hostname is %.*s\r\n", (int)hostname->slen, hostname->ptr);

    /* SOCK_ADDR_GetHostAddr32() */

    return 0;
}

#define IPv4	1
#define IPv6	2

static int parse_test(void)
{
    struct test_t {
		const char  *input;
		int	     result_af;
		const char  *result_ip;
		uint16_t  result_port;
    };
    struct test_t valid_tests[] = 
    {
		/* IPv4 */
		{ "10.0.0.1:80", IPv4, "10.0.0.1", 80},
		{ "10.0.0.1", IPv4, "10.0.0.1", 0},
		{ "10.0.0.1:", IPv4, "10.0.0.1", 0},
		{ "10.0.0.1:0", IPv4, "10.0.0.1", 0},
		{ ":80", IPv4, "0.0.0.0", 80},
		{ ":", IPv4, "0.0.0.0", 0},
		{ "localhost", IPv4, "127.0.0.1", 0},
		{ "localhost:", IPv4, "127.0.0.1", 0},
		{ "localhost:80", IPv4, "127.0.0.1", 80},


#if 0	//ipv6
		{ "fe::01:80", IPv6, "fe::01:80", 0},
		{ "[fe::01]:80", IPv6, "fe::01", 80},
		{ "fe::01", IPv6, "fe::01", 0},
		{ "[fe::01]", IPv6, "fe::01", 0},
		{ "fe::01:", IPv6, "fe::01", 0},
		{ "[fe::01]:", IPv6, "fe::01", 0},
		{ "::", IPv6, "::0", 0},
		{ "[::]", IPv6, "::", 0},
		{ ":::", IPv6, "::", 0},
		{ "[::]:", IPv6, "::", 0},
		{ ":::80", IPv6, "::", 80},
		{ "[::]:80", IPv6, "::", 80},
#endif
    };
	
    struct test_t invalid_tests[] = 
		{
		/* IPv4 */
		{ "10.0.0.1:abcd", IPv4},   /* port not numeric */
		{ "10.0.0.1:-1", IPv4},	    /* port contains illegal character */
		{ "10.0.0.1:123456", IPv4}, /* port too big	*/
		//this actually is fine on my Mac OS 10.9
		//it will be resolved with gethostbyname() and something is returned!
		//{ "1.2.3.4.5:80", IPv4},    /* invalid IP */
		{ "10:0:80", IPv4},	    /* hostname has colon */

#if 0
		{ "[fe::01]:abcd", IPv6},   /* port not numeric */
		{ "[fe::01]:-1", IPv6},	    /* port contains illegal character */
		{ "[fe::01]:123456", IPv6}, /* port too big	*/
		{ "fe::01:02::03:04:80", IPv6},	    /* invalid IP */
		{ "[fe::01:02::03:04]:80", IPv6},   /* invalid IP */
		{ "[fe:01", IPv6},	    /* Unterminated bracket */
#endif
    };

    unsigned i;

    printf("IP address parsing\r\n");
	char tmp[64] = {0};
    for (i = 0; i < ARRAY_SIZE(valid_tests); ++i){
		int status;
		str_t input;
		T_CSockAddr addr, result;

		switch (valid_tests[i].result_af){
			case IPv4:
				valid_tests[i].result_af = SOCK_AF_INET;
				break;
			case IPv6:
				valid_tests[i].result_af = SOCK_AF_INET6;
				break;
			default:
				assert(!"Invalid AF!");
				continue;
		}

		status = SOCK_CSockAddrParse(SOCK_AF_INET, 0, c_cstr(&input, valid_tests[i].input), &addr);
		if (status != 0){
			printf("failed when parsing %s (i=%d)\r\n", valid_tests[i].input, i);
			return -10;
		}
		memset(tmp, 0, 64);
		printf("  %s\r\n", SOCK_AddrPrint((T_SockAddr *)&addr, tmp, 64, 1));
		
		status = SOCK_InitCSockAddr(valid_tests[i].result_af, &result, c_cstr(&input, valid_tests[i].result_ip), valid_tests[i].result_port);
		if (status != 0){
			printf(".... error building IP address %s\r\n", valid_tests[i].input);
			return -30;
		}

		if (SOCK_AddrCompare(&addr, &result) != 0){
			printf(".... parsed result mismatched for %s\r\n", valid_tests[i].input);
			return -40;
		}

		status = SOCK_CSockAddrParse(valid_tests[i].result_af, 0, c_cstr(&input, valid_tests[i].input), &addr);
		if (status != 0){
			printf(".... failed when parsing %s\r\n", valid_tests[i].input);
			return -50;
		}

		if (SOCK_AddrCompare(&addr, &result) != 0){
			printf(".... parsed result mismatched for %s\r\n", valid_tests[i].input);
			return -60;
		}
    }

    for (i = 0; i < ARRAY_SIZE(invalid_tests); ++i){
		int status;
		str_t input;
		T_CSockAddr addr;

		switch (invalid_tests[i].result_af){
			case IPv4:
				invalid_tests[i].result_af = SOCK_AF_INET;
				break;
			case IPv6:
				invalid_tests[i].result_af = SOCK_AF_INET6;
				break;
			default:
				assert(!"Invalid AF!");
				continue;
		}

		status = SOCK_CSockAddrParse(SOCK_AF_UNSPEC, 0, c_cstr(&input, invalid_tests[i].input), &addr);
		if (status == 0) {
			printf(".... expecting failure when parsing %s\r\n", invalid_tests[i].input);
			return -100;
		}
    }

    return 0;
}


static int simple_sock_test(void)
{
    int types[2];
    int sock;
    int i;
    int rc = 0;

    types[0] = SOCK_TYPE_STREAM;
    types[1] = SOCK_TYPE_DGRAM;

    printf("simple_sock_test()\r\n");

    for (i = 0; i < (int)(sizeof(types)/sizeof(types[0])); ++i){
		rc = SOCK_Socket(SOCK_AF_INET, types[i], 0, &sock);
		if (rc != 0){
			printf("error: unable to create socket\r\n");
			break;
		}else{
			rc = SOCK_Close(sock);
			if (rc != 0){
				printf("error: close socket\r\n");
				break;
			}
		}
    }
    return rc;
}


static int send_recv_test(int sock_type, int ss, int cs, T_SockAddrIn *dstaddr, T_SockAddrIn *srcaddr, int addrlen)
{
    enum { DATA_LEN = 16 };
    char senddata[DATA_LEN+4], recvdata[DATA_LEN+4];
    ssize_t sent, received, total_received;
    int rc;
	char tmp[32] = {0};
    c_create_random_string(senddata, DATA_LEN);
    senddata[DATA_LEN-1] = '\0';
	printf("create_random_string:%s\r\n", senddata);
	memset(tmp, 0, 32);

    if (dstaddr){
        sent = DATA_LEN;
		rc = SOCK_Sendto(cs, senddata, &sent, 0, dstaddr, addrlen);
		if (rc != 0 || sent != DATA_LEN){
			printf("...sendto error\r\n");
			rc = -140; goto on_error;
		}
		printf(" sendto():%s, size = %d\r\n", SOCK_AddrPrint(dstaddr, tmp, 32, 1), sent);
    } else {
        sent = DATA_LEN;
		rc = SOCK_Send(cs, senddata, &sent, 0);
		if (rc != 0 || sent != DATA_LEN){
			printf("...send error\r\n");
			rc = -145; goto on_error;
		}
    }
	memset(tmp, 0, 32);

    if (srcaddr){
		T_SockAddrIn addr;
		int srclen = sizeof(addr);
		memset(&addr, 0, sizeof(addr));
		received = DATA_LEN;
		rc = SOCK_Recvfrom(ss, recvdata, &received, 0, &addr, &srclen);
		if (rc != 0 || received != DATA_LEN){
			printf("...recvfrom error\r\n");
			rc = -150; goto on_error;
		}
		if (srclen != addrlen)
			return -151;
		if (SOCK_AddrCompare(&addr, srcaddr) != 0){
			char srcaddr_str[32], addr_str[32];
			strcpy(srcaddr_str, INET_CNTOA(srcaddr->sin_addr));
			strcpy(addr_str, INET_CNTOA(addr.sin_addr));
			printf("...error: src address mismatch (original=%s, recvfrom addr=%s)\r\n", srcaddr_str, addr_str);
			return -152;
		}
		printf(" recv():%s, size = %d\r\n", SOCK_AddrPrint(srcaddr, tmp, 32, 1), received);
    } else {
        total_received = 0;
        do {
            received = DATA_LEN-total_received;
			rc = SOCK_Recv(ss, recvdata+total_received, &received, 0);
			if (rc != 0){
				printf("...recv error\r\n");
				rc = -155; goto on_error;
			}
			if (received <= 0){
				printf("...error: socket has closed! (received=%d)\r\n",received);
				rc = -156; goto on_error;
			}
			
			if (received != DATA_LEN-total_received){
				if (sock_type != SOCK_TYPE_STREAM){
					printf("...error: expecting %u bytes, got %u bytes\r\n",DATA_LEN-total_received, received);
					rc = -157; goto on_error;
				}
			}
			
			total_received += received;
        } while (total_received < DATA_LEN);
    }

    printf("memcmp()\r\n");
    if (memcmp(senddata, recvdata, DATA_LEN) != 0){
		printf("...error: received data mismatch (got:'%s' expecting:'%s'\r\n",recvdata, senddata);
		rc = -160; goto on_error;
    }
	memset(tmp, 0, 32);

    if (dstaddr){
        sent = BIG_DATA_LEN;
		rc = SOCK_Sendto(cs, bigdata, &sent, 0, dstaddr, addrlen);
		if (rc != 0 || sent != BIG_DATA_LEN){
			printf("...sendto error\r\n");
			rc = -161; goto on_error;
		}
		printf(" sendto():%s, size = %d\r\n", SOCK_AddrPrint(dstaddr, tmp, 32, 1), sent);
    } else {
        sent = BIG_DATA_LEN;
		rc = SOCK_Send(cs, bigdata, &sent, 0);
		if (rc != 0 || sent != BIG_DATA_LEN){
			printf("...send error\r\n");
			rc = -165; goto on_error;
		}
    }
	memset(tmp, 0, 32);
    printf("recv()\r\n");
    total_received = 0;
    do {
        received = BIG_DATA_LEN-total_received;
		rc = SOCK_Recv(ss, bigbuffer+total_received, &received, 0);
		if (rc != 0){
			printf("...recv error\r\n");
			rc = -170; goto on_error;
		}
			if (received <= 0){
				printf("...error: socket has closed! (received=%d)\r\n", received);
				rc = -173; goto on_error;
			}
		if (received != BIG_DATA_LEN-total_received){
				if (sock_type != SOCK_TYPE_STREAM){
				printf("...error: expecting %u bytes, got %u bytes\r\n", BIG_DATA_LEN-total_received, received);
				rc = -176; goto on_error;
				}
		}
			total_received += received;
    } while (total_received < BIG_DATA_LEN);

    printf("memcmp()\r\n");
    if (memcmp(bigdata, bigbuffer, BIG_DATA_LEN) != 0){
        printf("...error: received data has been altered!\r\n");
		rc = -180; goto on_error;
    }
    
    rc = 0;

on_error:
    return rc;
}

static int udp_test(void)
{
    int cs = SOCK_INVALID_FD, ss = SOCK_INVALID_FD;
    T_SockAddrIn dstaddr, srcaddr;
    str_t s;
    int rc = 0, retval;

    printf("udp_test()\r\n");
    rc = SOCK_Socket(SOCK_AF_INET, SOCK_TYPE_DGRAM, 0, &ss);
    if (rc != 0) {
		printf("error: unable to create socket\r\n");
		return -100;
    }

    rc = SOCK_Socket(SOCK_AF_INET, SOCK_TYPE_DGRAM, 0, &cs);
    if (rc != 0) return -110;

    memset(&dstaddr, 0, sizeof(dstaddr));
    dstaddr.sin_family = SOCK_AF_INET;
    dstaddr.sin_port = htons(UDP_PORT);
    dstaddr.sin_addr = INET_CADDR(c_cstr(&s, LOCAL_ADDRESS));
    if ((rc = SOCK_Bind(ss, &dstaddr, sizeof(dstaddr))) != 0){
		printf("bind error udp: %s\r\n", LOCAL_ADDRESS);
		rc = -120; goto on_error;
    }

    memset(&srcaddr, 0, sizeof(srcaddr));
    srcaddr.sin_family = SOCK_AF_INET;
    srcaddr.sin_port = htons(UDP_PORT-1);
    srcaddr.sin_addr = INET_CADDR(c_cstr(&s, LOCAL_ADDRESS));
    if ((rc = SOCK_Bind(cs, &srcaddr, sizeof(srcaddr))) != 0){
		printf("bind error\r\n");
		rc = -121; goto on_error;
    }
	
    rc = send_recv_test(SOCK_TYPE_DGRAM, ss, cs, &dstaddr, NULL, sizeof(dstaddr));
    if (rc != 0) goto on_error;

    rc = send_recv_test(SOCK_TYPE_DGRAM, ss, cs, &dstaddr, &srcaddr, sizeof(dstaddr));
    if (rc != 0) goto on_error;
  

    rc = SOCK_Connect(cs, &dstaddr, sizeof(dstaddr));
    if (rc != 0) {
		printf("connect() error\r\n");
		rc = -122; goto on_error;
    }

    rc = send_recv_test(SOCK_TYPE_DGRAM, ss, cs, NULL, NULL, 0);
    if (rc != 0) goto on_error;
	
    rc = send_recv_test(SOCK_TYPE_DGRAM, ss, cs, NULL, &srcaddr, sizeof(srcaddr));
    if (rc != 0) goto on_error;

on_error:
    retval = rc;
    if (cs != SOCK_INVALID_FD){
        rc = SOCK_Close(cs);
        if (rc != 0) {
            printf("error in closing socket\r\n");
            return -1000;
        }
    }
    if (ss != SOCK_INVALID_FD){
        rc = SOCK_Close(ss);
        if (rc != 0){
            printf("error in closing socket\r\n");
            return -1010;
        }
    }

    return retval;
}

static int tcp_test(void)
{
    int cs, ss;
    int rc = 0, retval = 0;

    printf("tcp_test()\r\n");
#if 0
    rc = app_socketpair(SOCK_AF_INET, SOCK_TYPE_STREAM, 0, &ss, &cs);
    if (rc != 0){
        printf("error: app_socketpair():\r\n");
        return -2000;
    }

    retval = send_recv_test(SOCK_TYPE_STREAM, ss, cs, NULL, NULL, 0);

    rc = SOCK_Close(cs);
    if (rc != 0){
        printf("error in closing socket\r\n");
        return -2000;
    }

    rc = SOCK_Close(ss);
    if (rc != 0){
        printf("error in closing socket\r\n");
        return -2010;
    }
#endif
    return retval;
}

static int ioctl_test(void)
{
    return 0;
}

static int gethostbyname_test(void)
{
    str_t host;
    T_Hostent he;
    int status;
    printf("gethostbyname_test()\r\n");
    host = c_str("an-invalid-host-name");
    status = SOCK_GetHostByName(&host, &he);
    if (status == 0)
		return -20100;
    else
		return 0;
}


int sock_test()
{
    int rc;
    c_create_random_string(bigdata, BIG_DATA_LEN);
    
    rc = format_test();
    if (rc != 0)
		return rc;
#if 0
    rc = parse_test();
    if (rc != 0)
		return rc;

    rc = gethostbyname_test();
    if (rc != 0)
		return rc;

    rc = simple_sock_test();
    if (rc != 0)
	return rc;

    rc = ioctl_test();
    if (rc != 0)
		return rc;

    rc = udp_test();
    if (rc != 0)
		return rc;

    rc = tcp_test();
    if (rc != 0)
		return rc;
#endif
    return 0;
}
