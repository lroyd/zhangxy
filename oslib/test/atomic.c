#include "os.h"


//原子锁
int simple_atomic_test(T_PoolInfo *pool)
{
    T_Atomic *atomic_var;
    int rc;

    /* create() */
    rc = ATOMIC_Create(pool, 111, &atomic_var);
    if (rc != 0) {
        return -20;
    }

    /* get: check the value. */
    if (ATOMIC_Get(atomic_var) != 111)
        return -30;

    /* increment. */
    ATOMIC_Inc(atomic_var);
    if (ATOMIC_Get(atomic_var) != 112)
        return -40;

    /* decrement. */
    ATOMIC_Dec(atomic_var);
    if (ATOMIC_Get(atomic_var) != 111)
        return -50;

    /* set */
    ATOMIC_Set(atomic_var, 211);
    if (ATOMIC_Get(atomic_var) != 211)
        return -60;

    /* add */
    ATOMIC_Add(atomic_var, 10);
    if (ATOMIC_Get(atomic_var) != 221)
        return -60;

    /* check the value again. */
    if (ATOMIC_Get(atomic_var) != 221)
        return -70;

    /* destroy */
    rc = ATOMIC_Destroy(atomic_var);
    if (rc != 0)
        return -80;

    return 0;
}

int atomic_test(void)
{
	int rc;
    T_PoolInfo *pool;
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);	
    pool = POOL_Create(&cp.in_pFactory, "", 4000, 4000, NULL);
	THREAD_Init();
	printf("======================================================\r\n");
	if (rc = simple_atomic_test(pool)){
		printf("FAILED[%d]\r\n", rc);
	}
	
    return 0;
}


