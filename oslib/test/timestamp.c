#include "timestamp.h"

//时间戳精度
int timestamp_accuracy(void)
{
    T_Timestamp freq, t1, t2;
    T_Timevalue tv1, tv2, tvtmp;
    int64_t msec, tics;
    int64_t diff;

    printf("testing frequency accuracy (pls wait)\r\n");

    TIME_GetTimestampfreq(&freq);

    /* 开始 */
    TIME_GetTimevalue(&tvtmp);
    do {
		TIME_GetTimevalue(&tv1);
    } while (TIME_VAL_EQ(tvtmp, tv1));
    TIME_GetTimestamp(&t1);

    /* Sleep for 5 seconds */
	sleep(5);

    /* 结束 */
    TIME_GetTimevalue(&tvtmp);
    do {
		TIME_GetTimevalue(&tv2);
    } while (TIME_VAL_EQ(tvtmp, tv2));
    TIME_GetTimestamp(&t2);

    /* 计算 */
	printf("st = %ld.%03ld, et = %ld.%03ld, ", tv1.sec, tv1.msec, tv2.sec, tv2.msec);
    TIME_VAL_SUB(tv2, tv1);
    msec = TIME_VAL_MSEC(tv2);
	printf("delta = %ld.%03ld(%ldms)\r\n", tv2.sec, tv2.msec, msec);
	
    /* Check that the frequency match the elapsed time */
    tics = t2.u64 - t1.u64;
    diff = tics - (msec * freq.u64 / 1000);
    if (diff < 0)
		diff = -diff;

    /* Only allow 1 msec mismatch */
    if (diff > (int64_t)(freq.u64 / 1000)) {
		printf("error: timestamp drifted by %d usec after %d msec\r\n", (uint32_t)(diff * 1000000 / freq.u64), msec);
		return -2000;
    /* Otherwise just print warning if timestamp drifted by >1 usec */
    } else if (diff > (int64_t)(freq.u64 / 1000000)) {
		printf("warning: timestamp drifted by %d usec after %d msec\r\n",(uint32_t)(diff * 1000000 / freq.u64), msec);
    } else {
		printf("good. Timestamp is accurate down to nearest usec.\r\n");
    }

    return 0;
}


int timestamp_test0(void)
{
    enum { CONSECUTIVE_LOOP = 100 };
    volatile unsigned i;
    T_Timestamp freq, t1, t2;
    T_Timevalue tv1, tv2;
    unsigned elapsed;
    int rc;

    printf("Testing timestamp (high res time)\r\n");
    
    //得到显示频率
    if ((rc = TIME_GetTimestampfreq(&freq)) != 0) {
		printf("ERROR: get timestamp freq\r\n", rc);
		return -1000;
    }

    printf("frequency: hiword = %lu loword = %lu\r\n", freq.u32.hi, freq.u32.lo);
    printf("checking if time can run backwards (pls wait)..\r\n");

    rc |= TIME_GetTimestamp(&t1);
	rc |= TIME_GetTimevalue(&tv1);
    if (rc != 0) {
		printf("ERROR: TIME_GetTimestamp\r\n");
		return -1001;
    }

    for (i = 0; i < CONSECUTIVE_LOOP; ++i) {
        //pj_thread_sleep(pj_rand() % 100);
		int r = rand() % 100;
		usleep(r * 1000);
		rc = TIME_GetTimestamp(&t2);
		rc |= TIME_GetTimevalue(&tv2);
		if (rc != 0) {
			printf("ERROR: TIME_GetTimestamp\r\n");
			return -1003;
		}

		//比较 t2，t1, 希望 t2 >= t1.
		if (t2.u32.hi < t1.u32.hi ||(t2.u32.hi == t1.u32.hi && t2.u32.lo < t1.u32.lo))
		{
			printf("...ERROR: timestamp run backwards!\r\n");
			return -1005;
		}

		//比较 tv2 ， tv1, 希望 tv2 >= tv1.
		if (TIME_VAL_LT(tv2, tv1)) {
			printf("...ERROR: time run backwards!\r\n");
			return -1006;
		}
    }

	//测试循环
    printf("testing simple 1000000 loop\r\n");

	TIME_GetTimestamp(&t1);

    for (i = 0; i < 1000000; ++i) {
		//null_func();
    }
	
    TIME_GetTimestamp(&t2);

    /* 计算消耗时间 usec. */
    elapsed = TIME_ElapsedUsec(&t1, &t2);
    printf("....elapsed: %u usec\r\n", (unsigned)elapsed);

    /* See if elapsed time is "reasonable". 
     * This should be good even on 50Mhz embedded powerpc.
     */
    if (elapsed < 1 || elapsed > 1000000) {
		printf("....error: elapsed time outside window (%u, "
					 "t1.u32.hi=%u, t1.u32.lo=%u, "
					 "t2.u32.hi=%u, t2.u32.lo=%u)\r\n",
					 elapsed, 
					 t1.u32.hi, t1.u32.lo, t2.u32.hi, t2.u32.lo);
		return -1030;
    }

    return 0;
}


int timestamp_test(void)
{
	int rc;

	THREAD_Init();
	printf("======================================================\r\n");
	if (rc = timestamp_test0()){
		printf("FAILED[%d]\r\n", rc);
	}
	
	printf("======================================================\r\n");
	if (rc = timestamp_accuracy()){
		printf("FAILED[%d]\r\n", rc);
	}	
	
    return 0;
}

