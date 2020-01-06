#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "uepoll.h"
#include "unet.h"
#include "console.h"

#include "mod1.h"
#include "mod2.h"

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"test", ZXY_LOG_DEBUG_LV, 0};
#endif
static int cnt = 0;



////
static int m1_log = -1;
static int m2_log = -1;

static int tcp_server_handle(char *p_data, int len)
{
	char addr = 1;
	char param_buf[12] = {0};
	printf("recv %s\r\n",p_data);
#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0	
	if (strstr(p_data, "on"))
	{
		s_tLocalLogInfo.m_bOn = 1;
	}
	else if (strstr(p_data, "off"))
	{
		s_tLocalLogInfo.m_bOn = 0;
	}
	else if (strstr(p_data, "info"))
	{
		s_tLocalLogInfo.m_enLevel = LOG_INFO;
	}	
	else if (strstr(p_data, "debug"))
	{
		s_tLocalLogInfo.m_enLevel = LOG_DEBUG;
	}
	else if (strstr(p_data, "trace"))
	{
		s_tLocalLogInfo.m_enLevel = LOG_TRACE;
	}	
	
	else if (strstr(p_data, "1 2 0"))
	{
		CONSOLE_SubLogSetParam(m1_log, LOG_DEBUG, 0);
	}	
	else if (strstr(p_data, "1 2 1"))
	{
		CONSOLE_SubLogSetParam(m1_log, LOG_DEBUG, 1);
	}	
	else if (strstr(p_data, "1 3 0"))
	{
		CONSOLE_SubLogSetParam(m1_log, LOG_INFO, 0);
	}	
	else if (strstr(p_data, "1 3 1"))
	{
		CONSOLE_SubLogSetParam(m1_log, LOG_INFO, 1);
	}
	
	else if (strstr(p_data, "2 4 0"))
	{
		CONSOLE_SubLogSetParam(m2_log, LOG_TRACE, 0);
	}	
	else if (strstr(p_data, "2 4 1"))
	{
		CONSOLE_SubLogSetParam(m2_log, LOG_TRACE, 1);
	}
#endif	
	
}
	
int log_test(void)
{
	int ret = -1, efd, tfd, cfd;
	int nfds, i;
	struct epoll_event	events[EPOLL_EVENT_NUM_MAX];  

	
	UEPOLL_Create(&efd);
	

	//3.TCP服务初始化
	NETTCP_ServerOpen(10087, &tfd);	//10087
	

	if (efd == -1 || tfd == -1)
	{
		printf("open efd = %d, tfd = %d error!!!\r\n", efd, tfd);
		return -1;
	}
	
	//4.加入监听
	UEPOLL_Add(efd, tfd);
	
	m1_log = mod1_init();
	m2_log = mod2_init();
	
	//CONSOLE_SubLogDummy();
	while(1)
	{
		nfds = epoll_wait(efd, events, EPOLL_EVENT_NUM_MAX, 3000);  
		if (nfds <= 0)  
		{
			//printf("epoll_wait timeout...\r\n");
			//ZXY_ELOG("epoll_wait timeout = %d", cnt);
			//ZXY_DLOG(LOG_WARNING, "epoll_wait timeout = %d", cnt);
			//ZXY_DLOG(LOG_DEBUG,   "epoll_wait timeout = %d", cnt);
			//ZXY_DLOG(LOG_INFO,    "epoll_wait timeout = %d", cnt);
			//ZXY_DLOG(LOG_TRACE,   "epoll_wait timeout = %d", cnt);
			
			//cnt++;
#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0			
			CONSOLE_SubLogDummy();
#endif			
		}
		else
		{
			for (i = 0; i < nfds; i++)  
			{
				if (events[i].data.fd == tfd)
				{
					//tcp 接收客户端连接
					struct sockaddr_in client_addr;
					socklen_t length = sizeof(client_addr);
        
					cfd = accept(tfd, (struct sockaddr*)&client_addr, &length);
					if (UEPOLL_Add(efd, cfd))
					{
						printf("conn_fd = %d, epoll_add error\r\n", cfd);
						close(cfd);
						continue;						
					}
					//printf("cfd = %d connect ok\r\n", cfd);
				}
				else
				{
					//tcp 客户端数据
					char buf[64] = {0};
					ret = read(events[i].data.fd, buf, sizeof(buf)-1); 
					if(ret <= 0)
					{
						UEPOLL_Del(efd, events[i].data.fd);
						close(events[i].data.fd);
						
						//printf("client fd close\r\n");
					}
					else
					{
						buf[ret] = 0;
						//printf("cli data = %s\r\n", buf);
						//解析数据
						tcp_server_handle(buf, ret);
					}					
				}
			}
		}
	}	
	return 0;
}









