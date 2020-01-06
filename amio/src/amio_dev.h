/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_AMIO_DEV_H__
#define __ZXY_AMIO_DEV_H__

#include "amio.h"



typedef struct _tagDevFactoryCtrl
{
    int (*funcInit)  (T_AmioDevFactory *_pFactory, const char *_strPathIni);//创建通道相关信息
    int (*funcDeinit)(T_AmioDevFactory *_pFactory);

	int (*funcGetDevInfo) (T_AmioDevFactory *_pFactory, T_AmioDevInfo **_ppOutInfo);	//dev info里面包括channel info

    int (*funcDefaultParam)(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, T_AmioChannelInfo **_ppOutInfo);

    int (*funcCreateStream)(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, PORT_DATA_CALLBCK _pCallbck, void *_pUserData, T_AmioStream **_ppOutStream);

    int (*funcRefresh)(T_AmioDevFactory *_pFactory, const char *_strPathIni, char _bClose);
}T_AmioDevFactoryCtrl;

struct _tagDevFactory
{
    struct {
		int m_u32Index;		//表示在drv中的索引
    }sys;
	
	T_AmioDevFactoryCtrl	*in_pCtrl;
	T_AmioDevInfo			*in_pInfo;
};

//_pStream 某个具体通道的流
//stream - channel
typedef struct _tagStreamCtrl
{
    int (*GetParam)(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOutInfo);

    int (*GetCap)(T_AmioStream *_pStream, int _enCap, void *_pOutValue);
    int (*SetCap)(T_AmioStream *_pStream, int _enCap, const void *_pInValue);

    int (*Start)(T_AmioStream *_pStream);
    int (*Stop)(T_AmioStream *_pStream);	//_bClose 是否保持之前的激活状态，刷新使用

    int (*Destroy)(T_AmioStream *_pStream); //和CreateStream相对应
	
}T_AmioStreamCtrl;

struct _tagStream
{

	T_PortInfo *m_pPort;
	
    struct {
		int m_u32Index;		//所属的工厂id(drv中的索引)
    }sys;

	T_AmioStreamCtrl *in_pCtrl;
};









#endif
