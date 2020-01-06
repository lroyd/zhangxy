/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include "dl_list.h"
#include "pool.h"
#include "pool_stack.h"
#include "os.h"

static T_PoolInfo* cpool_create_pool(T_PoolFactory *_pFactory, const char *_strName, size_t _stInitSize, size_t _stIncSize, POOL_CALLBCK *_pCallbck);
static int cpool_release_pool(T_PoolFactory *_pFactory, T_PoolInfo *_pPool);
static void cpool_dump_status(T_PoolFactory *_pFactory, int _bDetail );
static int cpool_on_block_alloc(T_PoolFactory *_pFactory, size_t _stSize);
static void cpool_on_block_free(T_PoolFactory *_pFactory, size_t _stSize);

static size_t pool_sizes[POOL_CACHING_ARRAY_SIZE] = 
{
    256, 512, 1024, 2048, 4096, 8192, 12288, 16384, 
    20480, 24576, 28672, 32768, 40960, 49152, 57344, 65536
};

/* Index where the search for size should begin.
 * Start with pool_sizes[5], which is 8192.
 */
#define POOL_START_SIZE  (5)


void POOL_CachingInit(T_PoolCaching *_pCaching, const T_PoolFactoryPolicy *_pPolicy, size_t _stCapacityMax)
{
    int i;
    T_PoolInfo *pPool;
    CHECK_STACK();
	memset(_pCaching, 0, sizeof(T_PoolCaching));
    
    _pCaching->m_szCapacityMax = _stCapacityMax;
    DL_ListInit(&_pCaching->used_list);
    for (i = 0; i < POOL_CACHING_ARRAY_SIZE; ++i){
		DL_ListInit(&_pCaching->free_list[i]);
	}

    if (_pPolicy == NULL) {
    	_pPolicy = PoolGetDefaultPolicy();
    }
    
    memcpy(&_pCaching->in_pFactory.in_tPolicy, _pPolicy, sizeof(T_PoolFactoryPolicy));
    _pCaching->in_pFactory.funcCreatePool		= &cpool_create_pool;
    _pCaching->in_pFactory.funcReleasePool		= &cpool_release_pool;
    _pCaching->in_pFactory.funcDumpStatus		= &cpool_dump_status;
    _pCaching->in_pFactory.onBlockAlloc			= &cpool_on_block_alloc;
    _pCaching->in_pFactory.onBlockFree			= &cpool_on_block_free;

    pPool = PoolCreateOnStack("cachingpool", _pCaching->m_aucPoolStack, sizeof(_pCaching->m_aucPoolStack));
	pthread_mutex_init(&_pCaching->m_tLock, NULL);
}

void POOL_CachingDeinit(T_PoolCaching *_pCaching)
{
    int i;
    CHECK_STACK();
	T_PoolInfo *pPool = NULL, *tmp = NULL;
    /* Delete all pool in free list */
    for (i = 0; i < POOL_CACHING_ARRAY_SIZE; ++i) {
		pPool = NULL; tmp = NULL;
		DL_ListForeachSafe(pPool, tmp, &_pCaching->free_list[i], T_PoolInfo, list){ //list为pj_pool_t结构体中链表的名字
			DL_ListAddDelete(&pPool->list);
			PoolDestroyInt(pPool);
		}
    }
	
	/* Delete all pools in used list */
	pPool = NULL; tmp = NULL;
	DL_ListForeachSafe(pPool, tmp, &_pCaching->used_list, T_PoolInfo, list){ //list为pj_pool_t结构体中链表的名字
		DL_ListAddDelete(&pPool->list);
		//POOL_LOG("Pool is not released by application, releasing now\r\n");		
		PoolDestroyInt(pPool);
	}	

	pthread_mutex_destroy(&_pCaching->m_tLock);
    
}

static T_PoolInfo *cpool_create_pool(T_PoolFactory *_pFactory, 
									 const char *_strName, 
									 size_t _stInitSize, 
									 size_t _stIncSize, 
									 POOL_CALLBCK *_pCallbck)
{
    T_PoolCaching *pCaching = (T_PoolCaching*)_pFactory;
    T_PoolInfo *pPool;
    int u32PoolIndex;

    CHECK_STACK();
	pthread_mutex_lock(&pCaching->m_tLock);

    if (_pCallbck == NULL) {
		_pCallbck = _pFactory->in_tPolicy.appFailCallbck;
    }

    if (_stInitSize <= pool_sizes[POOL_START_SIZE]) {
		for (u32PoolIndex = POOL_START_SIZE - 1; u32PoolIndex >= 0 && pool_sizes[u32PoolIndex] >= _stInitSize; --u32PoolIndex);
		++u32PoolIndex;
    }
	else {
		for (u32PoolIndex = POOL_START_SIZE + 1; u32PoolIndex < POOL_CACHING_ARRAY_SIZE && pool_sizes[u32PoolIndex] < _stInitSize; ++u32PoolIndex);
    }

    if (u32PoolIndex == POOL_CACHING_ARRAY_SIZE || DL_ListIsEmpty(&pCaching->free_list[u32PoolIndex])) {
		/* No pool is available. */
		if (u32PoolIndex < POOL_CACHING_ARRAY_SIZE)
			_stInitSize =  pool_sizes[u32PoolIndex];

		pPool = PoolCreateInt(&pCaching->in_pFactory, _strName, _stInitSize, _stIncSize, _pCallbck);
		if (!pPool) {
			pthread_mutex_unlock(&pCaching->m_tLock);
			return NULL;
		}

    } 
	else {
		//m_szCapacityMax = 0，不会进入
		pPool = DL_ListGetNext(&pCaching->free_list[u32PoolIndex], T_PoolInfo, list);
		DL_ListAddDelete(&pPool->list);

		PoolInitInt(pPool, _strName, _stIncSize, _pCallbck);

		if (pCaching->m_szCapacity > POOL_GetCapacity(pPool)) {
			pCaching->m_szCapacity -= POOL_GetCapacity(pPool);
		} 
		else {
			pCaching->m_szCapacity = 0;
		}

		//POOL_LOG("pool reused, size = %zu\r\n", pPool->m_szCapacity);
    }

	DL_ListAdd(&pCaching->used_list, &pPool->list);
	
    pPool->m_pFactoryData	= (void*)(&u32PoolIndex);
	pPool->m_u32Magic		= POOL_MAGIC_CODE;
    ++pCaching->m_u32UsedCnt;

    pthread_mutex_unlock(&pCaching->m_tLock);
    return pPool;
}

static int cpool_release_pool(T_PoolFactory *_pFactory, T_PoolInfo *_pPool)
{
    T_PoolCaching *pCaching = (T_PoolCaching*)_pFactory;
    size_t stCapacity;
    unsigned i, find = 0;
    CHECK_STACK();

    ASSERT_ON_FAIL(_pFactory && _pPool, return);

	if (_pPool->m_u32Magic != POOL_MAGIC_CODE){
		ZXY_ELOG("!!!this is not standard pool magic error");
		return -1;
	}
	
    pthread_mutex_lock(&pCaching->m_tLock);

    /* Make sure pool is still in our used list */
	T_PoolInfo *ipool = NULL;
    DL_ListForeach(ipool, &pCaching->used_list, T_PoolInfo, list){
		if (ipool == _pPool){
			find = 1;
			break;
		}
    }	
	
	if (!find){
		ZXY_ELOG("attempt to destroy pool that has been destroyed before");
		pthread_mutex_unlock(&pCaching->m_tLock);
		return 0;		
	}

	DL_ListAddDelete(&_pPool->list);

    /* Decrement used count. */
    --pCaching->m_u32UsedCnt;

    stCapacity = POOL_GetCapacity(_pPool);

    if (stCapacity > pool_sizes[POOL_CACHING_ARRAY_SIZE - 1] || pCaching->m_szCapacity + stCapacity > pCaching->m_szCapacityMax)
    {
		PoolDestroyInt(_pPool);
		pthread_mutex_unlock(&pCaching->m_tLock);
		return 0;
    }
	//m_szCapacityMax = 0,不会进入
    /* Reset pool. */
    //POOL_LOG("recycle(): cap=%zu, used=%zu(%zu%%)\r\n", stCapacity, POOL_GetUsedSize(_pPool), POOL_GetUsedSize(_pPool)*100/stCapacity);
    POOL_Reset(_pPool);

    stCapacity = POOL_GetCapacity(_pPool);

    i = (unsigned) (unsigned long)_pPool->m_pFactoryData;

    assert(i<POOL_CACHING_ARRAY_SIZE);
    if (i >= POOL_CACHING_ARRAY_SIZE ) {
		/* Something has gone wrong with the pool. */
		PoolDestroyInt(_pPool);
		pthread_mutex_unlock(&pCaching->m_tLock);
		return -1;
    }
	DL_ListAdd(&pCaching->free_list[i], &_pPool->list);
    pCaching->m_szCapacity += stCapacity;

    pthread_mutex_unlock(&pCaching->m_tLock);
	
	return 0;
}

static void cpool_dump_status(T_PoolFactory *_pFactory, int _bDetail)
{
#if 0
    T_PoolCaching *pCaching = (T_PoolCaching*)_pFactory;
    pthread_mutex_lock(&pCaching->m_tLock);
	POOL_LOG(">>>>>>>>>>>:dumping pool state\r\n");
    POOL_LOG("    capacity = %zu, max capacity = %zu, pool used cnt = %d\r\n", pCaching->m_szCapacity, pCaching->m_szCapacityMax, pCaching->m_u32UsedCnt);
    if (_bDetail) {
		T_PoolInfo *pPool = NULL;
		size_t total_used = 0, total_capacity = 0;
		POOL_LOG("    dumping all active pools:use Bytes\r\n");		
		POOL_LOG("         [pool name]  [total]   [used]  (percent)\r\n");		
		DL_ListForeach(pPool, &pCaching->used_list, T_PoolInfo, list){
			size_t stCapacity = POOL_GetCapacity(pPool);
			POOL_LOG("   %16s: %8zu %8zu  (%3zu%%)\r\n", 
					  POOL_GetName(pPool), 
					  POOL_GetUsedSize(pPool), 
					  stCapacity,
					  POOL_GetUsedSize(pPool)*100/stCapacity);
			total_used += POOL_GetUsedSize(pPool);
			total_capacity += stCapacity;		
		}
		
		POOL_LOG("    statistics:[%8zu Bytes]-[%8zu Bytes](%3zu%%) used!\r\n", total_used, total_capacity, total_capacity?total_used * 100 / total_capacity:0*100/100);
    }
	POOL_LOG(">>>>>>>>>>>:end\r\n");
    pthread_mutex_unlock(&pCaching->m_tLock);
#endif
}


static int cpool_on_block_alloc(T_PoolFactory *_pFactory, size_t _stSize)
{
    T_PoolCaching *pCaching = (T_PoolCaching*)_pFactory;

    //Can't m_tLock because mutex is not recursive
    //if (pCaching->mutex) MUTEX_Lock(pCaching->mutex);

    pCaching->m_szUsedSize += _stSize;
    if (pCaching->m_szUsedSize > pCaching->m_szPeakUsedSize)
		pCaching->m_szPeakUsedSize = pCaching->m_szUsedSize;

    //if (pCaching->mutex) MUTEX_Unlock(pCaching->mutex);

    return 0;
}


static void cpool_on_block_free(T_PoolFactory *_pFactory, size_t _stSize)
{
    T_PoolCaching *pCaching = (T_PoolCaching*)_pFactory;

    //MUTEX_Lock(pCaching->mutex);
    pCaching->m_szUsedSize -= _stSize;
    //MUTEX_Unlock(pCaching->mutex);
}


