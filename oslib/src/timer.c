#include <stdio.h>
#include <stdlib.h>
#include <timer.h>
#include <assert.h>
#include <errno.h>

#include "timer.h"
#include "lock.h"
#include "climits.h"

#define HEAP_PARENT(X)	(X == 0 ? 0 : (((X) - 1) / 2))
#define HEAP_LEFT(X)	(((X)+(X))+1)


#define DEFAULT_MAX_TIMED_OUT_PER_POLL  (64)


#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"timer", ZXY_LOG_DEBUG_LV, 0};	//默认关闭
static char s_u8LogId = -1;

void TIMER_SubLogRegister(void)
{
	s_u8LogId = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
}

int TIMER_SubLogSet(char _enLogLv ,char _bLogOn)
{
	CONSOLE_SubLogSetParam(s_u8LogId, _enLogLv, _bLogOn);
}

T_SubLogInfo* TIMER_SubLogGet(void)
{
	return CONSOLE_SubLogGetParam(s_u8LogId);
}
#endif

enum
{
    F_DONT_CALL = 1,
    F_DONT_ASSERT = 2,
    F_SET_ID = 4
};


struct _tagTimerHeap
{
    T_PoolInfo	*m_pPool;
    size_t		m_stSize;
    size_t		m_stCurrSize;
    int			m_u32PollMax;
    T_Lock		*m_pLock;
    char		m_bAutoDelLock;
    T_TimerEntity **m_aptHeap;
    T_TimerId	*m_pTimerIds;
    T_TimerId	m_tTimerIdsFreeList;
    TIMER_CALLBCK *funcCallbck;
};

 
inline void lock_timer_heap(T_TimerHeap *_pHeap)
{
    if (_pHeap->m_pLock) {
		LOCK_Acquire(_pHeap->m_pLock);
    }
}

inline void unlock_timer_heap(T_TimerHeap *_pHeap)
{
    if (_pHeap->m_pLock) {
		LOCK_Release(_pHeap->m_pLock);
    }
}


static void copy_node(T_TimerHeap *_pHeap, int _u32Slot, T_TimerEntity *_pMoved)
{
    CHECK_STACK();
    _pHeap->m_aptHeap[_u32Slot] = _pMoved;
    _pHeap->m_pTimerIds[_pMoved->m_tTimerId] = (int)_u32Slot;
}

static T_TimerId pop_freelist(T_TimerHeap *_pHeap)
{
    T_TimerId tNewId = _pHeap->m_tTimerIdsFreeList;
    CHECK_STACK();
    _pHeap->m_tTimerIdsFreeList = -_pHeap->m_pTimerIds[_pHeap->m_tTimerIdsFreeList];
    return tNewId;
}

static void push_freelist (T_TimerHeap *_pHeap, T_TimerId _u32OldId)
{
    CHECK_STACK();
    _pHeap->m_pTimerIds[_u32OldId] = -_pHeap->m_tTimerIdsFreeList;
    _pHeap->m_tTimerIdsFreeList = _u32OldId;
}

static void reheap_down(T_TimerHeap *_pHeap, T_TimerEntity *_pMoved, int _u32Slot, size_t _stChild)
{
    CHECK_STACK();
    while (_stChild < _pHeap->m_stCurrSize){
		if (_stChild + 1 < _pHeap->m_stCurrSize && TIME_VAL_LT(_pHeap->m_aptHeap[_stChild + 1]->m_tTimerValue, _pHeap->m_aptHeap[_stChild]->m_tTimerValue))
			_stChild++;
		if (TIME_VAL_LT(_pHeap->m_aptHeap[_stChild]->m_tTimerValue, _pMoved->m_tTimerValue)){
			copy_node(_pHeap, _u32Slot, _pHeap->m_aptHeap[_stChild]);
			_u32Slot = _stChild;
			_stChild = HEAP_LEFT(_stChild);
		}
		else
			break;
    }
    copy_node(_pHeap, _u32Slot, _pMoved);
}

static void reheap_up(T_TimerHeap *_pHeap, T_TimerEntity *_pMoved, int _u32Slot, size_t _stParent)
{
    while (_u32Slot > 0){
		if (TIME_VAL_LT(_pMoved->m_tTimerValue, _pHeap->m_aptHeap[_stParent]->m_tTimerValue)){
			copy_node(_pHeap, _u32Slot, _pHeap->m_aptHeap[_stParent]);
			_u32Slot = _stParent;
			_stParent = HEAP_PARENT(_u32Slot);
		}
		else
			break;
    }
    copy_node(_pHeap, _u32Slot, _pMoved);
}

static T_TimerEntity *remove_node(T_TimerHeap *_pHeap, int _u32Slot)
{
    T_TimerEntity *pNode = _pHeap->m_aptHeap[_u32Slot];
    push_freelist(_pHeap, pNode->m_tTimerId);
    _pHeap->m_stCurrSize--;
    pNode->m_tTimerId = -1;
    if (_u32Slot < _pHeap->m_stCurrSize){
		size_t stParent;
		T_TimerEntity *pMoved = _pHeap->m_aptHeap[_pHeap->m_stCurrSize];
		copy_node(_pHeap, _u32Slot, pMoved);
		stParent = HEAP_PARENT (_u32Slot);
	
		if (TIME_VAL_GTE(pMoved->m_tTimerValue, _pHeap->m_aptHeap[stParent]->m_tTimerValue))
			reheap_down(_pHeap, pMoved, _u32Slot, HEAP_LEFT(_u32Slot));
		else
			reheap_up(_pHeap, pMoved, _u32Slot, stParent);
    }
    return pNode;
}

static void grow_heap(T_TimerHeap *_pHeap)
{
    size_t stSize = _pHeap->m_stSize * 2;
    T_TimerId *new_timer_ids;
    size_t i;

    T_TimerEntity **new_heap = 0;
    
    new_heap = (T_TimerEntity**)POOL_Alloc(_pHeap->m_pPool, sizeof(T_TimerEntity*) * stSize);
    memcpy(new_heap, _pHeap->m_aptHeap, _pHeap->m_stSize * sizeof(T_TimerEntity*));
    _pHeap->m_aptHeap = new_heap;

    new_timer_ids = 0;
    new_timer_ids = (T_TimerId*)POOL_Alloc(_pHeap->m_pPool, stSize * sizeof(T_TimerId));
    
    memcpy( new_timer_ids, _pHeap->m_pTimerIds, _pHeap->m_stSize * sizeof(T_TimerId));
    _pHeap->m_pTimerIds = new_timer_ids;
    for (i = _pHeap->m_stSize; i < stSize; i++)
		_pHeap->m_pTimerIds[i] = -((T_TimerId) (i + 1));
    _pHeap->m_stSize = stSize;
}

static void insert_node(T_TimerHeap *_pHeap, T_TimerEntity *_pNode)
{
    if (_pHeap->m_stCurrSize + 2 >= _pHeap->m_stSize) grow_heap(_pHeap);
    reheap_up(_pHeap, _pNode, _pHeap->m_stCurrSize, HEAP_PARENT(_pHeap->m_stCurrSize));
    _pHeap->m_stCurrSize++;
}

static int schedule_entry(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pTime)
{
    if (_pHeap->m_stCurrSize < _pHeap->m_stSize){
		_pNode->m_tTimerId = pop_freelist(_pHeap);
		_pNode->m_tTimerValue = *_pTime;
		insert_node(_pHeap, _pNode);
		return 0;
    }
    else
		return -1;
}


static int cancel(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, unsigned flags)
{
	long timer_node_slot;
	CHECK_STACK();

	if (_pNode->m_tTimerId < 0 || (size_t)_pNode->m_tTimerId > _pHeap->m_stSize){
		_pNode->m_tTimerId = -1;
		return 0;
	}

	timer_node_slot = _pHeap->m_pTimerIds[_pNode->m_tTimerId];
	if (timer_node_slot < 0){
		_pNode->m_tTimerId = -1;
		return 0;
	}

	if (_pNode != _pHeap->m_aptHeap[timer_node_slot]){
		if ((flags & F_DONT_ASSERT) == 0) assert(_pNode == _pHeap->m_aptHeap[timer_node_slot]);
		_pNode->m_tTimerId = -1;
		return 0;
	}
	else{
		remove_node(_pHeap, timer_node_slot);
		if ((flags & F_DONT_CALL) == 0) (*_pHeap->funcCallbck)(_pHeap, _pNode);
		return 1;
	}
}

size_t TIMER_HeapEstimateSize(int _u32Count)
{
    return sizeof(T_TimerHeap) + (_u32Count + 2) * (sizeof(T_TimerEntity*) + sizeof(T_TimerId)) + 132;
}

int TIMER_HeapCreate(T_PoolInfo *_pPool, size_t _stSize, T_TimerHeap **_p2Out)
{
    T_TimerHeap *pHeap;
    size_t i;
    ASSERT_RETURN(_pPool && _p2Out, EO_INVAL);
    *_p2Out = NULL;

    _stSize += 2;

    pHeap = POOL_ALLOC_T(_pPool, T_TimerHeap);
    if (!pHeap) return EO_NOMEM;

    pHeap->m_stSize = _stSize;
    pHeap->m_stCurrSize = 0;
    pHeap->m_u32PollMax = DEFAULT_MAX_TIMED_OUT_PER_POLL;
    pHeap->m_tTimerIdsFreeList = 1;
    pHeap->m_pPool = _pPool;

    pHeap->m_pLock = NULL;
    pHeap->m_bAutoDelLock = 0;

    pHeap->m_aptHeap = (T_TimerEntity**)POOL_Alloc(_pPool, sizeof(T_TimerEntity*) * _stSize);
    if (!pHeap->m_aptHeap) return EO_NOMEM;

    pHeap->m_pTimerIds = (T_TimerId *)POOL_Alloc(_pPool, sizeof(T_TimerId) * _stSize);
    if (!pHeap->m_pTimerIds) return EO_NOMEM;

    for (i = 0; i < _stSize; ++i)
		pHeap->m_pTimerIds[i] = -((T_TimerId)(i + 1));

    *_p2Out = pHeap;
	ZXY_DLOG(LOG_INFO, "timer heap created addr = %p, size = %d, poll max = %d", pHeap, _stSize, pHeap->m_u32PollMax);
    return EO_SUCCESS;
}

void TIMER_HeapDestroy(T_TimerHeap *_pHeap)
{
	ZXY_DLOG(LOG_INFO, "timer heap destroy addr = %p", _pHeap);
    if (_pHeap->m_pLock && _pHeap->m_bAutoDelLock) {
        LOCK_Destroy(_pHeap->m_pLock);
        _pHeap->m_pLock = NULL;
    }
}

void TIMER_HeapSetLock(T_TimerHeap *_pHeap, T_Lock *_pLock, char _bAutoDel)
{
    if (_pHeap->m_pLock && _pHeap->m_bAutoDelLock)
        LOCK_Destroy(_pHeap->m_pLock);
    _pHeap->m_pLock = _pLock;
    _pHeap->m_bAutoDelLock = _bAutoDel;
}


int TIMER_HeapSetPollMax(T_TimerHeap *_pHeap, int _u32Count)
{
    int old_count = _pHeap->m_u32PollMax;
    _pHeap->m_u32PollMax = _u32Count;
    return old_count;
}

T_TimerEntity* TIMER_EntityInit(T_TimerEntity *_pNode, int _u32id, void *_pUserData, TIMER_CALLBCK *_pHandle)
{
    assert(_pNode && _pHandle);

    _pNode->m_tTimerId = -1;
    _pNode->m_u32UsrId = _u32id;
    _pNode->m_pUsrData = _pUserData;
    _pNode->funcHandle = _pHandle;
    _pNode->m_pGLock = NULL;

    return _pNode;
}

//查看定时器是否在运行
char TIMER_IsEntityRunning(T_TimerEntity *_pNode)
{
    return (_pNode->m_tTimerId >= 1);
}

#if ZXY_EN_TIMER_DEBUG
static int schedule_w_grp_lock_dbg(T_TimerHeap *_pHeap,
                                    T_TimerEntity *_pNode,
                                    const T_Timevalue *_pDelay,
                                    char _u8SetId, int _u32Id,
									T_GrpLock *_pGLock,
									const char *_strFile, int _u32Line)
#else
static int schedule_w_grp_lock(T_TimerHeap *_pHeap,
							   T_TimerEntity *_pNode,
							   const T_Timevalue *_pDelay,
							   char _u8SetId, int _u32Id,
							   T_GrpLock *_pGLock)
#endif
{
    int u32Ret;
    T_Timevalue tExpires;

    ASSERT_RETURN(_pHeap && _pNode && _pDelay, EO_INVAL);
    ASSERT_RETURN(_pNode->funcHandle != NULL, EO_INVAL);
    ASSERT_RETURN(_pNode->m_tTimerId < 1, EO_INVALIDOP);

#if ZXY_EN_TIMER_DEBUG
    _pNode->m_strFile = _strFile;
    _pNode->m_u32Line = _u32Line;
#endif
    TIME_GetTickCount(&tExpires);
    TIME_VAL_ADD(tExpires, *_pDelay);
    
    lock_timer_heap(_pHeap);
    u32Ret = schedule_entry(_pHeap, _pNode, &tExpires);
    if (u32Ret == EO_SUCCESS) {
		if (_u8SetId) _pNode->m_u32UsrId = _u32Id;
		_pNode->m_pGLock = _pGLock;
#if 0		
		if (_pNode->m_pGLock) {
			grp_lock_add_ref(_pNode->m_pGLock);
		}
#endif		
    }
    unlock_timer_heap(_pHeap);

    return u32Ret;
}


#if ZXY_EN_TIMER_DEBUG
int TIMER_HeapScheduleDbg( T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pDelay, const char *_strFile, int _u32Line)
{
    return schedule_w_grp_lock_dbg(_pHeap, _pNode, _pDelay, 0, 1, NULL, _strFile, _u32Line);
}

int TIMER_HeapScheduleGrpLockDbg(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pDelay, int _u32Id, T_GrpLock *_pGLock, const char *_strFile, int _u32Line)
{
    return schedule_w_grp_lock_dbg(_pHeap, _pNode, _pDelay, 1, _u32Id, _pGLock, _strFile, _u32Line);
}

#else
int TIMER_HeapSchedule(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pDelay)
{
    return schedule_w_grp_lock(_pHeap, _pNode, _pDelay, 0, 1, NULL);
}

int TIMER_HeapScheduleGrpLock(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, const T_Timevalue *_pDelay, int _u32Id, T_GrpLock *_pGLock)
{
    return schedule_w_grp_lock(_pHeap, _pNode, _pDelay, 1, _u32Id, _pGLock);
}
#endif

static int cancel_timer(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, unsigned flags, int _u32Id)
{
    int u32Count;
    ASSERT_RETURN(_pHeap && _pNode, EO_INVAL);
    lock_timer_heap(_pHeap);
    u32Count = cancel(_pHeap, _pNode, flags | F_DONT_CALL);
    if (flags & F_SET_ID) {
		_pNode->m_u32UsrId = _u32Id;
    }
#if 0	
    if (_pNode->m_pGLock) {		
		T_GrpLock *pGLock = _pNode->m_pGLock;
		_pNode->m_pGLock = NULL;
		grp_lock_dec_ref(pGLock);
    }
#endif	
    unlock_timer_heap(_pHeap);

    return u32Count;
}

int TIMER_EntityCancel(T_TimerHeap *_pHeap, T_TimerEntity *_pNode)
{
    return cancel_timer(_pHeap, _pNode, 0, 0);
}

int TIMER_EntityCancelIfActive(T_TimerHeap *_pHeap, T_TimerEntity *_pNode, int _u32Id)
{
    return cancel_timer(_pHeap, _pNode, F_SET_ID | F_DONT_ASSERT, _u32Id);
}


//-1无定时器，0还有定时器但未到时间，0+已经触发几个
//next_delay 最近一个要出发的时间
int TIMER_HeapPoll(T_TimerHeap *_pHeap, T_Timevalue *next_delay)
{
    T_Timevalue now;
    int u32Count;
    ASSERT_RETURN(_pHeap, 0);
    lock_timer_heap(_pHeap);
    if (!_pHeap->m_stCurrSize && next_delay) {
		next_delay->sec = next_delay->msec = C_MAXINT32;
		unlock_timer_heap(_pHeap);
		return -1;
    }

    u32Count = 0;
    TIME_GetTickCount(&now);

    while (_pHeap->m_stCurrSize && TIME_VAL_LTE(_pHeap->m_aptHeap[0]->m_tTimerValue, now) && u32Count < _pHeap->m_u32PollMax) 
    {
		T_TimerEntity *node = remove_node(_pHeap, 0);
		T_GrpLock *pGLock;

		++u32Count;

		pGLock = node->m_pGLock;
		node->m_pGLock = NULL;

		unlock_timer_heap(_pHeap);

		if (node->funcHandle)
			(*node->funcHandle)(_pHeap, node);
#if 0
		if (pGLock)
			grp_lock_dec_ref(pGLock);
#endif
		lock_timer_heap(_pHeap);
    }
	
    if (_pHeap->m_stCurrSize && next_delay) {
		*next_delay = _pHeap->m_aptHeap[0]->m_tTimerValue;
		TIME_VAL_SUB(*next_delay, now);
		if (next_delay->sec < 0 || next_delay->msec < 0)
			next_delay->sec = next_delay->msec = 0;
    } else if (next_delay) {
		next_delay->sec = next_delay->msec = C_MAXINT32;
    }
    unlock_timer_heap(_pHeap);

    return u32Count;
}

int TIMER_HeapGetCount(T_TimerHeap *_pHeap)
{
    ASSERT_RETURN(_pHeap, 0);
    return _pHeap->m_stCurrSize;
}

int TIMER_HeapEarliestTime(T_TimerHeap * _pHeap, T_Timevalue *timeval)
{
    assert(_pHeap->m_stCurrSize != 0);
    if (_pHeap->m_stCurrSize == 0)
        return EO_NOTFOUND;

    lock_timer_heap(_pHeap);
    *timeval = _pHeap->m_aptHeap[0]->m_tTimerValue;
    unlock_timer_heap(_pHeap);

    return EO_SUCCESS;
}


void TIMER_HeapDump(T_TimerHeap *_pHeap)
{
	char aucPrint[1024] = {0};
	int u32Len = 0;
	CHECK_STACK();
    lock_timer_heap(_pHeap);
	u32Len += sprintf(aucPrint + u32Len, "Dumping timer heap:\r\n");
    u32Len += sprintf(aucPrint + u32Len, "  Cur size: %d entries, max: %d\r\n", (int)_pHeap->m_stCurrSize, (int)_pHeap->m_stSize);

    if (_pHeap->m_stCurrSize) {
		unsigned i;
		T_Timevalue now;

		u32Len += sprintf(aucPrint + u32Len, "  Entries: \r\n");
		u32Len += sprintf(aucPrint + u32Len, "    tid\tuid\tElapsed\tSource\r\n");
		u32Len += sprintf(aucPrint + u32Len, "    ----------------------------------\r\n");

		TIME_GetTickCount(&now);

		for (i = 0; i < (int)_pHeap->m_stCurrSize; ++i) {
			T_TimerEntity *e = _pHeap->m_aptHeap[i];
			T_Timevalue delta;

			if (TIME_VAL_LTE(e->m_tTimerValue, now))
				delta.sec = delta.msec = 0;
			else {
				delta = e->m_tTimerValue;
				TIME_VAL_SUB(delta, now);
			}
#if ZXY_EN_TIMER_DEBUG			
			u32Len += sprintf(aucPrint + u32Len, "    %d\t%d\t%d.%03d\t%s:%d\r\n", e->m_tTimerId, e->m_u32UsrId, (int)delta.sec, (int)delta.msec, e->m_strFile, e->m_u32Line);
#else
			u32Len += sprintf(aucPrint + u32Len, "    %d\t%d\t%d.%03d\tnil\r\n", e->m_tTimerId, e->m_u32UsrId, (int)delta.sec, (int)delta.msec);
#endif
		}
    }
	printf("%s", aucPrint);	//不使用log系统输出
    unlock_timer_heap(_pHeap);
}


