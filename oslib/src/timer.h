/*******************************************************************************
	> File Name: .h
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_TIMER_H__
#define __ZXY_TIMER_H__

#include "lock.h"
#include "timestamp.h"



typedef int T_TimerId;
typedef struct _tagTimerHeap T_TimerHeap;

struct _tagTimerEntity;
typedef void TIMER_CALLBCK(T_TimerHeap *timer_heap, struct _tagTimerEntity *_pNode);

typedef struct _tagTimerEntity
{
    void		*m_pUsrData;
    int			m_u32UsrId;
    TIMER_CALLBCK *funcHandle;
    T_TimerId	m_tTimerId;
    T_Timevalue m_tTimerValue;		//过期时间
    T_GrpLock 	*m_pGLock;
#if ZXY_EN_TIMER_DEBUG
    const char	*m_strFile;
    int		 m_u32Line;
#endif
}T_TimerEntity;


#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
API_DECL(void)			TIMER_SubLogRegister(void);
API_DECL(int) 			TIMER_SubLogSet(char _enLogLv ,char _bLogOn);
API_DECL(T_SubLogInfo*) TIMER_SubLogGet(void);
#endif

//根据实体的个数估算占用大小
API_DECL(size_t) TIMER_HeapEstimateSize(int _u32Count);
API_DECL(int) TIMER_HeapCreate(T_PoolInfo *pool, size_t _stCount, T_TimerHeap **_p2Out); 
API_DECL(void) TIMER_HeapDestroy(T_TimerHeap *_pHeap);
API_DECL(void) TIMER_HeapSetLock(T_TimerHeap *_pHeap, T_Lock *_pLock, char _bAutoDel);
API_DECL(T_TimerEntity*) TIMER_EntityInit(T_TimerEntity *_pNode, int _u32id, void *_pUserData, TIMER_CALLBCK *_pHandle);
API_DECL(char) TIMER_IsEntityRunning(T_TimerEntity *_pNode);
API_DECL(int) TIMER_EntityCancel(T_TimerHeap *_pHeap, T_TimerEntity *_pNode);
API_DECL(int) TIMER_EntityCancelIfActive(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, int _u32Id);
API_DECL(int) TIMER_HeapGetCount(T_TimerHeap *_pHeap);
API_DECL(int) TIMER_HeapEarliestTime(T_TimerHeap *_pHeap, T_Timevalue *timeval);
API_DECL(int) TIMER_HeapSetPollMax(T_TimerHeap *_pHeap, int _u32Count);
API_DECL(int) TIMER_HeapPoll(T_TimerHeap *_pHeap, T_Timevalue *next_delay);
API_DECL(void) TIMER_HeapDump(T_TimerHeap *_pHeap);	//注意栈大小，默认1k

#if ZXY_EN_TIMER_DEBUG
	#define TIMER_HeapSchedule(h, e, d) TIMER_HeapScheduleDbg(h, e, d, __FILE__, __LINE__)
	API_DECL(int) TIMER_HeapScheduleDbg(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pDelay, const char *_strFile, int _strLine);
	
	#define TIMER_HeapScheduleGrpLock(h,e,d,id,g) TIMER_HeapScheduleGrpLockDbg(h, e, d, id, g, __FILE__, __LINE__)
	API_DECL(int) TIMER_HeapScheduleGrpLockDbg(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pDelay, int _u32Id, T_GrpLock *grp_lock, const char *_strFile, int _strLine);
	
#else
	API_DECL(int) TIMER_HeapSchedule(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pDelay);
	API_DECL(int) TIMER_HeapScheduleGrpLock(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pDelay, int _u32Id, T_GrpLock *grp_lock);
#endif


#endif	/* __ZXY_TIMER_H__ */

