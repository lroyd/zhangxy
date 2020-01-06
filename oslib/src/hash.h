#ifndef __ZXY_HASH_H__
#define __ZXY_HASH_H__

#include "typedefs.h"
#include "cstring.h"
#include "pool.h"

#define HASH_KEY_TYPE_STRING		((unsigned)-1)
#define HASH_ENTRY_BUF_SIZE			(3*sizeof(void*) + 2 * sizeof(uint32_t))

typedef void *pj_hash_entry_buf[(HASH_ENTRY_BUF_SIZE + sizeof(void*) - 1)/(sizeof(void*))];

typedef struct _tagHashTable T_HashTable;
typedef struct _tagHashEntry T_HashEntry;
typedef struct _tagHashIterator
{
    uint32_t	     index;     /**< Internal index.     */
    T_HashEntry   *entry;     /**< Internal entry.     */
}T_HashIterator;

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
API_DEF(void)			HASH_SubLogRegister(void);
API_DEF(int)			HASH_SubLogSet(char _enLogLv ,char _bLogOn);
API_DEF(T_SubLogInfo*)	HASH_SubLogGet(void);
#endif

API_DEF(uint32_t)			HASH_Calc(uint32_t hval, const void *key, unsigned keylen);
API_DEF(uint32_t)			HASH_CalcToLower(uint32_t hval, char *result, const str_t *key);
API_DEF(T_HashTable*)		HASH_Create(T_PoolInfo *pool, unsigned size);
API_DEF(void *)				HASH_Get(T_HashTable *ht, const void *key, unsigned keylen, uint32_t *hval);
API_DEF(void *)				HASH_GetLower(T_HashTable *ht, const void *key, unsigned keylen, uint32_t *hval);
API_DEF(void)				HASH_Set(T_PoolInfo *pool, T_HashTable *ht, const void *key, unsigned keylen, uint32_t hval, void *value);
API_DEF(void)				HASH_SetLower(T_PoolInfo *pool, T_HashTable *ht, const void *key, unsigned keylen, uint32_t hval, void *value);
API_DEF(void)				HASH_SetNp(T_HashTable *ht, const void *key, unsigned keylen, uint32_t hval, pj_hash_entry_buf entry_buf, void *value);
API_DEF(void)				HASH_SetNpLower(T_HashTable *ht, const void *key, unsigned keylen, uint32_t hval, pj_hash_entry_buf entry_buf, void *value);
API_DEF(unsigned)			HASH_Count(T_HashTable *ht);
API_DEF(T_HashIterator*)	HASH_ItFirst(T_HashTable *ht, T_HashIterator *it);
API_DEF(T_HashIterator*)	HASH_ItNext(T_HashTable *ht, T_HashIterator *it);
API_DEF(void*)				HASH_ItThis(T_HashTable *ht, T_HashIterator *it);


#endif


