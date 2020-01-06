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

#define BUF_MIN_SIZE	    32

#define PJ_EPENDING		(-5)


static int	callback_call_count = 0;
static int	callback_read_size = 0, callback_write_size = 0, callback_accept_status = 0, callback_connect_status = 0;

static pj_ioqueue_key_t *callback_read_key = NULL, *callback_write_key = NULL, *callback_accept_key = NULL, *callback_connect_key = NULL;
static pj_ioqueue_op_key_t  *callback_read_op = NULL, *callback_write_op = NULL, *callback_accept_op = NULL;

static void on_ioqueue_read(pj_ioqueue_key_t *key,  pj_ioqueue_op_key_t *op_key, ssize_t bytes_read)
{
	//printf("22222222 %d\r\n", bytes_read);
    callback_read_key = key;
    callback_read_op = op_key;
    callback_read_size = bytes_read;
    callback_call_count++;
}

static void on_ioqueue_write(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, ssize_t bytes_written)
{
	//printf("111111111  %d\r\n", bytes_written);
    callback_write_key = key;
    callback_write_op = op_key;
    callback_write_size = bytes_written;
    callback_call_count++;
}

//callback_accept_status:-61/0
static void on_ioqueue_accept(pj_ioqueue_key_t *key,  pj_ioqueue_op_key_t *op_key, int sock, int status)
{
    if (sock == -1) {
		if (status != 0) {
			/* Ignore. Could be blocking error */
			printf(".....warning: received error in on_ioqueue_accept() callback\r\n");
		} else {
			callback_accept_status = -61;
			printf("..... on_ioqueue_accept() callback was given invalid socket and status is %d\r\n", status);
		}
    } else {
		struct sockaddr_in addr;
		int addr_len = sizeof(addr);

        if (getsockname(sock, (struct sockaddr *)&addr, (socklen_t*)&addr_len)) {
            printf("...ERROR in pj_sock_getsockname()\r\n");
        }

		callback_accept_key		= key;
		callback_accept_op		= op_key;
		callback_accept_status	= status;
		callback_call_count++;
		printf("on_ioqueue_accept ok fd = %d, ret = %d\r\n", sock, status);
    }
}

static void on_ioqueue_connect(pj_ioqueue_key_t *key, int status)
{
	printf("on_ioqueue_connect = %d\r\n", status);
    callback_connect_key = key;
    callback_connect_status = status;
    callback_call_count++;
}



static pj_ioqueue_callback test_cb = 
{
    &on_ioqueue_read,
    &on_ioqueue_write,
    &on_ioqueue_accept,
    &on_ioqueue_connect,
};


static int send_recv_test(pj_ioqueue_t *ioque, pj_ioqueue_key_t *skey, pj_ioqueue_key_t *ckey,
						  void *send_buf, void *recv_buf,
						  ssize_t bufsize,
						  pj_timestamp *t_elapsed)
{
    int status;
    ssize_t bytes;
    pj_time_val timeout;
    pj_timestamp t1, t2;
    int pending_op = 0;
    pj_ioqueue_op_key_t read_op, write_op;
	//printf("===========================================\r\n");
    // Start reading on the server side.
    bytes = bufsize;
    status = pj_ioqueue_recv(skey, &read_op, recv_buf, &bytes, 0);
    if (status != 0 && status != PJ_EPENDING) {
        printf("...pj_ioqueue_recv error\r\n");
		return -100;
    }
    
    if (status == PJ_EPENDING)
        ++pending_op;
    else 
        return -115;

    //测试数据
	memcpy(send_buf, "1234567890", 10 + 1);
    //pj_create_random_string((char*)send_buf, bufsize);

    //客户端开始发送
    bytes = strlen(send_buf);
    status = pj_ioqueue_send(ckey, &write_op, send_buf, &bytes, 0);
    if (status != 0 && status != PJ_EPENDING) {
		return -120;
    }
	
    if (status == PJ_EPENDING) ++pending_op;
	

    //开始计时
    pj_get_timestamp(&t1);

    //重置标识
    callback_read_size = callback_write_size = 0;
    callback_read_key = callback_write_key = NULL;
    callback_read_op = callback_write_op = NULL;

    // Poll the queue until we've got completion event in the server side.
    while (pending_op) 
    {
		timeout.sec = 1; timeout.msec = 0;
		status = pj_ioqueue_poll(ioque, &timeout);
		if (status > 0) 
		{
            if (callback_read_size) 
            {
                if (callback_read_size != 10) return -160;
                if (callback_read_key != skey)return -161;
                if (callback_read_op != &read_op)return -162;
            }
            
            if (callback_write_size) 
            {
                if (callback_write_key != ckey)return -163;
                if (callback_write_op != &write_op)return -164;
            }
		    pending_op -= status;
		}
		
        if (status == 0) printf("...error: timed out\r\n");
        
		if (status < 0) return -170;
    }


    //没有事件再次调用应该是0
    timeout.sec = timeout.msec = 0;
    status = pj_ioqueue_poll(ioque, &timeout);
    if (status != 0) return -173;
        

    //结束时间
    pj_get_timestamp(&t2);
    t_elapsed->u32.lo += (t2.u32.lo - t1.u32.lo);

    //比较发送接收缓冲区数据是否一致
    if (memcmp(send_buf, recv_buf, bufsize) != 0) return -180;

	printf("send_recv_test ok (%s)(%s) size = %d\r\n", send_buf, recv_buf, strlen(send_buf));
	
    return 0;
}

static int compliance_test_0(T_PoolFactory *pfactory, char allow_concur)
{
	int status = -1, pending_op = 0, rc = 0, process_cnt = 0;
    pj_timestamp t_elapsed;
    //pj_str_t s;
	pj_ioqueue_op_key_t accept_op;
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
    int ssock = -1, csock0 = -1, csock1 = -1, enable = 1;
    
	ssock = socket(AF_INET,SOCK_STREAM, 0);
	setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR,(void *)&enable, sizeof (enable));
	
	csock1 = socket(AF_INET,SOCK_STREAM, 0);
	printf("create ssock = %d, csock1 = %d\r\n", ssock, csock1);
	
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
    pj_ioqueue_key_t *skey = NULL, *ckey0 = NULL, *ckey1 = NULL;
 
	rc |= pj_ioqueue_create(pool, PJ_IOQUEUE_MAX_HANDLES, &ioque);
    rc |= pj_ioqueue_set_default_concurrency(ioque, allow_concur);
	
    rc |= pj_ioqueue_register_sock(pool, ioque, ssock, NULL, &test_cb, &skey);
	rc |= pj_ioqueue_register_sock(pool, ioque, csock1, NULL, &test_cb, &ckey1);
	if (rc){
		printf("create ioqueue failed\r\n");
		return -1;
	}

    //开始监听
	listen(ssock, 5);
	/****************************************************************************/
    // Server socket accept()
    struct sockaddr_in rmt_addr;
    int rmt_addr_len = sizeof(rmt_addr);
	
    status = pj_ioqueue_accept(skey, &accept_op, &csock0, NULL, &rmt_addr, &rmt_addr_len);
    if (status != PJ_EPENDING) 
    {
        printf("...ERROR in pj_ioqueue_accept()\r\n");
		return -1;
    }
 
    if (status == PJ_EPENDING) ++pending_op;
	
    status = pj_ioqueue_connect(ckey1, &addr, sizeof(addr));
    if (status !=0 && status != PJ_EPENDING) 
    {
        printf("...ERROR in pj_ioqueue_connect()\r\n");
		return -1;
    }
    
    if (status == PJ_EPENDING) ++pending_op;

    //等待连接完成
    callback_accept_status = callback_connect_status = -2;

    while (pending_op) 
    {
		pj_time_val timeout = {1, 0};
		process_cnt = pj_ioqueue_poll(ioque, &timeout);
		//printf("pj_ioqueue_poll event = %d\r\n", process_cnt);
		if (process_cnt > 0){
			if (callback_accept_status != -2){
				if (callback_accept_status != 0){
					status=-41; goto on_error;
				}
				
				if (callback_accept_key != skey){
					status=-42; goto on_error;
				}
				
				if (callback_accept_op != &accept_op){
					status=-43; goto on_error;
				}
				callback_accept_status = -2;
			}

			if (callback_connect_status != -2){
				if (callback_connect_status != 0){
					status=-50; goto on_error;
				}
				
				if (callback_connect_key != ckey1){
					status=-51; goto on_error;
				}
				callback_connect_status = -2;
			}

		    if (process_cnt > pending_op) 
		    {
				printf("...error: pj_ioqueue_poll() returned %d ""(only expecting %d)\r\n", status, pending_op);
				return -52;
		    }
		    pending_op -= process_cnt;

		    if (pending_op == 0) status = 0;
		}
    }
	printf("pj_ioqueue_poll exit, pending = %d\r\n", pending_op);
    // There's no pending operation.
    // When we poll the ioqueue, there must not be events.
    if (pending_op == 0) {
        pj_time_val timeout = {1, 0};
        status = pj_ioqueue_poll(ioque, &timeout);
        if (status != 0) {
            status =- 60; goto on_error;
        }
    }

    //检查ssock accept的csock0是否成功
    if (csock0 == -1) {
		status = -69;
        printf("...accept() error\r\n");
		goto on_error;
    }

    //将csock0注册上，和csock1通信
    rc = pj_ioqueue_register_sock(pool, ioque, csock0, NULL, &test_cb, &ckey0);
    if (rc != 0) {
        printf("...ERROR in pj_ioqueue_register_sock\r\n");
		status = -70;
		goto on_error;
    }

    //ckey1(c) -> ckey0(s)
    t_elapsed.u32.lo = 0;
	
    status = send_recv_test(ioque, ckey0, ckey1, send_buf, recv_buf, bufsize, &t_elapsed);
    if (status != 0) {
		goto on_error;
    }

    status = 0;

on_error:

    if (skey != NULL)
    	pj_ioqueue_unregister(skey);
    else if (ssock != -1)
		close(ssock);
    
    if (ckey1 != NULL)
    	pj_ioqueue_unregister(ckey1);
    else if (csock1 != -1)
		close(csock1);
    
    if (ckey0 != NULL)
    	pj_ioqueue_unregister(ckey0);
    else if (csock0 != -1)
		close(csock0);
    
    if (ioque != NULL)
		pj_ioqueue_destroy(ioque);
    POOL_Destroy(pool);


	
    return status;
}

//客户都安连接到一个不存在的服务上
static int compliance_test_1(T_PoolFactory *pfactory, char allow_concur)
{
	int status = -1, pending_op = 0, rc = 0, process_cnt = 0;
    pj_timestamp t_elapsed;
    //pj_str_t s;
	pj_ioqueue_op_key_t accept_op;
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
    int csock1 = -1;
	csock1 = socket(AF_INET,SOCK_STREAM, 0);
	printf("create csock1 = %d\r\n", csock1);
	

	/****************************************************************************/
	//创建ioqueue
    pj_ioqueue_t *ioque = NULL;
    pj_ioqueue_key_t *ckey1 = NULL;
 
	rc |= pj_ioqueue_create(pool, PJ_IOQUEUE_MAX_HANDLES, &ioque);
    rc |= pj_ioqueue_set_default_concurrency(ioque, allow_concur);
	
	rc |= pj_ioqueue_register_sock(pool, ioque, csock1, NULL, &test_cb, &ckey1);
	if (rc){
		printf("create ioqueue failed\r\n");
		return -1;
	}

	/****************************************************************************/
    // Server socket accept()
    char *ip = "192.168.1.164";
	//char *ip = "0.0.0.0";	
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);	//htonl(INADDR_ANY);
    addr.sin_port = htons(60001);
	
    status = pj_ioqueue_connect(ckey1, &addr, sizeof(addr));
    if (status !=0 && status != PJ_EPENDING) 
    {
        printf("...ERROR in pj_ioqueue_connect()\r\n");
		return -30;
    }
    
    if (status == PJ_EPENDING) ++pending_op;

    callback_connect_status = -2;
    callback_connect_key = NULL;

    while (pending_op) 
    {
		pj_time_val timeout = {1, 0};
		process_cnt = pj_ioqueue_poll(ioque, &timeout);
		if (process_cnt > 0){
			if (callback_connect_status != -2){
				if (callback_connect_status == 0){
					//不希望的结果
					status = -50; goto on_error;
				}
				else{
					if (ckey1)	//status != 0 表示连接异常，需要移除fd监听
						pj_ioqueue_unregister(ckey1);					
				}
			}

		    if (process_cnt > pending_op) 
		    {
				printf("...error: pj_ioqueue_poll() returned %d ""(only expecting %d)\r\n", status, pending_op);
				return -52;
		    }
		    pending_op -= process_cnt;

		    if (pending_op == 0) status = 0;
		}
    }

	printf("pj_ioqueue_poll exit, pending = %d\r\n", pending_op);
    if (pending_op == 0) {
        pj_time_val timeout = {1, 0};
        status = pj_ioqueue_poll(ioque, &timeout);
        if (status != 0) {
            status =- 60; goto on_error;
        }
    }

    status = 0;

on_error:
    
    if (ckey1 != NULL)
    	pj_ioqueue_unregister(ckey1);
    else if (csock1 != -1)
		close(csock1);
    
    if (ioque != NULL)
		pj_ioqueue_destroy(ioque);
    POOL_Destroy(pool);

    return status;
}


//重复connect/accept在同一个监听的sock上
static int compliance_test_2(T_PoolFactory *pfactory, char allow_concur)
{
	int status = -1, pending_op = 0, rc = 0, process_cnt = 0;
    pj_timestamp t_elapsed;
    //pj_str_t s;
	pj_ioqueue_op_key_t accept_op;
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
    int ssock = -1, enable = 1, i, test_loop;
    
	ssock = socket(AF_INET,SOCK_STREAM, 0);
	setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR,(void *)&enable, sizeof (enable));
	printf("create ssock = %d\r\n", ssock);
	
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
    pj_ioqueue_key_t *skey = NULL;
 
	rc |= pj_ioqueue_create(pool, PJ_IOQUEUE_MAX_HANDLES, &ioque);
    rc |= pj_ioqueue_set_default_concurrency(ioque, allow_concur);
    rc |= pj_ioqueue_register_sock(pool, ioque, ssock, NULL, &test_cb, &skey);
	if (rc){
		printf("create ioqueue failed\r\n");
		return -1;
	}

    //开始监听
	listen(ssock, 5);
	/****************************************************************************/
	//创建client
	enum { MAX_PAIR = 4, TEST_LOOP = 2 };
	
    struct server
    {
		int						sock;
		pj_ioqueue_key_t    	*key;
		struct sockaddr_in		local_addr;
		struct sockaddr_in		rem_addr;
		int						rem_addr_len;
		pj_ioqueue_op_key_t		accept_op;
    }server[MAX_PAIR];	
	
    struct client
    {
		int	     sock;
		pj_ioqueue_key_t *key;
    }client[MAX_PAIR];	
	
    for (i = 0; i < MAX_PAIR; ++i) {
    	client[i].sock = -1;
    	client[i].key = NULL;	
    }
	
    for (i = 0; i < MAX_PAIR; ++i) {
    	server[i].sock = -1;
    	server[i].key = NULL;
    }	
	/****************************************************************************/
    for (test_loop = 0; test_loop < TEST_LOOP; ++test_loop)
	{
		for (i = 0; i < MAX_PAIR; ++i) 
		{
			//1.client fd
			client[i].sock = socket(AF_INET,SOCK_STREAM, 0);
			rc = pj_ioqueue_register_sock(pool, ioque, client[i].sock, NULL, &test_cb, &client[i].key);
			if (rc){
				printf("...ERROR pj_ioqueue_register_sock\r\n");
				status = -80; goto on_error;
			}
			pj_ioqueue_op_key_init(&server[i].accept_op, sizeof(server[i].accept_op));
			server[i].rem_addr_len = sizeof(struct sockaddr_in);
			//2.server accept
			status = pj_ioqueue_accept(skey, &server[i].accept_op, &server[i].sock, &server[i].local_addr, &server[i].rem_addr, &server[i].rem_addr_len);
			if (status != 0 && status != PJ_EPENDING) {
				printf("...ERROR in pj_ioqueue_accept()\r\n");
				status = -90; goto on_error;
			}
			if (status == PJ_EPENDING) ++pending_op;

			//3.client connect
			status = pj_ioqueue_connect(client[i].key, &addr, sizeof(addr));
			if (status != 0 && status != PJ_EPENDING) {
				printf("...ERROR in pj_ioqueue_connect()\r\n");
				status = -100; goto on_error;
			}
			if (status == PJ_EPENDING) ++pending_op;

			//4. Poll until connection of this pair established
			while (pending_op) {
				pj_time_val timeout = {1, 0};
				status = pj_ioqueue_poll(ioque, &timeout);
				if (status > 0) {
					if (status > pending_op) {
						printf("...error: pj_ioqueue_poll() returned %d (only expecting %d)", status, pending_op);
						return -110;
					}
					pending_op -= status;

					if (pending_op == 0) status = 0;
				}
			}
		}		
	}
	
	/****************************************************************************/
	//检查测试阶段
	if (pending_op == 0) {
	    pj_time_val timeout = {1, 0};
	    status = pj_ioqueue_poll(ioque, &timeout);
	    if (status != 0) {
			status=-120; goto on_error;
	    }
	}	
	
	for (i = 0; i < MAX_PAIR; ++i) {
	    //检查server
	    if (server[i].sock == -1) {
			status = -130;
			printf("...accept() error\r\n");
			goto on_error;
	    }

	    //检查server addresses
	    if (server[i].local_addr.sin_family != AF_INET || server[i].local_addr.sin_addr.s_addr == 0 || server[i].local_addr.sin_port == 0)
	    {
			printf("...ERROR address not set\r\n");
			status = -140;
			goto on_error;
	    }

	    if (server[i].rem_addr.sin_family != AF_INET || server[i].rem_addr.sin_addr.s_addr == 0 || server[i].rem_addr.sin_port == 0)
	    {
			printf("...ERROR address not set\r\n");
			status = -150;
			goto on_error;
	    }

	    //注册新的 accepted socket.
	    rc = pj_ioqueue_register_sock(pool, ioque, server[i].sock, NULL, &test_cb, &server[i].key);
	    if (rc != 0) {
			printf("...ERROR in pj_ioqueue_register_sock\r\n");
			status = -160;
			goto on_error;
	    }

	    //测试发送接收.
	    t_elapsed.u32.lo = 0;
	    status = send_recv_test(ioque, server[i].key, client[i].key, send_buf, recv_buf, bufsize, &t_elapsed);
	    if (status != 0) goto on_error;
	}	

	status = 0;
	
	/****************************************************************************/
	//关闭
	for (i = 0; i < MAX_PAIR; ++i) {
	    if (server[i].key != NULL) {
			pj_ioqueue_unregister(server[i].key);
			server[i].key = NULL;
			server[i].sock = -1;
	    } else if (server[i].sock != -1) {
			close(server[i].sock);
			server[i].sock = -1;
	    }

	    if (client[i].key != NULL) {
			pj_ioqueue_unregister(client[i].key);
			client[i].key = NULL;
			client[i].sock = -1;
	    } else if (client[i].sock != -1) {
			close(client[i].sock);
			client[i].sock = -1;
	    }
	}

    status = 0;
	/****************************************************************************/	
on_error:
    for (i=0; i<MAX_PAIR; ++i) {
		if (server[i].key != NULL) {
			pj_ioqueue_unregister(server[i].key);
			server[i].key = NULL;
			server[i].sock = -1;
		} else if (server[i].sock != -1) {
			close(server[i].sock);
			server[i].sock = -1;
		}

		if (client[i].key != NULL) {
			pj_ioqueue_unregister(client[i].key);
			client[i].key = NULL;
			server[i].sock = -1;
		} else if (client[i].sock != -1) {
			close(client[i].sock);
			client[i].sock = -1;
		}
    }

    if (skey) {
		pj_ioqueue_unregister(skey);
		skey = NULL;
    } else if (ssock != -1) {
		close(ssock);
		ssock = -1;
    }

    if (ioque != NULL)
		pj_ioqueue_destroy(ioque);
    POOL_Destroy(pool);
	
    return status;
}


static int tcp_ioqueue_test_impl(char allow_concur)
{
    int status;
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);
#if 0	 
	printf("================ compliance test 0[allow:%s] (success scenario)\r\n", allow_concur?"true":"false");
	if (status = compliance_test_0(&cp.in_pFactory, allow_concur)){
		printf("FAILED (status=%d)\r\n", status);
	}
#endif

#if 1
	printf("================ compliance test 1[allow:%s] (connect failed scenario)\r\n", allow_concur?"true":"false");
	if (status = compliance_test_1(&cp.in_pFactory, allow_concur)){
		printf("FAILED (status=%d)\r\n", status);
	}
	
#endif
	
#if 0	
	printf("================ compliance test 2[allow:%s] (repeated accept)\r\n", allow_concur?"true":"false");
	if (status = compliance_test_2(&cp.in_pFactory, allow_concur)){
		printf("FAILED (status=%d)\r\n", status);
	}	
#endif
    return 0;
}


int main()
{
	tcp_ioqueue_test_impl(1);
	
	//tcp_ioqueue_test_impl(0);
	
	return 0;
}