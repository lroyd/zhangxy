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
#include "parse_ini.h"




//刷新设备会增加内存！！！
static int parser_channel(T_PoolInfo *pool, dictionary *_pConf, const char *_srtFieldName, T_AmioChannelInfo **_ppInfo, char *_pOutCnt)
{
	char aucFieldName[INI_FIELD_NAME_LEN_MAX] = {0};
	char enType = AMIO_DEV_TYPE_NONE, u8Count;
	int i;
	T_AmioChannelInfo **pInfoList = _ppInfo;
	const char *pChannel = NULL;

	//1.1解析设备类型
	if (strstr(_srtFieldName, "audio")){
		enType = AMIO_DEV_TYPE_AUDIO;
	}else if (strstr(_srtFieldName, "video")){
		enType = AMIO_DEV_TYPE_VIDEO;
	}else if (strstr(_srtFieldName, "comm")){
		enType = AMIO_DEV_TYPE_COMM;
	}
	
	//1.2解析通道公共部分,单个设备最大3路通道
	for (i = 0; i < AMIO_DEV_CHANNEL_MAX; ++i){
		sprintf(aucFieldName, "%s:channel_%d", _srtFieldName, i);
		if (pChannel = iniparser_getstring(_pConf, aucFieldName, NULL)){
			pInfoList[i]->m_u32ChnlId	= i;
			pInfoList[i]->m_enType		= enType;
			//pInfoList[i]->m_vpStream	= 0;	//保留之前状态
			
			int u32Ret = 0;
			switch(enType){
				case AMIO_DEV_TYPE_AUDIO:
				{
					u32Ret = ParseAudio(pool, _pConf, _srtFieldName, pChannel, pInfoList[i]);
					break;
				}
				case AMIO_DEV_TYPE_VIDEO:
				{
					u32Ret = ParseVideo(pool, _pConf, _srtFieldName, pChannel, pInfoList[i]);
					break;
				}
				case AMIO_DEV_TYPE_COMM:
				{
					u32Ret = ParseComm(pool, _pConf, _srtFieldName, pChannel, pInfoList[i]);
					break;
				}				
			}
			
			if (!u32Ret){
				u8Count++;
			}
			else
				return EO_FORMAT;
		}else{
			//通道不存在
			pInfoList[i]->m_enType = AMIO_DEV_TYPE_NONE;
		}
	}
	*_pOutCnt = 3;	//写死3个
	return EO_SUCCESS;
}


int INI_AMIO_ParserConfig(T_PoolInfo *pool, const char *_pConfigPath, const char *_strName, const char *_srtFieldName, T_AmioDevInfo *_pInfo)
{
	char u8Type = 0, aucFieldName[INI_FIELD_NAME_LEN_MAX] = {0}, u8Len = 0, u8Cnt = 0;
	dictionary *conf;
	int u32Ret = EO_SUCCESS;
	if (access(_pConfigPath, F_OK)) return EO_NOTFOUND;

    conf = iniparser_load(_pConfigPath);
    if(conf == NULL) return EO_NOTFOUND;
	
	//1.检查ini文件名字是否一致
	u8Len = sprintf(aucFieldName, "%s:name", _srtFieldName); aucFieldName[u8Len] = 0;
	const char *str = iniparser_getstring(conf, aucFieldName, NULL);
	if (str){
		if (strcmp(_strName, str)) return EO_NOTFOUND;			
	}
	else
		return EO_FORMAT;
	
	if (u32Ret = parser_channel(pool, conf, _srtFieldName, _pInfo->m_aptChannelInfo, &u8Cnt)){
		return u32Ret;
	}
	
	_pInfo->m_u8ChnnlTotal = u8Cnt;
	return u32Ret;
}


