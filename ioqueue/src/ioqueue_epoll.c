
#include <assert.h>
#include <asm/ioctls.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#include "ioqueue.h"
#include "ioqueue_common_abs.h"

#define epoll_data		data.ptr




struct queue
{
    pj_ioqueue_key_t			*key;
    enum ioqueue_event_type		event_type;
};


static void scan_closing_keys(pj_ioqueue_t *ioqueue);

PJ_DEF(const char*) pj_ioqueue_name(void)
{
	return "epoll";
}


PJ_DEF(int) pj_ioqueue_create(T_PoolInfo *pool, size_t max_fd, pj_ioqueue_t **p_ioqueue)
{
    pj_ioqueue_t *ioqueue;
    int rc;
    pj_lock_t *lock;
    int i;

    PJ_ASSERT_RETURN(pool != NULL && p_ioqueue != NULL && max_fd > 0, PJ_EINVAL);
    PJ_ASSERT_RETURN(sizeof(pj_ioqueue_op_key_t)-sizeof(void*) >= sizeof(union operation_key), PJ_EBUG);

    ioqueue = POOL_Alloc(pool, sizeof(pj_ioqueue_t));
    ioqueue_init(ioqueue);
    ioqueue->max = max_fd;
    ioqueue->count = 0;
	
    DL_ListInit(&ioqueue->active_list);

    rc = pj_mutex_create_simple(pool, NULL, &ioqueue->ref_cnt_mutex);
    if (rc != 0)
		return rc;

    DL_ListInit(&ioqueue->free_list);
    DL_ListInit(&ioqueue->closing_list);

    for (i = 0; i < max_fd; ++i) {
		pj_ioqueue_key_t *key;

		key = POOL_ALLOC_T(pool, pj_ioqueue_key_t);
		key->ref_count = 0;
		rc = pj_lock_create_recursive_mutex(pool, NULL, &key->lock);		
		if (rc != 0) {			
			//分配失败，释放之前挂载的key
			printf("lock create recursive mutex error!!!\r\n");
			DL_ListForeach(key, &ioqueue->free_list, pj_ioqueue_key_t, list){
				pj_lock_destroy(key->lock);
			}
		    pj_mutex_destroy(ioqueue->ref_cnt_mutex);
		    return rc;
		}
		DL_ListAddTail(&ioqueue->free_list, &key->list); //pj_list_push_back(&ioqueue->free_list, key);
    }

    rc = pj_lock_create_simple_mutex(pool, "ioq%p", &lock);
    if (rc != 0) return rc;

    rc = pj_ioqueue_set_lock(ioqueue, lock, 1);
    if (rc != 0) return rc;

    ioqueue->epfd = epoll_create(max_fd);
    if (ioqueue->epfd < 0) {
		ioqueue_destroy(ioqueue);
		return -1;
    }
    
    printf("epoll I/O Queue created (%p)", ioqueue);

    *p_ioqueue = ioqueue;
    return 0;
}


PJ_DEF(int) pj_ioqueue_destroy(pj_ioqueue_t *ioqueue)
{
    pj_ioqueue_key_t *key, *tmp;

    PJ_ASSERT_RETURN(ioqueue, PJ_EINVAL);
    PJ_ASSERT_RETURN(ioqueue->epfd > 0, PJ_EINVALIDOP);

    pj_lock_acquire(ioqueue->lock);
    close(ioqueue->epfd);
    ioqueue->epfd = 0;

	//释放所有active_list的lock
	DL_ListForeach(key, &ioqueue->active_list, pj_ioqueue_key_t, list){
		pj_lock_destroy(key->lock);
	}

	//释放所有closing_list的lock
	DL_ListForeach(key, &ioqueue->closing_list, pj_ioqueue_key_t, list){
		pj_lock_destroy(key->lock);
	}	

	//释放所有free_list的lock
	DL_ListForeach(key, &ioqueue->free_list, pj_ioqueue_key_t, list){
		pj_lock_destroy(key->lock);
	}

    pj_mutex_destroy(ioqueue->ref_cnt_mutex);

    return ioqueue_destroy(ioqueue);
}


PJ_DEF(int) pj_ioqueue_register_sock2(T_PoolInfo *pool,
					      pj_ioqueue_t *ioqueue,
					      int sock,
					      void *grp_lock,
					      void *user_data,
					      const pj_ioqueue_callback *cb,
                          pj_ioqueue_key_t **p_key)
{
    pj_ioqueue_key_t *key = NULL;
    int value;
    struct epoll_event ev;
    int status;
    int rc = 0;
    
    PJ_ASSERT_RETURN(pool && ioqueue && sock != PJ_INVALID_SOCKET && cb && p_key, PJ_EINVAL);

    pj_lock_acquire(ioqueue->lock);

    if (ioqueue->count >= ioqueue->max) 
    {
        rc = -3;
		
		printf("pj_ioqueue_register_sock error: too many files\r\n");
		goto on_return;
    }
#if 1
	//设置非阻塞	
    value = 1;
    if ((rc = ioctl(sock, FIONBIO, (unsigned long)&value))) 
    {
		perror("error");
		printf("pj_ioqueue_register_sock error: ioctl rc=%d\r\n", rc);
        rc = -1;
		goto on_return;
    }
#endif	
	/* 扫描closing_keys，将其重新放入free_list中 */
    scan_closing_keys(ioqueue);

    assert(!DL_ListIsEmpty(&ioqueue->free_list)); //不可以为空
    if (DL_ListIsEmpty(&ioqueue->free_list)) 
    {
		rc = -3;
		goto on_return;
    }
	
	//从空闲表中取出一个
	key = DL_ListGetNext(&ioqueue->free_list, pj_ioqueue_key_t, list);	//key = ioqueue->free_list.next;
	DL_ListAddDelete(&key->list);	//pj_list_erase(key);

    rc = ioqueue_init_key(pool, ioqueue, key, sock, grp_lock, user_data, cb);
    if (rc != 0) 
    {
		key = NULL;
		goto on_return;
    }

    ev.events = EPOLLIN | EPOLLERR;
    ev.epoll_data = (void *)key;
    status = epoll_ctl(ioqueue->epfd, EPOLL_CTL_ADD, sock, &ev);
    if (status < 0)
    {
		rc = -1;
		pj_lock_destroy(key->lock);
		key = NULL;
		printf("pj_ioqueue_register_sock error: epoll_ctl rc=%d\r\n",  status);
		goto on_return;
    }
	
    DL_ListAddTail(&ioqueue->active_list, &key->list); //pj_list_insert_before(&ioqueue->active_list, key);
    ++ioqueue->count;

    printf("socket registered, count=%d\r\n", ioqueue->count);

on_return:
    if (rc != 0) 
    {
#if 0		
		if (key && key->grp_lock)
		    pj_grp_lock_dec_ref_dbg(key->grp_lock, "ioqueue", 0);
#endif		
    }
    *p_key = key;
    pj_lock_release(ioqueue->lock);
    
    return rc;
}

PJ_DEF(int) pj_ioqueue_register_sock( T_PoolInfo *pool,
					      pj_ioqueue_t *ioqueue,
					      int sock,
					      void *user_data,
					      const pj_ioqueue_callback *cb,
					      pj_ioqueue_key_t **p_key)
{
    return pj_ioqueue_register_sock2(pool, ioqueue, sock, NULL, user_data, cb, p_key);
}


static void increment_counter(pj_ioqueue_key_t *key)
{
    pj_mutex_lock(key->ioqueue->ref_cnt_mutex);
    ++key->ref_count;
    pj_mutex_unlock(key->ioqueue->ref_cnt_mutex);
}

//ioqueue_init_key时会初始化1
//epoll中使用会+-1
//pj_ioqueue_unregister回-1
//key->ref_count = 0会被放入到closing_list中，由poll中的scan_closing_keys再将其放入free_list中
static void decrement_counter(pj_ioqueue_key_t *key)
{
    pj_lock_acquire(key->ioqueue->lock);
    pj_mutex_lock(key->ioqueue->ref_cnt_mutex);
    --key->ref_count;
    if (key->ref_count == 0) {
		assert(key->closing == 1);
		pj_gettickcount(&key->free_time);	//获取释放时间
		key->free_time.msec += PJ_IOQUEUE_KEY_FREE_DELAY;
		pj_time_val_normalize(&key->free_time);

		DL_ListAddDelete(&key->list);	//pj_list_erase(key);
		DL_ListAddTail(&key->ioqueue->closing_list, &key->list);	//pj_list_push_back(&key->ioqueue->closing_list, key);
    }
    pj_mutex_unlock(key->ioqueue->ref_cnt_mutex);
    pj_lock_release(key->ioqueue->lock);
}


PJ_DEF(int) pj_ioqueue_unregister(pj_ioqueue_key_t *key)
{
    pj_ioqueue_t *ioqueue;
    struct epoll_event ev;
    int status;
    
    PJ_ASSERT_RETURN(key != NULL, PJ_EINVAL);

    ioqueue = key->ioqueue;

    pj_ioqueue_lock_key(key);

    if (IS_CLOSING(key)) {
		pj_ioqueue_unlock_key(key);
		return 0;
    }

    pj_lock_acquire(ioqueue->lock);

    if (ioqueue->count > 0) {
		--ioqueue->count;
    } else {
		assert(!"Bad ioqueue count in key unregistration!");
		printf("Bad ioqueue count in key unregistration!\r\n");
    }

    ev.events = 0;
    ev.epoll_data = (void *)key;
    status = epoll_ctl( ioqueue->epfd, EPOLL_CTL_DEL, key->fd, &ev);
    if (status != 0) {
		pj_lock_release(ioqueue->lock);
		pj_ioqueue_unlock_key(key);
		return -1;
    }

    close(key->fd);
    pj_lock_release(ioqueue->lock);
	
    key->closing = 1;
    decrement_counter(key);

    if (key->grp_lock) {
#if 0
		void *grp_lock = key->grp_lock;
		pj_grp_lock_dec_ref_dbg(grp_lock, "ioqueue", 0);
		pj_grp_lock_release(grp_lock);
#endif		
    } else {
		pj_ioqueue_unlock_key(key);
    }

    return 0;
}



static void scan_closing_keys(pj_ioqueue_t *ioqueue)
{
    pj_time_val now;
    pj_ioqueue_key_t *h, *t;

    pj_gettickcount(&now);

	DL_ListForeachSafe(h, t, &ioqueue->closing_list, pj_ioqueue_key_t, list){
		assert(h->closing != 0);
		if (PJ_TIME_VAL_GTE(now, h->free_time)){
			DL_ListAddDelete(&h->list);
			DL_ListAddTail(&ioqueue->free_list, &h->list);
		}		
	}
	
#if 0
	h = ioqueue->closing_list.next;
    while (h != &ioqueue->closing_list){
		pj_ioqueue_key_t *next = h->next;
		assert(h->closing != 0);

		h = next;
    }
#endif	
}

PJ_DEF(int) pj_ioqueue_poll( pj_ioqueue_t *ioqueue, const pj_time_val *timeout)
{
    int i, count, event_cnt, processed_cnt;
    int msec;
    enum { MAX_EVENTS = PJ_IOQUEUE_MAX_CAND_EVENTS };
    struct epoll_event events[MAX_EVENTS];
    struct queue queue[MAX_EVENTS];
    pj_timestamp t1, t2;
    pj_ioqueue_key_t *h;
	
    PJ_CHECK_STACK();

    msec = timeout ? PJ_TIME_VAL_MSEC(*timeout) : 9000;

    //printf("start epoll_wait, msec=%d\r\n", msec);
    pj_get_timestamp(&t1);
 
    count = epoll_wait(ioqueue->epfd, events, MAX_EVENTS, msec);
    if (count == 0){
	    if (count == 0 && !DL_ListIsEmpty(&ioqueue->closing_list))
	    {
			pj_lock_acquire(ioqueue->lock);
			scan_closing_keys(ioqueue);
			pj_lock_release(ioqueue->lock);
	    }
		
		//查看是否由连接超时
	    if (!DL_ListIsEmpty(&ioqueue->active_list))
	    {
			ioqueue_connect_timeout(ioqueue);
	    }
		printf("epoll_wait timed out\r\n");
		return count;
    }
    else if (count < 0) 
    {
		printf("epoll_wait error\r\n");
		return -1;
    }

    pj_get_timestamp(&t2);
    //printf("epoll_wait returns %d, time=%d usec\r\n", count, pj_elapsed_usec(&t1, &t2));

    pj_lock_acquire(ioqueue->lock);
	//正常同一时刻只能出发一个事件
	//先判断fd是否有EPOLLERR
	//[NOTE]不可以在回调里面执行移除的操作
    for (event_cnt = 0, i = 0; i < count; ++i) 
	{
		h = (pj_ioqueue_key_t*)(void *)events[i].epoll_data;
		//printf("[%d] event %d: events=0x%02x(0x%02x-0x%02x-0x%02x-0x%02x-0x%02x)\r\n", count, i, events[i].events, EPOLLIN, EPOLLPRI, EPOLLOUT, EPOLLERR, EPOLLHUP);

		if ((events[i].events & EPOLLERR) && !IS_CLOSING(h)) 
		{
		    if (h->connecting) 
		    {
				increment_counter(h);
				queue[event_cnt].key = h;
				queue[event_cnt].event_type = EXCEPTION_EVENT;
				++event_cnt;
		    } 
		    else if (key_has_pending_read(h) || key_has_pending_accept(h)) 
		    {
				increment_counter(h);
				queue[event_cnt].key = h;
				queue[event_cnt].event_type = READABLE_EVENT;
				++event_cnt;
		    }
		    continue;
		}		
		
		if ((events[i].events & EPOLLIN) && (key_has_pending_read(h) || key_has_pending_accept(h)) && !IS_CLOSING(h) ) 
		{
		    increment_counter(h);
		    queue[event_cnt].key = h;
		    queue[event_cnt].event_type = READABLE_EVENT;
		    ++event_cnt;
		    continue;
		}

		if ((events[i].events & EPOLLOUT) && key_has_pending_write(h) && !IS_CLOSING(h)) 
		{
		    increment_counter(h);
		    queue[event_cnt].key = h;
		    queue[event_cnt].event_type = WRITEABLE_EVENT;
		    ++event_cnt;
		    continue;
		}

		if ((events[i].events & EPOLLOUT) && (h->connecting) && !IS_CLOSING(h)) 
		{
		    increment_counter(h);
		    queue[event_cnt].key = h;
		    queue[event_cnt].event_type = WRITEABLE_EVENT;
		    ++event_cnt;
		    continue;
		}
		//printf("event_cnt = %d\r\n", event_cnt);
	}
	
    for (i=0; i<event_cnt; ++i)
    {
#if 0		
		if (queue[i].key->grp_lock)
		    pj_grp_lock_add_ref_dbg(queue[i].key->grp_lock, "ioqueue", 0);
#endif		
    }
	
    pj_lock_release(ioqueue->lock);
    processed_cnt = 0;

    for (i=0; i < event_cnt; ++i) 
    {
		if (processed_cnt < PJ_IOQUEUE_MAX_EVENTS_IN_SINGLE_POLL) 
		{
		    switch (queue[i].event_type)
		    {
			    case READABLE_EVENT:
					if (ioqueue_dispatch_read_event(ioqueue, queue[i].key)) ++processed_cnt;
					break;
			    case WRITEABLE_EVENT:
					if (ioqueue_dispatch_write_event(ioqueue, queue[i].key)) ++processed_cnt;
					break;
			    case EXCEPTION_EVENT:
					if (ioqueue_dispatch_exception_event(ioqueue, queue[i].key)) ++processed_cnt;
					break;
			    case NO_EVENT:
					pj_assert(!"Invalid event!\r\n");
					break;
		    }
		}
		decrement_counter(queue[i].key);
#if 0
		if (queue[i].key->grp_lock)
		    pj_grp_lock_dec_ref_dbg(queue[i].key->grp_lock, "ioqueue", 0);
#endif		
    }

    if (count > 0 && !event_cnt && msec > 0)
    {
		//pj_thread_sleep(msec);
		usleep(msec * 1000);
    }

    //printf("     poll: count=%d events=%d processed=%d\r\n", count, event_cnt, processed_cnt);

    pj_get_timestamp(&t1);
    //printf("ioqueue_poll() returns %d, time=%d usec\r\n",processed_cnt, pj_elapsed_usec(&t2, &t1));

    return processed_cnt;
}

