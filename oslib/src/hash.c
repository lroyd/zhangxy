#include "hash.h"
#include "ctype.h"

#define HASH_MULTIPLIER	33

#define CHECK_STACK()

struct _tagHashEntry
{
    struct _tagHashEntry *next;
    void *key;
    uint32_t hash;
    uint32_t keylen;
    void *value;
};


struct _tagHashTable
{
    T_HashEntry     **table;
    unsigned		count, rows;
    T_HashIterator	iterator;
};


#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"hash", ZXY_LOG_DEBUG_LV, 0};	//默认关闭
static char s_u8LogId = -1;

void HASH_SubLogRegister(void)
{
	s_u8LogId = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
}

int HASH_SubLogSet(char _enLogLv ,char _bLogOn)
{
	CONSOLE_SubLogSetParam(s_u8LogId, _enLogLv, _bLogOn);
}

T_SubLogInfo* HASH_SubLogGet(void)
{
	return CONSOLE_SubLogGetParam(s_u8LogId);
}
#endif



uint32_t HASH_Calc(uint32_t hash, const void *key, unsigned keylen)
{
    CHECK_STACK();
    if (keylen == HASH_KEY_TYPE_STRING){
		const uint8_t *p = (const uint8_t*)key;
		for ( ; *p; ++p ){
			hash = (hash * HASH_MULTIPLIER) + *p;
		}
    } else {
		const uint8_t *p = (const uint8_t*)key, *end = p + keylen;
		for ( ; p!=end; ++p){
			hash = (hash * HASH_MULTIPLIER) + *p;
		}
    }
    return hash;
}

uint32_t HASH_CalcToLower(uint32_t hval, char *result, const str_t *key)
{
    long i;
    for (i=0; i<key->slen; ++i){
        int lower = c_tolower(key->ptr[i]);
		if (result)
			result[i] = (char)lower;

		hval = hval * HASH_MULTIPLIER + lower;
    }

    return hval;
}


T_HashTable* HASH_Create(T_PoolInfo *_pPool, unsigned size)
{
    T_HashTable *h;
    unsigned table_size;
    ASSERT_RETURN(sizeof(T_HashEntry) <= HASH_ENTRY_BUF_SIZE, NULL);

    h = POOL_ALLOC_T(_pPool, T_HashTable);
    h->count = 0;

    ZXY_DLOG(LOG_DEBUG, "hash table %p created from pool %s", h, POOL_GetName(_pPool));
    table_size = 8;
    do {
		table_size <<= 1;    
    }while(table_size < size);
    table_size -= 1;
    
    h->rows = table_size;
    h->table = (T_HashEntry**)POOL_Calloc(_pPool, table_size+1, sizeof(T_HashEntry*));
    return h;
}

static T_HashEntry **find_entry(T_PoolInfo *_pPool, T_HashTable *ht, 
								   const void *key, unsigned keylen,
								   void *val, uint32_t *hval,
								   void *entry_buf, char lower)
{
    uint32_t hash;
    T_HashEntry **p_entry, *entry;
	//1.设定hash(*hval)和keylen值
    if (hval && *hval != 0){
		hash = *hval;
		if (keylen == HASH_KEY_TYPE_STRING){
			keylen = (unsigned)strlen((const char*)key);
		}
    }else{
		hash = 0;
		if (keylen == HASH_KEY_TYPE_STRING){
			const uint8_t *p = (const uint8_t*)key;
			for ( ; *p; ++p ){
				if (lower)
					hash = hash * HASH_MULTIPLIER + c_tolower(*p);
				else 
					hash = hash * HASH_MULTIPLIER + *p;
			}
			keylen = (unsigned)(p - (const unsigned char*)key);
		} else {
			const uint8_t *p = (const uint8_t*)key, *end = p + keylen;
			for ( ; p!=end; ++p){
				if (lower)
					hash = hash * HASH_MULTIPLIER + c_tolower(*p);
				else
					hash = hash * HASH_MULTIPLIER + *p;
			}
		}

		if (hval) *hval = hash;
    }

    for (p_entry = &ht->table[hash & ht->rows], entry = *p_entry; entry; p_entry = &entry->next, entry = *p_entry){
		if (entry->hash == hash && entry->keylen == keylen &&
			((lower && strnicmp((const char*)entry->key,
			(const char*)key, keylen)==0) || (!lower && memcmp(entry->key, key, keylen)==0)))
		{
			break;
		}
    }

    if (entry || val == NULL) return p_entry;

    if (entry_buf){
		entry = (T_HashEntry*)entry_buf;
    } 
	else{
		ASSERT_RETURN(_pPool != NULL, NULL);
		entry = POOL_ALLOC_T(_pPool, T_HashEntry);
		ZXY_DLOG(LOG_DEBUG, "%p: new p_entry %p created, pool used = %zu, cap = %zu", ht, entry,  POOL_GetUsedSize(_pPool), POOL_GetCapacity(_pPool));
    }
    entry->next = NULL;
    entry->hash = hash;
    if (_pPool){
		entry->key = POOL_Alloc(_pPool, keylen);
		memcpy(entry->key, key, keylen);
    }
	else{
		entry->key = (void*)key;
    }
    entry->keylen = keylen;
    entry->value = val;
    *p_entry = entry;
    
    ++ht->count;
    
    return p_entry;
}

void *HASH_Get(T_HashTable *ht, const void *key, unsigned keylen, uint32_t *hval)
{
    T_HashEntry *entry;
    entry = *find_entry(NULL, ht, key, keylen, NULL, hval, NULL, 0);
    return entry ? entry->value : NULL;
}

void *HASH_GetLower(T_HashTable *ht, const void *key, unsigned keylen, uint32_t *hval)
{
    T_HashEntry *entry;
    entry = *find_entry(NULL, ht, key, keylen, NULL, hval, NULL, 1);
    return entry ? entry->value : NULL;
}

static void hash_set(T_PoolInfo *_pPool, T_HashTable *ht, const void *key, unsigned keylen, uint32_t hval, void *value, void *entry_buf, char lower)
{
    T_HashEntry **p_entry;
    p_entry = find_entry(_pPool, ht, key, keylen, value, &hval, entry_buf, lower);
    if (*p_entry){
		if (value == NULL){
			/* delete entry */
			ZXY_DLOG(LOG_DEBUG, "hash table %p: p_entry %p deleted", ht, *p_entry);
			*p_entry = (*p_entry)->next;
			--ht->count;
		} 
		else {
			/* overwrite */
			(*p_entry)->value = value;
			ZXY_DLOG(LOG_DEBUG, "hash table %p: p_entry %p value set to %p", ht, *p_entry, value);
		}
    }
}

void HASH_Set(T_PoolInfo *_pPool, T_HashTable *ht, const void *key, unsigned keylen, uint32_t hval, void *value)
{
    hash_set(_pPool, ht, key, keylen, hval, value, NULL, 0);
}

void HASH_SetLower(T_PoolInfo *_pPool, T_HashTable *ht, const void *key, unsigned keylen, uint32_t hval, void *value)
{
    hash_set(_pPool, ht, key, keylen, hval, value, NULL, 1);
}

void HASH_SetNp(T_HashTable *ht,
			     const void *key, unsigned keylen, 
			     uint32_t hval, pj_hash_entry_buf entry_buf, 
			     void *value)
{
    hash_set(NULL, ht, key, keylen, hval, value, (void *)entry_buf, 0);
}

void HASH_SetNpLower(T_HashTable *ht,
						const void *key, unsigned keylen,
						uint32_t hval,
						pj_hash_entry_buf entry_buf,
						void *value)
{
    hash_set(NULL, ht, key, keylen, hval, value, (void *)entry_buf, 1);
}

unsigned HASH_Count(T_HashTable *ht)
{
    return ht->count;
}

T_HashIterator* HASH_ItFirst(T_HashTable *ht, T_HashIterator *it)
{
    it->index = 0;
    it->entry = NULL;

    for (; it->index <= ht->rows; ++it->index) {
	it->entry = ht->table[it->index];
	if (it->entry) {
	    break;
	}
    }

    return it->entry ? it : NULL;
}

T_HashIterator* HASH_ItNext(T_HashTable *ht, T_HashIterator *it)
{
    it->entry = it->entry->next;
    if (it->entry) return it;

    for (++it->index; it->index <= ht->rows; ++it->index){
		it->entry = ht->table[it->index];
		if (it->entry)
			break;
    }

    return it->entry ? it : NULL;
}

void *HASH_ItThis(T_HashTable *ht, T_HashIterator *it)
{
    CHECK_STACK();
    return it->entry->value;
}

#if 0
void pj_hash_dump_collision( T_HashTable *ht )
{
    unsigned min=0xFFFFFFFF, max=0;
    unsigned i;
    char line[120];
    int len, totlen = 0;

    for (i=0; i<=ht->rows; ++i) {
	unsigned count = 0;    
	T_HashEntry *entry = ht->table[i];
	while (entry) {
	    ++count;
	    entry = entry->next;
	}
	if (count < min)
	    min = count;
	if (count > max)
	    max = count;
	len = pj_snprintf( line+totlen, sizeof(line)-totlen, "%3d:%3d ", i, count);
	if (len < 1)
	    break;
	totlen += len;

	if ((i+1) % 10 == 0) {
	    line[totlen] = '\0';
	    printf(4,(__FILE__, line));
	}
    }

    printf(4,(__FILE__,"Count: %d, min: %d, max: %d\n", ht->count, min, max));
}
#endif


