#include "port.h"

#define CHECK_STACK();

int PORT_Create(T_PoolInfo *_pPool, const char *_strName, void *_pUsrData, T_PortHandle *_pHandle, T_PortInfo **_ppOut)
{
	ASSERT_RETURN(_pPool && _ppOut && _strName, EO_INVAL);
    T_PortInfo *pPort = POOL_ZALLOC_T(_pPool, T_PortInfo);
    ASSERT_RETURN(pPort, EO_NOMEM);	
	
	c_strdup2_with_null(_pPool, &pPort->m_strName, _strName);
	pPort->m_pUsrData	= _pUsrData;
	pPort->m_onFrame	= _pHandle;
	
	*_ppOut = pPort;
	return EO_SUCCESS;
}

//销毁下属端口
int PORT_Destroy(T_PortInfo *_pPort)
{
	//_pPort->m_onFrame	= NULL;
	T_PortInfo *pMaster = _pPort->in_pMaster;
	pMaster->in_apPortList[_pPort->m_u32Slot] = NULL;
	
	return EO_SUCCESS;
}

int PORT_Connect(T_PortInfo *_pMaster, T_PortInfo *_pSub)
{
	ASSERT_RETURN(_pMaster && _pSub, EO_INVAL);
	_pSub->in_pMaster = _pMaster;
	int i;
	for (i = 0; i < PORT_ARRAY_MAX; i++){
		if (!_pMaster->in_apPortList[i]){
			_pMaster->in_apPortList[i] = _pSub;
			_pSub->m_u32Slot = i;
			return EO_SUCCESS;
		}
	}
	
	return EO_FAILED;
}




//注意 栈检查
int PORT_Dispatch(char _enType, void *_pPort, T_Frame *_pFrame)
{
	ASSERT_RETURN(_pPort, EO_INVAL);
	int i = 0, u32Ret = EO_SUCCESS;
	T_PortInfo *pPort = (T_PortInfo *)_pPort, *pTmp = NULL;
	PORT_DATA_CALLBCK *atpPortHandle = (PORT_DATA_CALLBCK *)pPort->m_onFrame;
	if (atpPortHandle[_enType]){
		u32Ret = (atpPortHandle[_enType])(pPort, _pFrame);
		if (u32Ret) goto EXIT;
	}
	while(i < PORT_ARRAY_MAX){
		if (pTmp = pPort->in_apPortList[i++]){
			u32Ret = PORT_Dispatch(_enType, pTmp, _pFrame);
			if (u32Ret) goto EXIT;			
		}
	}
EXIT:
	return u32Ret;
}

int PORT_Process(void *_pPort, T_Frame *_pFrame)
{
	return PORT_Dispatch(PORT_FUNC_DATA, _pPort, _pFrame);	
}
//格式"port0(3): 1.port1, 2.port2, 3.port3"
char *PORT_Dump0(T_PortInfo *_pPort, char *_pBuffer, unsigned _uSize)
{
	char u8Cnt = 0, u8Len = 0, i;
	char pBuffer[64] = {0}
	CHECK_STACK();
	for (i = 0; i < PORT_ARRAY_MAX; i++){
		if (_pPort->in_apPortList[i]){
			T_PortInfo *pNode = _pPort->in_apPortList[i];
			u8Len += sprintf(pBuffer + u8Len, "%d.%s ", i + 1, c_strbuf(&pNode->m_strName));
			u8Cnt++;
		}
	}

	snprintf(_pBuffer, _uSize, "%s(%d): %s", c_strbuf(&_pPort->m_strName), u8Cnt, pBuffer);
	return _pBuffer;
}



