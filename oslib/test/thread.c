#include "os.h"

static volatile int quit_flag=0;


//子线程100ms增加计数
static void *thread_proc(uint32_t *pcounter)
{
    T_ThreadDesc desc;
    T_ThreadInfo *this_thread;
    unsigned id;
    int rc;

    id = *pcounter;
    printf("sub thread %d running..\r\n", id);

    memset(desc, 0, sizeof(desc));

    rc = THREAD_Register("thread", desc, &this_thread);
    if (rc != 0) {
        printf("...error in THREAD_Register\r\n");
        return NULL;
    }

    this_thread = THREAD_This();
    if (this_thread == NULL) {
        printf("error: THREAD_This() returns NULL!\r\n");
        return NULL;
    }

    if (THREAD_GetName(this_thread) == NULL) {
        printf("error: THREAD_GetName() returns NULL!\r\n");
        return NULL;
    }

    for (;!quit_flag;) {
		(*pcounter)++;
		//THREAD_MSleep(100);	//Must sleep if platform doesn't do time-slicing.
    }

    printf("sub thread %d quitting..\r\n", id);
    return NULL;
}

//主线程检查计数是否等于休眠的s数
static int simple_thread(T_PoolFactory *pfactory, const char *title, unsigned flags)
{
    T_PoolInfo *pool;
    T_ThreadInfo *thread;
    int rc;
    uint32_t counter = 0;

    printf("[%s]\r\n", title);

    pool = POOL_Create(pfactory, NULL, 4000, 4000, NULL);
    if (!pool) return -1000;

    quit_flag = 0;

    printf("Creating sub thread\r\n");
    rc = THREAD_Create(pool, "thread", (THREAD_PROC*)&thread_proc,
						  &counter,
						  ZXY_SET_THREAD_DEF_STACK_SIZE,
						  flags,
						  &thread);

    if (rc != 0) {
		printf("error: unable to create thread, code = %d\r\n", rc);
		return -1010;
    }

    printf("Main thread waiting 2s\r\n");
    THREAD_MSleep(2000);
    printf("Main thread resuming\r\n");

    if (flags & THREAD_FLAG_SUSPENDED) {
		if (counter != 0) {
		    printf("...error: thread is not suspended\r\n");
		    return -1015;
		}

		rc = THREAD_Resume(thread);
		if (rc != 0) {
		    printf("...error: resume thread error\r\n");
		    return -1020;
		}
    }

    printf("Waiting for sub thread to quit 1.5s\r\n");

    THREAD_MSleep(1500);
    quit_flag = 1;
    THREAD_Join(thread);

    POOL_Destroy(pool);

    if (counter == 0) {
		printf("error: thread is not running\r\n");
		return -1025;
    }
	printf("Check counter = %d\r\n", counter);
    printf("[%s] test success\r\n", title);
    return 0;
}


static int timeslice_test(T_PoolFactory *pfactory)
{
    enum {NUM_THREADS = 4};
    T_PoolInfo *pool;
    uint32_t counter[NUM_THREADS], lowest, highest, diff;
    T_ThreadInfo *thread[NUM_THREADS];
    unsigned i;
    int rc;

    quit_flag = 0;

    pool = POOL_Create(pfactory, NULL, 4000, 4000, NULL);
    if (!pool) return -10;

    printf("[timeslice testing] with %d threads\r\n", NUM_THREADS);

    for (i = 0; i < NUM_THREADS; ++i) {
        counter[i] = i;
        rc = THREAD_Create(pool, "thread", (THREAD_PROC*)&thread_proc,
								&counter[i],
								ZXY_SET_THREAD_DEF_STACK_SIZE,
								THREAD_FLAG_SUSPENDED,
								&thread[i]);
        if (rc != 0) {
            printf("ERROR in THREAD_Create()\r\n");
            return -20;
        }
    }

    printf("Main thread waiting (1s)...\r\n");
    THREAD_MSleep(1000);	//1000 * 1000 = 1s
    printf("Main thread resuming\r\n");

    for (i = 0; i < NUM_THREADS; ++i) {
        if (counter[i] > i) {
            printf("ERROR! Thread %d-th is not suspended!\r\n",i);
            return -30;
        }
    }

    for (i = 0; i < NUM_THREADS; ++i) {
	printf("Resuming thread %d [%p]..\r\n", i, thread[i]);
        rc = THREAD_Resume(thread[i]);
        if (rc != 0) {
            printf("ERROR in THREAD_Resume()\r\n");
            return -40;
        }
    }

    printf("Main thread waiting (5s)...\r\n");
    THREAD_MSleep(5000);
    printf("Main thread resuming\r\n");

    quit_flag = 1;

    for (i = 0; i < NUM_THREADS; ++i) {
		printf("Main thread joining thread %d [%p]..\r\n",i, thread[i]);
		rc = THREAD_Join(thread[i]);
		if (rc != 0) {
			printf("ERROR in THREAD_Join()\r\n");
			return -50;
		}
		printf("Destroying thread %d [%p]..\r\n", i, thread[i]);
		rc = THREAD_Destroy(thread[i]);
		if (rc != 0) {
			printf("ERROR in THREAD_Destroy()\r\n");
			return -60;
		}
    }

    printf("Main thread calculating time slices..\r\n");

    lowest = 0xFFFFFFFF;
    highest = 0;
    for (i = 0; i < NUM_THREADS; ++i) {
		printf("Check counter[%d] = %d\r\n", i, counter[i]);
        if (counter[i] < lowest) lowest = counter[i];
        if (counter[i] > highest) highest = counter[i];
    }

    if (lowest < 2) {
        printf("ERROR: not all threads were running!\r\n");
        return -70;
    }

    //最低值和最高值之间的差异应小于50%
    diff = (highest - lowest)*100 / ((highest + lowest)/2);
    if (diff >= 50) {
        printf("ERROR: thread didn't have equal timeslice!\r\n");
        printf("lowest counter = %u, highest counter=%u, diff=%u%%\r\n",lowest, highest, diff);
        return -80;
    } else {
        printf("info: timeslice diff between lowest & highest = %u%%\r\n",diff);
    }

    POOL_Destroy(pool);
    return 0;
}

#if 0
static pthread_key_t str_key;
//define a static variable that only be allocated once
static pthread_once_t str_alloc_key_once=PTHREAD_ONCE_INIT;
static void str_alloc_key();
static void str_alloc_destroy_accu(void* accu);

char* str_accumulate(const char* s)
{    char* accu;
    
    pthread_once(&str_alloc_key_once,str_alloc_key);//str_alloc_key()这个函数只调用一次
    accu=(char*)pthread_getspecific(str_key);//取得该线程对应的关键字所关联的私有数据空间首址
    if(accu==NULL)//每个新刚创建的线程这个值一定是NULL（没有指向任何已分配的数据空间）
    {    accu=malloc(1024);//用上面取得的值指向新分配的空间
        if(accu==NULL)    return NULL;
        accu[0]=0;//为后面strcat()作准备
      
        pthread_setspecific(str_key,(void*)accu);//设置该线程对应的关键字关联的私有数据空间
        printf("Thread %lx: allocating buffer at %p\n",pthread_self(),accu);
     }
     strcat(accu,s);
     return accu;
}
//设置私有数据空间的释放内存函数
static void str_alloc_key()
{    pthread_key_create(&str_key,str_alloc_destroy_accu);/*创建关键字及其对应的内存释放函数，当进程创建关键字后，这个关键字是NULL。之后每创建一个线程os都会分给一个对应的关键字，关键字关联线程私有数据空间首址，初始化时是NULL*/
    printf("Thread %lx: allocated key %d\n",pthread_self(),str_key);
}
/*线程退出时释放私有数据空间,注意主线程必须调用pthread_exit()(调用exit()不行)才能执行该函数释放accu指向的空间*/
static void str_alloc_destroy_accu(void* accu)
{    printf("Thread %lx: freeing buffer at %p\n",pthread_self(),accu);
    free(accu);
}
//线程入口函数
void* process(void *arg)
{    char* res;
    res=str_accumulate("Resule of ");
    if(strcmp((char*)arg,"first")==0)
        sleep(3);
    res=str_accumulate((char*)arg);
    res=str_accumulate(" thread");
    printf("Thread %lx: \"%s\"\n",pthread_self(),res);
    return NULL;
}
//主线程函数
int tmain(void)
{    char* res;
    pthread_t th1,th2;
    res=str_accumulate("Result of ");
    pthread_create(&th1,NULL,process,(void*)"first");
    pthread_create(&th2,NULL,process,(void*)"second");
    res=str_accumulate("initial thread");
    printf("Thread %lx: \"%s\"\n",pthread_self(),res);
    pthread_join(th1,NULL);
    pthread_join(th2,NULL);
    pthread_exit(0);
}
#endif


int thread_test(void)
{
    int rc;
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);	
	
	//tmain();
	OSLIB_Init();
	
#if 1	
	printf("======================================================\r\n");
	if (rc = simple_thread(&cp.in_pFactory, "simple thread test", 0)){
		printf("FAILED[%d]\r\n", rc);
	}
	printf("======================================================\r\n");
	if (rc = simple_thread(&cp.in_pFactory, "suspended thread test", THREAD_FLAG_SUSPENDED)){
		printf("FAILED[%d]\r\n", rc);
	}
	printf("======================================================\r\n");
	if (rc = timeslice_test(&cp.in_pFactory)){
		printf("FAILED[%d]\r\n", rc);
	}
#endif	
    return 0;
}



