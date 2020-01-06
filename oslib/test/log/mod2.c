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

#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
static T_SubLogInfo s_tLocalLogInfo = {"m2", ZXY_LOG_DEBUG_LV, 0};
#endif
static int cnt = 0;


static void *mod2_handle(void *a)
{
	
	while(1)
	{
		//ZXY_ELOG("mod2 = %d", cnt);
		ZXY_DLOG(LOG_WARNING, "mod2 = %d", cnt);
		ZXY_DLOG(LOG_DEBUG,   "mod2 = %d", cnt);
		ZXY_DLOG(LOG_INFO,    "mod2 = %d", cnt);
		ZXY_DLOG(LOG_TRACE,   "mod2 = %d", cnt);		
		cnt++;
		sleep(2);
	}
}


int mod2_init(void)
{

	int log_id = -1;
#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0	
	log_id = CONSOLE_SubLogRegister(&s_tLocalLogInfo);
#endif
	printf("register mod2 = %d\r\n", log_id);
	
	pthread_create(&tid, 0, mod2_handle, NULL);

	return log_id;
}








