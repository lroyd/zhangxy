/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "amio.h"
#include "error_code.h"
#include "iniparser.h"

//解析复合型参数
static int parse_audio_channel(T_PoolInfo *pool, const char *p_data, T_AudioChannel *ainfo)
{
	int ret = EO_FORMAT, d;
	char *start = NULL, *end = NULL, buf[16] = {0};

	start = strchr(p_data, '(');
	if (!start) goto EXIT;	
	
	//channel name
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memcpy(buf, start + 1, end - start - 1);
	
	c_strdup2_with_null(pool, &ainfo->m_strName, buf);
	//printf("=====channel name [%s]\n", buf);

	//channel index
	start = end + 1;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);
	ainfo->m_u32ChnlFd = atoi(buf);
	//printf("=====channel index [%d]\n", d);

	//channel fmt 
	start = end + 1;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);	
	//printf("=====channel fmt  [%d]\n", d);	
	ainfo->m_enChnnlFmt	= atoi(buf);
		
	//vol
	start = end + 1;
	end = strchr(start, ')');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);
	ainfo->m_s32Volume = atoi(buf);
	//printf("===== vol [%d]\n", d);
	
	ret = EO_SUCCESS;
EXIT:	
	return ret;	
}


//解析音频配置文件参数
//使用_srtFieldName就是为了驱动多卡情况，目前只支持单一卡
int ParseAudio(T_PoolInfo *pool, dictionary *_pConf, const char *_srtFieldName, const char *_pChannel, T_AmioChannelInfo *_pInfo)
{
	T_AudioChannel *pAudio = (T_AudioChannel *)_pInfo;
	//公共部分
	pAudio->m_bEC				= iniparser_getint(_pConf, "audio:ec", 0);
	pAudio->m_bLookup			= iniparser_getint(_pConf, "audio:lookup", 0);
	
	pAudio->m_u8SndCnt			= iniparser_getint(_pConf, "audio:sound", 0);
	pAudio->m_u32SampleRate		= iniparser_getint(_pConf, "audio:rate", 0);
	pAudio->m_u32PTime			= iniparser_getint(_pConf, "audio:ptime", 0);
	pAudio->m_u32BitWidth		= iniparser_getint(_pConf, "audio:width", 0);
	pAudio->m_u32FrameSize		= pAudio->m_u32SampleRate * pAudio->m_u8SndCnt * (pAudio->m_u32BitWidth/8) * pAudio->m_u32PTime/1000;
	//printf("=========== %d, %d\r\n", pAudio->m_u32BitWidth, pAudio->m_u32FrameSize);
	
	return parse_audio_channel(pool, _pChannel, pAudio);
}




