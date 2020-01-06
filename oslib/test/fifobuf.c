#include "fifobuf.h"
#include "pool.h"


int fifobuf_test()
{
    enum{ 
		SIZE		= 1024, 
		MAX_ENTRIES = 12, 
		MIN_SIZE	= 4, 
		MAX_SIZE	= 64, 
		LOOP		= 10
	};
    T_Fifobuf fifo;
    unsigned available = SIZE;
    void *entries[MAX_ENTRIES];
    void *buffer;
    int i;	
	THREAD_Init();
    T_PoolInfo *pool;
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);	
    pool = POOL_Create(&cp.in_pFactory, "", SIZE + 256, 0, NULL);	
	
    buffer = POOL_Alloc(pool, SIZE);
    if (!buffer) return -20;

    FIFO_Init(&fifo, buffer, SIZE);
    
    for (i = 0; i < LOOP*MAX_ENTRIES; ++i){
		int size;
		int c, f;
		c = i%2;
		f = (i+1)%2;
		do {
		    size = MIN_SIZE + (rand() % MAX_SIZE);
		    entries[c] = FIFO_Alloc(&fifo, size);
		}while (entries[c] == 0);
		
		if (i!=0){
		    FIFO_Free(&fifo, entries[f]);
		}
    }
    
    if (entries[(i+1)%2])
		FIFO_Free(&fifo, entries[(i+1)%2]);

    if (FIFO_GetMaxSize(&fifo) < SIZE-4){
		assert(0);
		return -1;
    }

    // Test 2
    entries[0] = FIFO_Alloc(&fifo, MIN_SIZE);
    if (!entries[0]) return -1;
    
    for (i=0; i<LOOP*MAX_ENTRIES; ++i){
		int size = MIN_SIZE + (rand() % MAX_SIZE);
		entries[1] = FIFO_Alloc(&fifo, size);
		if (entries[1])
		    FIFO_Unalloc(&fifo, entries[1]);
    }
    FIFO_Unalloc(&fifo, entries[0]);
    
    if (FIFO_GetMaxSize(&fifo) < SIZE-4){
		assert(0);
		return -2;
    }

    // Test 3
    for (i = 0; i < LOOP; ++i){
		int count, j;
		for (count=0; available >= MIN_SIZE+4 && count < MAX_ENTRIES;){
		    int size = MIN_SIZE +( rand() % MAX_SIZE);
		    entries[count] = FIFO_Alloc(&fifo, size);
		    if (entries[count]){
				available -= (size + 4);
				++count;
		    }
		}
		for (j = 0; j < count; ++j){
		    FIFO_Free(&fifo, entries[j]);
		}
		available = SIZE;
    }

    if (FIFO_GetMaxSize(&fifo) < SIZE-4){
		assert(0);
		return -3;
    }
	
    POOL_Destroy(pool);
    return 0;
}




