/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include "pool_stack.h"


typedef struct _tagLocalParam
{
	int				m_bInit;
	T_PoolFactory	in_tBaseFactory;
	T_PoolInfo		*in_pPool;
	
    void			*m_pStack;
    size_t			m_stSize;
}T_LocalParam;

static T_LocalParam *s_tPoolLocal = NULL;		//全局唯一


static void *stack_alloc(T_PoolFactory *_pFactory, size_t _stSize)
{
    T_LocalParam *pLocal = s_tPoolLocal;
    void *pBuf;
    ASSERT_RETURN(_stSize <= pLocal->m_stSize, NULL);
    pBuf = pLocal->m_pStack;
    return pBuf;
}


T_PoolInfo *PoolCreateOnStack(const char *_strName, void *_pBuf, size_t _stSize)
{
    T_LocalParam *pLocal = (T_LocalParam *)_pBuf;
    size_t align_diff;

    ASSERT_RETURN(_pBuf && _stSize, NULL);
	
	pLocal->m_bInit = 1;
	pLocal->in_tBaseFactory.in_tPolicy.funcBlockAlloc = &stack_alloc;

	void *pTmp = (void *)((char *)_pBuf + sizeof(T_LocalParam));

    align_diff = (size_t)pTmp;
    if (align_diff & (POOL_ALIGNMENT-1)) {
		align_diff &= (POOL_ALIGNMENT-1);
		pTmp = (void*) (((char*)pTmp) + align_diff);
		_stSize -= align_diff;
    }

    pLocal->m_pStack = pTmp;
    pLocal->m_stSize = _stSize - sizeof(T_LocalParam);
	
	s_tPoolLocal = pLocal;
	
	pLocal->in_pPool = PoolCreateInt(&pLocal->in_tBaseFactory, _strName, pLocal->m_stSize, 0, (PoolGetDefaultPolicy())->appFailCallbck);	
	
    return pLocal->in_pPool;
}

