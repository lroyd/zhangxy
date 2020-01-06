/*******************************************************************************
	> File Name: .h
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_OSLIB_H__
#define __ZXY_OSLIB_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <pthread.h>

#include "typedefs.h"
#include "error_code.h"
#include "config_site.h"
#include "pool.h"
#include "console.h"



#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
API_DECL(void)			OSLIB_SubLogRegister(void);
API_DECL(int) 			OSLIB_SubLogSet(char _enLogLv ,char _bLogOn);
API_DECL(T_SubLogInfo*) OSLIB_SubLogGet(void);
#endif

API_DECL(int) OSLIB_Init(void);		//使用此库必须调用
API_DECL(int) OSLIB_SystemCheck(const char *_strCmd);
/////////////////////////////////////////////////////////////////////////////

#define THREAD_DESC_SIZE	    (64)

enum 
{
    THREAD_FLAG_SUSPENDED = 1
};

typedef struct _tagThreadInfo T_ThreadInfo;
typedef int (THREAD_PROC)(void *);
typedef long T_ThreadDesc[THREAD_DESC_SIZE];

API_DECL(int)	THREAD_Init(void);
API_DECL(int)	THREAD_Register(const char *_strName, T_ThreadDesc _tDesc, T_ThreadInfo **_p2Out);
API_DECL(char)	THREAD_IsRegistered(void);

API_DECL(int)	THREAD_Create(T_PoolInfo *_pPool, const char *_strName, THREAD_PROC *_pHandle, void *_pArg, size_t _stStackSize, unsigned _enFlags, T_ThreadInfo **_p2Out);
API_DECL(int) 	THREAD_Join(T_ThreadInfo *_pThread);
API_DECL(int) 	THREAD_Destroy(T_ThreadInfo *_pThread);
API_DECL(int) 	THREAD_Resume(T_ThreadInfo *_pThread);
API_DECL(void)	THREAD_MSleep(unsigned msec);

API_DECL(int)	THREAD_GetPrio(T_ThreadInfo *_pThread);
API_DECL(int)	THREAD_SetPrio(T_ThreadInfo *_pThread, int prio);
API_DECL(void*)	THREAD_GetHandle(T_ThreadInfo *_pThread);

API_DECL(T_ThreadInfo*)THREAD_This(void);
API_DECL(const char *)THREAD_GetName(T_ThreadInfo *_pThread);
API_DECL(unsigned int) THREAD_GetPid(void);

#if defined(ZXY_EN_THREAD_CHECK_STACK) && ZXY_EN_THREAD_CHECK_STACK!=0
	#define CHECK_STACK() THREAD_CheckStack(__FILE__, __LINE__)
	API_DECL(void)	THREAD_CheckStack(const char *, int);
	API_DECL(int)	THREAD_GetStackInfo(T_ThreadInfo *, const char **, int *);
	API_DECL(unsigned int) THREAD_GetStackMaxUsage(T_ThreadInfo *);
#else
	#define CHECK_STACK()
	#define THREAD_GetStackMaxUsage(p)			(0)
	#define THREAD_GetStackInfo(p, f, l)		(*(f)="",*(l)=0)
#endif

///////////////////////////////////////////////////////////////////////////////////////
typedef struct _tagMutex T_Mutex;

enum
{
    MUTEX_TYPE_DEFAULT,
    MUTEX_TYPE_SIMPLE,
    MUTEX_TYPE_RECURSE
};

#define ZXY_MAX_NAME_LEN	32

API_DECL(void) MUTEX_EnterCritical(void);
API_DECL(void) MUTEX_LeaveCritical(void);

API_DECL(int) MUTEX_Create(T_PoolInfo *_pPool, const char *_strName, int _enType, T_Mutex **_p2Out);
API_DECL(int) MUTEX_CreateSimple(T_PoolInfo *_pPool, const char *_strName, T_Mutex **_p2Out);
API_DECL(int) MUTEX_CreateRecursive(T_PoolInfo *_pPool, const char *_strName, T_Mutex **_p2Out);
API_DECL(int) MUTEX_Lock(T_Mutex *_pMutex);
API_DECL(int) MUTEX_Unlock(T_Mutex *_pMutex);
API_DECL(int) MUTEX_Trylock(T_Mutex *_pMutex);
API_DECL(int) MUTEX_Destroy(T_Mutex *_pMutex);

#if ZXY_EN_MUTEX_DEBUG
char MUTEX_IsLocked(T_Mutex *_pMutex);
#endif

////////////////////////////////////////////////////////////////////

#if defined(ZXY_HAS_ATOMIC) && ZXY_HAS_ATOMIC!=0

typedef struct _tagAtomic T_Atomic;

API_DECL(int) 	ATOMIC_Create(T_PoolInfo *pool, int initial, T_Atomic **ptr_atomic);
API_DECL(int) 	ATOMIC_Destroy(T_Atomic *atomic_var);
API_DECL(void) 	ATOMIC_Set(T_Atomic *atomic_var, int value);
API_DECL(int)	ATOMIC_Get(T_Atomic *atomic_var);
API_DECL(void) 	ATOMIC_Inc(T_Atomic *atomic_var);
API_DECL(void) 	ATOMIC_Dec(T_Atomic *atomic_var);
API_DECL(void)	ATOMIC_Add(T_Atomic *_pAtomic, int _u32Value);

API_DECL(int)	ATOMIC_IncGet(T_Atomic *_pAtomic);
API_DECL(int)	ATOMIC_DecGet(T_Atomic *_pAtomic);
API_DECL(int)	ATOMIC_AddGet(T_Atomic *_pAtomic, int _u32Value);

#endif




#endif
