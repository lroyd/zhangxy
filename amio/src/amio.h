/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_AMIO_H__
#define __ZXY_AMIO_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config_site.h"
#include "typedefs.h"
#include "cstring.h"
#include "pool.h"
#include "error_code.h"
#include "frame.h"
#include "port.h"

#include "audio_info.h"
#include "video_info.h"
#include "comm_info.h"

#define AMIO_DEVICE_MAX			(16)
#define AMIO_DEVICE_SUB_MAX		(48)	//驱动中最大通道数，管理所有设备通道
#define AMIO_DEV_CHANNEL_MAX	(3)		//单个设备支持的最大通道数


#define DEFINE_CAP(name, info)	{name, info}



#define AMIO_DEV_DEFAULT		(-1)
#define AMIO_DEV_INVALID		(-3)


#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
	#define AMIO_SubLogRegister(dev)	dev##_SubLogRegister()
	#define AMIO_SubLogSet(dev, lv, on)	dev##_SubLogSet(lv, on)
	#define AMIO_SubLogGet(dev)			dev##_SubLogGet()
#endif

enum
{
	AMIO_DEV_TYPE_NONE	= 0,	//未启用
    AMIO_DEV_TYPE_AUDIO	= 1,	//音频
    AMIO_DEV_TYPE_VIDEO	= 2,	//视频
	AMIO_DEV_TYPE_COMM	= 4,	//通用外设
};

enum
{
    AMIO_DIR_NONE				= 0,
    AMIO_DIR_ENCODING			= 1,
    AMIO_DIR_CAPTURE			= AMIO_DIR_ENCODING,
    AMIO_DIR_DECODING			= 2,
    AMIO_DIR_PLAYBACK			= AMIO_DIR_DECODING,
    AMIO_DIR_RENDER				= AMIO_DIR_DECODING,
    AMIO_DIR_ENCODING_DECODING 	= 4,
    AMIO_DIR_CAPTURE_PLAYBACK 	= AMIO_DIR_ENCODING_DECODING,
    AMIO_DIR_CAPTURE_RENDER 	= AMIO_DIR_ENCODING_DECODING
};

typedef struct _tagAmioChannelInfo
{
	str_t			m_strName;
	int				m_u32ChnlId;	//逻辑通道id,1+
	int				m_u32ChnlFd;	//真实通道fd,0+
	char			m_enType;		//设备类型(和DevInfo中的m_enType一致)]
	/* 以上信息由配置文件生成 */
	char			m_enDir;		//设备类型方向
	char			m_enCap;		//设备能力
	void			*m_vpStream;
}T_AmioChannelInfo;


//一开始就创建
typedef struct _tagDevInfo
{
	/**************************************************/
	str_t				m_strName;
	str_t				m_strVer;
	str_t				m_strPathIni;
	char				m_enType;		//当前驱动支持的设备类型
	/**************************************************/
	char				m_u8ChnnlTotal;	//数组都分配写死3个，通过AMIO_DEV_TYPE_NONE来判度是否启动
	
	T_AmioChannelInfo	*m_aptChannelInfo[AMIO_DEV_CHANNEL_MAX];//最大3个通道，严格位置对应
	

}T_AmioDevInfo;


typedef struct _tagDevFactory	T_AmioDevFactory;

typedef struct _tagStream		T_AmioStream;

typedef T_AmioDevFactory *(*AMIO_DEV_FUNC_CREATE)(T_PoolFactory *);


API_DEF(int)	AMIO_DrvLoad(T_PoolFactory *);		//装载当前支持的所有驱动,返回已经加载驱动的个数
API_DEF(char*)	AMIO_DrvEnum(char *_pBuffer, unsigned _uSize);				//列举当前支持的所有驱动
API_DEF(int) 	AMIO_DrvUnload(void);


//对device的操作是通过name
API_DEF(int) AMIO_DevActive(const char *_pDevName, const char *_strPathIni);	//创建通道属性
API_DEF(int) AMIO_DevRefresh(const char *_pDevName, const char *_strPathIni);	//设备必须激活才可以使用, NULL自动重载上次配置文件
API_DEF(int) AMIO_DevFrozen(const char *_pDevName);							//销毁通道属性，和所有流

API_DEF(int) AMIO_DevGetInfo(const char *_pDevName, T_AmioDevInfo **_ppOut);
API_DEF(char*) AMIO_DevDump(T_AmioDevInfo *_pInfo, char *_pBuffer, int _u32Len);
API_DEF(int) AMIO_DevLookup(const char *_pDevName, const char *_pChnlName, int *_pOutListIdx);	//每一路chnnl对应一个索引

//对channel的操作是通过全局list index
API_DEF(int) AMIO_DevDefaultParam(int _pListIdx, T_AmioChannelInfo **_ppOut);	//装载channle属性
API_DEF(int) AMIO_StreamCreate(int _pListIdx, PORT_DATA_CALLBCK _pCallBck, void *_pUserData, T_PortInfo *_pPort, T_AmioStream **_ppOutStream);


//对stream的操作是通过stream
API_DEF(int) AMIO_StreamGetInfo(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOut); //得到某个通道属性
API_DEF(int) AMIO_StreamGetCap(T_AmioStream *_pStream, int _enCap, void *_pOutValue);
API_DEF(int) AMIO_StreamSetCap(T_AmioStream *_pStream, int _enCap, const void *_pInValue);
API_DEF(int) AMIO_StreamStart(T_AmioStream *_pStream);
API_DEF(int) AMIO_StreamStop(T_AmioStream *_pStream);
API_DEF(int) AMIO_StreamDestroy(T_AmioStream *_pStream);





#endif	/* __ZXY_AMIO_H__ */
