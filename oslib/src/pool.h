/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_POOL_H__
#define __ZXY_POOL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "typedefs.h"
#include "error_code.h"
#include "dl_list.h"
#include "console.h"
#include "config_site.h"


#define POOL_MAGIC_CODE			(0x210EF0BA)
#define POOL_ALIGNMENT			(4)
#define POOL_NAME_LEN_MAX		(32)
#define POOL_CACHING_ARRAY_SIZE	(16)

typedef struct _tagPoolInfo		T_PoolInfo;
typedef struct _tagPoolFactory	T_PoolFactory;
typedef void POOL_CALLBCK(T_PoolInfo *_pPool, size_t _stSize);	//pool分配失败的回调函数，用户不定义的话，默认使用pj_pool_factory_policy

typedef struct _tagPoolFactoryPolicy
{
    void*(*funcBlockAlloc)(T_PoolFactory *_pFactory, size_t _stSize);
    void (*funcBlockFree) (T_PoolFactory *_pFactory, void *_pMem, size_t _stSize);
    POOL_CALLBCK *appFailCallbck;		//分配失败回调
    unsigned m_uFlags;	
}T_PoolFactoryPolicy;

struct _tagPoolFactory
{
    T_PoolFactoryPolicy in_tPolicy;
    T_PoolInfo*	(*funcCreatePool)(T_PoolFactory *_pFactory, const char *_strName, size_t _szInitSize, size_t _szIncmSize, POOL_CALLBCK *_pCallbck);

    int  (*funcReleasePool)(T_PoolFactory *_pFactory, T_PoolInfo *_pPool);
    void (*funcDumpStatus)(T_PoolFactory *_pFactory, int _bDetail);

    int  (*onBlockAlloc)(T_PoolFactory *_pFactory, size_t _stSize);
    void (*onBlockFree) (T_PoolFactory *_pFactory, size_t _stSize);
};

//pool中的子block
typedef struct _tagPoolBlockInfo
{ 
	T_DoubleList	list;						//可以用不够时，按照增长size继续分配
    unsigned char    *m_pStartAddr;                      //当前pool中的buf开始地址
    unsigned char    *m_pCurAddr;                      
    unsigned char    *m_pEndAddr;                      //当前pool中的buf结束地址
} T_PoolBlockInfo;

//子pool
struct _tagPoolInfo
{
	T_DoubleList	list;        //存在于free_list[](max_capacity为0则直接释放，不会使用)或者used_list中
    char			m_strName[POOL_NAME_LEN_MAX];
	int				m_u32Magic;
	
    T_PoolFactory	*in_pFactory;
    void			*m_pFactoryData;

    size_t			m_szCapacity;		//当前pool可用大小，create pool时设置
    size_t			m_szIncSize;

    T_PoolBlockInfo	in_tBlockList;		//增长block链表

    POOL_CALLBCK	*appFailCallbck;	
};

//全局唯一
typedef struct _tagPoolCaching 
{
    T_PoolFactory	in_pFactory;

    size_t			m_szCapacity;	//缓存容量，max_capacity不为0的时候，release pool的时候设置
    size_t			m_szCapacityMax; //一般设置为0

    unsigned int	m_u32UsedCnt;		//分配的pool的个数
    size_t			m_szUsedSize;		//当前已经使用的总的字节数，on_block_alloc/on_block_free中设置(policy->block_alloc中调用)
    size_t			m_szPeakUsedSize;  //分配过的最高峰值

    T_DoubleList	free_list[POOL_CACHING_ARRAY_SIZE];	//已经释放的缓存可用链表，max_capacity不为0时，使用
    T_DoubleList	used_list;	//create pool时使用

    char			m_aucPoolStack[256 * (sizeof(size_t) / 4)];  //存储caching本身

    pthread_mutex_t	m_tLock;
	
}T_PoolCaching;



#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
API_DECL(void)			POOL_SubLogRegister(void);
API_DECL(int) 			POOL_SubLogSet(char _enLogLv ,char _bLogOn);
API_DECL(T_SubLogInfo*) POOL_SubLogGet(void);
#endif

API_DECL(void)	POOL_CachingInit  (T_PoolCaching *_pCaching, const T_PoolFactoryPolicy *_pPolicy, size_t _szCapacityMax);
API_DECL(void)	POOL_CachingDeinit(T_PoolCaching *_pCaching);

API_DECL(T_PoolInfo*) POOL_Create(T_PoolFactory *_pFactory, const char *_strName, size_t _szInitSize, size_t _szIncmSize, POOL_CALLBCK *_pCallbck);
API_DECL(int)	POOL_Destroy(T_PoolInfo *_pPool);

API_DECL(void) pj_pool_safe_release(T_PoolInfo **ppool);

API_DECL(const char*) POOL_GetName(const T_PoolInfo *_pPool);

API_DECL(void)	POOL_Reset(T_PoolInfo *_pPool);

API_DECL(size_t) POOL_GetCapacity(T_PoolInfo *_pPool);
API_DECL(size_t) POOL_GetUsedSize(T_PoolInfo *_pPool);

API_DECL(void*)	POOL_Alloc(T_PoolInfo *_pPool, size_t _stSize);
API_DECL(void*)	POOL_Calloc(T_PoolInfo *_pPool, size_t _szCount, size_t _szElem);
#define POOL_Zalloc(pool, size) POOL_Calloc(pool, 1, size)

#define POOL_ALLOC_T(pool, type) ((type*)POOL_Alloc(pool, sizeof(type)))
#define POOL_ZALLOC_T(pool,type) ((type*)POOL_Zalloc(pool, sizeof(type)))

API_DECL(void) POOL_FactoryDump(T_PoolFactory *_pFactory);


const T_PoolFactoryPolicy *PoolGetDefaultPolicy(void);
T_PoolInfo* PoolCreateInt(T_PoolFactory *_pFactory, const char *_strName, size_t _szInitSize, size_t _szIncmSize, POOL_CALLBCK *_pCallbck);
void PoolInitInt(T_PoolInfo *_pPool, const char *_strName, size_t _szIncmSize, POOL_CALLBCK *_pCallbck);
void PoolDestroyInt(T_PoolInfo *_pPool);


///////////////////////////////////////////





    
#endif

