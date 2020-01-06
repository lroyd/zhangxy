/*******************************************************************************
	> File Name: .c
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h> 
 
#include "os.h"


#define SIGNATURE1  (0xDEAFBEEF)
#define SIGNATURE2  (0xDEADC0DE)

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"oslib", ZXY_LOG_DEBUG_LV, 0};	//默认关闭
static char s_u8LogId;

void OSLIB_SubLogRegister(void)
{
	s_u8LogId = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
}

int OSLIB_SubLogSet(char _enLogLv ,char _bLogOn)
{
	CONSOLE_SubLogSetParam(s_u8LogId, _enLogLv, _bLogOn);
}

T_SubLogInfo* OSLIB_SubLogGet(void)
{
	return CONSOLE_SubLogGetParam(s_u8LogId);
}

#endif


struct _tagThreadInfo
{
    char			m_aucName[ZXY_MAX_NAME_LEN];
    pthread_t	    m_tTid;
    THREAD_PROC 	*m_pHandle;
    void	   		*m_pArg;
    unsigned int	signature1;
    unsigned int	signature2;

    T_Mutex			*m_pMutexPend;
#if defined(ZXY_EN_THREAD_CHECK_STACK) && ZXY_EN_THREAD_CHECK_STACK!=0
    unsigned int	m_u32StkSize;
    unsigned int	m_u32StkMaxUsage;
    char			*m_pStkStart;
    const char		*m_pCallerFile;
    int				m_u32CallerLine;
#endif
};

static T_ThreadInfo s_tMainThread;
static long s_u32ThreadTLS;

static int thread_local_alloc(long *_pIndex)
{
    pthread_key_t key;
    int u32Ret;
    ASSERT_RETURN(_pIndex != NULL, EO_INVAL);
    assert(sizeof(pthread_key_t) <= sizeof(long));
    if ((u32Ret = pthread_key_create(&key, NULL)) != 0) return u32Ret;
    *_pIndex = key;
    return EO_SUCCESS;
}

static void thread_local_free(long index)
{
    pthread_key_delete(index);
}

static int thread_local_set(long index, void *value)
{
    return pthread_setspecific(index, value);
}

static void *thread_local_get(long index)
{
    return pthread_getspecific(index);
}

//获取当前线程pid
unsigned int THREAD_GetPid(void)
{
    return getpid();
}

char THREAD_IsRegistered(void)
{
    return thread_local_get(s_u32ThreadTLS) != 0;
}

int THREAD_GetPrio(T_ThreadInfo *_pThread)
{
    struct sched_param param;
    int policy;
    if (pthread_getschedparam(_pThread->m_tTid, &policy, &param)) return EO_FAILED;

    return param.sched_priority;
}


int THREAD_SetPrio(T_ThreadInfo *_pThread, int prio)
{
    struct sched_param param;
    int policy;
    if (pthread_getschedparam(_pThread->m_tTid, &policy, &param)) return EO_FAILED;
    param.sched_priority = prio;
    if (pthread_setschedparam(_pThread->m_tTid, policy, &param))return EO_FAILED;
    return EO_SUCCESS;
}


void* THREAD_GetHandle(T_ThreadInfo *_pThread)
{
    ASSERT_RETURN(_pThread, NULL);
    return &_pThread->m_tTid;
}


int THREAD_Register(const char *_strName, T_ThreadDesc _tDesc, T_ThreadInfo **_p2Out)
{
	char u8StackPtr;
    T_ThreadInfo *pThread = (T_ThreadInfo *)_tDesc;
    if (sizeof(T_ThreadDesc) < sizeof(T_ThreadInfo)) {
		assert(!"Not enough T_ThreadDesc size!");
		return EO_NOMEM;
    }

    if (thread_local_get(s_u32ThreadTLS) != 0) {
		ZXY_DLOG(LOG_WARNING, "possibly re-registering existing thread");
    }

    assert(pThread->signature1 != SIGNATURE1 ||pThread->signature2 != SIGNATURE2 ||(pThread->m_tTid == pthread_self()));

    memset(_tDesc, 0, sizeof(T_ThreadInfo));
    pThread->m_tTid = pthread_self();
    pThread->signature1 = SIGNATURE1;
    pThread->signature2 = SIGNATURE2;

    if(_strName)
		snprintf(pThread->m_aucName, sizeof(pThread->m_aucName),_strName, pThread->m_tTid);
    else
		snprintf(pThread->m_aucName, sizeof(pThread->m_aucName),"thr%p", (void*)pThread->m_tTid);

    if (thread_local_set(s_u32ThreadTLS, pThread)) {
		memset(_tDesc, 0, sizeof(T_ThreadInfo));
		return EO_UNKNOWN;
    }
	
#if defined(ZXY_EN_THREAD_CHECK_STACK) && ZXY_EN_THREAD_CHECK_STACK!=0
    pThread->m_pStkStart = &u8StackPtr;
    pThread->m_u32StkSize = 0xFFFFFFFFUL;
    pThread->m_u32StkMaxUsage = 0;
#endif

    *_p2Out = pThread;
    return EO_SUCCESS;
}


int THREAD_Init(void)
{
    T_ThreadInfo *dummy;
    if (thread_local_alloc(&s_u32ThreadTLS)) return EO_FAILED;
    return THREAD_Register("thr%p", (long*)&s_tMainThread, &dummy);
}

static void *thread_main(void *param)
{
    T_ThreadInfo *pThread = (T_ThreadInfo*)param;
    void *result;

#if defined(ZXY_EN_THREAD_CHECK_STACK) && ZXY_EN_THREAD_CHECK_STACK!=0
    pThread->m_pStkStart = (char*)&pThread;
#endif

    if (thread_local_set(s_u32ThreadTLS, pThread)) 
		assert(!"Thread TLS ID is not set (OSLIB_Init() error?)");

    if (pThread->m_pMutexPend){
		MUTEX_Lock(pThread->m_pMutexPend);
		MUTEX_Unlock(pThread->m_pMutexPend);
    }

	ZXY_DLOG(LOG_DEBUG, "Thread %s start", pThread->m_aucName);
    result = (void*)(long)(*pThread->m_pHandle)(pThread->m_pArg);
	ZXY_DLOG(LOG_DEBUG, "Thread %s exit !!!", pThread->m_aucName);

    return result;
}

int THREAD_Create(T_PoolInfo *_pPool, const char *_strName, THREAD_PROC *_pHandle, void *_pArg, size_t _stStackSize, unsigned _enFlags, T_ThreadInfo **_p2Out)
{
    T_ThreadInfo *pThread;    
    CHECK_STACK();
    ASSERT_RETURN(_pPool && _pHandle && _p2Out, EO_INVAL);
    pThread = (T_ThreadInfo *)POOL_Zalloc(_pPool, sizeof(T_ThreadInfo));
    ASSERT_RETURN(pThread, EO_NOMEM);

    if (!_strName) _strName = "thr%p";
    if (strchr(_strName, '%')){
		snprintf(pThread->m_aucName, ZXY_MAX_NAME_LEN, _strName, pThread);
    } 
    else{
		strncpy(pThread->m_aucName, _strName, ZXY_MAX_NAME_LEN);
		pThread->m_aucName[ZXY_MAX_NAME_LEN-1] = '\0';
    }

    if (_stStackSize == 0) _stStackSize = ZXY_SET_THREAD_DEF_STACK_SIZE;	//8192

#if defined(ZXY_EN_THREAD_CHECK_STACK) && ZXY_EN_THREAD_CHECK_STACK!=0
    pThread->m_u32StkSize = _stStackSize;
    pThread->m_u32StkMaxUsage = 0;
#endif

    if (_enFlags & THREAD_FLAG_SUSPENDED){
		if (MUTEX_CreateSimple(_pPool, NULL, &pThread->m_pMutexPend)){
			ZXY_ELOG("mutex create simple error");
			return EO_FAILED;
		} 
		MUTEX_Lock(pThread->m_pMutexPend);
    }
    else{
		assert(pThread->m_pMutexPend == NULL);
    }
	
    pthread_attr_t tAttr;
    pthread_attr_init(&tAttr);
#if defined(ZXY_EN_THREAD_SET_STACK_SIZE) && ZXY_EN_THREAD_SET_STACK_SIZE!=0
    if (pthread_attr_setstacksize(&tAttr, _stStackSize)) {
		//pthread_attr_destroy(&tAttr);
		ZXY_DLOG(LOG_WARNING, "pthread attr setstacksize error, use default value!!!");
		//return EO_FAILED;
	}
#endif

#if defined(ZXY_EN_THREAD_ALLOCATE_STACK) && ZXY_EN_THREAD_ALLOCATE_STACK!=0
	void *pStackAddr;	
    pStackAddr = pj_pool_alloc(_pPool, _stStackSize);
    ASSERT_RETURN(pStackAddr, EO_NOMEM);
    if (pthread_attr_setstackaddr(&tAttr, pStackAddr)) {
		pthread_attr_destroy(&tAttr);
		return EO_FAILED;
	}
#endif

    pThread->m_pHandle = _pHandle;
    pThread->m_pArg = _pArg;
    if (pthread_create(&pThread->m_tTid, &tAttr, &thread_main, pThread)) return EO_FAILED;
    *_p2Out = pThread;
	
	pthread_attr_destroy(&tAttr);
	ZXY_DLOG(LOG_INFO, "Thread created %s", pThread->m_aucName);
    return EO_SUCCESS;
}


const char *THREAD_GetName(T_ThreadInfo *_pThread)
{
    T_ThreadInfo *pThread = (T_ThreadInfo*)_pThread;
    ASSERT_RETURN(_pThread, "");
    return pThread->m_aucName;
}

int THREAD_Resume(T_ThreadInfo *_pThread)
{
    int u32Ret;
    CHECK_STACK();
    ASSERT_RETURN(_pThread, EO_INVAL);
    u32Ret = MUTEX_Unlock(_pThread->m_pMutexPend);
    return u32Ret;
}

T_ThreadInfo *THREAD_This(void)
{
    T_ThreadInfo *pThread = (T_ThreadInfo*)thread_local_get(s_u32ThreadTLS);
    if (pThread == NULL) {
		assert(!"Calling oslib from unknown/external thread. You must "
		   "register external threads with THREAD_Register() "
		   "before calling any oslib functions.");
    }
    return pThread;
}

int THREAD_Join(T_ThreadInfo *_pThread)
{
    T_ThreadInfo *pThread = (T_ThreadInfo *)_pThread;
    void *pRet;
    int u32Ret;

    CHECK_STACK();
    if (_pThread == THREAD_This()) {
		ZXY_DLOG(LOG_WARNING, "%s is not THREAD_This()", pThread->m_aucName);
		return EO_UNKNOWN;
	}

	ZXY_DLOG(LOG_DEBUG, "Joining thread %s", pThread->m_aucName);
    u32Ret = pthread_join(pThread->m_tTid, &pRet);
    if (u32Ret){
		u32Ret == ESRCH ? 0 : u32Ret;	//ESRCH无此进程
	}
	return u32Ret;	
}

int THREAD_Destroy(T_ThreadInfo *_pThread)
{
    CHECK_STACK();
    if (_pThread->m_pMutexPend) {
		MUTEX_Destroy(_pThread->m_pMutexPend);
		_pThread->m_pMutexPend = NULL;
    }
	//不需要释放pool的申请
	ZXY_DLOG(LOG_INFO, "Thread destroy %s", _pThread->m_aucName);
    return EO_SUCCESS;
}

void THREAD_MSleep(unsigned msec)
{
    usleep(msec * 1000);
}

#if defined(ZXY_EN_THREAD_CHECK_STACK) && ZXY_EN_THREAD_CHECK_STACK!=0
void THREAD_CheckStack(const char *_pFile, int _u32Line)
{
    char pStkPtr;
    unsigned int u32Usage;
    T_ThreadInfo *pThread = THREAD_This();
    u32Usage = (&pStkPtr > pThread->m_pStkStart) ? &pStkPtr - pThread->m_pStkStart : pThread->m_pStkStart - &pStkPtr;
    assert("STACK OVERFLOW!! " && (u32Usage <= pThread->m_u32StkSize - 128));
    if (u32Usage > pThread->m_u32StkMaxUsage) {
		pThread->m_u32StkMaxUsage = u32Usage;
		pThread->m_pCallerFile = _pFile;
		pThread->m_u32CallerLine = _u32Line;
    }
}

unsigned int THREAD_GetStackMaxUsage(T_ThreadInfo *_pThread)
{
    return _pThread->m_u32StkMaxUsage;
}

int THREAD_GetStackInfo(T_ThreadInfo *_pThread, const char **_pFile, int *_u32Line )
{
    assert(_pThread);
    *_pFile = _pThread->m_pCallerFile;
    *_u32Line = _pThread->m_u32CallerLine;
    return 0;
}

#endif


///////////////////////////////////////////////////////////////////////////////
struct _tagMutex
{
    pthread_mutex_t		m_tMutex;
    char				m_aucName[ZXY_MAX_NAME_LEN];
	
#if ZXY_EN_MUTEX_DEBUG
    int					m_u32Nesting;
    T_ThreadInfo		*m_ptOwer;		//线程所有者
    char				m_aucOwerName[ZXY_MAX_NAME_LEN];	//所有者名字
#endif
};

static T_Mutex s_tCriticalSection;

void MUTEX_EnterCritical(void)
{
    MUTEX_Lock(&s_tCriticalSection);
}

void MUTEX_LeaveCritical(void)
{
    MUTEX_Unlock(&s_tCriticalSection);
}


static int init_mutex(T_Mutex *_pMutex, const char *name, int type)
{
    pthread_mutexattr_t attr;
    int u32Ret;

    CHECK_STACK();

    u32Ret = pthread_mutexattr_init(&attr);
    if (u32Ret != 0) return u32Ret;

    if (type == MUTEX_TYPE_SIMPLE) 
		u32Ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    else 
		u32Ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (u32Ret != 0) return u32Ret;

    u32Ret = pthread_mutex_init(&_pMutex->m_tMutex, &attr);
    if (u32Ret != 0) return (u32Ret);

    u32Ret = pthread_mutexattr_destroy(&attr);
    if (u32Ret != 0) {
		int status = (u32Ret);
		pthread_mutex_destroy(&_pMutex->m_tMutex);
		return status;
    }

#if ZXY_EN_MUTEX_DEBUG
    /* Set owner. */
    _pMutex->m_u32Nesting = 0;
    _pMutex->m_ptOwer = NULL;
    _pMutex->m_aucOwerName[0] = '\0';
#endif

    if (!name) name = "mtx%p";

    if (strchr(name, '%')) {
		snprintf(_pMutex->m_aucName, ZXY_MAX_NAME_LEN, name, _pMutex);
    } else {
		strncpy(_pMutex->m_aucName, name, ZXY_MAX_NAME_LEN);
		_pMutex->m_aucName[ZXY_MAX_NAME_LEN-1] = '\0';
    }

	ZXY_DLOG(LOG_TRACE, "mutex created %s", _pMutex->m_aucName);
    return 0;
}

int MUTEX_Create(T_PoolInfo *_pPool, const char *_strName, int _enType, T_Mutex **_p2Out)
{
    int u32Ret;
    T_Mutex *pMutex;

    ASSERT_RETURN(_pPool && _p2Out, EO_INVAL);

    pMutex = POOL_ALLOC_T(_pPool, T_Mutex);
    ASSERT_RETURN(pMutex, EO_NOMEM);

    if ((u32Ret = init_mutex(pMutex, _strName, _enType)) != 0)
		return u32Ret;

    *_p2Out = pMutex;
    return 0;
}

int MUTEX_CreateSimple(T_PoolInfo *_pPool, const char *_strName, T_Mutex **_p2Out)
{
    return MUTEX_Create(_pPool, _strName, MUTEX_TYPE_SIMPLE, _p2Out);
}

int MUTEX_CreateRecursive(T_PoolInfo *_pPool, const char *_strName, T_Mutex **_p2Out)
{
    return MUTEX_Create(_pPool, _strName, MUTEX_TYPE_RECURSE, _p2Out);
}

int MUTEX_Lock(T_Mutex *_pMutex)
{
    int status;

    CHECK_STACK();
    ASSERT_RETURN(_pMutex, EO_INVAL);

#if ZXY_EN_MUTEX_DEBUG
	ZXY_DLOG(LOG_DEBUG, "mutex[%s]: thread %s is waiting (mutex owner = %s)", _pMutex->m_aucName, THREAD_This()->m_aucName, _pMutex->m_aucOwerName[0]!=0?_pMutex->m_aucOwerName:"null");
#else
    ZXY_DLOG(LOG_DEBUG, "mutex[%s]: thread %s is waiting", _pMutex->m_aucName, THREAD_This()->m_aucName);
#endif

    status = pthread_mutex_lock(&_pMutex->m_tMutex);

#if ZXY_EN_MUTEX_DEBUG
    if (status == 0) {
		_pMutex->m_ptOwer = THREAD_This();
		strcpy(_pMutex->m_aucOwerName, _pMutex->m_ptOwer->m_aucName);
		++_pMutex->m_u32Nesting;
    }

    ZXY_DLOG(LOG_DEBUG, "mutex[%s]:%s %s (level = %d)",_pMutex->m_aucName, 
	      (status==0 ? "acquired by thread" : "acquisition FAILED by"),
	      THREAD_This()->m_aucName, _pMutex->m_u32Nesting);
#else
    ZXY_DLOG(LOG_DEBUG, "mutex[%s]:%s %s", _pMutex->m_aucName,(status==0 ? "acquired by thread" : "FAILED by"), THREAD_This()->m_aucName);
#endif

	return status;
}

int MUTEX_Unlock(T_Mutex *_pMutex)
{
    int status;

    CHECK_STACK();
    ASSERT_RETURN(_pMutex, EO_INVAL);

#if ZXY_EN_MUTEX_DEBUG
    assert(_pMutex->m_ptOwer == THREAD_This());
    if (--_pMutex->m_u32Nesting == 0) {
		_pMutex->m_ptOwer = NULL;
		_pMutex->m_aucOwerName[0] = '\0';
    }
    ZXY_DLOG(LOG_DEBUG, "mutex[%s]:released by thread %s (level = %d)", _pMutex->m_aucName, THREAD_This()->m_aucName, _pMutex->m_u32Nesting);
#else
    ZXY_DLOG(LOG_DEBUG, "mutex[%s]:released by thread %s", _pMutex->m_aucName, THREAD_This()->m_aucName);
#endif

    status = pthread_mutex_unlock(&_pMutex->m_tMutex);
	return status;
}

int MUTEX_Trylock(T_Mutex *_pMutex)
{
    int status;

    CHECK_STACK();
    ASSERT_RETURN(_pMutex, EO_INVAL);

    ZXY_DLOG(LOG_DEBUG, "mutex[%s]:thread %s is trying", _pMutex->m_aucName, THREAD_This()->m_aucName);

    status = pthread_mutex_trylock(&_pMutex->m_tMutex);

    if (status==0) {
#if ZXY_EN_MUTEX_DEBUG
		_pMutex->m_ptOwer = THREAD_This();
		strcpy(_pMutex->m_aucOwerName, _pMutex->m_ptOwer->m_aucName);
		++_pMutex->m_u32Nesting;
		ZXY_DLOG(LOG_DEBUG, "mutex[%s]:acquired by thread %s (level = %d)", _pMutex->m_aucName, THREAD_This()->m_aucName, _pMutex->m_u32Nesting);
#else
		ZXY_DLOG(LOG_DEBUG, "mutex[%s]:acquired by thread %s", _pMutex->m_aucName, THREAD_This()->m_aucName);
#endif
    } else {
		ZXY_ELOG("mutex[%s]:thread %s's trylock() failed", _pMutex->m_aucName, THREAD_This()->m_aucName);
    }

	return status;
}

int MUTEX_Destroy(T_Mutex *_pMutex)
{
    enum {RETRY = 4};
    int status = 0;
    unsigned retry;

    CHECK_STACK();
    ASSERT_RETURN(_pMutex, EO_INVAL);

    ZXY_DLOG(LOG_DEBUG, "mutex[%s]:destroyed by thread %s", _pMutex->m_aucName, THREAD_This()->m_aucName);

    for (retry = 0; retry < RETRY; ++retry) {
		status = pthread_mutex_destroy(&_pMutex->m_tMutex);
		if (status == 0)
			break;
		else if (retry < RETRY-1 && status == EBUSY)
			pthread_mutex_unlock(&_pMutex->m_tMutex);
    }

	return status;
}

#if ZXY_EN_MUTEX_DEBUG
char MUTEX_IsLocked(T_Mutex *_pMutex)
{
    return _pMutex->m_ptOwer == THREAD_This();
}
#endif

///////////////////////////////////////////////////////////////////////////////
//可选配置
#if defined(ZXY_HAS_ATOMIC) && ZXY_HAS_ATOMIC!=0

struct _tagAtomic
{
    T_Mutex	*m_ptMutex;
    int			m_u32Value;
};

API_DEF(int) ATOMIC_Create(T_PoolInfo *_ptPool, int _u32Value, T_Atomic **_p2Out)
{
    int u32Ret;
    T_Atomic *pAtomic;
    pAtomic = POOL_ZALLOC_T(_ptPool, T_Atomic);
    ASSERT_RETURN(pAtomic, -1);

    u32Ret = MUTEX_Create(_ptPool, "atm%p", MUTEX_TYPE_SIMPLE, &pAtomic->m_ptMutex);
    if (u32Ret != 0) return u32Ret;

    pAtomic->m_u32Value = _u32Value;
    *_p2Out = pAtomic;
    return 0;
}

API_DEF(int) ATOMIC_Destroy(T_Atomic *_pAtomic)
{
    ASSERT_RETURN(_pAtomic, -1);
    return MUTEX_Destroy(_pAtomic->m_ptMutex);
}

API_DEF(void) ATOMIC_Set(T_Atomic *_pAtomic, int _u32Value)
{
    //CHECK_STACK();
    MUTEX_Lock(_pAtomic->m_ptMutex);
    _pAtomic->m_u32Value = _u32Value;
    MUTEX_Unlock(_pAtomic->m_ptMutex);
}

API_DEF(int) ATOMIC_Get(T_Atomic *_pAtomic)
{
    int u32Value;
    CHECK_STACK();
    MUTEX_Lock(_pAtomic->m_ptMutex);
    u32Value = _pAtomic->m_u32Value;
    MUTEX_Unlock(_pAtomic->m_ptMutex);
    return u32Value;
}

API_DEF(int) ATOMIC_IncGet(T_Atomic *_pAtomic)
{
    int u32NewValue;
    CHECK_STACK();
    MUTEX_Lock( _pAtomic->m_ptMutex);
    u32NewValue = ++_pAtomic->m_u32Value;
    MUTEX_Unlock(_pAtomic->m_ptMutex);
    return u32NewValue;
}

API_DEF(void) ATOMIC_Inc(T_Atomic *_pAtomic)
{
    ATOMIC_IncGet(_pAtomic);
}

API_DEF(int) ATOMIC_DecGet(T_Atomic *_pAtomic)
{
    int u32NewValue;
    CHECK_STACK();
    MUTEX_Lock(_pAtomic->m_ptMutex);
    u32NewValue = --_pAtomic->m_u32Value;
    MUTEX_Unlock(_pAtomic->m_ptMutex);
    return u32NewValue;
}

API_DEF(void) ATOMIC_Dec(T_Atomic *_pAtomic)
{
    ATOMIC_DecGet(_pAtomic);
}

API_DEF(int) ATOMIC_AddGet(T_Atomic *_pAtomic, int _u32Value)
{
    int u32NewValue;
    MUTEX_Lock(_pAtomic->m_ptMutex);
    _pAtomic->m_u32Value += _u32Value;
    u32NewValue = _pAtomic->m_u32Value;
    MUTEX_Unlock(_pAtomic->m_ptMutex);
    return u32NewValue;
}

API_DEF(void) ATOMIC_Add(T_Atomic *_pAtomic, int _u32Value)
{
    ATOMIC_AddGet(_pAtomic, _u32Value);
}

#endif

///////////////////////////////////////////////////////////////////////////
static char s_bInitialized = 0;

int OSLIB_Init(void)
{
    if (s_bInitialized) {
		++s_bInitialized;
		return 0;
    }
	
	//1.初始化线程
    if (THREAD_Init()!= 0) return -1;
	//2.初始化全局锁
    if (init_mutex(&s_tCriticalSection, "critsec", MUTEX_TYPE_RECURSE)!= 0) return -1;	
    ++s_bInitialized;
    assert(s_bInitialized == 1);
	
	ZXY_DLOG(LOG_TRACE, "OSLIB for POSIX initialized");
	
	return 0;
}


int OSLIB_SystemCheck(const char *_strCmd)
{
	int u32Ret = system(_strCmd);
	if (-1 == u32Ret){
		printf("system check error!");
	}else{
		ZXY_DLOG(LOG_DEBUG, "system [%s] exit status value = [0x%x]\n", _strCmd, u32Ret);
		if (WIFEXITED(u32Ret)){
			if (0 == WEXITSTATUS(u32Ret)){
				ZXY_DLOG(LOG_DEBUG, "run system [%s] successfully.\n", _strCmd);
			}else{
				ZXY_DLOG(LOG_WARNING, "run system [%s] fail, exit code: %d\n", _strCmd, WEXITSTATUS(u32Ret));
			}
		}else{
			ZXY_DLOG(LOG_WARNING, "system [%s] exit status = %d\n", _strCmd, WEXITSTATUS(u32Ret));
		}
	}

	return u32Ret;
}

