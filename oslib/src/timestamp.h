/*******************************************************************************
	> File Name: .h
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_TIMESTAMP_H__
#define __ZXY_TIMESTAMP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "typedefs.h"
#include "error_code.h"
#include "config_site.h"



#include "precision.h"		//精度配置文件

////////////////////////////////////////////////////////////////////
typedef union _tagTimestamp
{
    struct
    {
#if defined(ZXY_HAS_TIME_LITTLE_ENDIAN) && ZXY_HAS_TIME_LITTLE_ENDIAN!=0
		uint32_t lo; 
		uint32_t hi; 
#else
		uint32_t hi;
		uint32_t lo;
#endif
    }u32;

#if ZXY_HAS_TIME_INT64
    uint64_t u64;
#endif

}T_Timestamp;


typedef struct _tagTimeValue
{
    long    sec;
    long    msec;
}T_Timevalue;

API_DECL(int) TIME_GetTickCount(T_Timevalue *tv);

API_DECL(int) TIME_GetTimevalue(T_Timevalue *_pTimeValue);	//获取当前时间：value格式
API_DECL(int) TIME_GetTimestamp(T_Timestamp *_pTs);			//获取当前时间：stamp格式
API_DECL(int) TIME_GetTimestampfreq(T_Timestamp *freq);

API_DECL(int) TIME_CmpTimestamp(const T_Timestamp *t1, const T_Timestamp *t2);
API_DECL(void) TIME_Normalize(T_Timevalue *t);		//规范计数
API_DECL(void) TIME_SetTimestamp32(T_Timestamp *t, uint32_t hi, uint32_t lo);

API_DECL(void) TIME_AddTimestamp(T_Timestamp *t1, const T_Timestamp *t2);
API_DECL(void) TIME_AddTimestamp32(T_Timestamp *t1, uint32_t t2);
API_DECL(void) TIME_SubTimestamp(T_Timestamp *t1, const T_Timestamp *t2);
API_DECL(void) TIME_SubTimestamp32(T_Timestamp *t1, uint32_t t2);

API_DECL(int32_t) TIME_DiffTimestamp32(const T_Timestamp *t1, const T_Timestamp *t2);

API_DECL(T_Timevalue) TIME_ElapsedTime(const T_Timestamp *start, const T_Timestamp *stop);
API_DECL(uint32_t) TIME_ElapsedCycle32(const T_Timestamp *start, const T_Timestamp *stop);
API_DECL(uint64_t) TIME_ElapsedMsec64(const T_Timestamp *start, const T_Timestamp *stop);
API_DECL(uint32_t) TIME_ElapsedMsec(const T_Timestamp *start, const T_Timestamp *stop );
API_DECL(uint32_t) TIME_ElapsedUsec(const T_Timestamp *start, const T_Timestamp *stop);
API_DECL(uint32_t) TIME_ElapsedNanosec(const T_Timestamp *start, const T_Timestamp *stop);




#define TIME_VAL_MSEC(t)		((t).sec * 1000 + (t).msec)	//转换成多少ms
#define TIME_VAL_EQ(t1, t2)		((t1).sec==(t2).sec && (t1).msec==(t2).msec)
#define TIME_VAL_GT(t1, t2)		((t1).sec>(t2).sec || ((t1).sec==(t2).sec && (t1).msec>(t2).msec))

#define TIME_VAL_GTE(t1, t2)	(TIME_VAL_GT(t1,t2) || TIME_VAL_EQ(t1,t2))	//t1>=t2 
#define TIME_VAL_LT(t1, t2)		(!(TIME_VAL_GTE(t1,t2)))	//t1<t2
#define TIME_VAL_LTE(t1, t2)	(!TIME_VAL_GT(t1, t2))		//t1<=t2

#define TIME_VAL_ADD(t1, t2)	do {(t1).sec += (t2).sec;(t1).msec += (t2).msec;TIME_Normalize(&(t1));} while (0)
#define TIME_VAL_SUB(t1, t2)	do {(t1).sec -= (t2).sec;(t1).msec -= (t2).msec;TIME_Normalize(&(t1));} while (0)






#endif
