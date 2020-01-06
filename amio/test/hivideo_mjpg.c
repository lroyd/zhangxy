/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include "amio.h"
#include "os.h"

#include "http_api.h"

/********************************************************************************
*测试内容：
*			1.http发送
*			2.动态log开关
********************************************************************************/

T_PoolCaching cp;


static int vi_cb(void *usr_data, T_Frame *frame)
{
	T_Frame *f = frame;
	//printf("vi callbck: fd = %d, type = %d, len = %d\r\n", f->m_u32fd, f->m_u8Type, f->m_uSize);
	//采集保存文件

	
	
	http_send_jpeg(frame->m_vpData, f->m_uSize);
	
	
	
	return 0;
}


static int echo_server_handle(char *p_data, int len)
{
	if (strstr(p_data, "on"))		
	{
		//HIVIDEO_SubLogSet(LOG_INFO, 1);
		AMIO_SubLogSet(HIVIDEO, LOG_INFO, 1);
	}
	if (strstr(p_data, "off"))		
	{
		//HIVIDEO_SubLogSet(LOG_INFO, 0);
		AMIO_SubLogSet(HIVIDEO, LOG_INFO, 0);
	}

	
	
	return 0;
}

int http_deamon(void)
{
	int ret = -1, nfds, i,j , efd = -1;
	int http_fd = -1, cfd = -1, ufd = -1;
	static unsigned int count = 0;
	struct epoll_event	events[16];  
	pthread_t client;
	UEPOLL_Create(&efd);	
	NETTCP_ServerOpen(HTTP_SER_PORT, &http_fd);
	
	NETUDP_ServerOpen(60000, &ufd);
	
	if (efd == -1 || http_fd == -1){
		printf("open efd = %d, http = %d error!!!\r\n", efd, http_fd);
		return -1;
	}	
	UEPOLL_Add(efd, http_fd);
	UEPOLL_Add(efd, ufd);
	
	http_client_init();
	
	char recv_buffer[HTTP_RECV_DATA_LEN_MAX] = {0};
	
	while(1){
		nfds = epoll_wait(efd, events, 16, 2000);  //1000ms = 1s
		if (nfds <= 0){
			if (count++ >= 7200){
				return 0;
			}
		}
		else{
			for (i = 0; i < nfds; i++){
				if (events[i].data.fd == ufd){
					memset(recv_buffer, 0, HTTP_RECV_DATA_LEN_MAX);
					ret = read(events[i].data.fd, recv_buffer, sizeof(recv_buffer)-1); 			
					printf("echo recv data = %s, len = %d\r\n", recv_buffer, ret);
					echo_server_handle(recv_buffer, ret);					
				}
				else if (events[i].data.fd == http_fd){
					struct sockaddr_in client_addr;
					socklen_t length = sizeof(client_addr);
					cfd = accept(http_fd, (struct sockaddr*)&client_addr, &length);
					if (UEPOLL_Add(efd, cfd)){
						printf("conn_fd = %d, epoll_add error\r\n", cfd);
						close(cfd);
						continue;						
					}					
				}	
				else{
					
					memset(recv_buffer, 0, HTTP_RECV_DATA_LEN_MAX);
					ret = read(events[i].data.fd, recv_buffer, HTTP_RECV_DATA_LEN_MAX); 
					if(ret <= 0){
						UEPOLL_Del(efd, events[i].data.fd);
						printf("client fd = %d ->close\r\n", events[i].data.fd);
						close(events[i].data.fd);
					}
					else{
						if(http_parse_path(events[i].data.fd, recv_buffer)) { 

						};						
					}					
				}
				
			}
		}			
	}
}


int hivideo_test(void) 
{
	char *HIVIDEO_DEV = "hivideo";
	char *HIVIDEO_INI = "hiavio.ini";

	THREAD_Init();
	POOL_CachingInit(&cp, NULL, 0);	
	AMIO_DrvLoad(&cp.in_pFactory);			
	
	char pbuf[512] = {0};
	printf("%s", AMIO_DrvEnum(pbuf, 512));
	
	//装载log系统
	//HIVIDEO_SubLogRegister();
	AMIO_SubLogRegister(HIVIDEO);
	
	
	
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
	
	
	
	
	//http_server_main(0, NULL);
	
#if 0	
	while (1)
	{
		sleep(1);
		//printf("poll..%d\r\n", cnt++);
	}	
#endif	



	http_deamon();


	
    return 0;
}










