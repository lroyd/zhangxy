#include "ioqueue.h"

#define IS_CLOSING(key)  (key->closing)

struct generic_operation
{
    T_DoubleList list;
    pj_ioqueue_operation_e  op;
};

struct read_operation
{
    T_DoubleList list;
    pj_ioqueue_operation_e  op;

    void		   *buf;
    size_t		size;
    unsigned		flags;
    pj_sockaddr_t	*rmt_addr;
    int			   *rmt_addrlen;
};

struct write_operation
{
    T_DoubleList list;
    pj_ioqueue_operation_e  op;

    char		   *buf;
    size_t		    size;
    ssize_t              written;
    unsigned                flags;
    pj_sockaddr_t	*rmt_addr;	//pj_sockaddr_in	    rmt_addr;
    int			    rmt_addrlen;
};

struct accept_operation
{
    T_DoubleList list;
    pj_ioqueue_operation_e  op;

    int              *accept_fd;
    pj_sockaddr_t	   *local_addr;
    pj_sockaddr_t	   *rmt_addr;
    int			   *addrlen;
};

union operation_key
{
    struct generic_operation generic_op;
    struct read_operation    read;
    struct write_operation   write;

    struct accept_operation  accept;
};


enum ioqueue_event_type
{
    NO_EVENT,
    READABLE_EVENT,
    WRITEABLE_EVENT,
    EXCEPTION_EVENT,
};

void ioqueue_init(pj_ioqueue_t *ioqueue);
int ioqueue_destroy(pj_ioqueue_t *ioqueue);

int ioqueue_init_key(T_PoolInfo *pool,
					 pj_ioqueue_t *ioqueue,
					 pj_ioqueue_key_t *key,
					 int sock,
					 void *grp_lock,
					 void *user_data,
					 const pj_ioqueue_callback *cb);

					 
void ioqueue_add_to_set(pj_ioqueue_t *ioqueue, pj_ioqueue_key_t *key, enum ioqueue_event_type event_type);
void ioqueue_remove_from_set(pj_ioqueue_t *ioqueue, pj_ioqueue_key_t *key, enum ioqueue_event_type event_type);

