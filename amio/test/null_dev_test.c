/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include "amio.h"
#include "os.h"

T_PoolCaching cp;

static int ai_cb(void *usr_data, T_Frame *frame)
{
	T_Frame *f = frame;
	printf("ai callbck: fd = %d, data = , len = %d\r\n", f->m_u32fd, f->m_uSize);
	return 0;
}

static int ao_cb(void *usr_data, T_Frame *frame)
{
	T_Frame *f = frame;
	printf("ao callbck: fd = %d, data = , len = %d\r\n", f->m_u32fd, f->m_uSize);
	return 0;
}

int null_dev_test(void) 
{
	char *NULL_DEV = "null_dev";
	char *NULL_INI = "null.ini";
	char *NULL_INI2 = "null2.ini";
	char *NULL_AI_CHNNL = "ai_0";
	char *NULL_AO_CHNNL = "ao_1";	
	
	THREAD_Init();
	POOL_CachingInit(&cp, NULL, 0);	
	AMIO_DrvLoad(&cp.in_pFactory);			
	
	char pbuf[512] = {0};
	//printf("%s", AMIO_DrvEnum(pbuf, 512));
	//激活设备
	if (AMIO_DevActive(NULL_DEV, NULL_INI)){
		printf("error not find %s dev's %s\r\n", NULL_DEV, NULL_INI);
		return 0;
	}
	//printf("%s", AMIO_DrvEnum(pbuf, 512));
	
	int ai_fd, ao_fd, cnt = 0;
	T_AmioStream *ai_stream = NULL, *ao_stream = NULL;
	
	/* 初始化ai */
	if (AMIO_DevLookup(NULL_DEV, NULL_AI_CHNNL, &ai_fd)){
		printf("error not find \"%s\" dev's channel \"%s\" \r\n", NULL_DEV, NULL_AI_CHNNL);
		return 0;		
	}
	printf("get ai %d\r\n", ai_fd);
	AMIO_DevDefaultInfo(ai_fd, NULL);
	AMIO_StreamCreate(ai_fd, ai_cb, NULL, NULL, &ai_stream);
	
	/* 初始化ao */
	if (AMIO_DevLookup(NULL_DEV, NULL_AO_CHNNL, &ao_fd)){
		printf("error not find \"%s\" dev's channel \"%s\" \r\n", NULL_DEV, NULL_AO_CHNNL);
		return 0;		
	}	
	printf("get ao %d\r\n", ao_fd);
	AMIO_DevDefaultInfo(ao_fd, NULL);
	AMIO_StreamCreate(ao_fd, ao_cb, NULL, NULL, &ao_stream);	
	
	/**********************************************************/
	AMIO_StreamStart(ai_stream);
	AMIO_StreamStart(ao_stream);
	
	printf("%s", AMIO_DrvEnum(pbuf, 512));
	
	POOL_FactoryDump(&cp.in_pFactory);
	while (1)
	{
#if 0		
		if (cnt == 5){
			AMIO_StreamStop(ai_stream);
		}
		else if (cnt == 10){
			AMIO_StreamStart(ai_stream);
			cnt = 0;
		}
#endif		

#if 1
		//重载配置文件继续运行
		if (cnt == 5){
			printf("===============  refresh  ==================\r\n");
			AMIO_DevRefresh(NULL_DEV, NULL_INI2);
			printf("%s", AMIO_DrvEnum(pbuf, 512));
		}
#endif

#if 0
		if (cnt == 5){
			AMIO_DevFrozen(NULL_DEV);
			//AMIO_DrvEnum(NULL);
			//POOL_FactoryDump(&cp.in_pFactory);
			AMIO_DevActive(NULL_DEV, NULL_INI2);
			AMIO_DevLookup(NULL_DEV, NULL_AI_CHNNL, &ai_fd);
			AMIO_DevDefaultInfo(ai_fd, NULL);
			AMIO_StreamCreate(ai_fd, (void *)ai_cb, NULL, NULL, &ai_stream);	
			AMIO_StreamStart(ai_stream);		
			//AMIO_DrvEnum(NULL);
			POOL_FactoryDump(&cp.in_pFactory);
		}		

#endif


		sleep(1);
		printf("poll..%d\r\n", cnt++);
	}	
	
	
    return 0;
}










