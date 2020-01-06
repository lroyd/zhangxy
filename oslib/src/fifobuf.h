#ifndef __ZXY_FIFOBUF_H__
#define __ZXY_FIFOBUF_H__

#include "config_site.h"
#include "typedefs.h"

typedef struct _tagFifobuf
{
    char *m_pStart, *m_pEnd;	//缓冲区地址的起始和结束
    char *m_pRead, *m_pWrite;		//指向数据的开始和结束
    char m_IsFull;
}T_Fifobuf;

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
API_DEF(void) 			FIFO_SubLogRegister(void);
API_DEF(int)			FIFO_SubLogSet(char _enLogLv ,char _bLogOn);
API_DEF(T_SubLogInfo*)	FIFO_SubLogGet(void);
#endif

API_DEF(void)		FIFO_Init(T_Fifobuf *_pThis, void *_vpBuffer, unsigned _uSize);
API_DEF(unsigned)	FIFO_GetMaxSize(T_Fifobuf *_pThis);
API_DEF(void*)		FIFO_Alloc(T_Fifobuf *_pThis, unsigned _uSize);	//返回已经分配的地址，注意，使用时不要超过自己分配的size(未作检查)
API_DEF(int)		FIFO_Unalloc(T_Fifobuf *_pThis, void *_vpBuffer);	//重置w
API_DEF(int)		FIFO_Free(T_Fifobuf *_pThis, void *_vpBuffer);	//重置r


#endif	/* __ZXY_FIFOBUF_H__ */

