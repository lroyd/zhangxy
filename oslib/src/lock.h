/*******************************************************************************
	> File Name: .h
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_LOCK_H__
#define __ZXY_LOCK_H__

#include "os.h"

typedef struct _tagLock T_Lock;
typedef void T_GrpLock;		//typedef struct T_GrpLock T_GrpLock;

API_DECL(int) LOCK_CreateSimpleMutex(T_PoolInfo *_pPool,const char *_strName, T_Lock **_p2Out);
API_DECL(int) LOCK_CreateRecursiveMutex(T_PoolInfo *_pPool,const char *_strName, T_Lock **_p2Out);

API_DECL(int) LOCK_Acquire(T_Lock *_pLock);
API_DECL(int) LOCK_Tryacquire(T_Lock *_pLock);
API_DECL(int) LOCK_Release(T_Lock *_pLock);
API_DECL(int) LOCK_Destroy(T_Lock *_pLock);




#endif

