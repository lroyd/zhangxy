#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/times.h>


#include "gki_buffer.h"






tGKI_COM_CB   gki_cb;



void *GKI_os_malloc (uint32_t size)
{
    return (malloc(size));
}


void GKI_os_free (void *p_mem)
{
    if(p_mem != NULL)
        free(p_mem);
    return;
}

static void gki_add_to_pool_list(uint8_t pool_id)
{
    int i, j;
    tGKI_COM_CB *p_cb = &gki_cb;

     /* Find the position where the specified pool should be inserted into the list */
    for(i=0; i < p_cb->curr_total_no_of_pools; i++)
    {
        if(p_cb->freeq[pool_id].size <= p_cb->freeq[p_cb->pool_list[i]].size)
            break;
    }

    /* Insert the new buffer pool ID into the list of pools */
    for(j = p_cb->curr_total_no_of_pools; j > i; j--)
    {
        p_cb->pool_list[j] = p_cb->pool_list[j-1];
    }

    p_cb->pool_list[i] = pool_id;

    return;
}


static void gki_remove_from_pool_list(uint8_t pool_id)
{
    tGKI_COM_CB *p_cb = &gki_cb;
    uint8_t i;

    for(i=0; i < p_cb->curr_total_no_of_pools; i++)
    {
        if(pool_id == p_cb->pool_list[i])
            break;
    }

    while (i < (p_cb->curr_total_no_of_pools - 1))
    {
        p_cb->pool_list[i] = p_cb->pool_list[i+1];
        i++;
    }

    return;
}

//将分配好的buf挂载到pool_start上
//参数(id, 大小, 个数, 地址)
static void gki_init_free_queue(uint8_t _ucId, uint16_t _usSize, uint16_t _usTotal, void *p_mem)
{
	uint16_t        i;
    uint16_t        act_size;
    BUFFER_HDR_T    *hdr;
    BUFFER_HDR_T    *hdr1 = NULL;
    uint32_t        *magic;
    uint32_t        tempsize = _usSize;
    tGKI_COM_CB     *p_cb = &gki_cb;

    tempsize = (uint32_t)ALIGN_POOL(_usSize);
    act_size = (uint16_t)(tempsize + BUFFER_PADDING_SIZE);	//计算后单个的大小（带头，向下取舍）

    if(p_mem)
    {
        p_cb->pool_start[_ucId] = (uint8_t *)p_mem;		//起始地址
        p_cb->pool_end[_ucId]   = (uint8_t *)p_mem + (act_size * _usTotal);	//结束地址
    }

    p_cb->pool_size[_ucId]  = act_size;

    p_cb->freeq[_ucId].size      = (uint16_t) tempsize;
    p_cb->freeq[_ucId].total     = _usTotal;
    p_cb->freeq[_ucId].cur_cnt   = 0;
    p_cb->freeq[_ucId].max_cnt   = 0;

    if(p_mem)
    {
        hdr = (BUFFER_HDR_T *)p_mem;
        p_cb->freeq[_ucId].p_first = hdr;
        for (i = 0; i < _usTotal; i++)
        {
            hdr->task_id = GKI_INVALID_TASK;
            hdr->q_id    = _ucId;
            hdr->status  = BUF_STATUS_FREE;
            magic        = (uint32_t *)((uint8_t *)hdr + BUFFER_HDR_SIZE + tempsize);
            *magic       = MAGIC_NO;
            hdr1         = hdr;
            hdr          = (BUFFER_HDR_T *)((uint8_t *)hdr + act_size);
            hdr1->p_next = hdr;
        }
        hdr1->p_next = NULL;
        p_cb->freeq[_ucId].p_last = hdr1;
    }

    return;
}
/*******************************************************************************
* Name: 
* Descriptions:
* Parameter:	
* Return:	
* *****************************************************************************/
static uint8_t gki_alloc_free_queue(uint8_t _ucId)
{
    FREE_QUEUE_T  *Q;
    tGKI_COM_CB *p_cb = &gki_cb;
    printf("gki_alloc_free_queue in, _ucId:%d \r\n",(int)_ucId );

    Q = &p_cb->freeq[p_cb->pool_list[_ucId]];

    if(Q->p_first == 0)
    {
        void* p_mem = GKI_os_malloc((Q->size + BUFFER_PADDING_SIZE) * Q->total);
        if(p_mem)
        {
            //re-initialize the queue with allocated memory
            printf("gki_alloc_free_queue calling  gki_init_free_queue, id:%d  size:%d, totol:%d\r\n", _ucId, Q->size, Q->total);
            gki_init_free_queue(_ucId, Q->size, Q->total, p_mem);
            printf("gki_alloc_free_queue ret OK, id:%d  size:%d, totol:%d\r\n", _ucId, Q->size, Q->total);
            return 1;
        }
        printf("gki_alloc_free_queue: Not enough memory\r\n");
    }
    printf("gki_alloc_free_queue out failed, id:%d\r\n", _ucId);
    return 0;
}





/* GKI_shutdown 时调用 */
void gki_dealloc_free_queue(void)
{
    uint8_t   i;
    tGKI_COM_CB *p_cb = &gki_cb;

    for (i=0; i < p_cb->curr_total_no_of_pools; i++)
    {
        if ( 0 < p_cb->freeq[i].max_cnt )
        {
            GKI_os_free(p_cb->pool_start[i]);

            p_cb->freeq[i].cur_cnt   = 0;
            p_cb->freeq[i].max_cnt   = 0;
            p_cb->freeq[i].p_first   = NULL;
            p_cb->freeq[i].p_last    = NULL;

            p_cb->pool_start[i] = NULL;
            p_cb->pool_end[i]   = NULL;
            p_cb->pool_size[i]  = 0;
        }
    }
}


//重要：初始化pool
//直接根据配置初始化所有pool，pool_list用不上
void gki_buffer_init(void)
{
    uint8_t i,tt,mb;
    tGKI_COM_CB *p_cb = &gki_cb;
    for(tt = 0; tt < GKI_NUM_TOTAL_BUF_POOLS; tt++)
    {        
        p_cb->freeq[tt].p_first  = 0;
        p_cb->freeq[tt].p_last   = 0;
        p_cb->freeq[tt].size     = 0;
        p_cb->freeq[tt].total    = 0;
        p_cb->freeq[tt].cur_cnt  = 0;
        p_cb->freeq[tt].max_cnt  = 0;

        p_cb->pool_start[tt]    = NULL;    
        p_cb->pool_end[tt]      = NULL;    
        p_cb->pool_size[tt]     = 0;		
    }

    p_cb->pool_access_mask = GKI_DEF_BUFPOOL_PERM_MASK;
	
    p_cb->bufpool0 = (uint8_t *)GKI_os_malloc((GKI_BUF0_SIZE + BUFFER_PADDING_SIZE)*GKI_BUF0_MAX);
    p_cb->bufpool1 = (uint8_t *)GKI_os_malloc((GKI_BUF1_SIZE + BUFFER_PADDING_SIZE)*GKI_BUF1_MAX);
    p_cb->bufpool2 = (uint8_t *)GKI_os_malloc((GKI_BUF2_SIZE + BUFFER_PADDING_SIZE)*GKI_BUF2_MAX);
    p_cb->bufpool3 = (uint8_t *)GKI_os_malloc((GKI_BUF3_SIZE + BUFFER_PADDING_SIZE)*GKI_BUF3_MAX);
    p_cb->bufpool4 = (uint8_t *)GKI_os_malloc ((GKI_BUF4_SIZE + BUFFER_PADDING_SIZE) * GKI_BUF4_MAX);
    p_cb->bufpool5 = (uint8_t *)GKI_os_malloc ((GKI_BUF5_SIZE + BUFFER_PADDING_SIZE) * GKI_BUF5_MAX);





    gki_init_free_queue(0, GKI_BUF0_SIZE, GKI_BUF0_MAX, p_cb->bufpool0);
    gki_init_free_queue(1, GKI_BUF1_SIZE, GKI_BUF1_MAX, p_cb->bufpool1);
    gki_init_free_queue(2, GKI_BUF2_SIZE, GKI_BUF2_MAX, p_cb->bufpool2);
    gki_init_free_queue(3, GKI_BUF3_SIZE, GKI_BUF3_MAX, p_cb->bufpool3);
    gki_init_free_queue(4, GKI_BUF4_SIZE, GKI_BUF4_MAX, p_cb->bufpool4);
    gki_init_free_queue(5, GKI_BUF5_SIZE, GKI_BUF5_MAX, p_cb->bufpool5);




    /* add pools to pool_list which is arranged */
    for(i=0; i < GKI_NUM_FIXED_BUF_POOLS; i++)
    {
        p_cb->pool_list[i] = i;
    }

    p_cb->curr_total_no_of_pools = GKI_NUM_FIXED_BUF_POOLS;
}


uint16_t GKI_get_buf_size(void *p_buf)
{
    BUFFER_HDR_T    *p_hdr;

    p_hdr = (BUFFER_HDR_T *)((uint8_t *) p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->q_id < GKI_NUM_TOTAL_BUF_POOLS)
    {
        return (gki_cb.freeq[p_hdr->q_id].size);
    }

    return (0);
}


char gki_chk_buf_damage(void *p_buf)
{
    uint32_t *magic;
    magic  = (uint32_t *)((uint8_t *) p_buf + GKI_get_buf_size(p_buf));

    if ((uint32_t)magic & 1)  //magic 是 地址?
        return (1);

    if (*magic == MAGIC_NO)
        return (0);

    return (1);
}



//根据地址来返回当前块的头地址
void *GKI_find_buf_start(void *p_user_area)
{
    uint16_t       xx, size;
    uint32_t       yy;
    tGKI_COM_CB *p_cb = &gki_cb;
    uint8_t       *p_ua = (uint8_t *)p_user_area;

    for (xx = 0; xx < GKI_NUM_TOTAL_BUF_POOLS; xx++)
    {
        if ((p_ua > p_cb->pool_start[xx]) && (p_ua < p_cb->pool_end[xx]))
        {
            yy = (uint32_t)(p_ua - p_cb->pool_start[xx]);

            size = p_cb->pool_size[xx];

            yy = (yy / size) * size;

            return ((void *) (p_cb->pool_start[xx] + yy + sizeof(BUFFER_HDR_T)) );
        }
    }

    printf("%s bad addr\r\n");

    return (NULL);
}

uint8_t GKI_set_pool_permission(uint8_t pool_id, uint8_t permission)
{
    tGKI_COM_CB *p_cb = &gki_cb;

    if (pool_id < GKI_NUM_TOTAL_BUF_POOLS)
    {
        if (permission == GKI_RESTRICTED_POOL)
            p_cb->pool_access_mask = (uint16_t)(p_cb->pool_access_mask | (1 << pool_id));

        else    /* mark the pool as public */
            p_cb->pool_access_mask = (uint16_t)(p_cb->pool_access_mask & ~(1 << pool_id));

        return (0);
    }
    else
        return (0xFF);
}


void *GKI_getbuf_raw(uint16_t size)
{
    uint8_t         i;
    FREE_QUEUE_T  *Q;
    BUFFER_HDR_T  *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb;

    if (size == 0) return (NULL);

	
	GKI_disable();

    /* 找到第一个符合大小的缓冲池 */
    for (i = 0; i < p_cb->curr_total_no_of_pools; i++){
        if (size <= p_cb->freeq[p_cb->pool_list[i]].size )
        {
            /* 根据掩码配置使用权限 */
            if (((uint16_t)1 << p_cb->pool_list[i]) & p_cb->pool_access_mask)
                continue;

			Q = &p_cb->freeq[p_cb->pool_list[i]];	/* 第i个缓冲池的空闲链表地址 */
			if(Q->cur_cnt < Q->total)	/* 空闲还有剩余 */
			{
				if(Q->p_first == 0 && gki_alloc_free_queue(i) != 1)
					return NULL;
				
				p_hdr = Q->p_first;
				Q->p_first = p_hdr->p_next;

				if (!Q->p_first) Q->p_last = NULL;

				if(++Q->cur_cnt > Q->max_cnt)
					Q->max_cnt = Q->cur_cnt;

				GKI_enable();

				p_hdr->task_id = GKI_get_taskid();		//开启task调度才有此功能

				p_hdr->status  = BUF_STATUS_UNLINKED;
				p_hdr->p_next  = NULL;
				p_hdr->Type    = 0;

				return ((void *) ((uint8_t *)p_hdr + BUFFER_HDR_SIZE));	//偏移过字头
			}
			else
			{
				printf("%s ERR:No free position",__func__);
			}
        }
    }

    GKI_enable();

	printf("%s getbuf: out of buffers",__func__);

    return (NULL);
}

//根据id获取pool中的第一个可用
void *GKI_getpoolbuf_raw(uint8_t pool_id)
{
    FREE_QUEUE_T  *Q;
    BUFFER_HDR_T  *p_hdr;
    tGKI_COM_CB *p_cb = &gki_cb;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
    {
        //GKI_exception(GKI_ERROR_GETPOOLBUF_BAD_QID, "getpoolbuf bad pool");
        return (NULL);
    }

    /* Make sure the buffers aren't disturbed til finished with allocation */
    GKI_disable();

    Q = &p_cb->freeq[pool_id];
    if(Q->cur_cnt < Q->total)
    {
        if(Q->p_first == 0 && gki_alloc_free_queue(pool_id) != 1)
            return NULL;

        p_hdr = Q->p_first;
        Q->p_first = p_hdr->p_next;

        if (!Q->p_first)
            Q->p_last = NULL;

        if(++Q->cur_cnt > Q->max_cnt)
            Q->max_cnt = Q->cur_cnt;

        GKI_enable();


        p_hdr->task_id = GKI_get_taskid();

        p_hdr->status  = BUF_STATUS_UNLINKED;		//send_mbox会变成BUF_STATUS_QUEUED read_mbox会变回BUF_STATUS_UNLINKED
        p_hdr->p_next  = NULL;
        p_hdr->Type    = 0;

        return ((void *) ((uint8_t *)p_hdr + BUFFER_HDR_SIZE));
    }

    GKI_enable();
	//指定pool中没有的话，会根据当前大小从其他pool中获取
    return (GKI_getbuf_raw(p_cb->freeq[pool_id].size));	

}

void GKI_freebuf_raw(void *p_buf)
{
    FREE_QUEUE_T    *Q;
    BUFFER_HDR_T    *p_hdr;

    if (!p_buf || gki_chk_buf_damage(p_buf))
    {
		printf("%s Free - Buf Corrupted\r\n");
        return;
    }

    p_hdr = (BUFFER_HDR_T *) ((uint8_t *)p_buf - BUFFER_HDR_SIZE);

    if (p_hdr->status != BUF_STATUS_UNLINKED)
    {
		printf("%s Freeing Linked Buf\r\n");
        return;
    }

    if (p_hdr->q_id >= GKI_NUM_TOTAL_BUF_POOLS)
    {
		printf("%s Bad Buf QId\r\n");
        return;
    }

    GKI_disable();

    /*
    ** 释放的buf会重新追加到尾部
    */
    Q  = &gki_cb.freeq[p_hdr->q_id];
    if (Q->p_last)
        Q->p_last->p_next = p_hdr;
    else
        Q->p_first = p_hdr;

    Q->p_last      = p_hdr;
    p_hdr->p_next  = NULL;
    p_hdr->status  = BUF_STATUS_FREE;
	
    p_hdr->task_id = GKI_INVALID_TASK;
	
    if (Q->cur_cnt > 0)
        Q->cur_cnt--;

    GKI_enable();

    return;
}

uint16_t GKI_poolcount(uint8_t pool_id)
{
    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
        return (0);

    return (gki_cb.freeq[pool_id].total);
}

uint16_t GKI_poolfreecount(uint8_t pool_id)
{
    FREE_QUEUE_T  *Q;

    if (pool_id >= GKI_NUM_TOTAL_BUF_POOLS)
        return (0);

    Q  = &gki_cb.freeq[pool_id];

    return ((uint16_t)(Q->total - Q->cur_cnt));
}

void GKI_PrintBufferUsage(uint8_t *p_num_pools, uint16_t *p_cur_used)
{
    int i;
    FREE_QUEUE_T    *p;
    uint8_t   num = gki_cb.curr_total_no_of_pools;
    uint16_t   cur[GKI_NUM_TOTAL_BUF_POOLS];

    printf("\r\n");
    printf("--- GKI Buffer Pool Summary (R - restricted, P - public) ---\r\n");

    printf("POOL     SIZE  USED  MAXU  TOTAL\r\n");
    printf("------------------------------\r\n");
    for (i = 0; i < gki_cb.curr_total_no_of_pools; i++)
    {
        p = &gki_cb.freeq[i];
        if ((1 << i) & gki_cb.pool_access_mask)
        {
            printf("%02d: (R), %4d, %3d, %3d, %3d\r\n",
                        i, p->size, p->cur_cnt, p->max_cnt, p->total);
        }
        else
        {
            printf("%02d: (P), %4d, %3d, %3d, %3d\r\n",
                        i, p->size, p->cur_cnt, p->max_cnt, p->total);
        }
        cur[i] = p->cur_cnt;
    }
    if (p_num_pools)
        *p_num_pools = num;
    if (p_cur_used)
        memcpy(p_cur_used, cur, num*2);
}


void GKI_PrintBuffer(void)
{
    uint16_t i;
    for(i=0; i<GKI_NUM_TOTAL_BUF_POOLS; i++)
	{
		printf("pool:%4u free %4u cur %3u max %3u  total%3u\r\n", i, gki_cb.freeq[i].size,
                    gki_cb.freeq[i].cur_cnt, gki_cb.freeq[i].max_cnt, gki_cb.freeq[i].total);
    }
}


void gki_print_buffer_statistics(int16_t pool)
{
    uint16_t           i;
    BUFFER_HDR_T    *hdr;
    uint16_t           size,act_size,maxbuffs;
    uint32_t           *magic;

    if (pool > GKI_NUM_TOTAL_BUF_POOLS || pool < 0)
    {
        printf("Not a valid Buffer pool\n");
        return;
    }

    size = gki_cb.freeq[pool].size;
    maxbuffs = gki_cb.freeq[pool].total;
    act_size = size + BUFFER_PADDING_SIZE;
    printf("Buffer Pool[%u] size=%u cur_cnt=%u max_cnt=%u  total=%u\n",
        pool, gki_cb.freeq[pool].size,
        gki_cb.freeq[pool].cur_cnt, gki_cb.freeq[pool].max_cnt, gki_cb.freeq[pool].total);

    printf("      Owner  State    Sanity\n");
    printf("----------------------------\n");
    hdr = (BUFFER_HDR_T *)(gki_cb.pool_start[pool]);
    for(i=0; i<maxbuffs; i++)
    {
        magic = (uint32_t *)((uint8_t *)hdr + BUFFER_HDR_SIZE + size);
        printf("%3d: 0x%02x %4d %10s\n", i, hdr->task_id, hdr->status, (*magic == MAGIC_NO)?"OK":"CORRUPTED");
        hdr          = (BUFFER_HDR_T *)((uint8_t *)hdr + act_size);
    }
    return;
}


//单独创建pool大小
int GKI_create_pool(uint16_t size, uint16_t count, uint8_t permission, char *out_id)
{
    uint8_t        xx;
    uint32_t       mem_needed;
    int32_t        tempsize = size;
    tGKI_COM_CB *p_cb = &gki_cb;

    if (size > MAX_USER_BUF_SIZE)
        return (GKI_INVALID_POOL);

    /* 查找第一个可用的位置 */
    for (xx = 0; xx < GKI_NUM_TOTAL_BUF_POOLS; xx++)
    {
        if (!p_cb->pool_start[xx])
            break;
    }

    if (xx == GKI_NUM_TOTAL_BUF_POOLS) return (GKI_INVALID_POOL);

    tempsize = (int32_t)ALIGN_POOL(size);

    mem_needed = (tempsize + BUFFER_PADDING_SIZE) * count;	//计算总共需要的字节数
	
	void *p_mem_pool = GKI_os_malloc(mem_needed);
    if (p_mem_pool)
    {
        gki_init_free_queue(xx, size, count, p_mem_pool);
        gki_add_to_pool_list(xx);
        (void) GKI_set_pool_permission(xx, permission);
        p_cb->curr_total_no_of_pools++;
		*out_id = xx;
        return (0);
    }
    else
        return (-1);
}



void GKI_delete_pool(uint8_t pool_id)
{
    FREE_QUEUE_T    *Q;
    tGKI_COM_CB     *p_cb = &gki_cb;

    if ((pool_id >= GKI_NUM_TOTAL_BUF_POOLS) || (!p_cb->pool_start[pool_id]))
        return;

    GKI_disable();
    Q  = &p_cb->freeq[pool_id];

    if (!Q->cur_cnt)
    {
        Q->size      = 0;
        Q->total     = 0;
        Q->cur_cnt   = 0;
        Q->max_cnt   = 0;
        Q->p_first   = NULL;
        Q->p_last    = NULL;

        GKI_os_free(p_cb->pool_start[pool_id]);

        p_cb->pool_start[pool_id] = NULL;
        p_cb->pool_end[pool_id]   = NULL;
        p_cb->pool_size[pool_id]  = 0;

        gki_remove_from_pool_list(pool_id);
        p_cb->curr_total_no_of_pools--;
    }
    else
        printf("Deleting bad pool\r\n");

    GKI_enable();

    return;
}


