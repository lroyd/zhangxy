#include "hash.h"
#include "console.h"

#define HASH_COUNT  31

static int hash_test_with_key(T_PoolInfo *pool, unsigned char key)
{
    T_HashTable *ht;
    unsigned value = 0x12345;
    T_HashIterator it_buf, *it;
    unsigned *entry;

    ht = HASH_Create(pool, HASH_COUNT);
    if (!ht) return -10;

    HASH_Set(pool, ht, &key, sizeof(key), 0, &value);

    entry = (unsigned*) HASH_Get(ht, &key, sizeof(key), NULL);
    if (!entry) return -20;

    if (*entry != value) return -30;

    if (HASH_Count(ht) != 1) return -30;

    it = HASH_ItFirst(ht, &it_buf);
    if (it == NULL) return -40;

    entry = (unsigned*) HASH_ItThis(ht, it);
    if (!entry) return -50;

    if (*entry != value) return -60;

    it = HASH_ItNext(ht, it);
    if (it != NULL) return -70;

    /* Erase item */
    HASH_Set(NULL, ht, &key, sizeof(key), 0, NULL);

    if (HASH_Get(ht, &key, sizeof(key), NULL) != NULL) return -80;

    if (HASH_Count(ht) != 0) return -90;

    it = HASH_ItFirst(ht, &it_buf);
    if (it != NULL) return -100;

    return 0;
}

static int hash_collision_test(T_PoolInfo *pool)
{
    enum {
		COUNT = HASH_COUNT * 4		//31 * 4
    };
    T_HashTable *ht;
    T_HashIterator it_buf, *it;
    unsigned char *values;
    unsigned i;

    ht = HASH_Create(pool, HASH_COUNT);
    if (!ht) return -200;

    values = (unsigned char*) POOL_Alloc(pool, COUNT);

    for (i = 0; i < COUNT; ++i){
		values[i] = (unsigned char)i;
		HASH_Set(pool, ht, &i, sizeof(i), 0, &values[i]);
    }

    if (HASH_Count(ht) != COUNT) return -210;

    for (i = 0; i < COUNT; ++i){
		unsigned char *entry;
		entry = (unsigned char*) HASH_Get(ht, &i, sizeof(i), NULL);
		if (!entry)
			return -220;
		if (*entry != values[i])
			return -230;
    }

    i = 0;
    it = HASH_ItFirst(ht, &it_buf);
    while (it) {
		++i;
		it = HASH_ItNext(ht, it);
    }

    if (i != COUNT) return -240;

    return 0;
}

int hash_test(void)
{
	THREAD_Init();
    T_PoolInfo *pool;
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);	
    pool = POOL_Create(&cp.in_pFactory, "hash", 512, 512, NULL);		

    int rc;
    unsigned i;

    for (i = 0; i <= HASH_COUNT; ++i){
		rc = hash_test_with_key(pool, (unsigned char)i);
		if (rc != 0){
			POOL_Destroy(pool);
			return rc;
		}
    }

    rc = hash_collision_test(pool);
    if (rc != 0){
		POOL_Destroy(pool);
		return rc;
    }

    POOL_Destroy(pool);
    return 0;
}

