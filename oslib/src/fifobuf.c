#include "fifobuf.h"

#include "console.h"
#include "error_code.h"

#define SZ  sizeof(unsigned)

#define CHECK_STACK()		//不使用检查


#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"fifo", ZXY_LOG_DEBUG_LV, 0};	//默认关闭
static char s_u8LogId = -1;

void FIFO_SubLogRegister(void)
{
	s_u8LogId = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
}

int FIFO_SubLogSet(char _enLogLv ,char _bLogOn)
{
	CONSOLE_SubLogSetParam(s_u8LogId, _enLogLv, _bLogOn);
}

T_SubLogInfo* FIFO_SubLogGet(void)
{
	return CONSOLE_SubLogGetParam(s_u8LogId);
}
#endif

void FIFO_Init(T_Fifobuf *_pThis, void *_vpBuffer, unsigned _uSize)
{
    CHECK_STACK();
    ZXY_DLOG(LOG_DEBUG, "fifobuf = %p, buffer = %p, size = %d", _pThis, _vpBuffer, _uSize);
    _pThis->m_pStart = (char*)_vpBuffer;
    _pThis->m_pEnd = _pThis->m_pStart + _uSize;
    _pThis->m_pRead = _pThis->m_pWrite = _pThis->m_pStart;
    _pThis->m_IsFull = 0;
}

unsigned FIFO_GetMaxSize(T_Fifobuf *_pThis)
{
    unsigned s1, s2;
    CHECK_STACK();
    if (_pThis->m_pWrite >= _pThis->m_pRead){
		s1 = (unsigned)(_pThis->m_pEnd - _pThis->m_pWrite);
		s2 = (unsigned)(_pThis->m_pRead - _pThis->m_pStart);
    }
	else{
		s1 = s2 = (unsigned)(_pThis->m_pRead - _pThis->m_pWrite);
    }
    return s1<s2 ? s2 : s1;
}

void *FIFO_Alloc(T_Fifobuf *_pThis, unsigned _uSize)
{
    unsigned available;
    char *start;
    CHECK_STACK();
    if (_pThis->m_IsFull){
		ZXY_ELOG("fifobuf = %p, size = %d, full!!!", _pThis, _uSize);
		return NULL;
    }

    if (_pThis->m_pWrite >= _pThis->m_pRead){
		available = (unsigned)(_pThis->m_pEnd - _pThis->m_pWrite);
		if (available >= _uSize + SZ){
		    char *ptr = _pThis->m_pWrite;
		    _pThis->m_pWrite += (_uSize + SZ);
		    if (_pThis->m_pWrite == _pThis->m_pEnd)
				_pThis->m_pWrite = _pThis->m_pStart;
		    if (_pThis->m_pWrite == _pThis->m_pRead)
				_pThis->m_IsFull = 1;
		    *(unsigned*)ptr = _uSize + SZ;
		    ptr += SZ;
		    ZXY_DLOG(LOG_DEBUG, "fifobuf = %p, size = %d: ret = %p, r = %p, w = %p", _pThis, _uSize, ptr, _pThis->m_pRead, _pThis->m_pWrite);
		    return ptr;
		}
    }

    start = (_pThis->m_pWrite <= _pThis->m_pRead) ? _pThis->m_pWrite : _pThis->m_pStart;
    available = (unsigned)(_pThis->m_pRead - start);
    if (available >= _uSize + SZ){
		char *ptr = start;
		_pThis->m_pWrite = start + _uSize + SZ;
		if (_pThis->m_pWrite == _pThis->m_pRead)
			_pThis->m_IsFull = 1;
		*(unsigned*)ptr = _uSize + SZ;
		ptr += SZ;
		ZXY_DLOG(LOG_DEBUG, "fifobuf = %p, size = %d: ret = %p, r = %p, w = %p", _pThis, _uSize, ptr, _pThis->m_pRead, _pThis->m_pWrite);
		return ptr;
    }

    ZXY_ELOG("fifobuf = %p, size = %d: no space left! r = %p, w = %p", _pThis, _uSize, _pThis->m_pRead, _pThis->m_pWrite);
    return NULL;
}

int FIFO_Unalloc(T_Fifobuf *_pThis, void *_vpBuffer)
{
    char *ptr = (char*)_vpBuffer;
    char *endptr;
    unsigned sz;
    CHECK_STACK();
    ptr -= SZ;	//找到当前已经分配缓冲的大小
    sz = *(unsigned*)ptr;

    endptr = _pThis->m_pWrite;
    if (endptr == _pThis->m_pStart)
		endptr = _pThis->m_pEnd;
	
    if (ptr + sz != endptr){
		ZXY_ELOG("invalid pointer to undo alloc");
		return EO_INVAL;
    }

    _pThis->m_pWrite = ptr;
    _pThis->m_IsFull = 0;
    ZXY_DLOG(LOG_DEBUG, "fifobuf = %p, ptr = %p, size = %d, r = %p, w = %p", _pThis, _vpBuffer, sz, _pThis->m_pRead, _pThis->m_pWrite);
    return EO_SUCCESS;
}

int FIFO_Free(T_Fifobuf *_pThis, void *_vpBuffer)
{
    char *ptr = (char*)_vpBuffer;
    char *end;
    unsigned sz;
    CHECK_STACK();
    ptr -= SZ;
    if (ptr < _pThis->m_pStart || ptr >= _pThis->m_pEnd){
		ZXY_ELOG("invalid pointer to free");
		return EO_INVAL;
    }

    if (ptr != _pThis->m_pRead && ptr != _pThis->m_pStart){
		ZXY_ELOG("invalid free() sequence!");
		return EO_INVAL;
    }

    end = (_pThis->m_pWrite > _pThis->m_pRead) ? _pThis->m_pWrite : _pThis->m_pEnd;
    sz = *(unsigned*)ptr;
    if (ptr + sz > end){
		ZXY_ELOG("invalid size!");
		return EO_INVAL;
    }

    _pThis->m_pRead = ptr + sz;

    if (_pThis->m_pRead == _pThis->m_pEnd)
		_pThis->m_pRead = _pThis->m_pStart;

    if (_pThis->m_pRead == _pThis->m_pWrite)
		_pThis->m_pRead = _pThis->m_pWrite = _pThis->m_pStart;
    _pThis->m_IsFull = 0;
    ZXY_DLOG(LOG_DEBUG, "fifobuf = %p, ptr = %p, size = %d, r = %p, w = %p", _pThis, _vpBuffer, sz, _pThis->m_pRead, _pThis->m_pWrite);
    return EO_SUCCESS;
}


