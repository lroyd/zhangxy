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
#include <assert.h>
#include <asm/ioctls.h>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#include "ioqueue.h"
#include "ioqueue_common_abs.h"

#define PENDING_RETRY	2
#define epoll_data		data.ptr

void ioqueue_remove_from_set(pj_ioqueue_t *ioqueue, pj_ioqueue_key_t *key, enum ioqueue_event_type event_type)
{
    if (event_type == WRITEABLE_EVENT) {
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLERR;
		ev.epoll_data = (void *)key;
		epoll_ctl(ioqueue->epfd, EPOLL_CTL_MOD, key->fd, &ev);
    }	
}

void ioqueue_add_to_set(pj_ioqueue_t *ioqueue, pj_ioqueue_key_t *key, enum ioqueue_event_type event_type )
{
    if (event_type == WRITEABLE_EVENT) {
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLERR;
		ev.epoll_data = (void *)key;
		epoll_ctl(ioqueue->epfd, EPOLL_CTL_MOD, key->fd, &ev);
    }	
}


void ioqueue_init(pj_ioqueue_t *ioqueue)
{
    ioqueue->lock = NULL;
    ioqueue->auto_delete_lock = 0;
    ioqueue->default_concurrency = PJ_IOQUEUE_DEFAULT_ALLOW_CONCURRENCY;
}

int ioqueue_destroy(pj_ioqueue_t *ioqueue)
{
    if (ioqueue->auto_delete_lock && ioqueue->lock ) {
		pj_lock_release(ioqueue->lock);
        return pj_lock_destroy(ioqueue->lock);
    }
    return 0;
}

PJ_DEF(int) pj_ioqueue_set_lock(pj_ioqueue_t *ioqueue, pj_lock_t *lock, char auto_delete)
{
    PJ_ASSERT_RETURN(ioqueue && lock, PJ_EINVAL);

    if (ioqueue->auto_delete_lock && ioqueue->lock) {
        pj_lock_destroy(ioqueue->lock);
    }

    ioqueue->lock = lock;
    ioqueue->auto_delete_lock = auto_delete;

    return 0;
}

int ioqueue_init_key(T_PoolInfo *pool,
							 pj_ioqueue_t *ioqueue,
							 pj_ioqueue_key_t *key,
							 int sock,
							 void *grp_lock,
							 void *user_data,
							 const pj_ioqueue_callback *cb)
{
    int rc;
    int optlen;

    key->ioqueue = ioqueue;
    key->fd = sock;
    key->user_data = user_data;
	
    DL_ListInit(&key->read_list);
    DL_ListInit(&key->write_list);

    DL_ListInit(&key->accept_list);
    key->connecting = 0;

    memcpy(&key->cb, cb, sizeof(pj_ioqueue_callback));

    assert(key->ref_count == 0);
    ++key->ref_count;

    key->closing = 0;


    rc = pj_ioqueue_set_concurrency(key, ioqueue->default_concurrency);
    if (rc != 0) return rc;

    optlen = sizeof(key->fd_type);
    rc = getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *)&key->fd_type, (socklen_t *)&optlen);
    if (rc != 0)
        key->fd_type = SOCK_STREAM;
	
#if 0
    key->grp_lock = grp_lock;
    if (key->grp_lock) {
		//pj_grp_lock_add_ref_dbg(key->grp_lock, "ioqueue", 0);
    }
#endif   
    return 0;
}

PJ_DEF(void*) pj_ioqueue_get_user_data(pj_ioqueue_key_t *key)
{
    PJ_ASSERT_RETURN(key != NULL, NULL);
    return key->user_data;
}

PJ_DEF(int) pj_ioqueue_set_user_data(pj_ioqueue_key_t *key, void *user_data, void **old_data)
{
    PJ_ASSERT_RETURN(key, PJ_EINVAL);

    if (old_data)
        *old_data = key->user_data;
    key->user_data = user_data;

    return 0;
}

PJ_INLINE(int) key_has_pending_write(pj_ioqueue_key_t *key)
{
    return !DL_ListIsEmpty(&key->write_list);
}

PJ_INLINE(int) key_has_pending_read(pj_ioqueue_key_t *key)
{
    return !DL_ListIsEmpty(&key->read_list);
}

PJ_INLINE(int) key_has_pending_accept(pj_ioqueue_key_t *key)
{
    return !DL_ListIsEmpty(&key->accept_list);
}

PJ_INLINE(int) key_has_pending_connect(pj_ioqueue_key_t *key)
{
	//if (ts) ts = &key->connect_time;
    return key->connecting;
}



char ioqueue_dispatch_write_event(pj_ioqueue_t *ioqueue, pj_ioqueue_key_t *h)
{
    int rc;
    rc = pj_ioqueue_trylock_key(h);
    if (rc != 0) {
		return 0;
    }

    if (IS_CLOSING(h)) {
		pj_ioqueue_unlock_key(h);
		return 1;
    }

    if (h->connecting) 
	{
		int status;
		char has_lock;

		h->connecting = 0;

        ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);
        ioqueue_remove_from_set(ioqueue, h, EXCEPTION_EVENT);

		int value;
		int vallen = sizeof(value);
		status = getsockopt(h->fd, SOL_SOCKET, SO_ERROR, (char *)&value, (socklen_t*)&vallen);
		if (h->allow_concurrent) {
		    has_lock = 0;
		    pj_ioqueue_unlock_key(h);
		} else {
		    has_lock = 1;
		}

		if (h->cb.on_connect_complete && !IS_CLOSING(h)) (*h->cb.on_connect_complete)(h, status);

		if (has_lock) {
		    pj_ioqueue_unlock_key(h);
		}
	}
	else if (key_has_pending_write(h)) 
	{
        struct write_operation *write_op;
        ssize_t sent;
        int send_rc = 0;
		write_op = DL_ListGetNext(&h->write_list, struct write_operation, list);	//write_op = h->write_list.next;
        
        if (h->fd_type == SOCK_DGRAM) {
            DL_ListAddDelete(&write_op->list);	//pj_list_erase(write_op);
            if (DL_ListIsEmpty(&h->write_list))
                ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);
        }

        sent = write_op->size - write_op->written;
        if (write_op->op == PJ_IOQUEUE_OP_SEND) {
            send_rc = send(h->fd, (char *)write_op->buf + write_op->written, (int)sent, write_op->flags);
        } else if (write_op->op == PJ_IOQUEUE_OP_SEND_TO) {
		    int retry = 2;
		    while (--retry >= 0) 
		    {
				send_rc = sendto(h->fd, 
							 (char *)write_op->buf + write_op->written,
							 (int)sent, write_op->flags,
							 (struct sockaddr *)&write_op->rmt_addr, 
							 write_op->rmt_addrlen);
				break;
		    }

        } else {
            assert(!"Invalid operation type!");
	    	write_op->op = PJ_IOQUEUE_OP_NONE;
            send_rc = -1;
        }

        if (send_rc >= 0) {
			//发送成功
            write_op->written += sent;
        } else {
			//发送失败
            assert(send_rc > 0);
            write_op->written = send_rc;
        }

        if (send_rc != 0 || write_op->written == (ssize_t)write_op->size || h->fd_type == SOCK_DGRAM) 
        {
	    	char has_lock;
	    	write_op->op = PJ_IOQUEUE_OP_NONE;

            if (h->fd_type != SOCK_DGRAM) {
                DL_ListAddDelete(&write_op->list);	//pj_list_erase(write_op);
                if (DL_ListIsEmpty(&h->write_list))
                    ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);
            }

		    if (h->allow_concurrent) {
				has_lock = 0;
				pj_ioqueue_unlock_key(h);
		    } else {
				has_lock = 1;
		    }

            if (h->cb.on_write_complete && !IS_CLOSING(h)) {
	        	(*h->cb.on_write_complete)(h, (pj_ioqueue_op_key_t*)write_op, write_op->written);
            }

		    if (has_lock) {
				pj_ioqueue_unlock_key(h);
		    }

        } else {
            pj_ioqueue_unlock_key(h);
        }
    } else {
		pj_ioqueue_unlock_key(h);
		return 0;
    }

    return 1;
}

char ioqueue_dispatch_read_event(pj_ioqueue_t *ioqueue, pj_ioqueue_key_t *h)
{
    int rc;
    rc = pj_ioqueue_trylock_key(h);
    if (rc != 0)
    {
		return 0;
    }

    if (IS_CLOSING(h))
    {
		pj_ioqueue_unlock_key(h);
		return 1;
    }

    if (!DL_ListIsEmpty(&h->accept_list)) 
	{
		struct accept_operation *accept_op;
		char has_lock;

		accept_op = DL_ListGetNext(&h->accept_list, struct accept_operation, list);	//accept_op = h->accept_list.next;
        DL_ListAddDelete(&accept_op->list);	//pj_list_erase(accept_op);
		
        accept_op->op = PJ_IOQUEUE_OP_NONE;

        if (DL_ListIsEmpty(&h->accept_list))
            ioqueue_remove_from_set(ioqueue, h, READABLE_EVENT);

		rc = accept(h->fd, accept_op->rmt_addr, accept_op->addrlen);
		if (rc >= 0)
		{
			//accept成功
			*accept_op->accept_fd = rc;
			if (accept_op->local_addr)
				getsockname(*accept_op->accept_fd, accept_op->local_addr, accept_op->addrlen);
			rc = 0;
		}

		if (h->allow_concurrent) 
		{
		    has_lock = 0;
		    pj_ioqueue_unlock_key(h);
		} 
		else
		{
		    has_lock = 1;
		}

        if (h->cb.on_accept_complete && !IS_CLOSING(h)) 
        {
		    (*h->cb.on_accept_complete)(h, (pj_ioqueue_op_key_t*)accept_op, *accept_op->accept_fd, rc);
		}

		if (has_lock) 
		{
		    pj_ioqueue_unlock_key(h);
		}
	}
    else if (key_has_pending_read(h)) 
    {
        struct read_operation *read_op;
        ssize_t bytes_read = 0;
		char has_lock;
		
		
		
		read_op = DL_ListGetNext(&h->read_list, struct read_operation, list);	//read_op = h->read_list.next;
        DL_ListAddDelete(&read_op->list);	//pj_list_erase(read_op);

        if (DL_ListIsEmpty(&h->read_list))
            ioqueue_remove_from_set(ioqueue, h, READABLE_EVENT);

		
        bytes_read = read_op->size;
		
		if (read_op->op == PJ_IOQUEUE_OP_RECV_FROM) 
		{
		    read_op->op = PJ_IOQUEUE_OP_NONE;
		    rc = recvfrom(h->fd, (char *)read_op->buf, (int)bytes_read, read_op->flags, (struct sockaddr *)read_op->rmt_addr, (socklen_t *)read_op->rmt_addrlen);
		}
		else if (read_op->op == PJ_IOQUEUE_OP_RECV) 
		{
		    read_op->op = PJ_IOQUEUE_OP_NONE;
		    rc = recv(h->fd, (char *)read_op->buf, (int)bytes_read,  read_op->flags);
        } 
        else
        {
            assert(read_op->op == PJ_IOQUEUE_OP_READ);
	    	read_op->op = PJ_IOQUEUE_OP_NONE;

			bytes_read = read(h->fd, read_op->buf, bytes_read);
			rc = (bytes_read >= 0) ? 0 : -1;

        }

		if (rc >= 0){
			//读数据成功
			bytes_read = rc;
		}

		if (h->allow_concurrent) 
		{
		    has_lock = 0;
		    pj_ioqueue_unlock_key(h);
		}
		else 
		{
		    has_lock = 1;
		}

	    if (h->cb.on_read_complete && !IS_CLOSING(h)) 
	    {
	    	(*h->cb.on_read_complete)(h, (pj_ioqueue_op_key_t*)read_op, bytes_read);
	    }

		if (has_lock) 
		{
		    pj_ioqueue_unlock_key(h);
		}

    }
    else 
    {
		pj_ioqueue_unlock_key(h);
		return 0;
    }

    return 1;
}

void ioqueue_connect_timeout(pj_ioqueue_t *ioqueue)
{
	pj_ioqueue_key_t *h = NULL;
	pj_time_val t1, *t2;
	pj_lock_acquire(ioqueue->lock);
	pj_gettimeofday(&t1);
	DL_ListForeach(h, &ioqueue->active_list, pj_ioqueue_key_t, list){
		if (key_has_pending_connect(h)){		
			if (PJ_TIME_VAL_GTE(t1, h->connect_time)){
				//connect超时
				char has_lock;
				if (pj_ioqueue_trylock_key(h)) continue;
				if (!h->connecting || IS_CLOSING(h)) {
					pj_ioqueue_unlock_key(h);
					continue;
				}
				h->connecting = 0;
				ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);
				ioqueue_remove_from_set(ioqueue, h, EXCEPTION_EVENT);
				if (h->allow_concurrent) {
					has_lock = 0;
					pj_ioqueue_unlock_key(h);
				} else {
					has_lock = 1;
				}
				if (h->cb.on_connect_complete && !IS_CLOSING(h)) {
					(*h->cb.on_connect_complete)(h, -1);
				}

				if (has_lock) {
					pj_ioqueue_unlock_key(h);
				}
				
			}	
		}
	}			

	pj_lock_release(ioqueue->lock);	
}

char ioqueue_dispatch_exception_event(pj_ioqueue_t *ioqueue, pj_ioqueue_key_t *h)
{
    char has_lock;
    int rc;
    rc = pj_ioqueue_trylock_key(h);
    if (rc != 0) {
		return 0;
    }

    if (!h->connecting) {
		pj_ioqueue_unlock_key(h);
		return 1;
    }

    if (IS_CLOSING(h)) {
		pj_ioqueue_unlock_key(h);
		return 1;
    }

    h->connecting = 0;

    ioqueue_remove_from_set(ioqueue, h, WRITEABLE_EVENT);
    ioqueue_remove_from_set(ioqueue, h, EXCEPTION_EVENT);

    if (h->allow_concurrent) {
		has_lock = 0;
		pj_ioqueue_unlock_key(h);
    } else {
		has_lock = 1;
    }

    if (h->cb.on_connect_complete && !IS_CLOSING(h)) {
		int status = -1;
		int value;
		int vallen = sizeof(value);
		int gs_rc = getsockopt(h->fd, SOL_SOCKET, SO_ERROR, (char *)&value, (socklen_t *)&vallen);
		if (gs_rc == 0) {
		    status = value;
		}
		
		(*h->cb.on_connect_complete)(h, status);
    }

    if (has_lock) {
		pj_ioqueue_unlock_key(h);
    }

    return 1;
}

int pj_ioqueue_recv(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key,void *buffer,ssize_t *length, int flags)
{
    struct read_operation *read_op;

    PJ_ASSERT_RETURN(key && op_key && buffer && length, PJ_EINVAL);
    PJ_CHECK_STACK();

    if (IS_CLOSING(key))
		return -4;

    read_op = (struct read_operation*)op_key;
    read_op->op = PJ_IOQUEUE_OP_NONE;

    if ((flags & PJ_IOQUEUE_ALWAYS_ASYNC) == 0) {
		int status;
		ssize_t size;

		size = *length;
		status = recv(key->fd, (char *)buffer, (int)size, flags);
		if (status == 0) {
		    *length = size;
		    return 0;
		} else {
		    if (errno != EAGAIN)
				return status;
		}
    }

    flags &= ~(PJ_IOQUEUE_ALWAYS_ASYNC);

    read_op->op = PJ_IOQUEUE_OP_RECV;
    read_op->buf = buffer;
    read_op->size = *length;
    read_op->flags = flags;

    pj_ioqueue_lock_key(key);

    if (IS_CLOSING(key)) {
		pj_ioqueue_unlock_key(key);
		return -4;//PJ_ECANCELLED;
    }
	DL_ListAddTail(&key->read_list, &read_op->list);	//pj_list_insert_before(&key->read_list, read_op);
    ioqueue_add_to_set(key->ioqueue, key, READABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return -5;//PJ_EPENDING;
}

PJ_DEF(int) pj_ioqueue_recvfrom(pj_ioqueue_key_t *key,
								pj_ioqueue_op_key_t *op_key,
								void *buffer,
								ssize_t *length,
								int flags,
								pj_sockaddr_t *addr,
								int *addrlen)
{
    struct read_operation *read_op;

    PJ_ASSERT_RETURN(key && op_key && buffer && length, PJ_EINVAL);
    PJ_CHECK_STACK();

    if (IS_CLOSING(key))
		return -4;

    read_op = (struct read_operation*)op_key;
    read_op->op = PJ_IOQUEUE_OP_NONE;

    if ((flags & PJ_IOQUEUE_ALWAYS_ASYNC) == 0) {
		int status;
		ssize_t size;

		size = *length;
		status = recvfrom(key->fd, (char *)buffer, (int)size, flags, (struct sockaddr*)addr, (socklen_t *)addrlen);
		if (status == 0) {
		    *length = size;
		    return 0;
		} else {
		    if (errno != EAGAIN)
			return status;
		}
    }

    flags &= ~(PJ_IOQUEUE_ALWAYS_ASYNC);

    read_op->op = PJ_IOQUEUE_OP_RECV_FROM;
    read_op->buf = buffer;
    read_op->size = *length;
    read_op->flags = flags;
    read_op->rmt_addr = addr;
    read_op->rmt_addrlen = addrlen;

    pj_ioqueue_lock_key(key);

    if (IS_CLOSING(key)) {
		pj_ioqueue_unlock_key(key);
		return -4;
    }
	
	DL_ListAddTail(&key->read_list, &read_op->list);	//pj_list_insert_before(&key->read_list, read_op);
    ioqueue_add_to_set(key->ioqueue, key, READABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return -5;	//PJ_EPENDING;
}

PJ_DEF(int) pj_ioqueue_send(pj_ioqueue_key_t *key,
							pj_ioqueue_op_key_t *op_key,
							const void *data,
							ssize_t *length,
							int flags)
{
    struct write_operation *write_op;
    int status;
    unsigned retry;
    ssize_t sent;

    PJ_ASSERT_RETURN(key && op_key && data && length, PJ_EINVAL);
    PJ_CHECK_STACK();

    if (IS_CLOSING(key))
		return -4;//PJ_ECANCELLED;

    flags &= ~(PJ_IOQUEUE_ALWAYS_ASYNC);

    if (DL_ListIsEmpty(&key->write_list)) {

        sent = *length;
        status = send(key->fd, data, sent, flags);
        if (status >= 0) {
			//发送成功
            *length = sent;
            return 0;
        } else {
            if (errno != EAGAIN) {
                return status;
            }
        }
    }
	
    write_op = (struct write_operation*)op_key;

    for (retry=0; write_op->op != 0 && retry<PENDING_RETRY; ++retry){
		//pj_thread_sleep(0);
	}

    if (write_op->op) {
		return -3;//PJ_EBUSY;
    }

    write_op->op = PJ_IOQUEUE_OP_SEND;
    write_op->buf = (char*)data;
    write_op->size = *length;
    write_op->written = 0;
    write_op->flags = flags;
    
    pj_ioqueue_lock_key(key);

    if (IS_CLOSING(key)) {
		pj_ioqueue_unlock_key(key);
		return -4;//PJ_ECANCELLED;
    }
    DL_ListAddTail(&key->write_list, &write_op->list);	//pj_list_insert_before(&key->write_list, write_op);
    ioqueue_add_to_set(key->ioqueue, key, WRITEABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return -5;//PJ_EPENDING;
}

PJ_DEF(int) pj_ioqueue_sendto(pj_ioqueue_key_t *key,
								pj_ioqueue_op_key_t *op_key,
								const void *data,
								ssize_t *length,
								int flags,
								const pj_sockaddr_t *addr,
								int addrlen)
{
    struct write_operation *write_op;
    unsigned retry;
    char restart_retry = 0;
    int status;
    ssize_t sent;

    PJ_ASSERT_RETURN(key && op_key && data && length, PJ_EINVAL);
    PJ_CHECK_STACK();

    if (IS_CLOSING(key))
		return -4;//PJ_ECANCELLED;

    flags &= ~(PJ_IOQUEUE_ALWAYS_ASYNC);

    if (DL_ListIsEmpty(&key->write_list)) {
        sent = *length;
        status = sendto(key->fd, data, sent, flags, addr, addrlen);
        if (status >= 0) {
            *length = sent;
            return 0;
        } else {
            if (errno != EAGAIN) {
                return status;
            }
        }
    }

    PJ_ASSERT_RETURN(addrlen <= (int)sizeof(pj_sockaddr_in), PJ_EBUG);

    write_op = (struct write_operation*)op_key;
    
    for (retry=0; write_op->op != 0 && retry<PENDING_RETRY; ++retry){
		//pj_thread_sleep(0);
	}
		

    if (write_op->op) {
		return -3;//PJ_EBUSY;
    }

    write_op->op = PJ_IOQUEUE_OP_SEND_TO;
    write_op->buf = (char*)data;
    write_op->size = *length;
    write_op->written = 0;
    write_op->flags = flags;
    memcpy(&write_op->rmt_addr, addr, addrlen);
    write_op->rmt_addrlen = addrlen;
    
    pj_ioqueue_lock_key(key);

    if (IS_CLOSING(key)) {
		pj_ioqueue_unlock_key(key);
		return -4;//PJ_ECANCELLED;
    }
    DL_ListAddTail(&key->write_list, &write_op->list);	//pj_list_insert_before(&key->write_list, write_op);
    ioqueue_add_to_set(key->ioqueue, key, WRITEABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return -5;//PJ_EPENDING;
}

//
PJ_DEF(int) pj_ioqueue_accept(pj_ioqueue_key_t *key,
								pj_ioqueue_op_key_t *op_key,
								int *new_sock,
								pj_sockaddr_t *local,
								pj_sockaddr_t *remote,
								int *addrlen)
{
    struct accept_operation *accept_op;
    int status;

    PJ_ASSERT_RETURN(key && op_key && new_sock, PJ_EINVAL);

    if (IS_CLOSING(key))
		return -4;

    accept_op = (struct accept_operation*)op_key;
    accept_op->op = PJ_IOQUEUE_OP_NONE;

    if (DL_ListIsEmpty(&key->accept_list)) {
		status = *new_sock = accept(key->fd, (struct sockaddr*)remote, (socklen_t*)addrlen);
		if (status >= 0){
			//accept成功
            if (local && addrlen) {
                status = getsockname(*new_sock, (struct sockaddr*)local, (socklen_t*)addrlen);
				//printf("%s, %d\r\n", inet_ntoa(((struct sockaddr_in *)local)->sin_addr), ntohs(((struct sockaddr_in *)local)->sin_port));
                if (status != 0) {
                    close(*new_sock);
                    *new_sock = PJ_INVALID_SOCKET;
                    return status;
                }
            }
            return 0;			
		}
		else{
			//accept失败
            if (errno != EAGAIN) {
                return status;
            }
		}
    }

    accept_op->op = PJ_IOQUEUE_OP_ACCEPT;
    accept_op->accept_fd = new_sock;
    accept_op->rmt_addr = remote;
    accept_op->addrlen= addrlen;
    accept_op->local_addr = local;

    pj_ioqueue_lock_key(key);

    if (IS_CLOSING(key)) {
		pj_ioqueue_unlock_key(key);
		return -4;//PJ_ECANCELLED;
    }
    DL_ListAddTail(&key->accept_list, &accept_op->list);	//pj_list_insert_before(&key->accept_list, accept_op);
    ioqueue_add_to_set(key->ioqueue, key, READABLE_EVENT);
    pj_ioqueue_unlock_key(key);

    return -5;//PJ_EPENDING;
}

PJ_DEF(int) pj_ioqueue_connect(pj_ioqueue_key_t *key, const pj_sockaddr_t *addr, int addrlen)
{
    int status;
    
    PJ_ASSERT_RETURN(key && addr && addrlen, PJ_EINVAL);

    if (IS_CLOSING(key))
		return -4;//PJ_ECANCELLED;

    if (key->connecting != 0)
        return -5;//PJ_EPENDING;
    
    status = connect(key->fd, (struct sockaddr*)addr, addrlen);
    if (status == 0) {
		//连接成功直接返回
		return 0;
    } 
	else {
		if (errno == EINPROGRESS) {
			//非阻塞
			pj_ioqueue_lock_key(key);
			if (IS_CLOSING(key)) {
				pj_ioqueue_unlock_key(key);
				return -4;//PJ_ECANCELLED;
			}
			key->connecting = 1;
			pj_gettimeofday(&key->connect_time);
			key->connect_time.sec += PJ_IOQUEUE_KEY_CONNECT_TIMEOUT;
			//printf("set connect timeout %ld.%ld\r\n", key->connect_time.sec, key->connect_time.msec);
			pj_time_val_normalize(&key->connect_time);
			
			ioqueue_add_to_set(key->ioqueue, key, WRITEABLE_EVENT);
			ioqueue_add_to_set(key->ioqueue, key, EXCEPTION_EVENT);
			pj_ioqueue_unlock_key(key);
			return -5;//PJ_EPENDING;
		} else {
			return status;
		}
    }
}


PJ_DEF(void) pj_ioqueue_op_key_init(pj_ioqueue_op_key_t *op_key, size_t size)
{
    //pj_bzero(op_key, size);
	memset((void *)op_key, 0, size);
}

PJ_DEF(char) pj_ioqueue_is_pending(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key)
{
    struct generic_operation *op_rec;
    op_rec = (struct generic_operation*)op_key;
    return op_rec->op != 0;
}

PJ_DEF(int) pj_ioqueue_post_completion(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, ssize_t bytes_status)
{
    struct generic_operation *op_rec;

    pj_ioqueue_lock_key(key);
    //op_rec = (struct generic_operation*)key->read_list.next;
    DL_ListForeach(op_rec, &key->read_list, struct generic_operation, list){
        if (op_rec == (void*)op_key) {
			DL_ListAddDelete(&op_rec->list);	//pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);
            (*key->cb.on_read_complete)(key, op_key, bytes_status);
            return 0;
        }		
	}
	
    DL_ListForeach(op_rec, &key->write_list, struct generic_operation, list){
        if (op_rec == (void*)op_key) {
			DL_ListAddDelete(&op_rec->list);	//pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);
            (*key->cb.on_write_complete)(key, op_key, bytes_status);
            return 0;
        }		
	}	

    DL_ListForeach(op_rec, &key->accept_list, struct generic_operation, list){
        if (op_rec == (void*)op_key) {
			DL_ListAddDelete(&op_rec->list);	//pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);
            (*key->cb.on_accept_complete)(key, op_key, PJ_INVALID_SOCKET, (int)bytes_status);
            return 0;
        }		
	}
	
#if 0	
	while (op_rec != (void*)&key->read_list) {
        if (op_rec == (void*)op_key) {
            pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);

            (*key->cb.on_read_complete)(key, op_key, bytes_status);
            return 0;
        }
        op_rec = op_rec->next;
    }

    op_rec = (struct generic_operation*)key->write_list.next;
    while (op_rec != (void*)&key->write_list) {
        if (op_rec == (void*)op_key) {
            pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);

            (*key->cb.on_write_complete)(key, op_key, bytes_status);
            return 0;
        }
        op_rec = op_rec->next;
    }

    op_rec = (struct generic_operation*)key->accept_list.next;
    while (op_rec != (void*)&key->accept_list) {
        if (op_rec == (void*)op_key) {
            pj_list_erase(op_rec);
            op_rec->op = PJ_IOQUEUE_OP_NONE;
            pj_ioqueue_unlock_key(key);

            (*key->cb.on_accept_complete)(key, op_key, PJ_INVALID_SOCKET, (int)bytes_status);
            return 0;
        }
        op_rec = op_rec->next;
    }
#endif
    pj_ioqueue_unlock_key(key);
    
    return -1;
}

PJ_DEF(int) pj_ioqueue_set_default_concurrency( pj_ioqueue_t *ioqueue, char allow)
{
    PJ_ASSERT_RETURN(ioqueue != NULL, PJ_EINVAL);
    ioqueue->default_concurrency = allow;
    return 0;
}

PJ_DEF(int) pj_ioqueue_set_concurrency(pj_ioqueue_key_t *key, char allow)
{
    PJ_ASSERT_RETURN(key, PJ_EINVAL);
    PJ_ASSERT_RETURN(allow || PJ_IOQUEUE_HAS_SAFE_UNREG, PJ_EINVAL);
    key->allow_concurrent = allow;
    return 0;
}

PJ_DEF(int) pj_ioqueue_lock_key(pj_ioqueue_key_t *key)
{
#if 0	
    if (key->grp_lock)
		return pj_grp_lock_acquire(key->grp_lock);
    else
#endif		
		return pj_lock_acquire(key->lock);
}

PJ_DEF(int) pj_ioqueue_trylock_key(pj_ioqueue_key_t *key)
{
#if 0		
    if (key->grp_lock)
		return pj_grp_lock_tryacquire(key->grp_lock);
    else
#endif			
		return pj_lock_tryacquire(key->lock);
}

PJ_DEF(int) pj_ioqueue_unlock_key(pj_ioqueue_key_t *key)
{
#if 0		
    if (key->grp_lock)
		return pj_grp_lock_release(key->grp_lock);
    else
#endif	
		return pj_lock_release(key->lock);
}


