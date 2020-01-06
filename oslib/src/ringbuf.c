/*************************************************************************
  > File Name:          ring_buffer.h
  > Author:             Lroyd.H
  > Mail:               
  > Description: 
  > Created Time:       
 ************************************************************************/
#include "ringbuf.h"
#include "console.h"

#define CHECK_STACK()		//不使用检查

struct _tagRingBuf
{
    char                *m_pBuffer;
    unsigned             m_uMaxSize;
    volatile unsigned	m_u32ReadIndex:31;
    volatile unsigned   m_u32ReadMirror:1;
    volatile unsigned   m_u32WriteIndex:31;
    volatile unsigned   m_u32WriteMirror:1;
};

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"_pThis", ZXY_LOG_DEBUG_LV, 0};	//默认关闭
static char s_u8LogId = -1;

void RING_SubLogRegister(void)
{
	s_u8LogId = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
}

int RING_SubLogSet(char _enLogLv ,char _bLogOn)
{
	CONSOLE_SubLogSetParam(s_u8LogId, _enLogLv, _bLogOn);
}

T_SubLogInfo* RING_SubLogGet(void)
{
	return CONSOLE_SubLogGetParam(s_u8LogId);
}
#endif

void RING_Init(T_RingBuf *_pThis, void *_vpBuffer, unsigned _uSize)
{
	CHECK_STACK();
	ZXY_DLOG(LOG_DEBUG, "ringbuf = %p, buffer = %p, size = %d", _pThis, _vpBuffer, _uSize);
    _pThis->m_pBuffer = (char *)_vpBuffer;
    _pThis->m_uMaxSize = _uSize;
    _pThis->m_u32ReadIndex = _pThis->m_u32WriteIndex = 0;
    _pThis->m_u32ReadMirror = _pThis->m_u32WriteMirror = 0;
}

unsigned RING_GetMaxSize(T_RingBuf *_pThis)
{
	return _pThis->m_uMaxSize;
}

unsigned RING_GetDataLen(T_RingBuf *_pThis)
{
    if(_pThis->m_u32ReadIndex == _pThis->m_u32WriteIndex){
        if(_pThis->m_u32ReadMirror == _pThis->m_u32WriteMirror) return 0;
        return _pThis->m_uMaxSize;
    }

    if(_pThis->m_u32ReadIndex > _pThis->m_u32WriteIndex){
        return _pThis->m_uMaxSize - (_pThis->m_u32ReadIndex - _pThis->m_u32WriteIndex);
    }
    return _pThis->m_u32WriteIndex - _pThis->m_u32ReadIndex;	
}

unsigned RING_GetSpaceLen(T_RingBuf *_pThis)
{
	return _pThis->m_uMaxSize - RING_GetDataLen(_pThis);
}


unsigned RING_Write(T_RingBuf *_pThis, const char *_pData, unsigned _uLen)
{
    unsigned free_space = RING_GetSpaceLen(_pThis);
	CHECK_STACK();
    if(_uLen < _pThis->m_uMaxSize - _pThis->m_u32WriteIndex){
        memcpy(&_pThis->m_pBuffer[_pThis->m_u32WriteIndex], &_pData[0], _uLen);
        _pThis->m_u32WriteIndex += _uLen;
        if(_uLen > free_space)
            _pThis->m_u32ReadIndex = _pThis->m_u32WriteIndex;
        return _uLen;
    }

    memcpy(&_pThis->m_pBuffer[_pThis->m_u32WriteIndex], &_pData[0], _pThis->m_uMaxSize - _pThis->m_u32WriteIndex);
    memcpy(&_pThis->m_pBuffer[0], &_pData[_pThis->m_uMaxSize - _pThis->m_u32WriteIndex], _uLen - (_pThis->m_uMaxSize - _pThis->m_u32WriteIndex));

    _pThis->m_u32WriteIndex = _uLen - (_pThis->m_uMaxSize - _pThis->m_u32WriteIndex);
    _pThis->m_u32WriteMirror = ~_pThis->m_u32WriteMirror;

    if(_uLen > free_space){
        _pThis->m_u32ReadMirror = ~_pThis->m_u32ReadMirror;
        _pThis->m_u32ReadIndex = _pThis->m_u32WriteIndex;
    }
    
    return _uLen;
}

unsigned RING_Read(T_RingBuf *_pThis, char *_pBuf, unsigned _uLen)
{
    unsigned uDataLen = RING_GetDataLen(_pThis);
	CHECK_STACK();
    if(uDataLen == 0) return 0;
    if(uDataLen < _uLen) _uLen  = uDataLen;

    if(_uLen < _pThis->m_uMaxSize - _pThis->m_u32ReadIndex){
        memcpy(&_pBuf[0], &_pThis->m_pBuffer[_pThis->m_u32ReadIndex], _uLen);
        _pThis->m_u32ReadIndex += _uLen;
        return _uLen;
    }

    memcpy(&_pBuf[0], &_pThis->m_pBuffer[_pThis->m_u32ReadIndex], _pThis->m_uMaxSize - _pThis->m_u32ReadIndex);
    memcpy(&_pBuf[_pThis->m_uMaxSize  -_pThis->m_u32ReadIndex], &_pThis->m_pBuffer[0], _uLen - (_pThis->m_uMaxSize - _pThis->m_u32ReadIndex));

    _pThis->m_u32ReadMirror = ~_pThis->m_u32ReadMirror;
    _pThis->m_u32ReadIndex = _uLen - (_pThis->m_uMaxSize - _pThis->m_u32ReadIndex);

    return _uLen;
}

