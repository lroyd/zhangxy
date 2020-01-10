#ifndef GKI_COMMON_H
#define GKI_COMMON_H


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//1.注意，不够的话 不会动态分配
/*********************************************/
#define GKI_INVALID_TASK    0xF0
#define GKI_INVALID_POOL    0xFF



/*********************************************/

#define GKI_DEF_BUFPOOL_PERM_MASK   0xfde0

#define GKI_NUM_FIXED_BUF_POOLS     (6)
#define GKI_NUM_TOTAL_BUF_POOLS     (6)


#define ALIGN_POOL(pl_size) ((((pl_size) + 3)/sizeof(uint32_t))* sizeof(uint32_t))	//向下取舍

#define BUFFER_HDR_SIZE (sizeof(BUFFER_HDR_T))

#define BUFFER_PADDING_SIZE (sizeof(BUFFER_HDR_T) + sizeof(uint32_t))


#define MAX_USER_BUF_SIZE ((uint16_t)0xffff - BUFFER_PADDING_SIZE)	//真实可用大小

#define MAGIC_NO (0xDDBADDBA)

#define BUF_STATUS_FREE     0
#define BUF_STATUS_UNLINKED 1
#define BUF_STATUS_QUEUED   2


/**************************************************************************/
#define GKI_BUF0_SIZE		(200)
#define GKI_BUF1_SIZE		(200)
#define GKI_BUF2_SIZE		(200)
#define GKI_BUF3_SIZE		(200)
#define GKI_BUF4_SIZE		(200)
#define GKI_BUF5_SIZE		(200)

#define GKI_BUF0_MAX		(10)
#define GKI_BUF1_MAX		(10)
#define GKI_BUF2_MAX		(10)
#define GKI_BUF3_MAX		(10)
#define GKI_BUF4_MAX		(10)
#define GKI_BUF5_MAX		(10)




#define GKI_PUBLIC_POOL         0       /* General pool accessible to GKI_getbuf() */
#define GKI_RESTRICTED_POOL     1       /* Inaccessible pool to GKI_getbuf() */


/**************************************************************************/
typedef struct _buffer_hdr {
    struct _buffer_hdr *p_next;
    uint8_t q_id;
    uint8_t task_id;
    uint8_t status;
    uint8_t Type;
}BUFFER_HDR_T;

typedef struct _free_queue
{
    BUFFER_HDR_T *p_first;	//记录当前第一个可用，会移动
    BUFFER_HDR_T *p_last;

    uint16_t    size;		//不带头，可使用大小
    uint16_t    total;		//个数
    uint16_t    cur_cnt;
    uint16_t    max_cnt;

}FREE_QUEUE_T;





typedef struct
{
    uint8_t *bufpool0;
    uint8_t *bufpool1; 
    uint8_t *bufpool2;
    uint8_t *bufpool3;
    uint8_t *bufpool4;
    uint8_t *bufpool5;




    /* buffer pool management */
    FREE_QUEUE_T    freeq[GKI_NUM_TOTAL_BUF_POOLS];

    /* buffer pool start addresses */
    uint8_t         *pool_start[GKI_NUM_TOTAL_BUF_POOLS];
    uint8_t         *pool_end[GKI_NUM_TOTAL_BUF_POOLS];
    uint16_t        pool_size[GKI_NUM_TOTAL_BUF_POOLS];	//带头，最大65535：65k

    uint16_t        pool_buf_size[GKI_NUM_TOTAL_BUF_POOLS];
    uint16_t        pool_max_count[GKI_NUM_TOTAL_BUF_POOLS];
    uint16_t        pool_additions[GKI_NUM_TOTAL_BUF_POOLS];	
	
    /* buffer pool access control var */
    uint16_t		pool_access_mask;
    uint8_t			pool_list[GKI_NUM_TOTAL_BUF_POOLS];	//访问权限使用
	
	
    uint8_t         curr_total_no_of_pools;	//等价于GKI_NUM_TOTAL_BUF_POOLS



}tGKI_COM_CB;



extern char   gki_chk_buf_damage(void *);
extern void	gki_buffer_init (void);

extern void      gki_dealloc_free_queue(void);

extern void gki_print_buffer_statistics(int16_t);
extern void gki_print_used_bufs (uint8_t);

extern void *GKI_os_malloc (uint32_t size);
extern void GKI_os_free (void *p_mem);

extern void   *GKI_getbuf_raw (uint16_t);
extern void    GKI_freebuf_raw (void *);
extern void   *GKI_getpoolbuf_raw (uint8_t);

//需要走内核调度的
#define GKI_getbuf(sz)      GKI_getbuf_raw((sz))
#define GKI_freebuf(ptr)    GKI_freebuf_raw((ptr))
#define GKI_getpoolbuf(id)  GKI_getpoolbuf_raw((id))


extern void *GKI_find_buf_start (void *p_user_area);



#define GKI_disable()
#define GKI_enable()
#define GKI_get_taskid()	(1)


#ifdef __cplusplus
}
#endif


#endif


