#include <pthread.h>
#include <sys/time.h>

#include "amio_dev_config.h"
#include "frame.h"

#if AMIO_DEV_HAS_NULL
#include "os.h"
#include "amio_dev.h"
#include "parse_ini.h"

//Null 替换成 自己的设备名称
#define NULL_DEV_NAME			("null_dev")	//未激活只有支持的设备名字
#define NULL_DRV_VERSION		("v1.0")


typedef struct _tagNullFactory
{
    T_AmioDevFactory	in_tFactory;	//必须是struct第一个成员
	T_AmioDevInfo		in_tDevInfo;	
	
	T_PoolInfo 			*in_pPool;		//由create创建
	T_PoolFactory		*in_pPoolFactory;
	
}T_NullFactory;

typedef struct _tagNullStream
{
    T_AmioStream		in_tStream;		//必须是struct第一个成员
	T_NullFactory		*in_pFactory;
	T_AudioChannel		*in_pChnnlInfo;	//+++根据类型修改
    T_PoolInfo 			*in_pPool;		//创建流时使用的内存
	
    void                *pUserData;
	
	int                 m_bQuit;
	T_ThreadInfo		*m_pThread;
	
	PORT_DATA_CALLBCK	m_pCallbck;
	
	/* 特定结构的媒体 */

}T_NullStream;

/******************************************************************************/
static int null_factory_init(T_AmioDevFactory *_pFactory, const char *_strPathIni);
static int null_factory_deinit(T_AmioDevFactory *_pFactory);
static int null_factory_get_dev_info(T_AmioDevFactory *_pFactory, T_AmioDevInfo **_ppOutDevInfo);
static int null_factory_default_param(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, T_AmioChannelInfo **_ppOutChnlInfo);
static int null_factory_create_stream(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, PORT_DATA_CALLBCK _pCallbck, void *_pData, T_AmioStream **_ppOutStream);
static int null_factory_refresh(T_AmioDevFactory *_pFactory, const char *_strPathIni, char _bClose);

static T_AmioDevFactoryCtrl s_tFactoryCtrl =
{
    &null_factory_init,
    &null_factory_deinit,
	&null_factory_get_dev_info,
    &null_factory_default_param,
    &null_factory_create_stream,
    &null_factory_refresh
};

static int null_stream_get_param(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOutChnlInfo);
static int null_stream_get_cap(T_AmioStream *_pStream, int _enCap, void *_pOutValue);
static int null_stream_set_cap(T_AmioStream *_pStream, int _enCap, const void *_pInValue);
static int null_stream_start(T_AmioStream *_pStream);
static int null_stream_stop(T_AmioStream *_pStream);
static int null_stream_destroy(T_AmioStream *_pStream);

static T_AmioStreamCtrl s_tStreamCtrl =
{
    &null_stream_get_param,
    &null_stream_get_cap,
    &null_stream_set_cap,
    &null_stream_start,
    &null_stream_stop,
    &null_stream_destroy
};
/******************************************************************************/
//创建设备和空通道，没有对应的释放点？
T_AmioDevFactory *AmioNullDevice(T_PoolFactory *_pPoolFactory)
{
    T_NullFactory	*pFactory;
	T_PoolInfo		*pPool;
	int i;
	pPool = POOL_Create(_pPoolFactory, "null_base", 512, 512, NULL);//注意
	if (pPool == NULL) return NULL;
	
	//创建设备
    pFactory = POOL_ZALLOC_T(pPool, T_NullFactory);
    pFactory->in_pPoolFactory		= _pPoolFactory;
    pFactory->in_pPool				= pPool;
    pFactory->in_tFactory.in_pCtrl	= &s_tFactoryCtrl;
	pFactory->in_tFactory.in_pInfo  = &pFactory->in_tDevInfo;
	
	//设备属性
	T_AmioDevInfo *pDevInfo = &pFactory->in_tDevInfo;
	c_cstr(&pDevInfo->m_strName, NULL_DEV_NAME);
	c_cstr(&pDevInfo->m_strVer, NULL_DRV_VERSION);
	pDevInfo->m_enType	= AMIO_DEV_TYPE_AUDIO;	//根据自己的驱动添加
	
	/* 以下内容，如果是动态通道需要放在init中创建 */
	pDevInfo->m_u8ChnnlTotal = AMIO_DEV_CHANNEL_MAX;
	
	//通道属性,默认创建三个通道
	for (i = 0; i < AMIO_DEV_CHANNEL_MAX; ++i){
		pDevInfo->m_aptChannelInfo[i] = (T_AmioChannelInfo *)POOL_ZALLOC_T(pPool, T_AudioChannel);	//!!!一定要根据驱动类型修改
	}
	
    return &pFactory->in_tFactory;
}
/******************************************************************************/
//激活调用,会对比设备名字和ini中的名字是否一致,不一致则出错返回
//解析ini文件
static int null_factory_init(T_AmioDevFactory *_pFactory, const char *_strPathIni)
{
    T_NullFactory *pFactory = (T_NullFactory *)_pFactory;
	int u32Ret = 0;
	//1.加载配置文件
	if (u32Ret = PARSER_CONFIG(AMIO, pFactory->in_pPool, _strPathIni, NULL_DEV_NAME, "audio", &pFactory->in_tDevInfo)){
		return u32Ret;
	}
	
	c_cstr(&pFactory->in_tDevInfo.m_strPathIni, _strPathIni);
	
	//2.硬件设备相关初始化
	
	
    return 0;
}

static int null_factory_deinit(T_AmioDevFactory *_pFactory)
{
    T_NullFactory *pFactory = (T_NullFactory *)_pFactory;
	int i;
	//1.停止当前设备下的所有流,非AMIO_DEV_TYPE_NONE流停止
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_AudioChannel *pChnnlInfo = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_NullStream *pStream = (T_NullStream *)pChnnlInfo->m_vpStream;
		if (pStream && pChnnlInfo->m_enType != AMIO_DEV_TYPE_NONE && !pStream->m_bQuit){
			null_stream_stop((T_AmioStream *)pStream);
			null_stream_destroy((T_AmioStream *)pStream);
		}
		pChnnlInfo->m_enType = AMIO_DEV_TYPE_NONE;
		pChnnlInfo->m_vpStream = NULL;
	}

	//2.硬件设备相关销毁
	
    return 0;
}

//_bClose是否保留之前激活状态，0的话，强制清除
static int null_factory_refresh(T_AmioDevFactory *_pFactory, const char *_strPathIni, char _bClose)
{
	T_NullFactory *pFactory = (T_NullFactory *)_pFactory;
	char u8Active[AMIO_DEV_CHANNEL_MAX] = {0}, i;
	int u32Ret;
	//1.先停掉已经开启的流
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_AudioChannel *pChnnlInfo = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_NullStream *pStream = (T_NullStream *)pChnnlInfo->m_vpStream;
		if (pStream && pChnnlInfo->m_enType != AMIO_DEV_TYPE_NONE && !pStream->m_bQuit){
			u8Active[i] = !pStream->m_bQuit & _bClose;
			null_stream_stop((T_AmioStream *)pStream);
		}
	}
	
	//2.硬件设备的关闭
	
	//3.重新init
	if (u32Ret = null_factory_init((T_AmioDevFactory *)pFactory, _strPathIni)){
		return u32Ret;
	}
	
	//4.开启之前激活的流
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_AudioChannel *pChnnlInfo = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_NullStream *pStream = (T_NullStream *)pChnnlInfo->m_vpStream;
		if (pStream && u8Active[i]){
			null_stream_start((T_AmioStream *)pStream);
		}
	}	
	
    return 0;
}


static int null_factory_get_dev_info(T_AmioDevFactory *_pFactory, T_AmioDevInfo **_ppOutDevInfo)
{
    T_NullFactory *pFactory = (T_NullFactory *)_pFactory;

    return 0;
}

//根据通道参数初始化相关平台通道参数
static int null_factory_default_param(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, T_AmioChannelInfo **_ppOut)
{
    T_NullFactory *pFactory = (T_NullFactory *)_pFactory;
    T_AudioChannel *pAudio = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[_u32ChnlIdx];
	
	if (_u32ChnlIdx == 0){
		//采集通道
		pAudio->m_enDir = AMIO_DIR_CAPTURE;	
		pAudio->m_enCap |= AMIO_AUD_DEV_CAP_INPUT_VOLUME_SETTING;		//输入音量
	}else{
		//播放通道
		pAudio->m_enDir = AMIO_DIR_PLAYBACK;	
		pAudio->m_enCap |= AMIO_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING;		//输出音量		
	}

	if (_ppOut) *_ppOut = (T_AmioChannelInfo *)pAudio;
	
    return 0;
}


static int null_factory_create_stream(T_AmioDevFactory *_pFactory, 
										int _u32ChnnlId, 
										PORT_DATA_CALLBCK _pCallbck, 
										void *_pData, 
										T_AmioStream **_ppOut)
{
    T_NullFactory *pFactory = (T_NullFactory *)_pFactory;
	T_NullStream *pFStream = NULL;
	
	T_PoolInfo		*pPool;
	pPool = POOL_Create(pFactory->in_pPoolFactory, "null%p", 1024, 1024, NULL);
	if (pPool == NULL) {
		return EO_NOMEM;
	}
	
    pFStream = POOL_ZALLOC_T(pPool, T_NullStream);
	pFStream->in_pPool					= pPool;
    pFStream->m_pCallbck				= _pCallbck;
    pFStream->pUserData					= _pData;
	pFStream->in_pFactory				= pFactory;
	pFStream->in_pChnnlInfo				= (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[_u32ChnnlId];
	pFStream->in_pChnnlInfo->m_vpStream = (void *)pFStream;
	
	//通道中使用的各种缓存变量等	
	
    pFStream->in_tStream.in_pCtrl = &s_tStreamCtrl;
	if (_ppOut) *_ppOut = &pFStream->in_tStream;
    return EO_SUCCESS;
}


static int null_stream_get_param(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOutChnlInfo)
{
    return 0;
}

static int null_stream_get_cap(T_AmioStream *_pStream, int _enCap, void *_pOutValue)
{
	return 0;
}

static int null_stream_set_cap(T_AmioStream *_pStream, int _enCap, const void *_pInValue)
{
    return 0;
}

static int null_stream_thread(void *_pArg)
{
    T_NullStream *pFStream = (T_NullStream *)_pArg;
	T_AudioChannel *pChnnlInfo = pFStream->in_pChnnlInfo;
	T_Frame tFrame = {0};
	tFrame.m_u8Type	= 1;
	tFrame.m_u32fd	= pChnnlInfo->m_u32ChnlFd;
	
    while (!pFStream->m_bQuit){
		//printf("[%d] stream %s\r\n", pChnnlInfo->m_u32ChnlFd, type?"video":"audio");
		(pFStream->m_pCallbck)(pFStream->pUserData, &tFrame);
		sleep(2);
    }
	
	printf("stop null dev [%d] stream exit!!!\r\n", pChnnlInfo->m_u32ChnlFd);
    return 0;
}

static int null_stream_start(T_AmioStream *_pStream)
{
    T_NullStream *pFStream = (T_NullStream *)_pStream;
    printf("Starting null stream\r\n");
    int u32Ret = 0;
	
    pFStream->m_bQuit = 0;	
	u32Ret = THREAD_Create(pFStream->in_pPool,
						   NULL,
						   null_stream_thread,
						   (void *)pFStream,
						   0,
						   0,
						   &pFStream->m_pThread);
    return u32Ret;
}

static int null_stream_stop(T_AmioStream *_pStream)
{
    T_NullStream *pFStream = (T_NullStream *)_pStream;

	if (pFStream->m_bQuit == 0){
		pFStream->m_bQuit = 1;
		THREAD_Join(pFStream->m_pThread);
	}

	printf("Stop null stream\r\n");
	
    return 0;
}

static int null_stream_destroy(T_AmioStream *_pStream)
{
    T_NullStream *pFStream = (T_NullStream *)_pStream;
	if (pFStream->in_pPool){
		POOL_Destroy(pFStream->in_pPool);
	}

    return 0;
}

#endif