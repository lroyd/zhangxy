#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "lock.h"

typedef void LOCK_OBJ;
typedef int (*FPTR)(LOCK_OBJ*);

struct _tagLock
{
    LOCK_OBJ *m_pLock;

    int	(*funcAcquire)		(LOCK_OBJ*);
    int	(*funcTryacquire)	(LOCK_OBJ*);
    int	(*funcRelease)		(LOCK_OBJ*);
    int	(*funcDestroy)		(LOCK_OBJ*);
};

static T_Lock mutex_lock_template = 
{
    NULL,
    (FPTR) &MUTEX_Lock,
    (FPTR) &MUTEX_Trylock,
    (FPTR) &MUTEX_Unlock,
    (FPTR) &MUTEX_Destroy
};

//主要创建T_Lock
static int create_mutex_lock(T_PoolInfo *_pPool, const char *_strName, int type, T_Lock **_p2Out)
{
    T_Lock *pLock;
    T_Mutex *pMutex;
    int u32Ret;

    ASSERT_RETURN(_pPool && _p2Out, EO_INVAL);

    pLock = POOL_ALLOC_T(_pPool, T_Lock);
    if (!pLock) return EO_NOMEM;

    memcpy(pLock, &mutex_lock_template, sizeof(T_Lock));
    u32Ret = MUTEX_Create(_pPool, _strName, type, &pMutex);
    if (u32Ret != 0) return u32Ret;

    pLock->m_pLock = pMutex;
    *_p2Out = pLock;
	return EO_SUCCESS;
}


int LOCK_CreateSimpleMutex(T_PoolInfo *_pPool, const char *_strName, T_Lock **_p2Out)
{
    return create_mutex_lock(_pPool, _strName, MUTEX_TYPE_SIMPLE, _p2Out);
}

int LOCK_CreateRecursiveMutex( T_PoolInfo *_pPool, const char *_strName, T_Lock **_p2Out )
{
    return create_mutex_lock(_pPool, _strName, MUTEX_TYPE_RECURSE, _p2Out);
}

int LOCK_Acquire(T_Lock *_pLock)
{
    ASSERT_RETURN(_pLock != NULL, EO_INVAL);
    return (*_pLock->funcAcquire)(_pLock->m_pLock);
}

int LOCK_Tryacquire(T_Lock *_pLock)
{
    ASSERT_RETURN(_pLock != NULL, EO_INVAL);
    return (*_pLock->funcTryacquire)(_pLock->m_pLock);
}

int LOCK_Release(T_Lock *_pLock)
{
    ASSERT_RETURN(_pLock != NULL, EO_INVAL);
    return (*_pLock->funcRelease)(_pLock->m_pLock);
}

int LOCK_Destroy(T_Lock *_pLock)
{
    ASSERT_RETURN(_pLock != NULL, EO_INVAL);
    return (*_pLock->funcDestroy)(_pLock->m_pLock);
}


