#include <stdio.h>

#include "timestamp.h"

#define U32MAX  (0xFFFFFFFFUL)
#define NANOSEC (1000000000UL)
#define USEC    (1000000UL)
#define MSEC    (1000)

#define u64tohighprec(u64)	((highprec_t)((int64_t)(u64)))


static highprec_t get_elapsed(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
#if defined(ZXY_HAS_TIME_INT64) && ZXY_HAS_TIME_INT64!=0
    return u64tohighprec(_pStop->u64 - _pStart->u64);
#else
    highprec_t elapsed_hi, elapsed_lo;

    elapsed_hi = _pStop->u32.hi - _pStart->u32.hi;
    elapsed_lo = _pStop->u32.lo - _pStart->u32.lo;

    /* elapsed_hi = elapsed_hi * U32MAX */
    HIGHPREC_MUL(elapsed_hi, U32MAX);

    return elapsed_hi + elapsed_lo;
#endif
}

static highprec_t elapsed_msec(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
    T_Timestamp ts_freq;
    highprec_t freq, elapsed;

    if (TIME_GetTimestampfreq(&ts_freq) != 0) return 0;

    /* Convert frequency timestamp */
#if defined(ZXY_HAS_TIME_INT64) && ZXY_HAS_TIME_INT64!=0
    freq = u64tohighprec(ts_freq.u64);
#else
    freq = ts_freq.u32.hi;
    HIGHPREC_MUL(freq, U32MAX);
    freq += ts_freq.u32.lo;
#endif

    /* Avoid division by zero. */
    if (freq == 0) freq = 1;

    /* Get elapsed time in cycles. */
    elapsed = get_elapsed(_pStart, _pStop);

    /* usec = elapsed * MSEC / freq */
    HIGHPREC_MUL(elapsed, MSEC);
    HIGHPREC_DIV(elapsed, freq);

    return elapsed;
}

static highprec_t elapsed_usec(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
    T_Timestamp ts_freq;
    highprec_t freq, elapsed;

    if (TIME_GetTimestampfreq(&ts_freq) != 0) return 0;

    /* Convert frequency timestamp */
#if defined(ZXY_HAS_TIME_INT64) && ZXY_HAS_TIME_INT64!=0
    freq = u64tohighprec(ts_freq.u64);
#else
    freq = ts_freq.u32.hi;
    HIGHPREC_MUL(freq, U32MAX);
    freq += ts_freq.u32.lo;
#endif

    /* Avoid division by zero. */
    if (freq == 0) freq = 1;

    /* Get elapsed time in cycles. */
    elapsed = get_elapsed(_pStart, _pStop);

    /* usec = elapsed * USEC / freq */
    HIGHPREC_MUL(elapsed, USEC);
    HIGHPREC_DIV(elapsed, freq);

    return elapsed;
}



uint32_t TIME_ElapsedNanosec(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
    T_Timestamp ts_freq;
    highprec_t freq, elapsed;

    if (TIME_GetTimestampfreq(&ts_freq) != 0)
        return 0;

    /* Convert frequency timestamp */
#if defined(ZXY_HAS_TIME_INT64) && ZXY_HAS_TIME_INT64!=0
    freq = u64tohighprec(ts_freq.u64);
#else
    freq = ts_freq.u32.hi;
    HIGHPREC_MUL(freq, U32MAX);
    freq += ts_freq.u32.lo;
#endif

    /* Avoid division by zero. */
    if (freq == 0) freq = 1;

    /* Get elapsed time in cycles. */
    elapsed = get_elapsed(_pStart, _pStop);

    /* usec = elapsed * USEC / freq */
    HIGHPREC_MUL(elapsed, NANOSEC);
    HIGHPREC_DIV(elapsed, freq);

    return (uint32_t)elapsed;
}

uint32_t TIME_ElapsedUsec(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
    return (uint32_t)elapsed_usec(_pStart, _pStop);
}

uint32_t TIME_ElapsedMsec(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
    return (uint32_t)elapsed_msec(_pStart, _pStop);
}

uint64_t TIME_ElapsedMsec64(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
    return (uint64_t)elapsed_msec(_pStart, _pStop);
}
//计算两个stamp之差，返回value格式
T_Timevalue TIME_ElapsedTime(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
    highprec_t elapsed = elapsed_msec(_pStart, _pStop);
    T_Timevalue tv_elapsed;

    if (HIGHPREC_VALUE_IS_ZERO(elapsed)) {
        tv_elapsed.sec = tv_elapsed.msec = 0;
        return tv_elapsed;
    } else {
        highprec_t sec, msec;

        sec = elapsed;
        HIGHPREC_DIV(sec, MSEC);
        tv_elapsed.sec = (long)sec;

        msec = elapsed;
        HIGHPREC_MOD(msec, MSEC);
        tv_elapsed.msec = (long)msec;

        return tv_elapsed;
    }
}

uint32_t TIME_ElapsedCycle32(const T_Timestamp *_pStart, const T_Timestamp *_pStop)
{
    return _pStop->u32.lo - _pStart->u32.lo;
}

int TIME_GetTickCount(T_Timevalue *tv)
{
    T_Timestamp pCurr, pStart;

    if (TIME_GetTimestamp(&pCurr)) return -1;

    TIME_SetTimestamp32(&pStart, 0, 0);
    *tv = TIME_ElapsedTime(&pStart, &pCurr);

    return 0;
}

void TIME_Normalize(T_Timevalue *t)
{
    if (t->msec >= 1000) {
		t->sec += (t->msec / 1000);
		t->msec = (t->msec % 1000);
    }
    else if (t->msec <= -1000) {
		do {
			t->sec--;
			t->msec += 1000;
		} while (t->msec <= -1000);
    }

    if (t->sec >= 1 && t->msec < 0) {
		t->sec--;
		t->msec += 1000;
    } else if (t->sec < 0 && t->msec > 0) {
		t->sec++;
		t->msec -= 1000;
    }
}

void TIME_SetTimestamp32(T_Timestamp *t, uint32_t hi, uint32_t lo)
{
    t->u32.hi = hi;
    t->u32.lo = lo;
}
//比较两个时间戳,-1(t1<t2),0(t1=t2),1(t1>t2)
int TIME_CmpTimestamp(const T_Timestamp *t1, const T_Timestamp *t2)
{
#if ZXY_HAS_TIME_INT64
    if (t1->u64 < t2->u64)
		return -1;
    else if (t1->u64 > t2->u64)
		return 1;
    else
		return 0;
#else
    if (t1->u32.hi < t2->u32.hi || (t1->u32.hi == t2->u32.hi && t1->u32.lo < t2->u32.lo))
		return -1;
    else if (t1->u32.hi > t2->u32.hi || (t1->u32.hi == t2->u32.hi && t1->u32.lo > t2->u32.lo))
		return 1;
    else
		return 0;
#endif
}

void TIME_AddTimestamp(T_Timestamp *t1, const T_Timestamp *t2)
{
#if ZXY_HAS_TIME_INT64
    t1->u64 += t2->u64;
#else
    uint32_t old = t1->u32.lo;
    t1->u32.hi += t2->u32.hi;
    t1->u32.lo += t2->u32.lo;
    if (t1->u32.lo < old)
		++t1->u32.hi;
#endif
}

void TIME_AddTimestamp32(T_Timestamp *t1, uint32_t t2)
{
#if ZXY_HAS_TIME_INT64
    t1->u64 += t2;
#else
    uint32_t old = t1->u32.lo;
    t1->u32.lo += t2;
    if (t1->u32.lo < old)
		++t1->u32.hi;
#endif
}

void TIME_SubTimestamp(T_Timestamp *t1, const T_Timestamp *t2)
{
#if ZXY_HAS_TIME_INT64
    t1->u64 -= t2->u64;
#else
    t1->u32.hi -= t2->u32.hi;
    if (t1->u32.lo >= t2->u32.lo)
		t1->u32.lo -= t2->u32.lo;
    else {
		t1->u32.lo -= t2->u32.lo;
		--t1->u32.hi;
    }
#endif
}

void TIME_SubTimestamp32(T_Timestamp *t1, uint32_t t2)
{
#if ZXY_HAS_TIME_INT64
    t1->u64 -= t2;
#else
    if (t1->u32.lo >= t2)
		t1->u32.lo -= t2;
    else {
		t1->u32.lo -= t2;
		--t1->u32.hi;
    }
#endif
}

int32_t TIME_DiffTimestamp32(const T_Timestamp *t1, const T_Timestamp *t2)
{
#if ZXY_HAS_TIME_INT64
    int64_t diff = t2->u64 - t1->u64;
    return (int32_t) diff;
#else
    int32_t diff = t2->u32.lo - t1->u32.lo;
    return diff;
#endif
}