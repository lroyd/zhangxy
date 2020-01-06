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
#include "timestamp.h"

#if AMIO_DEV_HAS_HIVIDEO
#include "os.h"
#include "sample_comm.h"

#include "amio_dev.h"
#include "console.h"
#include "parse_ini.h"

#define HIVIDEO_DEV_NAME			("hivideo")
#define HIVIDEO_DEV_VERSION			("v1.1")	//支持264，mjpg编码



typedef struct _tagHivideoFactory
{
    T_AmioDevFactory	in_tFactory;
	T_AmioDevInfo		in_tDevInfo;	
	
	T_PoolInfo 			*in_pPool;
	T_PoolFactory		*in_pPoolFactory;

	/* 海思硬件相关参数 */
	SAMPLE_VI_CONFIG_S 	m_stViConfig;
	VIDEO_NORM_E		m_enNorm;
	SAMPLE_RC_E 		m_enRcMode;
	
	VPSS_GRP 			m_stVpssGrp;
	
	
}T_HivideoFactory;

//create stream的时候创建
typedef struct _tagHivideoStream
{
    T_AmioStream		in_tStream;
	T_HivideoFactory	*in_pFactory;
	T_VideoChannel		*in_pChnnlInfo;		//+++
    T_PoolInfo 			*in_pPool;
	
    void                *pUserData;
	
	int                 m_bQuit;
	T_ThreadInfo		*m_pThread;
	
	PORT_DATA_CALLBCK	m_pCallbck;
	
	/* 采集使用的缓冲 */
#define	STREAM_PACK_CNT_MAX	(4)
	char				m_u8PackCnt;	//最多支持缓冲包的个数，目前最多支持4个
	VENC_PACK_S			*m_ptPack;
	
}T_HivideoStream;





/******************************************************************************/
#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"hivideo", ZXY_LOG_DEBUG_LV, 0};	//默认关闭
static char s_u8LogId = -1;

void HIVIDEO_SubLogRegister(void)
{
	s_u8LogId = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
}

int HIVIDEO_SubLogSet(char _enLogLv ,char _bLogOn)
{
	CONSOLE_SubLogSetParam(s_u8LogId, _enLogLv, _bLogOn);
}

T_SubLogInfo *HIVIDEO_SubLogGet(void)
{
	return CONSOLE_SubLogGetParam(s_u8LogId);
}

#endif



/******************************************************************************/
static int hivideo_factory_init(T_AmioDevFactory *_pFactory, const char *_strPathIni);
static int hivideo_factory_deinit(T_AmioDevFactory *_pFactory);
static int hivideo_factory_get_dev_info(T_AmioDevFactory *_pFactory, T_AmioDevInfo **_ppOut);
static int hivideo_factory_default_param(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, T_AmioChannelInfo **_ppOut);
static int hivideo_factory_create_stream(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, PORT_DATA_CALLBCK _pCallbck, void *_pData, T_AmioStream **_ppOut);
static int hivideo_factory_refresh(T_AmioDevFactory *_pFactory, const char *_strPathIni, char _bClose);

static T_AmioDevFactoryCtrl s_tFactoryCtrl =
{
    &hivideo_factory_init,
    &hivideo_factory_deinit,
	&hivideo_factory_get_dev_info,
    &hivideo_factory_default_param,
    &hivideo_factory_create_stream,
    &hivideo_factory_refresh
};

static int hivideo_stream_get_param(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOut);
static int hivideo_stream_get_cap(T_AmioStream *_pStream, int _enCap, void *_pOut);
static int hivideo_stream_set_cap(T_AmioStream *_pStream, int _enCap, const void *_pInValue);
static int hivideo_stream_start(T_AmioStream *_pStream);
static int hivideo_stream_stop(T_AmioStream *_pStream);
static int hivideo_stream_destroy(T_AmioStream *_pStream);

static T_AmioStreamCtrl s_tStreamCtrl =
{
    &hivideo_stream_get_param,
    &hivideo_stream_get_cap,
    &hivideo_stream_set_cap,
    &hivideo_stream_start,
    &hivideo_stream_stop,
    &hivideo_stream_destroy
};
/******************************************************************************/
T_AmioDevFactory *AmioHivideoDevice(T_PoolFactory *_pPoolFactory)
{
    T_HivideoFactory	*pFactory;
	T_PoolInfo			*pPool;
	int i;
	pPool = POOL_Create(_pPoolFactory, "hivideo", 512, 512, NULL);
	if (pPool == NULL) return NULL;
	
	//创建设备
    pFactory = POOL_ZALLOC_T(pPool, T_HivideoFactory);
    pFactory->in_pPoolFactory		= _pPoolFactory;
    pFactory->in_pPool				= pPool;
    pFactory->in_tFactory.in_pCtrl	= &s_tFactoryCtrl;
	pFactory->in_tFactory.in_pInfo  = &pFactory->in_tDevInfo;

	//设备属性
	T_AmioDevInfo *pDevInfo = &pFactory->in_tDevInfo;
	c_cstr(&pDevInfo->m_strName, 	HIVIDEO_DEV_NAME);
	c_cstr(&pDevInfo->m_strVer, 	HIVIDEO_DEV_VERSION);
	pDevInfo->m_enType	= AMIO_DEV_TYPE_VIDEO;
	
	pDevInfo->m_u8ChnnlTotal = AMIO_DEV_CHANNEL_MAX;
	
	for (i = 0; i < AMIO_DEV_CHANNEL_MAX; ++i){
		pDevInfo->m_aptChannelInfo[i] = (T_AmioChannelInfo *)POOL_ZALLOC_T(pPool, T_VideoChannel);
	}
	
    return &pFactory->in_tFactory;
}
/******************************************************************************/
static int hivideo_factory_init(T_AmioDevFactory *_pFactory, const char *_strPathIni)
{
    T_HivideoFactory *pFactory = (T_HivideoFactory *)_pFactory;
	int u32Ret = 0, u32BlkSize, i, j = 0;
	T_VideoChannel *pVideo = NULL;	
	SIZE_S stSize;
	
	//1.加载配置文件
	if (u32Ret = PARSER_CONFIG(AMIO, pFactory->in_pPool, _strPathIni, HIVIDEO_DEV_NAME, "video", &pFactory->in_tDevInfo)){
		return u32Ret;
	}

	c_cstr(&pFactory->in_tDevInfo.m_strPathIni, _strPathIni);
	
	//2.硬件设备相关初始化	
	pFactory->m_enNorm 		= VIDEO_ENCODING_MODE_NTSC;
	pFactory->m_enRcMode 	= SAMPLE_RC_CBR;//SAMPLE_RC_VBR;
	
	//2.1内存分配计算
	VB_CONF_S stVbConf = {0};
	for (i = 0; i < AMIO_DEV_CHANNEL_MAX; i++){
		pVideo = (T_VideoChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		if (pVideo->m_enType == AMIO_DEV_TYPE_VIDEO){
			u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(pFactory->m_enNorm, pVideo->m_enCapPixSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
			stVbConf.astCommPool[j].u32BlkSize	= u32BlkSize;
			stVbConf.astCommPool[j].u32BlkCnt	= pVideo->m_u8BlkSize;
			ZXY_DLOG(LOG_INFO, "use channel [%d] pool size = %d, cnt = %d", pVideo->m_u32ChnlId, u32BlkSize, pVideo->m_u8BlkSize);
			j++;
		}
	}
	stVbConf.u32MaxPoolCnt = 128;
	if (SAMPLE_COMM_SYS_Init(&stVbConf)){
		ZXY_ELOG("sys init vb failed");
		return EO_HARDWARE;
	}

	//3.1初始化VI
    pFactory->m_stViConfig.enViMode   = SENSOR_TYPE;
    pFactory->m_stViConfig.enRotate   = ROTATE_NONE;
    pFactory->m_stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    pFactory->m_stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    pFactory->m_stViConfig.enWDRMode  = WDR_MODE_NONE;	
    if (SAMPLE_PROC_VI_StartVi(&pFactory->m_stViConfig)){
		ZXY_ELOG("start vi failed");
        goto ERROR_1;
    }

	//3.2获取第一个可用通道的group size大小
	//!!!应该找到第一个“可用”通道的m_enGroupSize
	for (i = 0; i < AMIO_DEV_CHANNEL_MAX; i++){
		pVideo = (T_VideoChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		if (pVideo->m_enType == AMIO_DEV_TYPE_VIDEO) break;
	}
	if (i == AMIO_DEV_CHANNEL_MAX) goto ERROR_1;
	
	if (SAMPLE_COMM_SYS_GetPicSize(pFactory->m_enNorm, pVideo->m_enGroupSize, &stSize)){
		goto ERROR_1;
	}

	//3.3设置group attr属性
	pFactory->m_stVpssGrp = 0;
	VPSS_GRP_ATTR_S stVpssGrpAttr;
	stVpssGrpAttr.u32MaxW		= stSize.u32Width;
	stVpssGrpAttr.u32MaxH		= stSize.u32Height;
	stVpssGrpAttr.bIeEn			= HI_FALSE;
	stVpssGrpAttr.bNrEn			= HI_TRUE;
	stVpssGrpAttr.bHistEn		= HI_FALSE;
	stVpssGrpAttr.bDciEn		= HI_FALSE;
	stVpssGrpAttr.enDieMode		= VPSS_DIE_MODE_NODIE;
	stVpssGrpAttr.enPixFmt		= PIXEL_FORMAT_YUV_SEMIPLANAR_420;	
	if (SAMPLE_PROC_VPSS_StartGroup(pFactory->m_stVpssGrp, &stVpssGrpAttr, pVideo->in_pCorp)){
		ZXY_ELOG("start vpss failed");
		goto ERROR_2;
	}

	if (SAMPLE_COMM_VI_BindVpss(pFactory->m_stViConfig.enViMode)){
		ZXY_ELOG("start vi bind vpss failed");
		goto ERROR_3;
	}
	ZXY_DLOG(LOG_DEBUG, "hivideo init ok");
    return EO_SUCCESS;
	
ERROR_3:     
    SAMPLE_COMM_VI_UnBindVpss(pFactory->m_stViConfig.enViMode);	
	
ERROR_2:	
	SAMPLE_COMM_VPSS_StopGroup(pFactory->m_stVpssGrp);
	
ERROR_1:
	SAMPLE_COMM_VI_StopVi(&pFactory->m_stViConfig);
	return EO_HARDWARE;
}

static int hivideo_factory_deinit(T_AmioDevFactory *_pFactory)
{
    T_HivideoFactory *pFactory = (T_HivideoFactory *)_pFactory;
	int i;
	//1.停止当前设备下的所有流,非AMIO_DEV_TYPE_NONE流停止
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_VideoChannel *pChnnlInfo = (T_VideoChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_HivideoStream *pStream = (T_HivideoStream *)pChnnlInfo->m_vpStream;
		if (pStream && pChnnlInfo->m_enType == AMIO_DEV_TYPE_VIDEO && !pStream->m_bQuit){
			hivideo_stream_stop((T_AmioStream *)pStream);
			hivideo_stream_destroy((T_AmioStream *)pStream);
		}
		pChnnlInfo->m_enType = AMIO_DEV_TYPE_NONE;
		pChnnlInfo->m_vpStream = NULL;
	}

	//2.硬件设备相关销毁	
	SAMPLE_COMM_VI_UnBindVpss(pFactory->m_stViConfig.enViMode);	
	SAMPLE_COMM_VPSS_StopGroup(pFactory->m_stVpssGrp);
	SAMPLE_COMM_VI_StopVi(&pFactory->m_stViConfig);
	
    return EO_SUCCESS;
}

//刷新所有通道(不删减)，重载配置文件
static int hivideo_factory_refresh(T_AmioDevFactory *_pFactory, const char *_strPathIni, char _bClose)
{
#if 0	
	T_HivideoFactory *pFactory = (T_HivideoFactory *)_pFactory;
	char u8Active[AMIO_DEV_CHANNEL_MAX] = {0}, i;
	int u32Ret;
	//1.先停掉已经开启的流，需要保存激活标志
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_VideoChannel *pChnnlInfo = (T_VideoChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_HivideoStream *pStream = (T_HivideoStream *)pChnnlInfo->m_vpStream;
		if (pStream && pChnnlInfo->m_enType == AMIO_DEV_TYPE_VIDEO && !pStream->m_bQuit){
			u8Active[i] = !pStream->m_bQuit & _bClose;
			hivideo_stream_stop((T_AmioStream *)pStream);
			hivideo_stream_destroy((T_AmioStream *)pStream);
		}
	}
	
	//2.硬件设备的关闭
	SAMPLE_COMM_VI_UnBindVpss(pFactory->m_stViConfig.enViMode);	
	SAMPLE_COMM_VPSS_StopGroup(pFactory->m_stVpssGrp);
	SAMPLE_COMM_VI_StopVi(&pFactory->m_stViConfig);	
	
	//3.重新init
	if (u32Ret = hivideo_factory_init((T_AmioDevFactory *)pFactory, _strPathIni)){
		return u32Ret;
	}
	
	//4.开启之前激活的流
	for (i = 0; i < pFactory->in_tDevInfo.m_u8ChnnlTotal; ++i){
		T_VideoChannel *pChnnlInfo = (T_VideoChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[i];
		T_HivideoStream *pStream = (T_HivideoStream *)pChnnlInfo->m_vpStream;
		if (pStream && u8Active[i]){
			hivideo_stream_start((T_AmioStream *)pStream);
		}
	}
#endif
    return 0;
}


static int hivideo_factory_get_dev_info(T_AmioDevFactory *_pFactory, T_AmioDevInfo **_ppOutDevInfo)
{
    T_HivideoFactory *pFactory = (T_HivideoFactory *)_pFactory;

    return 0;
}

//设置通道dir和cap
static int hivideo_factory_default_param(T_AmioDevFactory *_pFactory, int _u32ChnlIdx, T_AmioChannelInfo **_ppOut)
{
    T_HivideoFactory *pFactory = (T_HivideoFactory *)_pFactory;
	T_VideoChannel *pVideo = (T_VideoChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[_u32ChnlIdx];
	pVideo->m_enDir = AMIO_DIR_ENCODING;
	//cap设置

	if (_ppOut) *_ppOut = (T_AmioChannelInfo *)pVideo;
	
    return EO_SUCCESS;
}

//创建流中使用的pool
static int hivideo_factory_create_stream(T_AmioDevFactory *_pFactory, 
										int _u32ChnlIdx, 
										PORT_DATA_CALLBCK _pCallbck, 
										void *_pData, 
										T_AmioStream **_ppOut)
{
    T_HivideoFactory *pFactory = (T_HivideoFactory *)_pFactory;
	T_HivideoStream *pFStream	= NULL;
	T_VideoChannel *pVideo		= NULL;
	T_PoolInfo		*pPool		= NULL;;
	pVideo = (T_VideoChannel *)pFactory->in_tDevInfo.m_aptChannelInfo[_u32ChnlIdx];	
	if (pVideo->m_enType != AMIO_DEV_TYPE_VIDEO) return EO_NOTFOUND;
	
	pPool = POOL_Create(pFactory->in_pPoolFactory, "hivideo%p", 1024, 1024, NULL);
	if (pPool == NULL) return EO_NOMEM;
	
    
    pFStream = POOL_ZALLOC_T(pPool, T_HivideoStream);
	pFStream->in_pFactory				= pFactory;
	pFStream->in_pChnnlInfo				= pVideo;
	pFStream->in_pPool					= pPool;
	
    pFStream->m_pCallbck				= _pCallbck;
    pFStream->pUserData					= _pData;
	pFStream->m_bQuit					= 1;
	
	pFStream->in_pChnnlInfo->m_vpStream = (void *)pFStream;
	
    /* 只有采集通道 */
    if (pVideo->m_enDir & AMIO_DIR_ENCODING){
		//分配采集使用的buf
		pFStream->m_u8PackCnt = STREAM_PACK_CNT_MAX;	
		pFStream->m_ptPack = (VENC_PACK_S *)POOL_Alloc(pPool, sizeof(VENC_PACK_S) * STREAM_PACK_CNT_MAX);
    }
	
	//硬件相关
	//.1获取当前通到采集大小（注意与group size的大小）
	SIZE_S stSize;
	if (SAMPLE_COMM_SYS_GetPicSize(pFactory->m_enNorm, pVideo->m_enCapPixSize, &stSize)){
		ZXY_ELOG("channel [%d] get picture size failed", _u32ChnlIdx);
		goto ERROR;
	}
	
	VPSS_CHN_MODE_S stVpssChnMode;
	stVpssChnMode.enChnMode			= VPSS_CHN_MODE_USER;
	stVpssChnMode.bDouble			= HI_FALSE;
	stVpssChnMode.enPixelFormat 	= PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	stVpssChnMode.u32Width			= stSize.u32Width;
	stVpssChnMode.u32Height 		= stSize.u32Height;
	stVpssChnMode.enCompressMode 	= pVideo->m_enComp;
	VPSS_CHN_ATTR_S stVpssChnAttr = {0};
	stVpssChnAttr.s32SrcFrameRate 	= -1;
	stVpssChnAttr.s32DstFrameRate 	= -1;	
	
	//.2是否有旋转
	if (pVideo->m_enRotate){
		if (HI_MPI_VPSS_SetRotate(pFactory->m_stVpssGrp, pVideo->m_u32ChnlFd,  pVideo->m_enRotate)){
			ZXY_DLOG(LOG_WARNING, "channel [%d] set rotate failed", _u32ChnlIdx);
		}
	}
	
	//.3使能vpss通道
	if (SAMPLE_COMM_VPSS_EnableChn(pFactory->m_stVpssGrp, pVideo->m_u32ChnlFd, &stVpssChnAttr, &stVpssChnMode, HI_NULL)){
		printf("Enable vpss chn failed!\n");
		ZXY_ELOG("channel [%d] enable vpss failed", _u32ChnlIdx);
		goto ERROR;
	}
	//.4使能venc通道		
	if (SAMPLE_PROC_VENC_Start(pVideo->m_u32ChnlFd, pVideo->m_enChnnlFmt, pFactory->m_enNorm, pVideo->m_enVencPixSize, pFactory->m_enRcMode, pVideo->m_enProfile)){
		ZXY_ELOG("channel [%d] start venc failed", _u32ChnlIdx);
		goto ERROR;
	}
	//.5绑定venc通道
	if (SAMPLE_COMM_VENC_BindVpss(pVideo->m_u32ChnlFd, pFactory->m_stVpssGrp, pVideo->m_u32ChnlFd)){
		ZXY_ELOG("channel [%d] venc bind vpss failed", _u32ChnlIdx);
		goto ERROR;
	}

    pFStream->in_tStream.in_pCtrl = &s_tStreamCtrl;
	if (_ppOut) *_ppOut = &pFStream->in_tStream;
	
	//ZXY_DLOG(LOG_DEBUG,   "epoll_wait timeout = %d", cnt);
    return EO_SUCCESS;
	
ERROR:
	SAMPLE_COMM_VENC_UnBindVpss(pVideo->m_u32ChnlFd, pFactory->m_stVpssGrp, pVideo->m_u32ChnlFd);
	SAMPLE_COMM_VENC_Stop(pVideo->m_u32ChnlFd);		
	SAMPLE_COMM_VPSS_DisableChn(pFactory->m_stVpssGrp, pVideo->m_u32ChnlFd);	
	return EO_HARDWARE;	
}


static int hivideo_stream_get_param(T_AmioStream *_pStream, T_AmioChannelInfo **_ppOut)
{
    return 0;
}

static int hivideo_stream_get_cap(T_AmioStream *_pStream, int _enCap, void *_pOutValue)
{
	return 0;
}

static int hivideo_stream_set_cap(T_AmioStream *_pStream, int _enCap, const void *_pInValue)
{
    return 0;
}


static int video_stream_thread(void *_pArg)
{
    T_HivideoStream *pFStream = (T_HivideoStream *)_pArg;
	T_VideoChannel *pVideo = pFStream->in_pChnnlInfo;
	PORT_DATA_CALLBCK pCallbck = pFStream->m_pCallbck;
	int		u32Len = 0, i;
	int u32fd, s32Ret;
	/* 平台相关 */
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
	
    struct timeval timeout;
    fd_set read_fds;	

	u32fd = HI_MPI_VENC_GetFd(pVideo->m_u32ChnlFd);
	if (u32fd < 0){
		//ZXY_ELOG("channel [%d] get venc fd [%d] failed", pVideo->m_u32ChnlId, pVideo->m_u32ChnlFd);
		return EO_HARDWARE;
	}
	
	T_Frame tFrame = {0};
	tFrame.m_u32fd	= pVideo->m_u32ChnlFd;	
    while (!pFStream->m_bQuit) 
    {	
        FD_ZERO(&read_fds);
        FD_SET(u32fd, &read_fds);

        timeout.tv_sec  = 2;
        timeout.tv_usec = 0;
        s32Ret = select(u32fd + 1, &read_fds, NULL, NULL, &timeout);
        if (s32Ret < 0){
            break;
        }else if (s32Ret == 0){
			ZXY_DLOG(LOG_WARNING, "channel [%d][%d] get venc stream time out, continue", pVideo->m_u32ChnlId, pVideo->m_u32ChnlFd);
            continue;
        }else{
			if (FD_ISSET(u32fd, &read_fds)){		
				memset(&stStream, 0, sizeof(stStream));
				if (HI_MPI_VENC_Query(pVideo->m_u32ChnlFd, &stStat)){
					//ZXY_ELOG("channel [%d][%d] mpi venc query failed", pVideo->m_u32ChnlId, pVideo->m_u32ChnlFd);
					break;
				}

				if(stStat.u32CurPacks){
					if (stStat.u32CurPacks > pFStream->m_u8PackCnt){
						//包的个数超过最大4个,重新分配,之前的不会释放，除非stream销毁
						pFStream->m_ptPack = (VENC_PACK_S *)POOL_Alloc(pFStream->in_pPool, sizeof(VENC_PACK_S) * STREAM_PACK_CNT_MAX * 2);
						pFStream->m_u8PackCnt = STREAM_PACK_CNT_MAX * 2;
						ZXY_DLOG(LOG_WARNING, "pack not enough mem");
					}
					
					stStream.pstPack = pFStream->m_ptPack;
					stStream.u32PackCount = stStat.u32CurPacks;
					if (HI_MPI_VENC_GetStream(pVideo->m_u32ChnlFd, &stStream, HI_TRUE)){
						//ZXY_ELOG("channel [%d][%d] mpi venc get stream failed", pVideo->m_u32ChnlId, pVideo->m_u32ChnlFd);
						break;
					}
					
					//编码格式兼容
					for (i = 0; i < stStream.u32PackCount; i++){
						tFrame.m_u8Type	= stStream.pstPack[i].DataType.enH264EType;
						tFrame.m_vpData	= (void *)stStream.pstPack[i].pu8Addr+stStream.pstPack[i].u32Offset;	
						tFrame.m_uSize	= stStream.pstPack[i].u32Len-stStream.pstPack[i].u32Offset;
						tFrame.m_ullTs	= stStream.pstPack[i].u64PTS;
						ZXY_DLOG(LOG_INFO, "channel [%d] stream get type %d, size:%d", pVideo->m_u32ChnlId, tFrame.m_u8Type, tFrame.m_uSize);
#if 1
						if (pCallbck){
							s32Ret = (pCallbck)(pFStream->pUserData, &tFrame);
						}	
#endif						
					}

					if (HI_MPI_VENC_ReleaseStream(pVideo->m_u32ChnlFd, &stStream)){
						//释放失败
						break;
					}

				}

			}
        }
    }
	ZXY_DLOG(LOG_WARNING, "channel [%d][%d] stop video stream exit", pVideo->m_u32ChnlId, pVideo->m_u32ChnlFd);
    return 0;
}


static int hivideo_stream_start(T_AmioStream *_pStream)
{
    T_HivideoStream *pFStream = (T_HivideoStream *)_pStream;
	T_VideoChannel *pVideo = pFStream->in_pChnnlInfo;
    ZXY_DLOG(LOG_INFO, "start hivideo stream");

    pFStream->m_bQuit = 0;
	if (pVideo->m_enType & AMIO_DEV_TYPE_VIDEO){
		//视频，注意不同的编码格式
		if (pVideo->m_enDir & AMIO_DIR_ENCODING){
			//采集
			THREAD_Create(pFStream->in_pPool, 
							NULL, 
							video_stream_thread,
							(void *)pFStream,
							0,
							0,
							&pFStream->m_pThread);			

		}

	}else{
		ZXY_DLOG(LOG_WARNING, "stream does not support video types");
	}

    return EO_SUCCESS;
}

static int hivideo_stream_stop(T_AmioStream *_pStream)
{
    T_HivideoStream *pFStream = (T_HivideoStream *)_pStream;
	T_VideoChannel *pVideo = pFStream->in_pChnnlInfo;
	if (pVideo->m_enType & AMIO_DEV_TYPE_VIDEO){
		//视频
		if (pFStream->m_bQuit == 0){
			pFStream->m_bQuit = 1;

			if (pVideo->m_enDir & AMIO_DIR_CAPTURE){
				THREAD_Join(pFStream->m_pThread);
			}		
		}

	}
	else{
		ZXY_DLOG(LOG_WARNING, "stream does not support video types");
	}	
	
    return EO_SUCCESS;
}

static int hivideo_stream_destroy(T_AmioStream *_pStream)
{
    T_HivideoStream *pFStream = (T_HivideoStream *)_pStream;
	if (pFStream->in_pPool){
		POOL_Destroy(pFStream->in_pPool);
	}
	
	T_HivideoFactory *pFactory = pFStream->in_pFactory;
	T_VideoChannel *pVideo = pFStream->in_pChnnlInfo;
	
	SAMPLE_COMM_VENC_UnBindVpss(pVideo->m_u32ChnlFd, pFactory->m_stVpssGrp, pVideo->m_u32ChnlFd);
	SAMPLE_COMM_VENC_Stop(pVideo->m_u32ChnlFd);		
	SAMPLE_COMM_VPSS_DisableChn(pFactory->m_stVpssGrp, pVideo->m_u32ChnlFd);		

    return EO_SUCCESS;
}





#endif