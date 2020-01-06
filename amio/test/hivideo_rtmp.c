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
*			1.
*			2.
********************************************************************************/
#define RTMP_URL_TEST	("rtmp://192.168.1.163/live/stream0")


static T_PoolCaching cp;

static volatile int flags = 0;
static int vi_cb(void *usr_data, T_Frame *frame)
{
	T_Frame *f = frame;
	
	if (!flags){
		flags = rtmp_callbck(f->m_u8Type, f->m_vpData, f->m_uSize, f->m_ullTs);
		//printf("[%d] vi callbck: fd = %d, type = %d, len = %d\r\n", flags, f->m_u32fd, f->m_u8Type, f->m_uSize);
	}
	
	return 0;
}



int hivideo_rtmp_test(void) 
{
	char *HIVIDEO_DEV = "hivideo";
	char *HIVIDEO_INI = "hiavio.ini";

	THREAD_Init();
	POOL_CachingInit(&cp, NULL, 0);	
	AMIO_DrvLoad(&cp.in_pFactory);			
	
	/*===================================================*/
	//服务相关
	rtmp_init();
	if (rtmp_create(RTMP_URL_TEST)){
		printf("rtmp connect [%s] failed\r\n", RTMP_URL_TEST);
		return 0;		
	}
	
	/*===================================================*/
	char pbuf[512] = {0};
	printf("%s", AMIO_DrvEnum(pbuf, 512));
	
	//激活设备
	if (AMIO_DevActive(HIVIDEO_DEV, HIVIDEO_INI)){
		printf("error not find %s dev's %s AMIO_DevActive failed\r\n", HIVIDEO_DEV, HIVIDEO_INI);
		return 0;
	}
	//printf("%s", AMIO_DrvEnum(pbuf, 512));
	
	char *HI_VI[3] = {"vi_0","vi_1","vi_2"};
	int i, vi_fd[3]; T_AmioStream *vi_stream[3] = {NULL};
	
	for(i = 0; i < 1; i++){
		/* 创建采集流 */
		if (AMIO_DevLookup(HIVIDEO_DEV, HI_VI[i], &vi_fd[i])){
			printf("error not find \"%s\" dev's channel \"%s\" \r\n", HIVIDEO_DEV, HI_VI[i]);
			return 0;		
		}
			
		AMIO_DevDefaultInfo(vi_fd[i], NULL);
		if (AMIO_StreamCreate(vi_fd[i], vi_cb, NULL, NULL, &vi_stream[i])){
			printf("error AMIO_StreamCreate \r\n");
			return 0;					
		}

		/**********************************************************/
		AMIO_StreamStart(vi_stream[i]);
	}
	
	printf("%s", AMIO_DrvEnum(pbuf, 512));
	int cnt = 0;

	while (1){
		sleep(1);
		if (flags){
			//重连rtmp
			flags = rtmp_refresh(RTMP_URL_TEST);
			printf("rtmp refresh %d\r\n", flags);
		}
	}
	
    return 0;
}










