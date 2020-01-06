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


//(320,120):(640,480)格式
static int parse_corp(const char *data, T_VideoCorp *corp)
{
	int ret = EO_FORMAT;
	char *start = NULL, *end = NULL, str[8] = {0};

	start = strchr(data, '(');
	if (!start) goto EXIT;	
	
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	//x
	memcpy(str, start + 1, end - start - 1);
	//printf("=====x %s\n", str);
	corp->x = atoi(str);
	//printf("=====x %d\n", corp->x);
	//y
	start = end + 1;
	end = strchr(start, ')');
	if (!end) goto EXIT;
	
	memset(str , 0, 8);
	memcpy(str, start, end - start);
	//printf("=====y %s\n", str);
	corp->y = atoi(str);
	//printf("=====y %d\n", corp->y);
	//w
	start = end + 3;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(str , 0, 8);
	memcpy(str, start, end - start);	
	//printf("=====w %s\n", str);	
	corp->w = atoi(str);
	//printf("=====w %d\n", corp->w);
	//h
	start = end + 1;
	end = strchr(start, ')');
	if (!end) goto EXIT;
	
	memset(str , 0, 8);
	memcpy(str, start, end - start);		
	//printf("=====h %s\n", str);
	corp->h = atoi(str);	
	//printf("=====h %d\n", corp->h);
	
	ret = EO_SUCCESS;
EXIT:	
	return ret;
}	

//(通道名字, 物理fd号, blk_cnt, pix_size, venc_pix_size, fmt, profile, compress)名字不能使用""
//channel_0	= (vi_0, 0, 4, 16, 7, 96, 2, 1)
static int parse_video_channel(T_PoolInfo *pool, const char *p_data, T_VideoChannel *ainfo)
{
	int ret = EO_FORMAT;
	char *start = NULL, *end = NULL, buf[16] = {0};

	start = strchr(p_data, '(');
	if (!start) goto EXIT;	
	
	//channel name
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memcpy(buf, start + 1, end - start - 1);
	
	c_strdup2_with_null(pool, &ainfo->m_strName, buf);
	//printf("=====channel name [%s]\n", buf);

	//channel fd
	start = end + 1;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);
	ainfo->m_u32ChnlFd = atoi(buf);
	//printf("=====channel fd [%d]\n", d);

	//blk_cnt,计算内存使用，目前未使用
	start = end + 1;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);	
	//printf("=====blk_cnt  [%d]\n", d);	
	ainfo->m_u8BlkSize	= atoi(buf);
		
	//pix_size
	start = end + 1;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);
	ainfo->m_enCapPixSize = atoi(buf);
	//printf("===== m_enCapPixSize [%d]\n", d);
	
	//venc_pix_size
	start = end + 1;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);	
	//printf("=====venc_pix_size  [%d]\n", d);	
	ainfo->m_enVencPixSize	= atoi(buf);	
	
	//fmt
	start = end + 1;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);	
	//printf("=====fmt  [%d]\n", d);	
	ainfo->m_enChnnlFmt	= atoi(buf);	
	
	//m_enProfile
	start = end + 1;
	end = strchr(start, ',');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);	
	//printf("=====m_enProfile  [%d]\n", d);	
	ainfo->m_enProfile	= atoi(buf);	
	
	//m_enComp
	start = end + 1;
	end = strchr(start, ')');
	if (!end) goto EXIT;
	
	memset(buf , 0, 16);
	memcpy(buf, start, end - start);	
	//printf("=====m_enComp  [%d]\n", d);	
	ainfo->m_enComp	= atoi(buf);	
	
	ret = EO_SUCCESS;
EXIT:	
	return ret;		
	
	
	
	
}


//不支持多视频卡
int ParseVideo(T_PoolInfo *pool, dictionary *_pConf, const char *_srtFieldName, const char *_pChannel, T_AmioChannelInfo *_pInfo)
{
	T_VideoChannel *pVideo = (T_VideoChannel *)_pInfo;
	//公共部分
	pVideo->m_enGroupSize		= iniparser_getint(_pConf, "video:group_size", 0);
	pVideo->m_enRotate			= iniparser_getint(_pConf, "video:rotate", 0);
	pVideo->m_u8fps				= iniparser_getint(_pConf, "video:fps", 0);
	
	const char *pCorp = NULL;
	pCorp = iniparser_getstring(_pConf, "video:corp", NULL);
	if(pCorp){
		pVideo->in_pCorp = POOL_ALLOC_T(pool, T_VideoCorp);
		if (parse_corp(pCorp, pVideo->in_pCorp)) return EO_FORMAT;
	}
	
	if (parse_video_channel(pool, _pChannel, pVideo)) return EO_FORMAT;
	
	return EO_SUCCESS;
}




