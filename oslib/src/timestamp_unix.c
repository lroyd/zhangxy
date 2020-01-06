/*******************************************************************************
	> File Name: .c
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include "timestamp.h"

#define USEC_PER_SEC	(1000000)			//1000 * 1000

int TIME_GetTimestamp(T_Timestamp *ts)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) return -1;

    ts->u64 = tv.tv_sec;
    ts->u64 *= USEC_PER_SEC;
    ts->u64 += tv.tv_usec;
    return 0;
}

int TIME_GetTimestampfreq(T_Timestamp *_pFreq)
{
    _pFreq->u32.hi = 0;
    _pFreq->u32.lo = USEC_PER_SEC;
    return 0;
}

int TIME_GetTimevalue(T_Timevalue *_pTimeValue)
{
    struct timeval tTime;
    if (gettimeofday(&tTime, NULL)) return -1;

    _pTimeValue->sec	= tTime.tv_sec;
    _pTimeValue->msec	= tTime.tv_usec / 1000;
    return 0;
}







