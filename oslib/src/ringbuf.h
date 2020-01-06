/*************************************************************************
  > File Name:          
  > Author:             Lroyd.H
  > Mail:               
  > Description: 
  > Created Time:       
 ************************************************************************/
#ifndef __ZXY_RINGBUF_H__
#define __ZXY_RINGBUF_H__

#include "config_site.h"
#include "typedefs.h"

#define RING_BUF_DEFAULT_SIZE       (512)

typedef struct _tagRingBuf T_RingBuf;

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
API_DEF(void) 			RING_SubLogRegister(void);
API_DEF(int)			RING_SubLogSet(char _enLogLv ,char _bLogOn);
API_DEF(T_SubLogInfo*)	RING_SubLogGet(void);
#endif

API_DEF(void)		RING_Init(T_RingBuf *_pThis, void *_vpBuffer, unsigned _uSize);
API_DEF(unsigned)	RING_GetMaxSize(T_RingBuf *_pThis);
API_DEF(unsigned)	RING_GetDataLen(T_RingBuf *_pThis);
API_DEF(unsigned)	RING_GetSpaceLen(T_RingBuf *_pThis);
API_DEF(unsigned) 	RING_Write(T_RingBuf *ring, const char *data, unsigned len);
API_DEF(unsigned) 	RING_Read(T_RingBuf *ring, char *buf, unsigned len);

#endif
