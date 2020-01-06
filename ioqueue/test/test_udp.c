#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/un.h>
#include <errno.h>

#include "ioqueue.h"
#include "pool.h"


#define PJ_EPENDING		(-5)

#define BUF_MIN_SIZE	    32
#define BUF_MAX_SIZE	    2048


static int	callback_call_count = 0;
static int	callback_read_size = 0, callback_write_size = 0, callback_accept_status = 0, callback_connect_status = 0;

static pj_ioqueue_key_t *callback_read_key = NULL, *callback_write_key = NULL, *callback_accept_key = NULL, *callback_connect_key = NULL;
static pj_ioqueue_op_key_t  *callback_read_op = NULL, *callback_write_op = NULL, *callback_accept_op = NULL;


static void on_ioqueue_read(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, ssize_t bytes_read)
{
    callback_read_key = key;
    callback_read_op = op_key;
    callback_read_size = bytes_read;
	printf("     callback_read_key = %p, bytes=%d\r\n", key, bytes_read);
}

static void on_ioqueue_write(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, ssize_t bytes_written)
{
    callback_write_key = key;
    callback_write_op = op_key;
    callback_write_size = bytes_written;
}

static void on_ioqueue_accept(pj_ioqueue_key_t *key,  pj_ioqueue_op_key_t *op_key, int sock, int status)
{
    callback_accept_key = key;
    callback_accept_op = op_key;
    callback_accept_status = status;
}

static void on_ioqueue_connect(pj_ioqueue_key_t *key, int status)
{
    callback_connect_key = key;
    callback_connect_status = status;
}

static pj_ioqueue_callback test_cb = 
{
    &on_ioqueue_read,
    &on_ioqueue_write,
    &on_ioqueue_accept,
    &on_ioqueue_connect,
};

//两个sock之间交换数据
static int compliance_test(T_PoolFactory *pfactory, char allow_concur)
{
	int status = -1, rc = 0;
	
	/****************************************************************************/
	//创建pool
	char *send_buf, *recv_buf;
	int bufsize = BUF_MIN_SIZE;
	T_PoolInfo *pool = NULL;
	pool = POOL_Create(pfactory, "test", 4000, 4000, NULL);
    send_buf = (char*)POOL_Alloc(pool, bufsize);
    recv_buf = (char*)POOL_Alloc(pool, bufsize);		
	/****************************************************************************/
	//创建sock相关	
    int ssock = -1, csock = -1;
    
	ssock = socket(AF_INET,SOCK_DGRAM, 0);
	csock = socket(AF_INET,SOCK_DGRAM, 0);
	printf("create udp ssock = %d, csock = %d\r\n", ssock, csock);	
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(60001);	
    if(bind(ssock, (struct sockaddr *)&addr, sizeof(addr))==-1)
    {
        perror("bind error");
        return -1;
    }
	/****************************************************************************/
	//创建ioqueue
    pj_ioqueue_t *ioque = NULL;
    pj_ioqueue_key_t *skey = NULL, *ckey = NULL;
 
	rc |= pj_ioqueue_create(pool, PJ_IOQUEUE_MAX_HANDLES, &ioque);
    rc |= pj_ioqueue_set_default_concurrency(ioque, allow_concur);
	
    rc |= pj_ioqueue_register_sock(pool, ioque, ssock, NULL, &test_cb, &skey);
	rc |= pj_ioqueue_register_sock(pool, ioque, csock, NULL, &test_cb, &ckey);
	if (rc){
		printf("create ioqueue failed\r\n");
		return -1;
	}	
	
	/****************************************************************************/
	//serv先接收
	char send_pending, recv_pending;
	pj_ioqueue_op_key_t read_op, write_op;
	int addrlen;
	ssize_t bytes;
	
	char *test_string = "ioq test udp";
	int test_len = strlen(test_string);
	memcpy(send_buf, test_string, test_len);
	
	printf("start skey recvfrom...\r\n");
    memset(&addr, 0, sizeof(addr));
    addrlen = sizeof(addr);
    bytes = bufsize;
    rc = pj_ioqueue_recvfrom(skey, &read_op, recv_buf, &bytes, 0, &addr, &addrlen);
    if (rc != 0 && rc != PJ_EPENDING) {
        printf("...error: pj_ioqueue_recvfrom\r\n");
		status =-28; goto on_error;
    } else if (rc == PJ_EPENDING) {
		recv_pending = 1;
		printf("skey: recvfrom pending\r\n");
    } else {
		printf("skey: recvfrom returned immediate ok!\r\n");
		status =-29; goto on_error;
    }	
	
	/****************************************************************************/
	//client开始发送	
	struct sockaddr_in dst_addr;
	dst_addr.sin_family = AF_INET;
    dst_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dst_addr.sin_port = htons(60001);
    printf("start ckey sendto...\r\n");
    bytes = test_len;
    rc = pj_ioqueue_sendto(ckey, &write_op, send_buf, &bytes, 0, &dst_addr, sizeof(dst_addr));
    if (rc != 0 && rc != PJ_EPENDING) {
        printf("...error: pj_ioqueue_sendto\r\n");
		status=-30; goto on_error;
    } else if (rc == PJ_EPENDING) {
		send_pending = 1;
		printf("ckey: sendto pending\r\n");
    } else {
		send_pending = 0;
		printf("ckey: sendto immediate success\r\n");
    }	

	/****************************************************************************/
	//开始poll
    callback_accept_status = callback_connect_status = -2;
    while (send_pending || recv_pending) 
	{
		int ret;
		pj_time_val timeout = { 5, 0 };
		printf("poll...\r\n");
		ret = pj_ioqueue_poll(ioque, &timeout);
		if (ret == 0) {
			printf("poll: timed out...\r\n");
			status =-45; goto on_error;
		} else if (ret < 0) {
			printf("...ERROR in ioqueue_poll()\r\n");
			status =-50; goto on_error;
		}

		if (callback_read_key != NULL) 
		{
			if (callback_read_size != test_len) {
				status =-61; goto on_error;
			}
			if (callback_read_key != skey) {
				status =-65; goto on_error;
			}
			if (callback_read_op != &read_op) {
				status =-66; goto on_error;
			}

			if (memcmp(send_buf, recv_buf, test_len) != 0) {
				status =-67; goto on_error;
			}
			recv_pending = 0;
		} 

		if (callback_write_key != NULL) {
			if (callback_write_size != test_len) {
				status =-73; goto on_error;
			}
			if (callback_write_key != ckey) {
				status =-75; goto on_error;
			}
			if (callback_write_op != &write_op) {
				status =-76; goto on_error;
			}
			send_pending = 0;
		}
    } 
   
    status = 0;

on_error:
    if (skey)
    	pj_ioqueue_unregister(skey);
    else if (ssock != -1)
		close(ssock);
    
    if (ckey)
    	pj_ioqueue_unregister(ckey);
    else if (csock != -1)
		close(csock);
    
    if (ioque != NULL)
		pj_ioqueue_destroy(ioque);
    POOL_Destroy(pool);
	
    return status;

}

static void on_read_complete(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, ssize_t bytes_read)
{
    unsigned *p_packet_cnt = (unsigned*) pj_ioqueue_get_user_data(key);

    (*p_packet_cnt)++;
}

//检查套接字注销或关闭后是否仍调用回调
static int unregister_test(T_PoolFactory *pfactory, char allow_concur)
{
	int status = -1, rc = 0;
	
	/****************************************************************************/
	//创建pool
	T_PoolInfo *pool = NULL;
	pool = POOL_Create(pfactory, "test", 4000, 4000, NULL);	
	/****************************************************************************/
	//创建sock相关
	enum { RPORT = 50000, SPORT = 50001 };
    int ssock = -1, rsock = -1;
	ssock = socket(AF_INET,SOCK_DGRAM, 0);
	rsock = socket(AF_INET,SOCK_DGRAM, 0);
	printf("create ssock = %d, rsock = %d\r\n", ssock, rsock);
	
    struct sockaddr_in addr, raddr;
    addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SPORT);	
	bind(ssock, (struct sockaddr *)&addr, sizeof(addr));
	
    raddr.sin_family = AF_INET;
	raddr.sin_addr.s_addr = htonl(INADDR_ANY);
    raddr.sin_port = htons(RPORT);	
	bind(rsock, (struct sockaddr *)&raddr, sizeof(raddr));
	/****************************************************************************/
	//创建ioqueue
    pj_ioqueue_t *ioque = NULL;
    pj_ioqueue_key_t *key = NULL;
 
	rc |= pj_ioqueue_create(pool, 16, &ioque);
    rc |= pj_ioqueue_set_default_concurrency(ioque, allow_concur);
	
	//注册rsock回调
	pj_ioqueue_callback cb;
	unsigned packet_cnt;	//回调的参数
	memset(&cb, 0, sizeof(cb));
	cb.on_read_complete = &on_read_complete;
	packet_cnt = 0;
    rc |= pj_ioqueue_register_sock(pool, ioque, rsock, &packet_cnt, &cb, &key);
	if (rc){
		printf("create ioqueue failed\r\n");
		return -1;
	}
	pj_ioqueue_op_key_t opkey;
	pj_ioqueue_op_key_init(&opkey, sizeof(opkey));
	/****************************************************************************/
	//开始读
	char sendbuf[10], recvbuf[10];
	ssize_t bytes;
    bytes = sizeof(recvbuf);
    status = pj_ioqueue_recv(key, &opkey, recvbuf, &bytes, 0); //接收挂起
    if (status != PJ_EPENDING) {
		printf("Expecting PJ_EPENDING, but got this\r\n");
		return -150;
    }	
	//udp直接发送	
	strcpy(sendbuf, "Hello0123");
	bytes = sizeof(sendbuf);
    status = sendto(ssock, (const char*)sendbuf, bytes, 0, (const struct sockaddr*)&raddr, sizeof(addr));
    if (status < 0) {
		printf("sendto error\r\n");
		return -170;
    }
	
	pj_time_val timeout = {1, 0};
	pj_ioqueue_poll(ioque, &timeout);

    if (packet_cnt != 1) {
		return -180;
    }

    //模拟处理事件
    usleep(100 * 1000);

	//再次读取
    bytes = sizeof(recvbuf);
    status = pj_ioqueue_recv(key, &opkey, recvbuf, &bytes, 0);
    if (status != PJ_EPENDING) {
		printf("Expecting PJ_EPENDING, but got this\r\n");
		return -190;
    }

    packet_cnt = 0;

    bytes = sizeof(sendbuf);
    status = sendto(ssock, (const char*)sendbuf, bytes, 0, (const struct sockaddr*)&raddr, sizeof(addr));
    if (status < 0) {
		printf("sendto error\r\n");
		return -200;
    }

    //开始注销
    pj_ioqueue_unregister(key);

    timeout.sec = 1; timeout.msec = 0;
    pj_ioqueue_poll(ioque, &timeout);


    //关闭后不可以接收任何一个包
    if (packet_cnt > 0) {
		printf("....errror: not expecting to receive packet after socket has been closed\r\n");
		return -210;
    }

    close(ssock);
    pj_ioqueue_destroy(ioque);

    POOL_Destroy(pool);

    return 0;
}

//测试多handles
static int many_handles_test(T_PoolFactory *pfactory, char allow_concur)
{
    enum { MAX = PJ_IOQUEUE_MAX_HANDLES };
	int status = -1, rc, count, i;
	/****************************************************************************/
	//创建pool
	T_PoolInfo *pool = NULL;
	pool = POOL_Create(pfactory, "test", 4000, 4000, NULL);
	/****************************************************************************/
    int *sock;
    pj_ioqueue_key_t **key;
    key = (pj_ioqueue_key_t**)POOL_Alloc(pool, MAX * sizeof(pj_ioqueue_key_t*));
    sock = (int*) POOL_Alloc(pool, MAX * sizeof(int));	
	/****************************************************************************/
	//创建ioqueue
    pj_ioqueue_t *ioqueue = NULL;
    pj_ioqueue_key_t *skey = NULL, *ckey0 = NULL, *ckey1 = NULL;
 
	rc |= pj_ioqueue_create(pool, PJ_IOQUEUE_MAX_HANDLES, &ioqueue);
    rc |= pj_ioqueue_set_default_concurrency(ioqueue, allow_concur);	
	/****************************************************************************/
	//创建多个sock
    for (count = 0; count < MAX; ++count) {
		sock[count] = socket(AF_INET,SOCK_DGRAM, 0);
		if (sock[count] == -1) {
			printf("....unable to create %d-th socket, rc=%d\r\n", count, rc);
			break;
		}
		key[count] = NULL;
		rc = pj_ioqueue_register_sock(pool, ioqueue, sock[count], NULL, &test_cb, &key[count]);
		if (rc != 0 || key[count] == NULL) {
			printf("....unable to register %d-th socket, rc=%d\r\n", count, rc));
			return -30;
		}
    }

    for (i = count-1; i >= 0; --i) {
		rc = pj_ioqueue_unregister(key[i]);
		if (rc != 0) {
			printf("...error in pj_ioqueue_unregister\r\n");
		}
    }

    rc = pj_ioqueue_destroy(ioqueue);
    if (rc != 0) {
		printf("...error in pj_ioqueue_destroy\r\n");
    }
    
    POOL_Destroy(pool);

    printf("....many_handles_test() ok\r\n"));

    return 0;
}


static int udp_ioqueue_test_imp(char allow_concur)
{
    int status;

	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);
	
#if 0	 
	printf("================ compliance test [allow:%s] (success scenario)\r\n", allow_concur?"true":"false");
	if (status = compliance_test(&cp.in_pFactory, allow_concur)){
		printf("FAILED (status=%d)\r\n", status);
	}
#endif


#if 0	 
	printf("================ unregister_test [allow:%s]\r\n", allow_concur?"true":"false");
	if (status = unregister_test(&cp.in_pFactory, allow_concur)){
		printf("FAILED (status=%d)\r\n", status);
	}
#endif

#if 1	 
	printf("================ many_handles_test [allow:%s]\r\n", allow_concur?"true":"false");
	if (status = many_handles_test(&cp.in_pFactory, allow_concur)){
		printf("FAILED (status=%d)\r\n", status);
	}
#endif

    return 0;
}


int main()
{
	udp_ioqueue_test_imp(1);
	
	//udp_ioqueue_test_imp(0);
	
	return 0;
}



