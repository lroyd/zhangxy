/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_AUDIO_INFO_H__
#define __ZXY_AUDIO_INFO_H__

#include "cstring.h"


enum
{
    AMIO_AUD_DEV_CAP_EXT_FORMAT = 1,
    AMIO_AUD_DEV_CAP_INPUT_LATENCY = 2,
	
    AMIO_AUD_DEV_CAP_INPUT_VOLUME_SETTING = 8,
    AMIO_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING = 16,	
};


typedef struct _tagAudioChannel
{
	str_t			m_strName;
	int				m_u32ChnlId;	
	int				m_u32ChnlFd;
	char			m_enType;
	
	char			m_enDir;
	char			m_enCap;
	void			*m_vpStream;

	/* 音频参数 for ini */	
	unsigned char	m_bEC;		//是否开启回音抑制
	unsigned char	m_bLookup;	//是否开启回环
	
	unsigned char	m_u8SndCnt;
	unsigned int 	m_u32SampleRate;		//音频采样率
	unsigned int	m_u32PTime; 	//每帧采样大小(按位宽计算,不是绝对字节)
	unsigned int	m_u32BitWidth;	//采样位宽8/16
	unsigned int	m_u32FrameSize;	//每帧大小

	int				m_enChnnlFmt;		//通道格式
	int				m_s32Volume;	
}T_AudioChannel;




#endif	
