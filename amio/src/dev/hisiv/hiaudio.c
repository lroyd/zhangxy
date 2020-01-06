#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "amio_dev_config.h"
#include "frame.h"

#if AMIO_DEV_HAS_HIAUDIO
#include "os.h"
#include "sample_comm.h"

#include "amio_dev.h"
#include "console.h"
#include "parse_ini.h"

#define HIAUDIO_DEV_NAME			("hiaudio")
#define HIAUDIO_DRV_VERSION			("v1.0")



typedef struct _tagHiaudioFactory
{
    T_AmioDevFactory	in_tFactory;
	T_AmioDevInfo		in_tDevInfo;	
	
	T_PoolInfo 			*in_pPool;
	T_PoolFactory		*in_pPoolFactory;

	/* 海思转悠参数 */
	AIO_ATTR_S 			in_tAioAttr;
	
}T_HiaudioFactory;

typedef struct _tagHiaudioStream
{
    T_AmioStream		in_tStream;
	T_HiaudioFactory	*in_pFactory;
	T_AudioChannel		*in_pChnnlInfo;		//+++
    T_PoolInfo 			*in_pPool;
	
    void                *pUserData;
	
	int                 m_bQuit;
	T_ThreadInfo		*m_pThread;
	
	PORT_DATA_CALLBCK	m_pCallbck;
	
	/*  */
	int					m_u32BufSize;
	char				*m_pBuf;
}T_HiaudioStream;


#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"hiaudio", ZXY_LOG_DEBUG_LV, 0};	//默认关闭
static char s_u8LogId = -1;

void HIAUDIO_SubLogRegister(void)
{
	s_u8LogId = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
}

int HIAUDIO_SubLogSet(char _enLogLv ,char _bLogOn)
{
	CONSOLE_SubLogSetParam(s_u8LogId, _enLogLv, _bLogOn);
}

T_SubLogInfo* HIAUDIO_SubLogGet(void)
{
	return CONSOLE_SubLogGetParam(s_u8LogId);
}
#endif


/******************************************************************************/
int HI_AUDIO_StartAi(int _s32Sample, int _s32FrameSample, AIO_ATTR_S *_pAioAttr, char _bEC);
int HI_AUDIO_StopAi(void);

int HI_AUDIO_StartAo(int _s32Sample, int _s32FrameSample, AIO_ATTR_S *_pAioAttr, char _bEC);
int HI_AUDIO_StopAo(void);

int HI_AUDIO_SetAiVolume(int _s32Volume);
int HI_AUDIO_SetAoVolume(int _s32Volume);




/******************************************************************************/
static int hiaudio_factory_init(T_AmioDevFactory *_pFactory, const char *_strPathIni);
static int hiaudio_factory_deinit(T_AmioDevFactory *_pFactory);
static int hiaudio_factory_get_dev_info(T_AmioDevFactory *_pFactory, T_AmioDevInfo **_ppOutDevInfo);
static int hiaudio_factory_default_param(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, T_AmioChannelInfo **_ppOutChnlInfo);
static int hiaudio_factory_create_stream(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, PORT_DATA_CALLBCK _pCallbck, void *_pData, T_AmioStream **_ppOutStream);
static int hiaudio_factory_refresh(T_AmioDevFactory *_pFactory, const char *_strPathIni, char _bClose);

static T_AmioDevFactoryCtrl s_tFactoryCtrl =
{
    &hiaudio_factory_init,
    &hiaudio_factory_deinit,
	&hiaudio_factory_get_dev_info,
    &hiaudio_factory_default_param,
    &hiaudio_factory_create_stream,
    &hiaudio_factory_refresh
};

static int hiaudio_stream_get_param(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOutChnlInfo);
static int hiaudio_stream_get_cap(T_AmioStream *_pStream, int _enCap, void *_pOutValue);
static int hiaudio_stream_set_cap(T_AmioStream *_pStream, int _enCap, const void *_pInValue);
static int hiaudio_stream_start(T_AmioStream *_pStream);
static int hiaudio_stream_stop(T_AmioStream *_pStream);
static int hiaudio_stream_destroy(T_AmioStream *_pStream);

static T_AmioStreamCtrl s_tStreamCtrl =
{
    &hiaudio_stream_get_param,
    &hiaudio_stream_get_cap,
    &hiaudio_stream_set_cap,
    &hiaudio_stream_start,
    &hiaudio_stream_stop,
    &hiaudio_stream_destroy
};
/******************************************************************************/
T_AmioDevFactory *AmioHiaudioDevice(T_PoolFactory *_pPoolFactory)
{
    T_HiaudioFactory	*pFactory;
	T_PoolInfo			*pPool;
	int i;
	pPool = POOL_Create(_pPoolFactory, "hiaudio", 512, 512, NULL);
	if (pPool == NULL) return NULL;
	
	//创建设备
    pFactory = POOL_ZALLOC_T(pPool, T_HiaudioFactory);
    pFactory->in_pPoolFactory		= _pPoolFactory;
    pFactory->in_pPool				= pPool;
    pFactory->in_tFactory.in_pCtrl	= &s_tFactoryCtrl;
	pFactory->in_tFactory.in_pInfo  = &pFactory->in_tDevInfo;

	//设备属性
	T_AmioDevInfo *pDevInfo = &pFactory->in_tDevInfo;
	c_cstr(&pDevInfo->m_strName, 	HIAUDIO_DEV_NAME);
	c_cstr(&pDevInfo->m_strVer, 	HIAUDIO_DRV_VERSION);
	pDevInfo->m_enType	= AMIO_DEV_TYPE_AUDIO;
	
	pDevInfo->m_u8ChnnlTotal = AMIO_DEV_CHANNEL_MAX;
	
	//通道属性,默认创建三个通道
	for (i = 0; i < AMIO_DEV_CHANNEL_MAX; ++i){
		pDevInfo->m_aptChannelInfo[i] = (T_AmioChannelInfo *)POOL_ZALLOC_T(pPool, T_AudioChannel);	//!!!一定要根据驱动类型修改
	}
	
    return &pFactory->in_tFactory;
}
/******************************************************************************/
static int hiaudio_factory_init(T_AmioDevFactory *_pFactory, const char *_strPathIni)
{
    T_HiaudioFactory *pFactory = (T_HiaudioFactory *)_pFactory;
	int u32Ret = 0;
	//1.加载配置文件
	if (u32Ret = PARSER_CONFIG(AMIO, pFactory->in_pPool, _strPathIni, HIAUDIO_DEV_NAME, "audio", &pFactory->in_tDevInfo)){
		return u32Ret;
	}

	c_cstr(&pFactory->in_tDevInfo.m_strPathIni, _strPathIni);
	
	//2.硬件设备相关初始化	
	//2.1海思初始化
	if (1){
		VB_CONF_S stVbConf = {0};
		if (SAMPLE_COMM_SYS_Init(&stVbConf)){
			printf("hisiv system init failed !!!\r\n");
			return -1;
		}
	}
	//2.2默认音频只有[0][1]通道
	T_AudioChannel *pAudio = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[0];
	//printf("xxxx %s, %d, %d\r\n", c_strbuf(&pAudio->m_strName), pAudio->m_u32BitWidth, pAudio->m_u32FrameSize);
	//随便找一路参数, 海思音频器
	pFactory->in_tAioAttr.enSamplerate   = pAudio->m_u32SampleRate;	//采样率
	pFactory->in_tAioAttr.enBitwidth     = (pAudio->m_u32BitWidth/8) - 1;
	pFactory->in_tAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
	pFactory->in_tAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
	pFactory->in_tAioAttr.u32EXFlag      = 0;
	pFactory->in_tAioAttr.u32FrmNum      = 30;
	pFactory->in_tAioAttr.u32PtNumPerFrm = pAudio->m_u32FrameSize;
	pFactory->in_tAioAttr.u32ChnCnt      = 1;
	pFactory->in_tAioAttr.u32ClkSel      = 0;
	if (SAMPLE_COMM_AUDIO_CfgAcodec(&pFactory->in_tAioAttr)){
		printf("audio config acodec error\n");
		return EO_FAILED;
	}
	
    return EO_SUCCESS;
}

static int hiaudio_factory_deinit(T_AmioDevFactory *_pFactory)
{
    T_HiaudioFactory *pFactory = (T_HiaudioFactory *)_pFactory;
	int i;
	//1.停止当前设备下的所有流,非AMIO_DEV_TYPE_NONE流停止
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_AudioChannel *pChnnlInfo = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_HiaudioStream *pStream = (T_HiaudioStream *)pChnnlInfo->m_vpStream;
		if (pStream && pChnnlInfo->m_enType == AMIO_DEV_TYPE_AUDIO && !pStream->m_bQuit){
			hiaudio_stream_stop((T_AmioStream *)pStream);
			hiaudio_stream_destroy((T_AmioStream *)pStream);
		}
		pChnnlInfo->m_enType = AMIO_DEV_TYPE_NONE;
		pChnnlInfo->m_vpStream = NULL;
	}

	//2.硬件设备相关销毁
	if (0){
		SAMPLE_COMM_SYS_Exit();
	}

    return 0;
}

//刷新所有通道(不删减)，重载配置文件
static int hiaudio_factory_refresh(T_AmioDevFactory *_pFactory, const char *_strPathIni, char _bClose)
{
	T_HiaudioFactory *pFactory = (T_HiaudioFactory *)_pFactory;
	char u8Active[AMIO_DEV_CHANNEL_MAX] = {0}, i;
	int u32Ret;
	//1.先停掉已经开启的流
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_AudioChannel *pChnnlInfo = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_HiaudioStream *pStream = (T_HiaudioStream *)pChnnlInfo->m_vpStream;
		if (pStream && pChnnlInfo->m_enType == AMIO_DEV_TYPE_AUDIO && !pStream->m_bQuit){
			u8Active[i] = !pStream->m_bQuit & _bClose;
			hiaudio_stream_stop((T_AmioStream *)pStream);
		}
	}
	
	//2.硬件设备的关闭
	
	//3.重新init
	if (u32Ret = hiaudio_factory_init((T_AmioDevFactory *)pFactory, _strPathIni)){
		return u32Ret;
	}
	
	//4.开启之前激活的流
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_AudioChannel *pChnnlInfo = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_HiaudioStream *pStream = (T_HiaudioStream *)pChnnlInfo->m_vpStream;
		if (pStream && u8Active[i]){
			hiaudio_stream_start((T_AmioStream *)pStream);
		}
	}

    return 0;
}


static int hiaudio_factory_get_dev_info(T_AmioDevFactory *_pFactory, T_AmioDevInfo **_ppOutDevInfo)
{
    T_HiaudioFactory *pFactory = (T_HiaudioFactory *)_pFactory;

    return 0;
}

//设置通道dir和cap
static int hiaudio_factory_default_param(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, T_AmioChannelInfo **_ppOut)
{
    T_HiaudioFactory *pFactory = (T_HiaudioFactory *)_pFactory;
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
	
    return EO_SUCCESS;
}

//创建流中使用的pool
static int hiaudio_factory_create_stream(T_AmioDevFactory *_pFactory, 
										int _u32ChnlIdx, 
										PORT_DATA_CALLBCK _pCallbck, 
										void *_pData, 
										T_AmioStream **_ppOut)
{
    T_HiaudioFactory *pFactory = (T_HiaudioFactory *)_pFactory;
	T_HiaudioStream *pFStream	= NULL;
	T_AudioChannel *pAudio		= NULL;
	T_PoolInfo		*pPool		= NULL;;
	pPool = POOL_Create(pFactory->in_pPoolFactory, "hiaudio%p", 1024, 1024, NULL);
	if (pPool == NULL) return EO_NOMEM;
	
    pAudio = (T_AudioChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[_u32ChnlIdx];	
    pFStream = POOL_ZALLOC_T(pPool, T_HiaudioStream);
	pFStream->in_pFactory				= pFactory;
	pFStream->in_pChnnlInfo				= pAudio;
	pFStream->in_pPool					= pPool;
	
    pFStream->m_pCallbck				= _pCallbck;
    pFStream->pUserData					= _pData;
	pFStream->m_bQuit					= 1;
	
	pFStream->in_pChnnlInfo->m_vpStream = (void *)pFStream;
	
    /* 采集通道 */
    if (pAudio->m_enDir & AMIO_DIR_CAPTURE){
		pFStream->m_u32BufSize = pAudio->m_u32FrameSize * pAudio->m_u32BitWidth/8;
		pFStream->m_pBuf = (char*) POOL_Alloc(pPool, pFStream->m_u32BufSize);
    }

    /* 播放通道 */
    if (pAudio->m_enDir & AMIO_DIR_PLAYBACK){
		if (pAudio->m_bLookup == 0){
			pFStream->m_u32BufSize = pAudio->m_u32FrameSize * pAudio->m_u32BitWidth/8;
			pFStream->m_pBuf = (char*) POOL_Alloc(pPool, pFStream->m_u32BufSize);
		}
    }
	
    pFStream->in_tStream.in_pCtrl = &s_tStreamCtrl;
	if (_ppOut) *_ppOut = &pFStream->in_tStream;
    return EO_SUCCESS;
}


static int hiaudio_stream_get_param(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOut)
{
    return 0;
}

static int hiaudio_stream_get_cap(T_AmioStream *_pStream, int _enCap, void *_pOutValue)
{
	return 0;
}

static int hiaudio_stream_set_cap(T_AmioStream *_pStream, int _enCap, const void *_pInValue)
{
    return 0;
}


static int audio_cap_stream_thread(void *_pArg)
{
    T_HiaudioStream *pFStream = (T_HiaudioStream *)_pArg;
	T_AudioChannel *pAudio = pFStream->in_pChnnlInfo;
	PORT_DATA_CALLBCK pCallbck = pFStream->m_pCallbck;
	int		u32Len = 0;
	char	*pBuf = pFStream->m_pBuf;
	int AiFd, s32Ret;
	/* 平台相关 */
	//初始化音频AI
	s32Ret = HI_AUDIO_StartAi(pAudio->m_u32SampleRate, pAudio->m_u32FrameSize, &pFStream->in_pFactory->in_tAioAttr, pAudio->m_bEC);
	if (s32Ret){
		printf("audio start ai error\n");
		return -1;
	}
	HI_AUDIO_SetAiVolume(pAudio->m_s32Volume);
	if (pAudio->m_bLookup){
		s32Ret = HI_AUDIO_StartAo(pAudio->m_u32SampleRate, pAudio->m_u32FrameSize, &pFStream->in_pFactory->in_tAioAttr, pAudio->m_bEC);
		if (s32Ret){
			printf("audio start ao error\n");
			return -1;
		}
	}
	
	//设置通道深度
	AUDIO_FRAME_S stFrame; 
	AEC_FRAME_S stAecFrm;
	AI_CHN_PARAM_S stAiChnPara;
    HI_MPI_AI_GetChnParam(0, 0, &stAiChnPara);
    stAiChnPara.u32UsrFrmDepth = 30;
    HI_MPI_AI_SetChnParam(0, 0, &stAiChnPara);
	//获取fd，设置select
    fd_set read_fds;
    FD_ZERO(&read_fds);
    AiFd = HI_MPI_AI_GetFd(0, 0);
    FD_SET(AiFd,&read_fds);	
	
	struct timeval timeout;
	
	T_Frame tFrame = {0};
	tFrame.m_u8Type	= FRAME_TYPE_AUDIO;
	tFrame.m_u32fd	= 0;	
	
    while (!pFStream->m_bQuit) 
    {
        timeout.tv_sec	= 1;
        timeout.tv_usec	= 0;
        FD_ZERO(&read_fds);
        FD_SET(AiFd,&read_fds);
        s32Ret = select(AiFd + 1, &read_fds, NULL, NULL, &timeout);
        if (s32Ret < 0){
            break;
        }
        else if (0 == s32Ret){
            printf("%s: get ai frame select time out\n", __FUNCTION__);
            break;
        }
        
        if (FD_ISSET(AiFd, &read_fds)){
			memset(&stAecFrm, 0, sizeof(AEC_FRAME_S));
            if (HI_MPI_AI_GetFrame(0, 0, &stFrame, &stAecFrm, HI_FALSE)){
				
				continue;
            }

            if (pAudio->m_bLookup == 0){
				//没有回环
				u32Len = stFrame.u32Len;
				memcpy(pBuf, stFrame.pVirAddr[0], stFrame.u32Len);
				tFrame.m_vpData	= (void *)pBuf;	
				tFrame.m_uSize	= stFrame.u32Len;
				if (pCallbck){
					//printf("cap audio data len = %d\r\n", u32Len);
					s32Ret = (pCallbck)(pFStream->pUserData, &tFrame);
				}
            }
			else{
				//回环的话，直接输出
				HI_MPI_AO_SendFrame(0, 0, &stFrame, 1000);
			}

            s32Ret = HI_MPI_AI_ReleaseFrame(0, 0, &stFrame, &stAecFrm);
            if (HI_SUCCESS != s32Ret ){
                printf("%s: HI_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n",__FUNCTION__, 0, 0, s32Ret);
                break;
            }
        }
    }
	
	HI_AUDIO_StopAi();
	if (pAudio->m_bLookup){
		HI_AUDIO_StopAo();
	}	
	
	printf("stop audio ai stream exit!!!\r\n");
    return 0;
}

static int audio_pb_stream_thread(void *_pArg)
{
    T_HiaudioStream *pFStream = (T_HiaudioStream *)_pArg;
	T_AudioChannel *pAudio = pFStream->in_pChnnlInfo;
	PORT_DATA_CALLBCK pCallbck = pFStream->m_pCallbck;
	int s32Ret;
	if (pAudio->m_bLookup == 0){
		s32Ret = HI_AUDIO_StartAo(pAudio->m_u32SampleRate, pAudio->m_u32FrameSize, &pFStream->in_pFactory->in_tAioAttr, pAudio->m_bEC);
		if (s32Ret){
			printf("audio start ao error\n");
			return -1;
		}
	}
	
	AUDIO_FRAME_S stFrame; 
	int u32Len = 0;
	char *pBuf = pFStream->m_pBuf;
	stFrame.pVirAddr[0] = pBuf;
	
	T_Frame tFrame = {0};
	tFrame.m_u8Type	= 0;
	tFrame.m_u32fd	= 0;	
	tFrame.m_vpData	= (void *)pBuf;	
	tFrame.m_uSize	= 320;	//计算得到	
    while (!pFStream->m_bQuit) 
    {
		//printf("[%d] stream %s\r\n", pFStream->m_u32ChnlId, "audio");
		u32Len = (pCallbck)(pFStream->pUserData, &tFrame);
		if (u32Len > 0){
			stFrame.u32Len = u32Len;
			
			stFrame.enBitwidth	= 1;
			stFrame.enSoundmode = 0;
			
			HI_MPI_AO_SendFrame(0, 0, &stFrame, 1000);			
		}
		//usleep(1000);
    }
	
	if (pAudio->m_bLookup == 0){
		HI_AUDIO_StopAo();
	}	
	
	printf("stop audio ao stream exit!!!\r\n");
    return 0;
}

static int hiaudio_stream_start(T_AmioStream *_pStream)
{
    T_HiaudioStream *pFStream = (T_HiaudioStream *)_pStream;
	T_AudioChannel *pAudio = pFStream->in_pChnnlInfo;
    printf("Starting hiaudio stream\r\n");
	
    pFStream->m_bQuit = 0;
	if (pAudio->m_enType & AMIO_DEV_TYPE_AUDIO){
		//音频
		if (pAudio->m_enDir & AMIO_DIR_CAPTURE){
			//采集
			THREAD_Create(pFStream->in_pPool, 
							NULL, 
							audio_cap_stream_thread,
							(void *)pFStream,
							0,
							0,
							&pFStream->m_pThread);			

		}
		else if (pAudio->m_enDir & AMIO_DIR_PLAYBACK){
			//播放(回环的话，不打开播放通道)
			if (pAudio->m_bLookup == 0){
				THREAD_Create(pFStream->in_pPool, 
								NULL, 
								audio_pb_stream_thread,
								(void *)pFStream,
								0,
								0,
								&pFStream->m_pThread);				
			}
		}
	}
	else{
		printf("this stream does not support audio types!!!\r\n");
	}

    return EO_SUCCESS;
}

static int hiaudio_stream_stop(T_AmioStream *_pStream)
{
    T_HiaudioStream *pFStream = (T_HiaudioStream *)_pStream;
	T_AudioChannel *pAudio = pFStream->in_pChnnlInfo;
	if (pAudio->m_enType & AMIO_DEV_TYPE_AUDIO){
		//音频
		if (pFStream->m_bQuit == 0){
			pFStream->m_bQuit = 1;
			//取消线程(如果有的话)
			if (pAudio->m_enDir & AMIO_DIR_CAPTURE){
				THREAD_Join(pFStream->m_pThread);
			}
			else if (pAudio->m_enDir & AMIO_DIR_PLAYBACK){
				if (pAudio->m_bLookup == 0){
					THREAD_Join(pFStream->m_pThread);
				}
			}			
		}

	}
	else{
		printf("this stream does not support audio types!!!\r\n");
	}	
	
	printf("Stop hiaudio stream\r\n");
	
    return EO_SUCCESS;
}

static int hiaudio_stream_destroy(T_AmioStream *_pStream)
{
    T_HiaudioStream *pFStream = (T_HiaudioStream *)_pStream;
	if (pFStream->in_pPool){
		POOL_Destroy(pFStream->in_pPool);
	}

    return EO_SUCCESS;
}



/***************************************************************************/
HI_S32 SAMPLE_PROC_AUDIO_StartAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,
                                 AIO_ATTR_S* pstAioAttr, 
								 AUDIO_SAMPLE_RATE_E enOutSampleRate, 
								 HI_BOOL bResampleEn, HI_VOID* pstAiVqeAttr, HI_U32 u32AiVqeType)
{
    HI_S32 i;
    HI_S32 s32Ret;
    s32Ret = HI_MPI_AI_SetPubAttr(AiDevId, pstAioAttr);
    if (s32Ret){
        printf("%s: HI_MPI_AI_SetPubAttr(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }
	
    s32Ret = HI_MPI_AI_Enable(AiDevId);
	if (s32Ret){
        printf("%s: HI_MPI_AI_Enable(%d) failed with %#x\n", __FUNCTION__, AiDevId, s32Ret);
        return s32Ret;
    }   
	
    for (i=0; i<s32AiChnCnt; i++)
    {
        s32Ret = HI_MPI_AI_EnableChn(AiDevId, i/(pstAioAttr->enSoundmode + 1));
        if (s32Ret)
        {
            printf("%s: HI_MPI_AI_EnableChn(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
            return s32Ret;
        }        

        if (HI_TRUE == bResampleEn)
        {
            s32Ret = HI_MPI_AI_EnableReSmp(AiDevId, i, enOutSampleRate);
			if (s32Ret)
            {
                printf("%s: HI_MPI_AI_EnableReSmp(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
                return s32Ret;
            }
        }

		if (NULL != pstAiVqeAttr)
        {
			HI_BOOL bAiVqe = HI_TRUE;
			switch (u32AiVqeType)
            {
				case 0:
                    s32Ret = HI_SUCCESS;
					bAiVqe = HI_FALSE;
                    break;
                case 1:
                    s32Ret = HI_MPI_AI_SetVqeAttr(AiDevId, i, SAMPLE_AUDIO_AO_DEV, i, (AI_VQE_CONFIG_S *)pstAiVqeAttr);
                    break;
                default:
                    s32Ret = HI_FAILURE;
                    break;
            }
			if (s32Ret)
		    {
                printf("%s: SetAiVqe%d(%d,%d) failed with %#x\n", __FUNCTION__, u32AiVqeType, AiDevId, i, s32Ret);
		        return s32Ret;
		    }
			
		    if (bAiVqe)
            {
				s32Ret = HI_MPI_AI_EnableVqe(AiDevId, i);
				if (s32Ret)
				{
					printf("%s: HI_MPI_AI_EnableVqe(%d,%d) failed with %#x\n", __FUNCTION__, AiDevId, i, s32Ret);
					return s32Ret;
				}
			}
		}
    }
    
    return HI_SUCCESS;
}





/***************************************************************************/
int HI_AUDIO_StartAi(int _s32Sample, int _s32FrameSample, AIO_ATTR_S *_pAioAttr, char _bEC)
{
	AI_VQE_CONFIG_S	stAiVqeAttr = {0};
	void *pAiVqeAttr = NULL;
	int	u32AiVqeType = 0, s32Ret = -1;
	if (_bEC){
		stAiVqeAttr.s32WorkSampleRate    = _s32Sample;
		stAiVqeAttr.s32FrameSample       = _s32FrameSample;
		stAiVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
		stAiVqeAttr.bAecOpen             = HI_TRUE;
		stAiVqeAttr.stAecCfg.bUsrMode    = HI_FALSE;
		stAiVqeAttr.stAecCfg.s8CngMode   = 0;
		stAiVqeAttr.bAgcOpen             = HI_TRUE;
		stAiVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
		stAiVqeAttr.bAnrOpen             = HI_TRUE;
		stAiVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
		stAiVqeAttr.bHpfOpen             = HI_TRUE;
		stAiVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
		stAiVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;
		stAiVqeAttr.bRnrOpen             = HI_FALSE;
		stAiVqeAttr.bEqOpen              = HI_FALSE;
		stAiVqeAttr.bHdrOpen             = HI_FALSE;
		pAiVqeAttr = (HI_VOID *)&stAiVqeAttr;	
		u32AiVqeType = 1;
	}
	s32Ret = SAMPLE_PROC_AUDIO_StartAi(0, 1, _pAioAttr, AUDIO_SAMPLE_RATE_BUTT, HI_FALSE, pAiVqeAttr, u32AiVqeType);
	return s32Ret;
}

int HI_AUDIO_StopAi(void)
{
	return SAMPLE_COMM_AUDIO_StopAi(0, 1, HI_FALSE, HI_FALSE);
}

int HI_AUDIO_StartAo(int _s32Sample, int _s32FrameSample, AIO_ATTR_S *_pAioAttr, char _bEC)
{
	AO_VQE_CONFIG_S stAoVqeAttr = {0};
	void *pAoVqeAttr = NULL;
	int u32AoVqeType = 0, s32Ret = -1;		
	if (_bEC){
		stAoVqeAttr.s32WorkSampleRate    = _s32Sample;
		stAoVqeAttr.s32FrameSample       = _s32FrameSample;
		stAoVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
		stAoVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
		stAoVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
		stAoVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
		stAoVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;

		stAoVqeAttr.bAgcOpen = HI_TRUE;
		stAoVqeAttr.bAnrOpen = HI_TRUE;
		stAoVqeAttr.bEqOpen  = HI_TRUE;
		stAoVqeAttr.bHpfOpen = HI_TRUE;
		
		pAoVqeAttr = (HI_VOID *)&stAoVqeAttr;
		u32AoVqeType = 1;			
	}
	s32Ret = SAMPLE_COMM_AUDIO_StartAo(0, 1, _pAioAttr, AUDIO_SAMPLE_RATE_BUTT, HI_FALSE, pAoVqeAttr, u32AoVqeType);
	return s32Ret;
}

int HI_AUDIO_StopAo(void)
{
	return SAMPLE_COMM_AUDIO_StopAo(0, 1, HI_FALSE, HI_FALSE);
}



int HI_AUDIO_SetAiVolume(int _s32Volume)
{
	HI_S32 s32Ret;
	s32Ret = HI_MPI_AI_SetVqeVolume(0, 0, _s32Volume);
	if (s32Ret){
		printf("Hisiv Audio Set Ai Volume Error!!!\r\n");
	}
	return s32Ret;	
	
}

int HI_AUDIO_SetAoVolume(int _s32Volume)
{
	HI_S32 s32Ret;
	s32Ret = HI_MPI_AO_SetVolume(0, _s32Volume);
	if (s32Ret){
		printf("Hisiv Audio Set Ao Volume Error!!!\r\n");
	}
	return s32Ret;
}


#endif