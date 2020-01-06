/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include "pool_signature.h"
#include "pool.h"

static void *default_block_alloc(T_PoolFactory *_pFactory, size_t _stSize)
{
    void *p;

    //CHECK_STACK();

    if (_pFactory->onBlockAlloc){
		_pFactory->onBlockAlloc(_pFactory, _stSize);
    }

    p = malloc(_stSize + (SIG_SIZE << 1));

    if (p == NULL){
		if (_pFactory->onBlockFree) 
			_pFactory->onBlockFree(_pFactory, _stSize);
    } 
	else {
		APPLY_SIG(p, _stSize);
    }

    return p;
}

static void default_block_free(T_PoolFactory *_pFactory, void *_pData, size_t _stSize)
{
    //CHECK_STACK();

    if (_pFactory->onBlockFree) 
        _pFactory->onBlockFree(_pFactory, _stSize);

    REMOVE_SIG(_pData, _stSize);

    free(_pData);
}

static void default_pool_fail_callback(T_PoolInfo *_pPool, size_t _stSize)
{
    //CHECK_STACK();
    //PJ_UNUSED_ARG(_pPool);
    //PJ_UNUSED_ARG(_stSize);

    //PJ_THROW(PJ_NO_MEMORY_EXCEPTION);
}

static T_PoolFactoryPolicy s_tPoolFactoryDefaultPolicy =
{
    &default_block_alloc,
    &default_block_free,
    &default_pool_fail_callback,
    0
};

const T_PoolFactoryPolicy *PoolGetDefaultPolicy(void)
{
    return &s_tPoolFactoryDefaultPolicy;
}

