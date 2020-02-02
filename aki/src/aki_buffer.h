#ifndef __ZXY_AKI_BUFFER_H__
#define __ZXY_AKI_BUFFER_H__

#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************/
//#define AKI_USE_OSLIB
//#define AKI_USE_LOCK  
//#define AKI_USE_TASK 
/*************************************************************************/

//不支持动态分配，因为不好记录释放
enum{
	AKI_BUF_STATUS_FREE,
	AKI_BUF_STATUS_UNLINKED,
	AKI_BUF_STATUS_QUEUED
};

#define AKI_POOL_PUBLIC         	(0)
#define AKI_POOL_RESTRICTED     	(1)

#define AKI_BUFFER_POOLS_TOTAL_MAX	(6)
#define AKI_ALIGN_SIZE(s) 			((((s) + 3)/sizeof(int))* sizeof(int))

#define AKI_BUFFER_HDR_SIZE			(sizeof(T_AkiPackHdr))
#define AKI_BUFFER_PADDING_SIZE		(sizeof(T_AkiPackHdr) + sizeof(int))
#define AKI_BUFFER_SIZE_MAX			((unsigned short)0xFFFF - AKI_BUFFER_PADDING_SIZE)

  
/**************************************************************************/
typedef struct _tagQueue
{
    void	*m_pFirst;	
    void    *m_pLast;
    unsigned short	m_u16Count;
}T_AkiQueue;


typedef struct _tagBufferHdr {
	struct _tagBufferHdr	*m_pNext;	//下一个包
    char m_u8PoolId;		//当前pool id
    char m_u8TaskId;	//task id多任务使用
    char m_u8Status;		//当前buffer状态
    char m_u8Type;		//未使用	
}T_AkiPackHdr;


typedef struct _tagFreeq
{
    T_AkiPackHdr *in_pFirst;	//记录当前第一个可用，会移动
    T_AkiPackHdr *in_pLast;

    unsigned short    m_u16PackSize;		//每个包可使用大小
    unsigned short    m_u16PackTotal;		//包的个数
    unsigned short    m_u16PackCurCnt;	//已使用包的个数
    unsigned short    m_u16PackMaxCnt;	//历史使用最大包个数

}T_AkiFreeq;

typedef struct _tagAkiBuffer
{
    T_AkiFreeq		in_atFreeq[AKI_BUFFER_POOLS_TOTAL_MAX];

    char			*m_apPoolStart[AKI_BUFFER_POOLS_TOTAL_MAX];
    char			*m_apPoolEnd[AKI_BUFFER_POOLS_TOTAL_MAX];
    unsigned short	m_ausPoolSize[AKI_BUFFER_POOLS_TOTAL_MAX];	//带头，最大65535：65k

    unsigned short	m_u16PoolAccessMask;
    char			m_aucPoolList[AKI_BUFFER_POOLS_TOTAL_MAX];	//访问权限使用
	
    char			m_u8CurrTotalPools;
	pthread_mutex_t m_tLock;
	
}T_AkiBuffer;


/**************************************************************************/

void AKI_BufferInit(T_AkiBuffer *_pAkiBuffer);

//id:0+
int AKI_BufferCreatePool(T_AkiBuffer *_pAki, void *_pUAddress, unsigned short _u16USize, unsigned short _u16Size, char *_pOut);
void *AKI_BufferDestroyPool(T_AkiBuffer *_pAki, char _u8Pid);

unsigned short AKI_PoolGetTotal(T_AkiBuffer *_pAki, char _u8Pid);

unsigned short AKI_PoolGetFreeTotal(T_AkiBuffer *_pAki, char _u8Pid);

typedef void (T_Package);
T_Package *AKI_GetPackage(T_AkiBuffer *_pAki, unsigned short _u16Size);
void AKI_PutPackage(T_AkiBuffer *_pAki, T_Package *_pPack);

T_Package *AKI_CheckStartAddr(T_AkiBuffer *_pAki, void *_pUsrAddr);
T_Package *AKI_PoolGetPackage(T_AkiBuffer *_pAki, char _u8Pid);

/*********************************************************************/
//使用aki_queue系队列，其中的buf必须是AKI_GetPackage的package
void AKI_QueueInit(T_AkiQueue *_pQueue);
void AKI_QueueAddTail(T_AkiQueue *_pQueue, T_Package *p_buf);
void AKI_QueueAddHead(T_AkiQueue *_pQueue, T_Package *p_buf);
T_Package *AKI_QueueDelHead(T_AkiQueue *_pQueue);
T_Package *AKI_QueueRemove(T_AkiQueue *_pQueue, T_Package *p_buf);
T_Package *AKI_QueueGetFirst(T_AkiQueue *_pQueue);
T_Package *AKI_QueueGetLast(T_AkiQueue *_pQueue);
T_Package *AKI_QueueGetNext(T_Package *p_buf);
char AKI_QueueIsEmpty(T_AkiQueue *_pQueue);


/*********************************************************************/
//debug
char *AKI_BufferPrint(T_AkiBuffer *_pAki, char *_pBuf, int _u32Len);
char *AKI_BufferPrintUsage(T_AkiBuffer *_pAki, char *_pBuf, int _u32Len);
char *AKI_PoolStatistics(T_AkiBuffer *_pAki, char _u8Pid, char *_pBuf, int _u32Len);


#ifdef __cplusplus
}
#endif


#endif


