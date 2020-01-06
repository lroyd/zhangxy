/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include "amio_dev.h"
#include "amio_drv.h"
#include "console.h"

#define CHECK_STACK()

#define MAKE_DEV_ID(f_id, index)	(((f_id & 0xFFFF) << 16) | (index & 0xFFFF))
#define GET_INDEX(dev_id)	   		((dev_id) & 0xFFFF)
#define GET_FID(dev_id)		   		((dev_id) >> 16)

static T_AmioSubInfo g_tSubInfo = {0};	//全局唯一

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0





#endif

/************************************************************************
* Name			:device_active
* Descriptions	:主要计算节点范围
* Parameter		:1.int _u32Index 驱动索引节点
*				:2.char _bRefresh(0/1) 刷新
*				:3.const char *_strPathIni 配置文件路径
* Return		:-1/0
* **********************************************************************/
static int device_active(int _u32Index, char _bRefresh, const char *_strPathIni)
{
    T_AmioDriver *pDriver = &g_tSubInfo.m_atDriver[_u32Index];
    T_AmioDevFactory *pFactory = pDriver->in_pFactory;
    int i, u32Ret;
	
    if (_bRefresh) {
		pFactory->in_pCtrl->funcDeinit(pFactory);
    } else {
		if(c_strbuf(&pFactory->in_pInfo->m_strPathIni)){
			return 0;
		}
    }

	u32Ret = pFactory->in_pCtrl->funcInit(pFactory, _strPathIni);
	if (u32Ret){
		pFactory->in_pCtrl->funcDeinit(pFactory);
		pDriver->m_u32DefaultIdx = AMIO_DEV_INVALID;
		return u32Ret;
	}

	//获取当前设备信息
	T_AmioDevInfo *pInfo = pDriver->in_pFactory->in_pInfo;
	//printf("device name:%s, total:%d, in:%d, out:%d\r\n", pInfo->m_strDevName, pInfo->m_u8ChannelCnt, pInfo->m_u8InputCnt, pInfo->m_u8OutputCnt);
	
	pDriver->m_u32DefaultIdx = 0;	//设置设备的默认channel
	
    pDriver->in_pFactory->sys.m_u32Index = _u32Index;

	pDriver->m_u32ChnlCnt	= pInfo->m_u8ChnnlTotal;
    pDriver->m_u32StartIdx	= g_tSubInfo.m_u8ChnlToal;	//起始为0
	
    for (i = 0; i < pInfo->m_u8ChnnlTotal; ++i) {
		//printf("make list[%d] = %d | %d\r\n", g_tSubInfo.m_u8ChnlToal, _u32Index, i);
		g_tSubInfo.m_au32ChnlList[g_tSubInfo.m_u8ChnlToal++] = MAKE_DEV_ID(_u32Index, i);
    }
	
    return 0;
}

static void device_frozen(int _u32Index)
{
    T_AmioDriver *pDriver = &g_tSubInfo.m_atDriver[_u32Index];
    if (pDriver->in_pFactory) {
		pDriver->in_pFactory->in_pCtrl->funcDeinit(pDriver->in_pFactory);
		//pDriver->in_pFactory = NULL;
    }
}

/************************************************************************
* Name			:
* Descriptions	:
* Parameter		:
* Return		:
* **********************************************************************/
//只初始化一次
int AMIO_DrvLoad(T_PoolFactory *_pPool)
{
	int i;
	T_AmioSubInfo *pSub = &g_tSubInfo;
    if (pSub->m_u8InitCnt++ != 0){
		return pSub->m_u8DrvTotal;
    }
    pSub->pf = _pPool;
    pSub->m_u8DrvTotal = 0;
	
	device_load(pSub);
	/* 枚举所有支持的设备的驱动 */
    for (i = 0; i < pSub->m_u8DrvTotal; ++i) {
		//一般不会失败，除非空间不足
		pSub->m_atDriver[i].in_pFactory = (pSub->m_atDriver[i].pCreate)(_pPool);
		if (pSub->m_atDriver[i].in_pFactory == NULL){
			printf("xxxxxxxxxxxxx\r\n");
			break;
		}
    }
	pSub->m_u8DrvTotal = i + 1;
    return pSub->m_u8DrvTotal;
}

int AMIO_DrvUnload(void)
{
    unsigned i;
    T_AmioSubInfo *pSub = &g_tSubInfo;
    if (pSub->m_u8InitCnt == 0){
		return 0;
    }
    --pSub->m_u8InitCnt;
    if (pSub->m_u8InitCnt == 0){
		for (i = 0; i < pSub->m_u8DrvTotal; ++i){
		    device_frozen(i);
		}
		pSub->pf = NULL;
    }
    return 0;
}
/************************************************************************
* Name			:
* Descriptions	:
* Parameter		:
* Return		:
* **********************************************************************/
int AMIO_DeviceTotal(void)
{
    return g_tSubInfo.m_u8DrvTotal;
}

/************************************************************************
* Name			:AMIO_DevDump
* Descriptions	:
* Parameter		:
* Return		:
* **********************************************************************/
char *AMIO_DevDump(T_AmioDevInfo *_pInfo, char *_pBuffer, int _u32Len)
{
	int i, u32Len = 0;
	char enType = 0;	//注意长度
	T_AmioDevInfo *pDevInfo = _pInfo;
	CHECK_STACK();
	if (pDevInfo->m_enType == AMIO_DEV_TYPE_NONE){
		u32Len += snprintf(_pBuffer + u32Len, _u32Len - u32Len,"device:\"null\"");
	}else{
		u32Len += snprintf(_pBuffer + u32Len, _u32Len - u32Len, "device:\"%s\", \"%s\", \"%s\", type = 0x%02x, count = %d", 
											c_strbuf(&pDevInfo->m_strName), c_strbuf(&pDevInfo->m_strVer), c_strbuf(&pDevInfo->m_strPathIni),
											pDevInfo->m_enType, pDevInfo->m_u8ChnnlTotal);		
		if (c_strbuf(&pDevInfo->m_strPathIni)){
			if (pDevInfo->m_u8ChnnlTotal){
				//通道信息
				T_AmioChannelInfo *pChnlInfo = NULL;
				for (i = 0; i < pDevInfo->m_u8ChnnlTotal; ++i){
					pChnlInfo = pDevInfo->m_aptChannelInfo[i];
					enType = pChnlInfo->m_enType;
					if (enType == AMIO_DEV_TYPE_NONE) continue;
					//基础参数
					u32Len += snprintf(_pBuffer + u32Len, _u32Len - u32Len,"\r\n    %d.\"%s\" fd[%d] type[0x%02x] dir[0x%02x] cap[0x%02x] stream[%p] ", i+1, 
										c_strbuf(&pChnlInfo->m_strName), 
										pChnlInfo->m_u32ChnlFd, 
										pChnlInfo->m_enType, 
										pChnlInfo->m_enDir, 
										pChnlInfo->m_enCap, 
										pChnlInfo->m_vpStream);
					switch(enType){
						case AMIO_DEV_TYPE_AUDIO:{
							T_AudioChannel *pAudio = (T_AudioChannel *)pChnlInfo;
							//音频参数
							u32Len += snprintf(_pBuffer + u32Len, _u32Len - u32Len,"audio[ec:%d, lookup:%d, sound:%d, rate:%d, width:%d, fmt:%d, vol:%d]",
												pAudio->m_bEC, pAudio->m_bLookup, pAudio->m_u8SndCnt, pAudio->m_u32SampleRate, pAudio->m_u32BitWidth, 
												pAudio->m_enChnnlFmt, pAudio->m_s32Volume);
							break;
						}
						
						case AMIO_DEV_TYPE_VIDEO:{
							T_VideoChannel *pAudio = (T_VideoChannel *)pChnlInfo;
							//视频参数
							
							
							
							break;
						}
						
					}
					
				}		

			}				
		}
	}
		
	return _pBuffer;
}

char *AMIO_DrvEnum(char *_pBuffer, unsigned _uSize)
{
	int i, j = 0;
	int u32Len = 0;
	T_AmioDevInfo *pDevInfo = NULL;
	CHECK_STACK();
	
    for (i = 0; i < g_tSubInfo.m_u8DrvTotal; ++i) {
		if (g_tSubInfo.m_atDriver[i].in_pFactory){
			u32Len += snprintf(_pBuffer + u32Len, _uSize, "[%d].", j++);
			pDevInfo = g_tSubInfo.m_atDriver[i].in_pFactory->in_pInfo;
			u32Len += strlen(AMIO_DevDump(pDevInfo, _pBuffer + u32Len, _uSize - u32Len));
			u32Len += snprintf(_pBuffer + u32Len, _uSize - u32Len, "\r\n");
		}
    }

	return _pBuffer;
}

/************************************************************************
* Name			:lookup_dev
* Descriptions	:1._pListIdx == AMIO_DEV_INVALID 直接返回-1/0
*				:2._pListIdx < 0 && _pListIdx != AMIO_DEV_INVALID 寻找默认节点，没有返回 -1
*				:3._pListIdx > 0 正常
* Parameter		:
* Return		:-1/0
* **********************************************************************/
static int lookup_dev(int _pListIdx, T_AmioDevFactory **_ppOutFactory, int *_pOutChnlIdx)
{
    int f_id, m_u32Idx, i;
    if (_pListIdx < 0){
		if (_pListIdx == AMIO_DEV_INVALID) return -1;
		for (i = 0; i < g_tSubInfo.m_u8DrvTotal; ++i){
		    T_AmioDriver *pDriver = &g_tSubInfo.m_atDriver[i];
		    if (pDriver->m_u32DefaultIdx >= 0){
				_pListIdx = g_tSubInfo.m_atDriver[i].m_u32StartIdx + pDriver->m_u32DefaultIdx;
				break;
		    }
		}
		if (_pListIdx < 0) return -1;
    }

    f_id = GET_FID(g_tSubInfo.m_au32ChnlList[_pListIdx]);  //driver id
    m_u32Idx = GET_INDEX(g_tSubInfo.m_au32ChnlList[_pListIdx]); //channel id

    if (f_id < 0 || f_id >= (int)g_tSubInfo.m_u8DrvTotal) return -1;

    if (m_u32Idx < 0 || m_u32Idx >= (int)g_tSubInfo.m_atDriver[f_id].m_u32ChnlCnt) return -1;

    *_ppOutFactory = g_tSubInfo.m_atDriver[f_id].in_pFactory;
    *_pOutChnlIdx = m_u32Idx;

    return 0;

}

/************************************************************************
* Name			:
* Descriptions	:
* Parameter		:
* Return		:
* **********************************************************************/
static int lookup_drv(const char *_pDevName, int *_pOutDrvIdx)
{
	int i, u32Ret = 1;
	T_AmioDevInfo *pDevInfo = NULL;
    for (i = 0; i < g_tSubInfo.m_u8DrvTotal; ++i) {
		if (g_tSubInfo.m_atDriver[i].in_pFactory){
			pDevInfo = g_tSubInfo.m_atDriver[i].in_pFactory->in_pInfo;
			if (pDevInfo){
				if (!c_strcmp2(&pDevInfo->m_strName, _pDevName)) {
					*_pOutDrvIdx = i;
					u32Ret = 0;
					break;
				}			
			}
			
		}
    }
	return u32Ret;
}

/************************************************************************
* Name			:
* Descriptions	:刷新设备的所有通道(不能删减)，重载配置文件
* Parameter		:
* Return		:
* **********************************************************************/
int AMIO_DevRefresh(const char *_pDevName, const char *_strPathIni)
{
	int u32DrvIdx;
	if (lookup_drv(_pDevName, &u32DrvIdx)){
		printf("no corresponding [%s]device driver found\r\n", _pDevName);
		return -1;		
	}
	
    T_AmioDevFactory *pFactory;
	pFactory = g_tSubInfo.m_atDriver[u32DrvIdx].in_pFactory;
    return pFactory->in_pCtrl->funcRefresh(pFactory, _strPathIni, 1);
}

/************************************************************************
* Name			:
* Descriptions	:
* Parameter		:
* Return		:
* **********************************************************************/
int AMIO_DevFrozen(const char *_pDevName)
{
	int u32DrvIdx;
	if (lookup_drv(_pDevName, &u32DrvIdx)){
		printf("no corresponding [%s]device driver found\r\n", _pDevName);
		return -1;
	}
	device_frozen(u32DrvIdx);
    return 0;
}

/************************************************************************
* Name			:AMIO_DevGetInfo
* Descriptions	:
* Parameter		:1.const char *_pDevName
*				:2.T_AmioDevInfo **_ppOutInfo
* Return		:-1/0
* **********************************************************************/
int AMIO_DevGetInfo(const char *_pDevName, T_AmioDevInfo **_ppOutInfo)
{
	int u32DrvIdx;
	if (lookup_drv(_pDevName, &u32DrvIdx)){
		//printf("no corresponding [%s]device driver found\r\n", _pDevName);
		*_ppOutInfo = NULL;
		return EO_NOTFOUND;		
	}
	
    T_AmioDevFactory *pFactory;
	pFactory = g_tSubInfo.m_atDriver[u32DrvIdx].in_pFactory;
    return pFactory->in_pCtrl->funcGetDevInfo(pFactory, _ppOutInfo);
}

/************************************************************************
* Name			:AMIO_DevActive
* Descriptions	:设备激活
* Parameter		:1.const char *_pDevName
*				:2.const char *_strPathIni配置文件路径
* Return		:-1/0+ 
* **********************************************************************/
int AMIO_DevActive(const char *_pDevName, const char *_strPathIni)
{
	int u32Ret, u32DrvIdx;
	if (lookup_drv(_pDevName, &u32DrvIdx)){
		return EO_NOTFOUND;
	}
	
	u32Ret = device_active(u32DrvIdx, 0, _strPathIni);
	if (u32Ret != 0) {
		device_frozen(u32DrvIdx);
	}
    return u32Ret;
}

/************************************************************************
* Name			:AMIO_DevLookup
* Descriptions	:根据设备名字和通道名字查找设备
* Parameter		:1.const char *_pDevName
*				:2.const char *_pChnlName
*				:3.int *_pOutListIdx
* Return		:
* **********************************************************************/
int AMIO_DevLookup(const char *_pDevName, const char *_pChnlName, int *_pOutListIdx)
{
    int u32DrvIdx, u32chnlIdx;
	if (lookup_drv(_pDevName, &u32DrvIdx)) goto ERROR;

	T_AmioDevInfo *pDevInfo = g_tSubInfo.m_atDriver[u32DrvIdx].in_pFactory->in_pInfo;
	if (g_tSubInfo.m_atDriver[u32DrvIdx].in_pFactory == NULL) goto ERROR;
	
    for (u32chnlIdx = 0; u32chnlIdx < pDevInfo->m_u8ChnnlTotal; ++u32chnlIdx) {
		T_AmioChannelInfo *pChnlInfo = pDevInfo->m_aptChannelInfo[u32chnlIdx];
		//printf("[%d]. find [%s]\r\n", u32chnlIdx, c_strbuf(&pChnlInfo->m_strName));
		if (!c_strcmp2(&pChnlInfo->m_strName, _pChnlName)) break;
    }

    if (u32chnlIdx == pDevInfo->m_u8ChnnlTotal) goto ERROR;

	*_pOutListIdx = g_tSubInfo.m_atDriver[u32DrvIdx].m_u32StartIdx + u32chnlIdx;
    return EO_SUCCESS;
ERROR:	
	*_pOutListIdx = -1;
	return EO_NOTFOUND;	
}

/************************************************************************
* Name			:AMIO_DevDefaultInfo
* Descriptions	:
* Parameter		:
* Return		:
* **********************************************************************/
int AMIO_DevDefaultInfo(int _pListIdx, T_AmioChannelInfo **_ppIOParam)
{
    T_AmioDevFactory *pFactory;
    int u32ChnlIdx;
    int u32Ret = -1;

	if (_pListIdx > g_tSubInfo.m_u8ChnlToal) goto EXIT;
    ASSERT_RETURN(_pListIdx != AMIO_DEV_INVALID, EO_INVAL);
	
    u32Ret = lookup_dev(_pListIdx, &pFactory, &u32ChnlIdx);
    if (u32Ret) goto EXIT;

    u32Ret = pFactory->in_pCtrl->funcDefaultParam(pFactory, u32ChnlIdx, _ppIOParam);
    if (u32Ret) goto EXIT;
EXIT:
    return u32Ret;
}

/************************************************************************
* Name			:AMIO_StreamCreate
* Descriptions	:
* Parameter		:
* Return		:
* **********************************************************************/
int AMIO_StreamCreate(int _pListIdx, PORT_DATA_CALLBCK _pCallBck, void *_pUserData, T_PortInfo *_pPort, T_AmioStream **_ppOutStream)
{
    T_AmioDevFactory *pFactory;
    int u32ChnlIdx;
    int u32Ret;
	PORT_DATA_CALLBCK pCallBck = _pCallBck;
    ASSERT_RETURN(_pListIdx != AMIO_DEV_INVALID, EO_INVAL);
    u32Ret = lookup_dev(_pListIdx, &pFactory, &u32ChnlIdx);
    if (u32Ret != 0) return u32Ret;	
	
	if (_pPort){
		ASSERT_RETURN(_pPort->m_onFrame->funcProcess, EO_INVAL);
		pCallBck = (PORT_DATA_CALLBCK)(PORT_Process);
	}
	
    u32Ret = pFactory->in_pCtrl->funcCreateStream(pFactory, u32ChnlIdx, pCallBck, _pUserData, _ppOutStream);
    if (u32Ret != 0) return u32Ret;	
	
	(*_ppOutStream)->m_pPort = _pPort;
	if (_pPort){
		//加载所有port的create方法
		u32Ret = PORT_Dispatch(PORT_FUNC_CREATE, _pPort, NULL);
	}
	
	(*_ppOutStream)->sys.m_u32Index = pFactory->sys.m_u32Index;
    return u32Ret;
}

/************************************************************************
* Name			:
* Descriptions	:
* Parameter		:
* Return		:
* **********************************************************************/
int AMIO_StreamGetInfo(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOutParam)
{
	int u32Ret;
    ASSERT_RETURN(_pStream && _ppOutParam, EO_INVAL);

    u32Ret = _pStream->in_pCtrl->GetParam(_pStream, _ppOutParam);
    if (u32Ret != 0) return u32Ret;
    return 0;
}

int AMIO_StreamGetCap(T_AmioStream *_pStream, int _enCap, void *_pOutValue)
{
	ASSERT_RETURN(_pStream, EO_INVAL);
    return _pStream->in_pCtrl->GetCap(_pStream, _enCap, _pOutValue);
}

int AMIO_StreamSetCap(T_AmioStream *_pStream, int _enCap, const void *_pInValue)
{
	ASSERT_RETURN(_pStream, EO_INVAL);
    return _pStream->in_pCtrl->SetCap(_pStream, _enCap, _pInValue);
}

int AMIO_StreamStart(T_AmioStream *_pStream)
{
	ASSERT_RETURN(_pStream, EO_INVAL);
	T_PortInfo *pPort = _pStream->m_pPort;
	if (pPort){
		int u32Ret = PORT_Dispatch(PORT_FUNC_START, pPort, NULL);
		if (u32Ret){
			printf("start error\r\n");
		}
	}
    return _pStream->in_pCtrl->Start(_pStream);
}

int AMIO_StreamStop(T_AmioStream *_pStream)
{
	ASSERT_RETURN(_pStream, EO_INVAL);
	int u32Ret;
	//先停止所有的取消线程
	T_PortInfo *pPort = _pStream->m_pPort;
	if (pPort){
		u32Ret = PORT_Dispatch(PORT_FUNC_CANCEL, pPort, NULL);
		if (u32Ret){
			printf("cancel error\r\n");
		}
	}
	//停止流
	u32Ret |= _pStream->in_pCtrl->Stop(_pStream);
	if (pPort){
		u32Ret = PORT_Dispatch(PORT_FUNC_STOP, pPort, NULL);
		if (u32Ret){
			printf("stop error\r\n");
		}
	}	
    return ;
}

int AMIO_StreamDestroy(T_AmioStream *_pStream)
{
	ASSERT_RETURN(_pStream, EO_INVAL);
	int u32Ret = 0;
	T_PortInfo *pPort = _pStream->m_pPort;
	u32Ret = AMIO_StreamStop(_pStream);
	u32Ret |= _pStream->in_pCtrl->Destroy(_pStream);
	if (pPort){
		u32Ret |= PORT_Dispatch(PORT_FUNC_DESTROY, pPort, NULL);
		if (u32Ret){
			printf("destroy error\r\n");
		}
	}
	
    return u32Ret;
}



