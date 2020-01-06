/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include "pool.h"


static void pool_demo_1(T_PoolFactory *pfactory)
{
	unsigned i;
	T_PoolInfo *pool;

	// Must create pool before we can allocate anything
	pool = POOL_Create(pfactory,	 // the factory
				  "test",	 // pool's name
				  4000,	 // initial size
				  4000,	 // increment size
				  NULL);	 // use default callback.
	if (pool == NULL) {
		printf("Error creating pool\r\n");
		return;
	}

	// Demo: allocate some memory chunks
	for (i=0; i<20; ++i) {
		void *p;
		int len = (rand()+1) % 512;
		//printf("allocate memory chunks size = %d\r\n", len);
		p = POOL_Alloc(pool, len);

		// Do something with p
		//...

		// Look! No need to free p!!
	}
	POOL_FactoryDump(pfactory);
	// Done with silly demo, must free pool to release all memory.
	POOL_Destroy(pool);
	
	POOL_FactoryDump(pfactory);
}




int pool_test()
{
	OSLIB_Init();
	
	
	T_PoolCaching cp;
	int status;

	POOL_CachingInit(&cp, NULL, 0);

	pool_demo_1(&cp.in_pFactory);

	POOL_CachingDeinit(&cp);

	return 0;
}