/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h> 
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#define SIG_SIZE		sizeof(uint32_t)

static void apply_signature(void *_pData, size_t _stSize);
static void check_pool_signature(void *_pData, size_t _stSize);

#define APPLY_SIG(p,sz)	apply_signature(p,sz), \
				p=(void*)(((char*)p)+SIG_SIZE)
#define REMOVE_SIG(p,sz)	check_pool_signature(p,sz), \
				p=(void*)(((char*)p)-SIG_SIZE)

#define SIG_BEGIN		(0x600DC0DE)
#define SIG_END			(0x0BADC0DE)

static void apply_signature(void *_pData, size_t _stSize)
{
    uint32_t u32Sig;

    u32Sig = SIG_BEGIN;
    memcpy(_pData, &u32Sig, SIG_SIZE);

    u32Sig = SIG_END;
    memcpy(((char*)_pData) + SIG_SIZE + _stSize, &u32Sig, SIG_SIZE);
}

static void check_pool_signature(void *_pData, size_t _stSize)
{
    uint32_t u32Sig;
    uint8_t *mem = (uint8_t*)_pData;

    u32Sig = SIG_BEGIN;
    assert(!memcmp(mem - SIG_SIZE, &u32Sig, SIG_SIZE));

    u32Sig = SIG_END;
    assert(!memcmp(mem + _stSize, &u32Sig, SIG_SIZE));
}

