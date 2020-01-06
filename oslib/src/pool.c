/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include "pool.h"
#include "os.h"

#define ALIGN_PTR(PTR,ALIGNMENT)    (PTR + (-(size_t)(PTR) & (ALIGNMENT-1)))

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"pool", ZXY_LOG_DEBUG_LV, 0};	//默认关闭
static char s_u8LogId = -1;

void POOL_SubLogRegister(void)
{
	s_u8LogId = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
}

int POOL_SubLogSet(char _enLogLv ,char _bLogOn)
{
	CONSOLE_SubLogSetParam(s_u8LogId, _enLogLv, _bLogOn);
}

T_SubLogInfo* POOL_SubLogGet(void)
{
	return CONSOLE_SubLogGetParam(s_u8LogId);
}

#endif


//在pool中创建一个新的block
static T_PoolBlockInfo *pool_create_block(T_PoolInfo *_pPool, size_t _stSize)
{
    T_PoolBlockInfo *pBlock;

    CHECK_STACK();
    assert(_stSize >= sizeof(T_PoolBlockInfo));

    ZXY_DLOG(LOG_DEBUG, "start create new block(sz=%zu), cur.cap=%zu, cur.used=%zu", _stSize, _pPool->m_szCapacity, POOL_GetUsedSize(_pPool));

    pBlock = (T_PoolBlockInfo*)(*_pPool->in_pFactory->in_tPolicy.funcBlockAlloc)(_pPool->in_pFactory, _stSize);
    if (pBlock == NULL) {
		(*_pPool->appFailCallbck)(_pPool, _stSize);
		return NULL;
    }

    _pPool->m_szCapacity += _stSize;

    pBlock->m_pStartAddr = ((unsigned char*)pBlock) + sizeof(T_PoolBlockInfo);
    pBlock->m_pEndAddr = ((unsigned char*)pBlock) + _stSize;

    pBlock->m_pCurAddr = ALIGN_PTR(pBlock->m_pStartAddr, POOL_ALIGNMENT);

	DL_ListAddTail(&_pPool->in_tBlockList.list, &pBlock->list);

    ZXY_DLOG(LOG_INFO, "new block created, cap = %zu(up:%zu), used = %zu, block [%p-%p]", _pPool->m_szCapacity, _stSize, POOL_GetUsedSize(_pPool), pBlock->m_pStartAddr, pBlock->m_pEndAddr);

    return pBlock;
}

//查看当前block是否有容量
static void *pool_alloc_from_block(T_PoolBlockInfo *_pBlock, size_t _stSize)
{
	void *ptr = NULL;
    if (_stSize & (POOL_ALIGNMENT-1)) {
		_stSize = (_stSize + POOL_ALIGNMENT) & ~(POOL_ALIGNMENT-1);
    }
	
    if ((size_t)(_pBlock->m_pEndAddr - _pBlock->m_pCurAddr) >= _stSize) {
		ptr = _pBlock->m_pCurAddr;
		_pBlock->m_pCurAddr += _stSize;
		ZXY_DLOG(LOG_INFO, "use block [%p-%p], allocate size = %8zu, free = %8zu(%3zu%%)", 
				_pBlock->m_pStartAddr, _pBlock->m_pEndAddr, _stSize, 
				_pBlock->m_pEndAddr - _pBlock->m_pCurAddr, (_pBlock->m_pEndAddr - _pBlock->m_pCurAddr)*100/(_pBlock->m_pEndAddr - _pBlock->m_pStartAddr));		
    }
	
    return ptr;
}

//pool中查找合适大小的block，没有则创建一个新的block
static void *pool_allocate_find(T_PoolInfo *_pPool, size_t _stSize)
{
    T_PoolBlockInfo *pBlock = NULL;
    void *p;
    size_t stBlockSize;

    CHECK_STACK();
    DL_ListForeach(pBlock, &_pPool->in_tBlockList.list, T_PoolBlockInfo, list){
		p = pool_alloc_from_block(pBlock, _stSize);
		if (p != NULL)
			return p;
    }

    if (_pPool->m_szIncSize == 0) {
		ZXY_DLOG(LOG_WARNING, "can't expand pool to allocate %zu bytes (used = %zu, cap = %zu)",
					_stSize, POOL_GetUsedSize(_pPool), _pPool->m_szCapacity);
		(*_pPool->appFailCallbck)(_pPool, _stSize);
		return NULL;
    }

    if (_pPool->m_szIncSize < _stSize + sizeof(T_PoolBlockInfo) + POOL_ALIGNMENT){
        size_t stCount;
        stCount = (_stSize + _pPool->m_szIncSize + sizeof(T_PoolBlockInfo) + POOL_ALIGNMENT) / _pPool->m_szIncSize;
        stBlockSize = stCount * _pPool->m_szIncSize;
    } 
	else {
        stBlockSize = _pPool->m_szIncSize;
    }

    ZXY_DLOG(LOG_DEBUG, "%zu bytes requested, resizing pool by %zu bytes (used = %zu, cap = %zu)",
				_stSize, stBlockSize, POOL_GetUsedSize(_pPool), _pPool->m_szCapacity);

    pBlock = pool_create_block(_pPool, stBlockSize);
    if (!pBlock) return NULL;

    p = pool_alloc_from_block(pBlock, _stSize);	//再次尝试
    assert(p != NULL);

    return p;
}

void *POOL_Alloc(T_PoolInfo *_pPool, size_t _stSize)
{
	if (_pPool->m_u32Magic != POOL_MAGIC_CODE){
		ZXY_ELOG("!!!this is not standard pool magic error");
		return NULL;
	}
	
	T_PoolBlockInfo *pBlock = NULL;
	pBlock = DL_ListGetNext(&_pPool->in_tBlockList.list, T_PoolBlockInfo, list);
	
	//+++ pBlock 判空断言
	
    void *ptr = pool_alloc_from_block(pBlock, _stSize);
    if (!ptr)
		ptr = pool_allocate_find(_pPool, _stSize);
    return ptr;
}


void *POOL_Calloc(T_PoolInfo *_pPool, size_t _stCount, size_t _stSize)
{
    void *pBuf = POOL_Alloc(_pPool, _stSize * _stCount);
    if (pBuf){
		memset(pBuf, 0, _stSize * _stCount);
	}
		
    return pBuf;
}

const char *POOL_GetName(const T_PoolInfo *_pPool)
{
    return _pPool->m_strName;
}

T_PoolInfo* POOL_Create(T_PoolFactory *_pFactory, const char *_strName, size_t _stInitSize, size_t _stIncSize, POOL_CALLBCK *_pCallbck)
{
    return (*_pFactory->funcCreatePool)(_pFactory, _strName, _stInitSize, _stIncSize, _pCallbck);
}

int POOL_Destroy(T_PoolInfo *_pPool)
{
	int u32Ret = -1;
    if (_pPool->in_pFactory->funcReleasePool)
		u32Ret = (*_pPool->in_pFactory->funcReleasePool)(_pPool->in_pFactory, _pPool);
	
	return u32Ret;
}


void pj_pool_safe_release(T_PoolInfo **_ppPool)
{
    T_PoolInfo *pPool = *_ppPool;
    *_ppPool = NULL;
    if (pPool) POOL_Destroy(pPool);
}

size_t POOL_GetCapacity(T_PoolInfo *_pPool)
{
    return _pPool->m_szCapacity;
}

size_t POOL_GetUsedSize(T_PoolInfo *_pPool)
{
	T_PoolBlockInfo *pBlock = NULL;
	size_t stUsedSize = sizeof(T_PoolInfo);
    DL_ListForeach(pBlock, &_pPool->in_tBlockList.list, T_PoolBlockInfo, list){
		stUsedSize += (pBlock->m_pCurAddr - pBlock->m_pStartAddr) + sizeof(T_PoolBlockInfo);
    }	
    return stUsedSize;
}



void PoolInitInt(T_PoolInfo *_pPool, const char *_strName, size_t _stIncSize, POOL_CALLBCK *_pCallbck)
{
    CHECK_STACK();
    _pPool->m_szIncSize = _stIncSize;
    _pPool->appFailCallbck = _pCallbck;

    if (_strName) {
		if (strchr(_strName, '%') != NULL) {
			snprintf(_pPool->m_strName, sizeof(_pPool->m_strName), _strName, _pPool);
		}
		else {
			strncpy(_pPool->m_strName, _strName, POOL_NAME_LEN_MAX);
			_pPool->m_strName[POOL_NAME_LEN_MAX - 1] = '\0';
		}
    } 
	else {
		_pPool->m_strName[0] = '\0';
    }
}

T_PoolInfo* PoolCreateInt(T_PoolFactory *_pFactory, const char *_strName, size_t _stInitSize, size_t _stIncSize, POOL_CALLBCK *_pCallbck)
{
    T_PoolInfo *pPool;
    T_PoolBlockInfo *pBlock;
    char *pBuffer;
    CHECK_STACK();
    ASSERT_RETURN(_stInitSize >= sizeof(T_PoolInfo) + sizeof(T_PoolBlockInfo), NULL);

    if (_pCallbck == NULL)
		_pCallbck = _pFactory->in_tPolicy.appFailCallbck;

    pBuffer = (char*)(*_pFactory->in_tPolicy.funcBlockAlloc)(_pFactory, _stInitSize);
    if (!pBuffer) return NULL;

    pPool = (T_PoolInfo*)pBuffer;
	memset(pPool, 0, sizeof(*pPool));
    DL_ListInit(&pPool->in_tBlockList.list);
    pPool->in_pFactory = _pFactory;

    pBlock = (T_PoolBlockInfo *)(pBuffer + sizeof(*pPool));
    pBlock->m_pStartAddr = ((unsigned char*)pBlock) + sizeof(T_PoolBlockInfo);
    pBlock->m_pEndAddr = pBuffer + _stInitSize;

    pBlock->m_pCurAddr = ALIGN_PTR(pBlock->m_pStartAddr, POOL_ALIGNMENT);
	DL_ListAddTail(&pPool->in_tBlockList.list, &pBlock->list);
    PoolInitInt(pPool, _strName, _stIncSize, _pCallbck);
    pPool->m_szCapacity = _stInitSize;

    ZXY_DLOG(LOG_INFO, "new pool[name:\"%s\"] created, size = %zu, increment = %zu, block0 [%p-%p]", 
				pPool->m_strName, _stInitSize, _stIncSize, 
				pBlock->m_pStartAddr, pBlock->m_pEndAddr);
    return pPool;
}

//删除所有block，除了第一块
static void reset_pool(T_PoolInfo *_pPool)
{
    T_PoolBlockInfo *pBlock, *itself;
    CHECK_STACK();
	
	itself = DL_ListGetNext(&_pPool->in_tBlockList.list, T_PoolBlockInfo, list);
	pBlock  = DL_ListGetPrev(&_pPool->in_tBlockList.list, T_PoolBlockInfo, list);
	
	if (itself == pBlock) return;
	ZXY_DLOG(LOG_DEBUG, "reset check block count = %d, next [%p], prev [%p]", DL_ListGetLen(&_pPool->in_tBlockList.list), itself, pBlock);
    while (pBlock != itself) {
		T_PoolBlockInfo *prev = DL_ListGetPrev(&pBlock->list, T_PoolBlockInfo, list);
		ZXY_DLOG(LOG_INFO, "delete block [%p-%p], size = %zu, free = %zu, used = %zu(%zu%%)", 
				pBlock->m_pStartAddr, pBlock->m_pEndAddr, 
				pBlock->m_pEndAddr - pBlock->m_pStartAddr, pBlock->m_pEndAddr - pBlock->m_pCurAddr, pBlock->m_pCurAddr - pBlock->m_pStartAddr, 
				(pBlock->m_pCurAddr - pBlock->m_pStartAddr)*100/(pBlock->m_pEndAddr - pBlock->m_pStartAddr));
				
		DL_ListAddDelete(&pBlock->list);
		(*_pPool->in_pFactory->in_tPolicy.funcBlockFree)(_pPool->in_pFactory, pBlock, pBlock->m_pEndAddr - (unsigned char*)pBlock);
		pBlock = prev;
    }
    itself->m_pCurAddr = ALIGN_PTR(itself->m_pStartAddr, POOL_ALIGNMENT);
    _pPool->m_szCapacity = itself->m_pEndAddr - (unsigned char*)_pPool;
}

void POOL_Reset(T_PoolInfo *_pPool)
{
	size_t szCapacity = _pPool->m_szCapacity;
	size_t szUsed = POOL_GetUsedSize(_pPool);
	size_t szPresent = szUsed * 100/szCapacity;

    reset_pool(_pPool);
    ZXY_DLOG(LOG_DEBUG, "reset[\"%s\"]: cap = %zu -> %zu, used = %zu(%zu%%) -> %zu(%zu%%)", 
			_pPool->m_strName, 
			szCapacity, _pPool->m_szCapacity, 
			szUsed, szPresent,
			POOL_GetUsedSize(_pPool), POOL_GetUsedSize(_pPool) * 100/_pPool->m_szCapacity);
}

void PoolDestroyInt(T_PoolInfo *_pPool)
{
    size_t stInitSize;
	T_PoolBlockInfo *pBlock = NULL;
	pBlock = DL_ListGetNext(&_pPool->in_tBlockList.list, T_PoolBlockInfo, list);	

    ZXY_DLOG(LOG_INFO, "destroy[\"%s\"]: cap = %zu, used = %zu(%zu%%), block0 [%p-%p]", 
			_pPool->m_strName, _pPool->m_szCapacity, POOL_GetUsedSize(_pPool), 
			POOL_GetUsedSize(_pPool)*100/_pPool->m_szCapacity,
			pBlock->m_pStartAddr, pBlock->m_pEndAddr);

    reset_pool(_pPool);
    stInitSize = pBlock->m_pEndAddr - (unsigned char*)_pPool;
	
    if (_pPool->in_pFactory->in_tPolicy.funcBlockFree)
		(*_pPool->in_pFactory->in_tPolicy.funcBlockFree)(_pPool->in_pFactory, _pPool, stInitSize);
}

void POOL_FactoryDump(T_PoolFactory *_pFactory)
{
	(*_pFactory->funcDumpStatus)(_pFactory, 1);
}


