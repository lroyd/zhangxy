
#ifndef __PJ_IOQUEUE_H__
#define __PJ_IOQUEUE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>

#include "lock.h"
#include "mutex.h"
#include "pool.h"
#include "dl_list.h"
#include "timestamp.h"

#ifndef PJ_DECL(x)
#define PJ_DECL(x)	x
#endif

#ifndef PJ_INLINE(z)
#define PJ_INLINE(z)	inline z
#endif
/*************************************************************************/




/*************************************************************************/
#define PJ_IOQUEUE_KEY_CONNECT_TIMEOUT	(5)	//默认连接超时s

#define PJ_IOQUEUE_KEY_FREE_DELAY	(500)	//由closing_list转变成free_list的延时时间(ms)

#define PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY   1
#define PJ_INVALID_SOCKET   (-1)
typedef void pj_sockaddr_t;

#define PJ_IOQUEUE_MAX_HANDLES	(64)

typedef struct pj_ioqueue_key_t	pj_ioqueue_key_t;
typedef struct pj_ioqueue_t	pj_ioqueue_t;
typedef struct pj_ioqueue_callback pj_ioqueue_callback;

struct pj_ioqueue_t
{
    pj_lock_t			*lock;
    char				auto_delete_lock;
    char				default_concurrency;

    unsigned			max, count;

    T_DoubleList		active_list;    
    int					epfd;

    pj_mutex_t			*ref_cnt_mutex;	//
    T_DoubleList		closing_list;
    T_DoubleList		free_list;

};

typedef struct pj_ioqueue_op_key_t
{ 
    void *internal__[32];           /**< Internal I/O Queue data.   */
    void *activesock_data;	    /**< Active socket data.	    */
    void *user_data;                /**< Application data.          */
} pj_ioqueue_op_key_t;

struct pj_ioqueue_callback
{
    void (*on_read_complete)(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, ssize_t bytes_read);
    void (*on_write_complete)(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, ssize_t bytes_sent);
    void (*on_accept_complete)(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, int sock, int status);
    void (*on_connect_complete)(pj_ioqueue_key_t *key, int status);
};

struct pj_ioqueue_key_t
{
	T_DoubleList		list;		//PJ_DECL_LIST_MEMBER(struct pj_ioqueue_key_t);
    pj_ioqueue_t		*ioqueue;
	
    void		*grp_lock;
    pj_lock_t			*lock;
	
    char				inside_callback;
    char				destroy_requested;
    char				allow_concurrent;
	
    int					fd;
    int					fd_type;
    void				*user_data;
    pj_ioqueue_callback	cb;
	
    int					connecting;
	pj_time_val			connect_time;
	
    T_DoubleList	read_list;
    T_DoubleList	write_list;
    T_DoubleList	accept_list;
	
	unsigned			ref_count;
	char				closing;
	pj_time_val			free_time;
	
};








typedef enum pj_ioqueue_operation_e
{
    PJ_IOQUEUE_OP_NONE		= 0,	/**< No operation.          */
    PJ_IOQUEUE_OP_READ		= 1,	/**< read() operation.      */
    PJ_IOQUEUE_OP_RECV          = 2,    /**< recv() operation.      */
    PJ_IOQUEUE_OP_RECV_FROM	= 4,	/**< recvfrom() operation.  */
    PJ_IOQUEUE_OP_WRITE		= 8,	/**< write() operation.     */
    PJ_IOQUEUE_OP_SEND          = 16,   /**< send() operation.      */
    PJ_IOQUEUE_OP_SEND_TO	= 32,	/**< sendto() operation.    */

    PJ_IOQUEUE_OP_ACCEPT	= 64,	/**< accept() operation.    */
    PJ_IOQUEUE_OP_CONNECT	= 128	/**< connect() operation.   */

} pj_ioqueue_operation_e;

#define PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL     (16)
#define PJ_IOQUEUE_MAX_CAND_EVENTS	PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL



#define PJ_IOQUEUE_ALWAYS_ASYNC	    ((int)1 << (int)31)


PJ_DECL(const char*) pj_ioqueue_name(void);
//创建多个free_list
PJ_DECL(int) pj_ioqueue_create(T_PoolInfo *pool, size_t max_fd, pj_ioqueue_t **ioqueue);
PJ_DECL(int) pj_ioqueue_destroy(pj_ioqueue_t *ioque);


PJ_DECL(int) pj_ioqueue_set_lock(pj_ioqueue_t *ioque, pj_lock_t *lock, char auto_delete);
PJ_DECL(int) pj_ioqueue_set_default_concurrency(pj_ioqueue_t *ioqueue, char allow);

PJ_DECL(int) pj_ioqueue_register_sock(T_PoolInfo *pool,
												pj_ioqueue_t *ioque,
												int sock,
												void *user_data,
												const pj_ioqueue_callback *cb,
												pj_ioqueue_key_t **key);
//会从free_list中获取一个
PJ_DECL(int) pj_ioqueue_register_sock2(T_PoolInfo *pool,
												pj_ioqueue_t *ioque,
												int sock,
												void *grp_lock,
												void *user_data,
												const pj_ioqueue_callback *cb,
												pj_ioqueue_key_t **key);

//会有close的操作，再次注册需要重新开启fd
PJ_DECL(int) pj_ioqueue_unregister(pj_ioqueue_key_t *key);	//将移除的key放入close_list中，pj_ioqueue_poll会将close_list重新放入free_list中

PJ_DECL(void*) pj_ioqueue_get_user_data(pj_ioqueue_key_t *key);
PJ_DECL(int) pj_ioqueue_set_user_data(pj_ioqueue_key_t *key, void *user_data, void **old_data);

PJ_DECL(int) pj_ioqueue_set_concurrency(pj_ioqueue_key_t *key, char allow);

PJ_DECL(int) pj_ioqueue_lock_key(pj_ioqueue_key_t *key);
PJ_DECL(int) pj_ioqueue_trylock_key(pj_ioqueue_key_t *key);
PJ_DECL(int) pj_ioqueue_unlock_key(pj_ioqueue_key_t *key);

PJ_DECL(void) pj_ioqueue_op_key_init(pj_ioqueue_op_key_t *op_key, size_t size);

PJ_DECL(char) pj_ioqueue_is_pending(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key);
PJ_DECL(int) pj_ioqueue_post_completion( pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, ssize_t bytes_status);

PJ_DECL(int) pj_ioqueue_accept(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, int *new_sock, pj_sockaddr_t *local, pj_sockaddr_t *remote, int *addrlen);
//异步连接	1.成功则直接回调connect		2.失败 a.直接返回错误(比如连接本地server)   b.无返回，直接poll超时(远端server),不可以通过getsockopt判断
//b的情况下需要人为的控制poll的timeout来判断
PJ_DECL(int) pj_ioqueue_connect(pj_ioqueue_key_t *key, const pj_sockaddr_t *addr, int addrlen);

PJ_DECL(int) pj_ioqueue_poll(pj_ioqueue_t *ioque, const pj_time_val *timeout);

PJ_DECL(int) pj_ioqueue_recv(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, void *buffer, ssize_t *length, int flags);
PJ_DECL(int) pj_ioqueue_recvfrom(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, void *buffer, ssize_t *length, int flags, pj_sockaddr_t *addr, int *addrlen);

PJ_DECL(int) pj_ioqueue_send(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, const void *data, ssize_t *length, int flags);
PJ_DECL(int) pj_ioqueue_sendto(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, const void *data, ssize_t *length, int flags, const pj_sockaddr_t *addr, int addrlen);


#endif

