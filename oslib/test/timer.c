
#include "os.h"
#include "timestamp.h"
#include "timer.h"

#define LOOP		16
#define MIN_COUNT	250
#define MAX_COUNT	(LOOP * MIN_COUNT)
#define MIN_DELAY	2
#define	D		(MAX_COUNT / 32000)
#define DELAY		(D < MIN_DELAY ? MIN_DELAY : D)
#define THIS_FILE	"timer_test"


static void timer_callback(T_TimerHeap *ht, T_TimerEntity *e)
{
	printf("timer_callback entry = %p, id = %d\r\n", e, e->m_u32UsrId);
}

static int test_timer_heap(T_PoolFactory *pfactory)
{
    int i, j;
    T_TimerEntity *entry;
    T_PoolInfo *pool;
    T_TimerHeap *timer;
    T_Timevalue delay;
    int status;
    int err=0;
    size_t size;
    unsigned count;

    size = TIMER_HeapEstimateSize(MAX_COUNT) + MAX_COUNT * sizeof(T_TimerEntity);

    pool = POOL_Create(pfactory, NULL, size, 4000, NULL);
    if (!pool) {
		printf("...error: unable to create pool of %u bytes\r\n", size);
		return -10;
    }

    entry = (T_TimerEntity*)POOL_Calloc(pool, MAX_COUNT, sizeof(*entry));
    if (!entry) return -20;

    for (i=0; i < MAX_COUNT; ++i) {
		entry[i].funcHandle = &timer_callback;
    }
	
    status = TIMER_HeapCreate(pool, MAX_COUNT, &timer);
    if (status != 0) {
        printf("...error: unable to create timer heap\r\n");
		return -30;
    }
	printf("delay = %d\r\n", DELAY);
    count = MIN_COUNT;
    for (i = 0; i < LOOP ; ++i) 	
	{
		int early = 0;
		int done=0;
		int cancelled=0;
		int rc;
		T_Timestamp t1, t2, t_sched, t_cancel, t_poll;
		T_Timevalue now, expire;

		TIME_GetTimevalue(&now);
		srand(now.sec);
		t_sched.u32.lo = t_cancel.u32.lo = t_poll.u32.lo = 0;

		//生成随机时间(DELAY),注册定时器
		for (j=0; j<(int)count; ++j) {
			delay.sec = rand() % DELAY;
			delay.msec = rand() % 1000;
			//调度定时器，堆排序
			TIME_GetTimestamp(&t1);
			rc = TIMER_HeapSchedule(timer, &entry[j], &delay);
			if (rc != 0)
				return -40;
			TIME_GetTimestamp(&t2);

			t_sched.u32.lo += (t2.u32.lo - t1.u32.lo);

			//检查是否有到时的
			TIME_GetTimestamp(&t1);
			rc = TIMER_HeapPoll(timer, NULL);
			TIME_GetTimestamp(&t2);
			if (rc > 0) {
				t_poll.u32.lo += (t2.u32.lo - t1.u32.lo);
				early += rc;
			}
		}

		// Set the time where all timers should finish
		TIME_GetTimevalue(&expire);
		delay.sec = DELAY; 
		delay.msec = 0;
		TIME_VAL_ADD(expire, delay);

		// Wait unfil all timers finish, cancel some of them.
		do {
			int index = rand() % count;
			TIME_GetTimestamp(&t1);
			rc = TIMER_EntityCancel(timer, &entry[index]);
			TIME_GetTimestamp(&t2);
			if (rc > 0) {
				cancelled += rc;
				t_cancel.u32.lo += (t2.u32.lo - t1.u32.lo);
			}

			TIME_GetTimevalue(&now);

			TIME_GetTimestamp(&t1);
			rc = TIMER_HeapPoll(timer, NULL);
			TIME_GetTimestamp(&t2);
			if (rc > 0) {
				done += rc;
				t_poll.u32.lo += (t2.u32.lo - t1.u32.lo);
			}

		}while (TIME_VAL_LTE(now, expire) && TIMER_HeapGetCount(timer) > 0);

		if (TIMER_HeapGetCount(timer)) {
			printf("ERROR: %d timers left\r\n", TIMER_HeapGetCount(timer));
			++err;
		}
		
		printf("%d. [%d] sched = %d, poll = %d, cancel = %d\r\n", i, count, t_sched.u32.lo, t_poll.u32.lo, t_cancel.u32.lo);

		t_sched.u32.lo /= count; 
		t_cancel.u32.lo /= count;
		t_poll.u32.lo /= count;
		printf("...ok (count:%d, early:%d, sched:%d, cancelled:%d, done:%d, cancel:%d poll:%d)\r\n", count, early, t_sched.u32.lo, cancelled, done, t_cancel.u32.lo, t_poll.u32.lo);
		count = count * 2;
		if (count > MAX_COUNT){
			printf("===== count %d(%d), quit\r\n", count, MAX_COUNT);
			break;
		}
			
    }

    POOL_Destroy(pool);
    return err;
}



static void timer_callback2(T_TimerHeap *ht, T_TimerEntity *e)
{
	printf("timer_callback entry: %p, id = %d\r\n", e, e->m_u32UsrId);
	T_Timevalue delay = {1,0};
	TIMER_HeapSchedule(ht, e, &delay);
}

static int test_timer_heap2(T_PoolFactory *pfactory)
{
	int rc, status;
    T_TimerEntity *entry;
    T_PoolInfo *pool;
    T_TimerHeap *timer;
	size_t size;
	
    size = TIMER_HeapEstimateSize(MAX_COUNT) + MAX_COUNT * sizeof(T_TimerEntity);

    pool = POOL_Create(pfactory, NULL, size, 4000, NULL);
    if (!pool) {
		printf("...error: unable to create pool of %u bytes\r\n", size);
		return -10;
    }
	
    status = TIMER_HeapCreate(pool, MAX_COUNT, &timer);
    if (status != 0) {
        printf("...error: unable to create timer heap\r\n");
		return -30;
    }

	entry = (T_TimerEntity*)POOL_Calloc(pool, 1, sizeof(*entry));
	TIMER_EntityInit(entry, 2, NULL, timer_callback2);

	T_Timevalue delay = {3,0}, next;
	TIMER_HeapSchedule(timer, entry, &delay);
	
	TIMER_HeapDump(timer);
	while(1)
	{
		rc = TIMER_HeapPoll(timer, &next);
		if (rc > 0) {
			printf("call bck\r\n");
		}
		else if (rc == 0)
		{
			//printf("timer heap poll next = {%d, %d}\r\n", next.sec,next.msec);
		}
		
	}
	




    POOL_Destroy(pool);
    return 0;
}

int timer_test()
{
	int rc;
	OSLIB_Init();
	
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);			
	
	
	printf("======================================================\r\n");
	if (rc = test_timer_heap2(&cp.in_pFactory)){
		printf("FAILED[%d]\r\n", rc);
	}	
    return 0;
}



