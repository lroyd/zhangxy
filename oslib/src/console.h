#ifndef __ZXY_LOG_H__
#define __ZXY_LOG_H__

#include "typedefs.h"
#include "config_site.h"


enum {
	LOG_ERROR,
	LOG_WARNING,
	LOG_DEBUG,
	LOG_INFO,
	LOG_TRACE,
};


#define ZXY_LOG_USE_UNIX		(1)			//默认使用printf系输出(单片机使用需要注掉)
//#define ZXY_LOG_USE_STM32		(1)

#if defined(ZXY_EN_LOG_DEBUG_ON) && ZXY_EN_LOG_DEBUG_ON!=0
	#define ZXY_LOG_DEBUG_LV		(LOG_TRACE)	//<=此级别打印
	////////////////////////////////////////////////////////////
	//[!!!]static T_SubLogInfo s_tLocalLogInfo = {"m1", ZXY_LOG_DEBUG_LV, 0};
	#define ZXY_LOG_DEBUG_DYNC_ON	(1)		//动态子开关
#endif


#if defined(ZXY_LOG_USE_UNIX) && ZXY_LOG_USE_UNIX!=0
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/un.h>
#include <error.h>
#include <errno.h>

#define ULOG_UNIX_DOMAIN				("./gundoom-log.socket")
#define ULOG_FILE_PATH					("./gundoom-logs.log")
#define ULOG_FILE_MAX_SIZE				(512 * 1024)


//简易操作簇
API_DECL(void) CONSOLE_LOG_ERROR(const char* _pFormat, ...);
API_DECL(void) CONSOLE_LOG_WARNING(const char* _pFormat, ...);
API_DECL(void) CONSOLE_LOG_DEBUG(const char* _pFormat, ...);
API_DECL(void) CONSOLE_LOG_INFO(const char* _pFormat, ...);
API_DECL(void) CONSOLE_LOG_TRACE(const char* _pFormat, ...);

API_DECL(int) CONSOLE_LogMain(int argc, char *argv[]);

#elif defined(ZXY_LOG_USE_STM32) && ZXY_LOG_USE_STM32 != 0


#endif


#define ZXY_ELOG(format, arg...) CONSOLE_ErrPrint("[%s:%d:%s]: "format"\n", __FILE__, __LINE__, __FUNCTION__, ##arg)

#if defined(ZXY_EN_LOG_DEBUG_ON) && ZXY_EN_LOG_DEBUG_ON!=0
	#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
		#define ZXY_DLOG(level, format, arg...) \
			CONSOLE_DyncPrint(level, s_tLocalLogInfo.m_enLevel, s_tLocalLogInfo.m_bOn, "[%s:%d:%s]: "format"\n", __FILE__, __LINE__, __FUNCTION__, ##arg)
	#else
		#define ZXY_DLOG(level, format, arg...) CONSOLE_LogPrint(level, "[%s:%d:%s]: "format"\n", __FILE__, __LINE__, __FUNCTION__, ##arg)
	#endif
#else
	#define ZXY_DLOG(level, format, arg...)
#endif


#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
#define CONSOLE_SUBLOG_MAX	(20)
typedef struct _tagSubLogInfo{
	char *m_strName;
	char m_enLevel;	
	char m_bOn;	
}T_SubLogInfo;

API_DECL(int) CONSOLE_SubLogRegister(T_SubLogInfo *_pInfo);
API_DECL(int) CONSOLE_SubLogLookup(T_SubLogInfo *_pInfo);
API_DECL(int) CONSOLE_SubLogSetParam(int _u32Index, char _enLogLv ,char _bLogOn);
API_DECL(T_SubLogInfo*) CONSOLE_SubLogGetParam(int _u32Index);

API_DECL(void) CONSOLE_SubLogDummy(void);
API_DECL(void) CONSOLE_DyncPrint(char _u8Level, char _u8LocalLogLv, char _u8LocalLogOn, const char *format, ...);
#endif
///////////////////////////////////////////////////////////////

API_DECL(void) CONSOLE_ErrPrint(const char *format, ...);
API_DECL(void) CONSOLE_LogPrint(char level, const char* format, ...);

#endif /* __ZXY_LOG_H__ */

