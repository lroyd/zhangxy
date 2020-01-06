#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "console.h"

static pthread_t tid;

static int cnt = 0;

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"m1", ZXY_LOG_DEBUG_LV, 0};
#endif


static void *mod1_handle(void *a)
{
	
	while(1)
	{
		//ZXY_ELOG("mod1 = %d", cnt);
		ZXY_DLOG(LOG_WARNING, "mod1 = %d", cnt);
		ZXY_DLOG(LOG_DEBUG,   "mod1 = %d", cnt);
		ZXY_DLOG(LOG_INFO,    "mod1 = %d", cnt);
		ZXY_DLOG(LOG_TRACE,   "mod1 = %d", cnt);		
		cnt++;
		sleep(2);
	}
}


int mod1_init(void)
{

	int log_id = -1;
#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0	
	log_id = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
#endif	
	printf("register mod1 = %d\r\n", log_id);
	
	pthread_create(&tid, 0, mod1_handle, NULL);

	return log_id;
}








