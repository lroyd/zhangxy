#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/times.h>

#include "aki_buffer.h"


#ifdef AKI_USE_OSLIB
	#include "console.h"
#else
	#define LOG_DEBUG		(2)

	#define ZXY_DLOG(level, format, arg...)	printf(format"\n", ##arg)
	#define ZXY_ELOG(format, arg...)		printf(format"\n", ##arg)
#endif

#ifdef AKI_USE_TASK
	#include "aki_task.h"
#else
	#define AKI_TASK_INVALID    		(0x0F)	//δ������
	#define AKI_TASK_GET_ID()			(0)		//�Ѿ�������
#endif


#ifdef AKI_USE_LOCK
	#define AKI_BUFFER_LOCK(c)		pthread_mutex_lock(&c->m_tLock)
	#define AKI_BUFFER_UNLOCK(c)	pthread_mutex_unlock(&c->m_tLock)
#else
	#define AKI_BUFFER_LOCK(c)
	#define AKI_BUFFER_UNLOCK(c)
#endif

#define MAGIC_NO (0xDDBADDBA)


static void add_to_pool_list(T_AkiBuffer *_pAki, char _u8Pid)
{
	T_AkiBuffer *p_cb = _pAki;
    int i, j;
    for(i = 0; i < p_cb->m_u8CurrTotalPools; i++){
        if(p_cb->in_atFreeq[_u8Pid].m_u16PackSize <= p_cb->in_atFreeq[p_cb->m_aucPoolList[i]].m_u16PackSize)
            break;
    }

    for(j = p_cb->m_u8CurrTotalPools; j > i; j--){
        p_cb->m_aucPoolList[j] = p_cb->m_aucPoolList[j-1];
    }

    p_cb->m_aucPoolList[i] = _u8Pid;

    return;
}

static void remove_from_pool_list(T_AkiBuffer *_pAki, char _u8Pid)
{
    T_AkiBuffer *p_cb = _pAki;
    int i;
    for(i=0; i < p_cb->m_u8CurrTotalPools; i++){
        if(_u8Pid == p_cb->m_aucPoolList[i])
            break;
    }

    while (i < (p_cb->m_u8CurrTotalPools - 1)){
        p_cb->m_aucPoolList[i] = p_cb->m_aucPoolList[i+1];
        i++;
    }
}

static void init_free_queue(T_AkiBuffer *_pAki, char _ucId, unsigned short _usSize, unsigned short _usTotal, void *_pAddress)
{
	T_AkiBuffer     *p_cb = _pAki;
    unsigned short        act_size;
    T_AkiPackHdr    *hdr, *hdr1 = NULL;
    int i, *magic, tempsize = _usSize;    
    

    tempsize = (int)AKI_ALIGN_SIZE(_usSize);
    act_size = (unsigned short)(tempsize + AKI_BUFFER_PADDING_SIZE);	//����󵥸��Ĵ�С����ͷ������ȡ�ᣩ

    if(_pAddress)
    {
        p_cb->m_apPoolStart[_ucId] = (char *)_pAddress;		//��ʼ��ַ
        p_cb->m_apPoolEnd[_ucId]   = (char *)_pAddress + (act_size * _usTotal);	//������ַ
    }

    p_cb->m_ausPoolSize[_ucId]  = act_size;

    p_cb->in_atFreeq[_ucId].m_u16PackSize      = (unsigned short) tempsize;
    p_cb->in_atFreeq[_ucId].m_u16PackTotal     = _usTotal;
    p_cb->in_atFreeq[_ucId].m_u16PackCurCnt   = 0;
    p_cb->in_atFreeq[_ucId].m_u16PackMaxCnt   = 0;

    if(_pAddress)
    {
        hdr = (T_AkiPackHdr *)_pAddress;
        p_cb->in_atFreeq[_ucId].in_pFirst = hdr;
        for (i = 0; i < _usTotal; i++)
        {
            hdr->m_u8TaskId = AKI_TASK_INVALID;
            hdr->m_u8PoolId    = _ucId;
            hdr->m_u8Status  = AKI_BUF_STATUS_FREE;
            magic        = (uint32_t *)((char *)hdr + AKI_BUFFER_HDR_SIZE + tempsize);
            *magic       = MAGIC_NO;
            hdr1         = hdr;
            hdr          = (T_AkiPackHdr *)((char *)hdr + act_size);
            hdr1->m_pNext = hdr;
        }
        hdr1->m_pNext = NULL;
        p_cb->in_atFreeq[_ucId].in_pLast = hdr1;
    }

    return;
}

//���ݵ�ǰ_pPack��ȡ_pPack����pool�е�size
static unsigned short get_buf_size(T_AkiBuffer *_pAki, void *_pPack)
{
	T_AkiBuffer	*p_cb = (T_AkiBuffer *)_pAki;
    T_AkiPackHdr *p_hdr;
    p_hdr = (T_AkiPackHdr *)((char *)_pPack - AKI_BUFFER_HDR_SIZE);
    if (p_hdr->m_u8PoolId < AKI_BUFFER_POOLS_TOTAL_MAX){
        return (p_cb->in_atFreeq[p_hdr->m_u8PoolId].m_u16PackSize);
    }

    return (0);
}

static char chk_buf_damage(T_AkiBuffer *_pAki, void *_pPack)
{
    int *magic;
    magic  = (int *)((char *) _pPack + get_buf_size(_pAki, _pPack));

    if ((int)magic & 1)
        return (1);

    if (*magic == MAGIC_NO)
        return (0);

    return (1);
}



void AKI_BufferInit(T_AkiBuffer *_pAki)
{
	T_AkiBuffer *p_cb = _pAki;
    int i;
    for(i = 0; i < AKI_BUFFER_POOLS_TOTAL_MAX; i++){        
        p_cb->in_atFreeq[i].in_pFirst  = 0;
        p_cb->in_atFreeq[i].in_pLast   = 0;
        p_cb->in_atFreeq[i].m_u16PackSize     = 0;
        p_cb->in_atFreeq[i].m_u16PackTotal    = 0;
        p_cb->in_atFreeq[i].m_u16PackCurCnt  = 0;
        p_cb->in_atFreeq[i].m_u16PackMaxCnt  = 0;

        p_cb->m_apPoolStart[i]    = NULL;    
        p_cb->m_apPoolEnd[i]      = NULL;    
        p_cb->m_ausPoolSize[i]     = 0;	
		
		p_cb->m_aucPoolList[i]		= 0;
    }	
	pthread_mutex_init(&(p_cb->m_tLock), NULL);
}


int AKI_PoolSetPermission(T_AkiBuffer *_pAkiBuffer, char _u8Pid, char _u8Perm)
{
	int u32Ret = -1;
    T_AkiBuffer *p_cb = _pAkiBuffer;
    if (_u8Pid < AKI_BUFFER_POOLS_TOTAL_MAX){
        if (_u8Perm == AKI_POOL_RESTRICTED)
            p_cb->m_u16PoolAccessMask = (unsigned short)(p_cb->m_u16PoolAccessMask | (1 << _u8Pid));
        else
            p_cb->m_u16PoolAccessMask = (unsigned short)(p_cb->m_u16PoolAccessMask & ~(1 << _u8Pid));
        u32Ret = 0;
    }

	return u32Ret;
}


//��������pool��С
int AKI_BufferCreatePool(T_AkiBuffer *_pAki, void *_pUAddress, unsigned short _u16USize, unsigned short _u16Size,  char *_pOut)
{
	T_AkiBuffer *p_cb = _pAki;
    char u8Id;
	unsigned short u16Size = 0, u16Total;
    if (_u16Size > AKI_BUFFER_SIZE_MAX) return (-1);
	
	u16Size = (_u16Size + AKI_BUFFER_PADDING_SIZE);	//���ö���
	u16Total = _u16USize/u16Size;
	
    /* ���ҵ�һ�����õ�λ�� */
    for (u8Id = 0; u8Id < AKI_BUFFER_POOLS_TOTAL_MAX; u8Id++){
        if (!p_cb->m_apPoolStart[u8Id])
            break;
    }

    if (u8Id == AKI_BUFFER_POOLS_TOTAL_MAX) return (-1);
	
	ZXY_DLOG(LOG_DEBUG, "aki buffer create pool[%d], (%p:%d) -> (%d * %d)", u8Id, _pUAddress, _u16USize, _u16Size, u16Total);
	
	init_free_queue(p_cb, u8Id, _u16Size, u16Total, _pUAddress);
	add_to_pool_list(p_cb, u8Id);
	AKI_PoolSetPermission(p_cb, u8Id, AKI_POOL_PUBLIC);	//Ĭ��Ȩ�ޣ�����Ҫ�������޸�
	p_cb->m_u8CurrTotalPools++;
	if (_pOut) *_pOut = u8Id;
	
	return 0;
}


//���ص�ǰid������ָ�룬�Թ�����ͷ�
void *AKI_BufferDestroyPool(T_AkiBuffer *_pAki, char _u8Pid)
{
    T_AkiBuffer	*p_cb = _pAki;
	T_AkiFreeq *Q;
	void *p = NULL;
    if ((_u8Pid >= AKI_BUFFER_POOLS_TOTAL_MAX) || (!p_cb->m_apPoolStart[_u8Pid]))
        return p;

    AKI_BUFFER_LOCK(p_cb);
    Q  = &p_cb->in_atFreeq[_u8Pid];
    if (!Q->m_u16PackCurCnt){
        Q->m_u16PackSize      = 0;
        Q->m_u16PackTotal     = 0;
        Q->m_u16PackCurCnt   = 0;
        Q->m_u16PackMaxCnt   = 0;
        Q->in_pFirst   = NULL;
        Q->in_pLast    = NULL;
		
		p = p_cb->m_apPoolStart[_u8Pid];
        p_cb->m_apPoolStart[_u8Pid] = NULL;
        p_cb->m_apPoolEnd[_u8Pid]   = NULL;
        p_cb->m_ausPoolSize[_u8Pid]  = 0;

        remove_from_pool_list(p_cb, _u8Pid);
        p_cb->m_u8CurrTotalPools--;
    }else{
		//��������ʹ�ã��������ͷ�
		ZXY_ELOG("delete pool failed, some pack not free!!");
	}

    AKI_BUFFER_UNLOCK(p_cb);

    return p;
}

//��ȡÿ��pool���ܹ�����
unsigned short AKI_PoolGetTotal(T_AkiBuffer *_pAki, char _u8Pid)
{
	T_AkiBuffer	*p_cb = _pAki;
    if (_u8Pid >= AKI_BUFFER_POOLS_TOTAL_MAX) return (0);
    return (p_cb->in_atFreeq[_u8Pid].m_u16PackTotal);
}

//��ȡÿ��pool�п��а���
unsigned short AKI_PoolGetFreeTotal(T_AkiBuffer *_pAki, char _u8Pid)
{
	T_AkiBuffer	*p_cb = _pAki;
    T_AkiFreeq  *Q;
    if (_u8Pid >= AKI_BUFFER_POOLS_TOTAL_MAX) return 0;
    Q  = &p_cb->in_atFreeq[_u8Pid];
    return ((unsigned short)(Q->m_u16PackTotal - Q->m_u16PackCurCnt));
}


void *AKI_GetPackage(T_AkiBuffer *_pAki, unsigned short _u16Size)
{
	T_AkiBuffer	*p_cb = _pAki;
    int i;
    T_AkiFreeq  *Q;
    T_AkiPackHdr  *p_hdr;

    if (_u16Size == 0) return (NULL);

	AKI_BUFFER_LOCK(p_cb);

    /* �ҵ���һ�����ϴ�С�Ļ���� */
    for (i = 0; i < p_cb->m_u8CurrTotalPools; i++){
        if (_u16Size <= p_cb->in_atFreeq[p_cb->m_aucPoolList[i]].m_u16PackSize){
            /* ������������ʹ��Ȩ�� */
            if (((unsigned short)1 << p_cb->m_aucPoolList[i]) & p_cb->m_u16PoolAccessMask){
				continue;
			}

			Q = &p_cb->in_atFreeq[p_cb->m_aucPoolList[i]];	/* ��i������صĿ��������ַ */
			if(Q->m_u16PackCurCnt < Q->m_u16PackTotal)	/* ���л���ʣ�� */
			{
#if 0				
				if(Q->in_pFirst == 0 && alloc_free_queue(i) != 1)
					return NULL;
#endif				
				p_hdr = Q->in_pFirst;
				Q->in_pFirst = p_hdr->m_pNext;

				if (!Q->in_pFirst) Q->in_pLast = NULL;

				if(++Q->m_u16PackCurCnt > Q->m_u16PackMaxCnt)
					Q->m_u16PackMaxCnt = Q->m_u16PackCurCnt;

				AKI_BUFFER_UNLOCK(p_cb);

				p_hdr->m_u8TaskId = AKI_TASK_GET_ID();		//����task���Ȳ��д˹���

				p_hdr->m_u8Status  = AKI_BUF_STATUS_UNLINKED;
				p_hdr->m_pNext  = NULL;
				p_hdr->m_u8Type    = 0;

				return ((void *) ((char *)p_hdr + AKI_BUFFER_HDR_SIZE));	//ƫ�ƹ���ͷ
			}else{
				ZXY_ELOG("error:no free position");
			}
        }
    }

    AKI_BUFFER_UNLOCK(p_cb);

	ZXY_ELOG("out of buffers");

    return (NULL);
}


void AKI_PutPackage(T_AkiBuffer *_pAki, void *_pPack)
{
	T_AkiBuffer	*p_cb = _pAki;	
    T_AkiFreeq    *Q;
    T_AkiPackHdr    *p_hdr;

    if (!_pPack || chk_buf_damage(_pAki, _pPack)){
		ZXY_ELOG("free - buf corrupted");
        return;
    }

    p_hdr = (T_AkiPackHdr *) ((char *)_pPack - AKI_BUFFER_HDR_SIZE);
    if (p_hdr->m_u8Status != AKI_BUF_STATUS_UNLINKED){
		ZXY_ELOG("freeing linked buf");
        return;
    }

    if (p_hdr->m_u8PoolId >= AKI_BUFFER_POOLS_TOTAL_MAX){
		ZXY_ELOG("bad buf qid");
        return;
    }

    AKI_BUFFER_LOCK(p_cb);

	//�ͷŵ�buf������׷�ӵ�β��
    Q  = &p_cb->in_atFreeq[p_hdr->m_u8PoolId];
    if (Q->in_pLast)
        Q->in_pLast->m_pNext = p_hdr;
    else
        Q->in_pFirst = p_hdr;

    Q->in_pLast      = p_hdr;
    p_hdr->m_pNext  = NULL;
    p_hdr->m_u8Status  = AKI_BUF_STATUS_FREE;
	
    p_hdr->m_u8TaskId = AKI_TASK_INVALID;
	
    if (Q->m_u16PackCurCnt > 0) Q->m_u16PackCurCnt--;

    AKI_BUFFER_UNLOCK(p_cb);

    return;
}

//���ݵ�ַ�����ص�ǰ���ͷ��ַ
void *AKI_CheckStartAddr(T_AkiBuffer *_pAki, void *_pUsrAddr)
{
	T_AkiBuffer *p_cb = _pAki;
    unsigned short i, u16Size;
    int       yy;
    
    char *p_ua = (char *)_pUsrAddr;

    for (i = 0; i < AKI_BUFFER_POOLS_TOTAL_MAX; i++){
        if ((p_ua > p_cb->m_apPoolStart[i]) && (p_ua < p_cb->m_apPoolEnd[i])){
            yy = (int)(p_ua - p_cb->m_apPoolStart[i]);

            u16Size = p_cb->m_ausPoolSize[i];

            yy = (yy / u16Size) * u16Size;

            return ((void *) (p_cb->m_apPoolStart[i] + yy + sizeof(T_AkiPackHdr)) );
        }
    }

    ZXY_ELOG("bad addr");

    return (NULL);
}


//����id��ȡpool�еĵ�һ������
void *AKI_PoolGetPackage(T_AkiBuffer *_pAki, char _u8Pid)
{
	T_AkiBuffer *p_cb = _pAki;
    T_AkiFreeq  *Q;
    T_AkiPackHdr  *p_hdr;
    
    if (_u8Pid >= AKI_BUFFER_POOLS_TOTAL_MAX) return (NULL);

    AKI_BUFFER_LOCK(p_cb);

    Q = &p_cb->in_atFreeq[_u8Pid];
    if(Q->m_u16PackCurCnt < Q->m_u16PackTotal){
#if 0		
        if(Q->in_pFirst == 0 && alloc_free_queue(_u8Pid) != 1)
            return NULL;
#endif
        p_hdr = Q->in_pFirst;
        Q->in_pFirst = p_hdr->m_pNext;

        if (!Q->in_pFirst)
            Q->in_pLast = NULL;

        if(++Q->m_u16PackCurCnt > Q->m_u16PackMaxCnt)
            Q->m_u16PackMaxCnt = Q->m_u16PackCurCnt;

        AKI_BUFFER_UNLOCK(p_cb);


        p_hdr->m_u8TaskId = AKI_TASK_GET_ID();

        p_hdr->m_u8Status  = AKI_BUF_STATUS_UNLINKED;		//send_mbox����BUF_STATUS_QUEUED read_mbox����BUF_STATUS_UNLINKED
        p_hdr->m_pNext  = NULL;
        p_hdr->m_u8Type    = 0;

        return ((void *) ((uint8_t *)p_hdr + AKI_BUFFER_HDR_SIZE));
    }

    AKI_BUFFER_UNLOCK(p_cb);
	//ָ��pool��û�еĻ�������ݵ�ǰ��С������pool�л�ȡ
    return (AKI_GetPackage(p_cb, p_cb->in_atFreeq[_u8Pid].m_u16PackSize));	

}


char *AKI_BufferPrint(T_AkiBuffer *_pAki, char *_pBuf, int _u32Len)
{
	T_AkiBuffer *p_cb = _pAki;
    int i;
	if (_pBuf == NULL) return _pBuf;
    for(i = 0; i < AKI_BUFFER_POOLS_TOTAL_MAX; i++){
		if (p_cb->in_atFreeq[i].m_u16PackSize){
			snprintf(_pBuf, _u32Len, "pool[%2d]:size %4u, cur %3u, max %3u, total%3u", 
				i, 
				p_cb->in_atFreeq[i].m_u16PackSize, p_cb->in_atFreeq[i].m_u16PackCurCnt, 
				p_cb->in_atFreeq[i].m_u16PackMaxCnt, p_cb->in_atFreeq[i].m_u16PackTotal);
		}
    }
	
	return _pBuf;
}

char *AKI_BufferPrintUsage(T_AkiBuffer *_pAki, char *_pBuf, int _u32Len)
{
	T_AkiBuffer *p_cb = _pAki;
    int i, u32Len = 0;
    T_AkiFreeq    *p;
	if (_pBuf == NULL) return _pBuf;
    u32Len += snprintf(_pBuf + u32Len, _u32Len - u32Len, "\r\n--- GKI Buffer Pool Summary (R - restricted, P - public) ---\r\n");
	u32Len += snprintf(_pBuf + u32Len, _u32Len - u32Len, "POOL     SIZE  USED  MAXU  TOTAL\r\n");
    u32Len += snprintf(_pBuf + u32Len, _u32Len - u32Len, "------------------------------\r\n");
    for (i = 0; i < p_cb->m_u8CurrTotalPools; i++){
        p = &p_cb->in_atFreeq[i];
		u32Len += snprintf(_pBuf + u32Len, _u32Len - u32Len, "%2d: (%s), %4d, %3d, %3d, %3d\r\n",
					i, ((1 << i) & p_cb->m_u16PoolAccessMask)?"R":"P",
					p->m_u16PackSize, p->m_u16PackCurCnt, p->m_u16PackMaxCnt, p->m_u16PackTotal);
    }
	
	return _pBuf;
}

char *AKI_PoolStatistics(T_AkiBuffer *_pAki, char _u8Pid, char *_pBuf, int _u32Len)
{
	T_AkiBuffer *p_cb = _pAki;
    T_AkiPackHdr *hdr;
    unsigned short i, size,act_size,maxbuffs;
    int *magic, u32Len = 0;
    if (!_pBuf || _u8Pid > AKI_BUFFER_POOLS_TOTAL_MAX || _u8Pid < 0){
        return NULL;
    }

    size = p_cb->in_atFreeq[_u8Pid].m_u16PackSize;
    maxbuffs = p_cb->in_atFreeq[_u8Pid].m_u16PackTotal;
    act_size = size + AKI_BUFFER_PADDING_SIZE;
    u32Len += snprintf(_pBuf + u32Len, _u32Len - u32Len, "\r\nbuffer pool[%u] size = %u cur_cnt = %u max_cnt = %u  total = %u\n",
        _u8Pid, p_cb->in_atFreeq[_u8Pid].m_u16PackSize,
        p_cb->in_atFreeq[_u8Pid].m_u16PackCurCnt, p_cb->in_atFreeq[_u8Pid].m_u16PackMaxCnt, p_cb->in_atFreeq[_u8Pid].m_u16PackTotal);

    u32Len += snprintf(_pBuf + u32Len, _u32Len - u32Len, "      owner  state    sanity\n");
    u32Len += snprintf(_pBuf + u32Len, _u32Len - u32Len, "----------------------------\n");
    hdr = (T_AkiPackHdr *)(p_cb->m_apPoolStart[_u8Pid]);
    for(i = 0; i < maxbuffs; i++){
        magic = (int *)((char *)hdr + AKI_BUFFER_HDR_SIZE + size);
        u32Len += snprintf(_pBuf + u32Len, _u32Len - u32Len, "%3d: 0x%02X %4d %10s\n", i, hdr->m_u8TaskId, hdr->m_u8Status, (*magic == MAGIC_NO)?"OK":"CORRUPTED");
        hdr = (T_AkiPackHdr *)((char *)hdr + act_size);
    }
    return _pBuf;
}




