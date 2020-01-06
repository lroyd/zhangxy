/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include "amio.h"
#include "os.h"

/********************************************************************************
*测试内容：
*			1.回环测试（修改配置文件）
*			2.采集保存audio.pcm文件（修改源码编译）
********************************************************************************/

T_PoolCaching cp;

FILE *pfd = NULL;

static int ai_cb(void *usr_data, T_Frame *frame)
{
	T_Frame *f = frame;
	printf("ai callbck: fd = %d, data = , len = %d\r\n", f->m_u32fd, f->m_uSize);
	//采集保存文件
	//fwrite(frame->m_vpData, 1, f->m_uSize, pfd);
	//fflush(pfd);
	
	return 0;
}

static int ao_cb(void *usr_data, T_Frame *frame)
{
	T_Frame *f = frame;
	printf("ao callbck: fd = %d, data = , len = %d\r\n", f->m_u32fd, f->m_uSize);
	return 0;
}

int hiaudio_test(void) 
{
	char *HIAUDIO_DEV = "hiaudio";
	char *HIAUDIO_INI = "hiavio.ini";

	char *HI_AI_CHNNL = "ai_0";
	char *HI_AO_CHNNL = "ao_1";	
	
	//pfd = fopen("audio.pcm", "w+");
	
	THREAD_Init();
	POOL_CachingInit(&cp, NULL, 0);	
	AMIO_DrvLoad(&cp.in_pFactory);			
	
	char pbuf[512] = {0};
	printf("%s", AMIO_DrvEnum(pbuf, 512));
	//激活设备
	if (AMIO_DevActive(HIAUDIO_DEV, HIAUDIO_INI)){
		printf("error not find %s dev's %s\r\n", HIAUDIO_DEV, HIAUDIO_INI);
		return 0;
	}
	//printf("%s", AMIO_DrvEnum(pbuf, 512));
	
	int ai_fd, ao_fd, cnt = 0;
	T_AmioStream *ai_stream = NULL, *ao_stream = NULL;
	
	/* 初始化ai */
	if (AMIO_DevLookup(HIAUDIO_DEV, HI_AI_CHNNL, &ai_fd)){
		printf("error not find \"%s\" dev's channel \"%s\" \r\n", HIAUDIO_DEV, HI_AI_CHNNL);
		return 0;		
	}
	printf("get ai %d\r\n", ai_fd);
	AMIO_DevDefaultInfo(ai_fd, NULL);
	AMIO_StreamCreate(ai_fd, ai_cb, NULL, NULL, &ai_stream);
	
	/* 初始化ao */
	if (AMIO_DevLookup(HIAUDIO_DEV, HI_AO_CHNNL, &ao_fd)){
		printf("error not find \"%s\" dev's channel \"%s\" \r\n", HIAUDIO_DEV, HI_AO_CHNNL);
		return 0;		
	}	
	printf("get ao %d\r\n", ao_fd);
	AMIO_DevDefaultInfo(ao_fd, NULL);
	AMIO_StreamCreate(ao_fd, ao_cb, NULL, NULL, &ao_stream);	
	
	/**********************************************************/
	AMIO_StreamStart(ai_stream);
	//AMIO_StreamStart(ao_stream);
	
	printf("%s", AMIO_DrvEnum(pbuf, 512));
	
	//POOL_FactoryDump(&cp.in_pFactory);
	
	while (1)
	{
		sleep(1);
		printf("poll..%d\r\n", cnt++);
	}	
	
	
    return 0;
}










