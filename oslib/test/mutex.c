#include "os.h"


//互斥锁
static int simple_mutex_test(T_PoolInfo *pool)
{
    int rc;
    T_Mutex *mutex;

    printf("testing simple mutex\r\n");
    
    printf("create mutex\r\n");
    rc = MUTEX_Create(pool, "simple", MUTEX_TYPE_SIMPLE, &mutex);
    if (rc != 0) {
		printf("error: MUTEX_Create\r\n");
		return -10;
    }

    printf("lock mutex\r\n");
    rc = MUTEX_Lock(mutex);
    if (rc != 0) {
		printf("error: MUTEX_Lock\r\n");
		return -20;
    }
    printf("unlock mutex\r\n");
    rc = MUTEX_Unlock(mutex);
    if (rc != 0) {
		printf("error: MUTEX_Unlock\r\n");
		return -30;
    }
    
    printf("lock mutex\r\n");
    rc = MUTEX_Lock(mutex);
    if (rc != 0) return -40;

    printf("trylock mutex\r\n");
    rc = MUTEX_Trylock(mutex);
    if (rc == 0)
		printf("info: looks like simple mutex is recursive\r\n");
	else
		printf("trylock failed\r\n");
	
    printf("unlock mutex\r\n");
    rc = MUTEX_Unlock(mutex);
    if (rc != 0) return -50;

    printf("destroy mutex\r\n");
    rc = MUTEX_Destroy(mutex);
    if (rc != 0) return -60;

    printf("done\r\n");
    return 0;
}

//递归锁
static int recursive_mutex_test(T_PoolInfo *pool)
{
    int rc;
    T_Mutex *mutex;

    printf("testing recursive mutex\r\n");

    printf("create mutex\r\n");
    rc = MUTEX_Create( pool, "recursive", MUTEX_TYPE_RECURSE, &mutex);
    if (rc != 0) {
		printf("error: MUTEX_Create\r\n");
		return -10;
    }

    printf("lock mutex\r\n");
    rc = MUTEX_Lock(mutex);
    if (rc != 0) {
		printf("error: MUTEX_Lock\r\n");
		return -20;
    }
    printf("unlock mutex\r\n");
    rc = MUTEX_Unlock(mutex);
    if (rc != 0) {
		printf("...error: MUTEX_Unlock\r\n");
		return -30;
    }
    
    printf("lock mutex\r\n");
    rc = MUTEX_Lock(mutex);
    if (rc != 0) return -40;

    printf("trylock mutex\r\n");
    rc = MUTEX_Trylock(mutex);
    if (rc != 0) {
		printf("error: recursive mutex is not recursive!\r\n");
		return -40;
    }

    printf("lock mutex\r\n");
    rc = MUTEX_Lock(mutex);
    if (rc != 0) {
		printf("error: recursive mutex is not recursive!\r\n");
		return -45;
    }

    /* Unlock several times and done. */
    printf("unlock mutex 3x\r\n");
    rc = MUTEX_Unlock(mutex);
    if (rc != 0) return -50;
    rc = MUTEX_Unlock(mutex);
    if (rc != 0) return -51;
    rc = MUTEX_Unlock(mutex);
    if (rc != 0) return -52;

    printf("destroy mutex\r\n");
    rc = MUTEX_Destroy(mutex);
    if (rc != 0) return -60;

    printf("....done\r\n");
    return 0;
}


int mutex_test(void)
{
	int rc;
    T_PoolInfo *pool;
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);	
    pool = POOL_Create(&cp.in_pFactory, "", 4000, 4000, NULL);

	THREAD_Init();
	printf("======================================================\r\n");
	if (rc = simple_mutex_test(pool)){
		printf("FAILED[%d]\r\n", rc);
	}
	
	printf("======================================================\r\n");
	if (rc = recursive_mutex_test(pool)){
		printf("FAILED[%d]\r\n", rc);
	}

    return 0;
}


