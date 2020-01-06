/*******************************************************************************
	> File Name: .h
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "console.h"

/*******************************************************************************
*	
********************************************************************************/
#define MAX_EPEVENT_SIZE				(4)

typedef struct _tagLogInfo{
	char		emLevel;
	const char	*pucPrefix;
} T_LogName;

#if defined(ZXY_EN_LOG_COLOUR) && ZXY_EN_LOG_COLOUR!=0
static T_LogName g_tLogNameTable[] = {
	{LOG_ERROR     , "\033[1;31;40mERROR\033[m"},	//大红
	{LOG_WARNING   , "\033[1;35;40mWARN\033[m "},	//洋红
	{LOG_DEBUG     , "\033[1;36;40mDEBUG\033[m"},	//青色
	{LOG_INFO      , "\033[1;32;40mINFO\033[m "},	//绿色
	{LOG_TRACE     , "TRACE"},
};
#else
static T_LogName g_tLogNameTable[] = {
	{LOG_ERROR     , "ERROR"},	//大红
	{LOG_WARNING   , "WARN "},	//洋红
	{LOG_DEBUG     , "DEBUG"},	//青色
	{LOG_INFO      , "INFO "},	//绿色
	{LOG_TRACE     , "TRACE"},
};
#endif

#if defined(ZXY_LOG_USE_UNIX) && ZXY_LOG_USE_UNIX!=0
static pthread_mutex_t g_tLogLock = PTHREAD_MUTEX_INITIALIZER;
static int g_tLogClientFd = -1;
static struct sockaddr_un g_tLogCAddr;

typedef struct _tagUlog{
	FILE *file;
	int offset;
	char path[128];
} T_LogInfo;

/*******************************************************************************
*		
********************************************************************************/
//前景色:30（黑色）、31（红色）、32（绿色）、 33（黄色）、34（蓝色）、35（洋红）、36（青色）、37（白色）

#define LOG_BUFFER_SIZE_MAX					(256)
static char buf_time[48];
static char *getLogTime()
{
    struct timeval time;
    struct tm *tmp;

    gettimeofday(&time, NULL);
    tmp = localtime(&time.tv_sec);
	if (tmp)
	{
#if defined(ZXY_EN_LOG_COLOUR) && ZXY_EN_LOG_COLOUR!=0		
		sprintf(buf_time, "[\033[1;33;40m%04d-%02d-%02d %02d:%02d:%02d.%03d\033[m]", 
							tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday,
							tmp->tm_hour, tmp->tm_min, tmp->tm_sec, (int)(time.tv_usec/1000));	
#else
		sprintf(buf_time, "[%04d-%02d-%02d %02d:%02d:%02d.%03d]", 
							tmp->tm_year + 1900, tmp->tm_mon + 1, tmp->tm_mday,
							tmp->tm_hour, tmp->tm_min, tmp->tm_sec, (int)(time.tv_usec/1000));	
#endif	
		return buf_time;
	}
	else
	{
		return NULL;
	}
}

/*******************************************************************************
* Name: 
* Descriptions: 
* Parameter:	
* Return:	
*******************************************************************************/
void CONSOLE_LOG_ERROR(const char* _pFormat, ...)
{
	char aucBuf[LOG_BUFFER_SIZE_MAX] = {0};
	va_list argp;
    va_start(argp, _pFormat);
    vsprintf(aucBuf, _pFormat, argp);
    va_end(argp);	
	fprintf(stdout, "%s [%s] %s\n", getLogTime(), g_tLogNameTable[LOG_ERROR].pucPrefix, aucBuf); //31（红色）
}

void CONSOLE_LOG_WARNING(const char* _pFormat, ...)
{
	char aucBuf[LOG_BUFFER_SIZE_MAX] = {0};
	va_list argp;
    va_start(argp, _pFormat);
    vsprintf(aucBuf, _pFormat, argp);
    va_end(argp);
	fprintf(stdout, "%s [%s] %s\n", getLogTime(), g_tLogNameTable[LOG_WARNING].pucPrefix, aucBuf); //35（洋红）
}

void CONSOLE_LOG_DEBUG(const char* _pFormat, ...)
{
	char aucBuf[LOG_BUFFER_SIZE_MAX] = {0};
	va_list argp;
    va_start(argp, _pFormat);
    vsprintf(aucBuf, _pFormat, argp);
    va_end(argp);	
	fprintf(stdout, "%s [%s] %s\n", getLogTime(), g_tLogNameTable[LOG_DEBUG].pucPrefix, aucBuf); //36（青色）
}

void CONSOLE_LOG_INFO(const char* _pFormat, ...)
{
	char aucBuf[LOG_BUFFER_SIZE_MAX] = {0};
	va_list argp;
    va_start(argp, _pFormat);
    vsprintf(aucBuf, _pFormat, argp);
    va_end(argp);	
	fprintf(stdout, "%s [%s] %s\n", getLogTime(), g_tLogNameTable[LOG_INFO].pucPrefix, aucBuf); 
}

void CONSOLE_LOG_TRACE(const char* _pFormat, ...)
{
	char aucBuf[LOG_BUFFER_SIZE_MAX] = {0};
	va_list argp;
    va_start(argp, _pFormat);
    vsprintf(aucBuf, _pFormat, argp);
    va_end(argp);	
	fprintf(stdout, "%s [%s] %s\n", getLogTime(), g_tLogNameTable[LOG_TRACE].pucPrefix, aucBuf); 
}

/*******************************************************************************
* Name: 
* Descriptions: 
* Parameter:	
* Return:	
*******************************************************************************/
static unsigned long getFileSize(const char *_pCPath) 
{
	unsigned long u64FileSize = -1;
	struct stat statbuff;
	if(stat(_pCPath, &statbuff) < 0)
	{
		return u64FileSize;
	}
	else
	{
		u64FileSize = statbuff.st_size;
	}
	return u64FileSize;
}

static int openLogFile(T_LogInfo *_pThis, const char *_pCPath) 
{
	long s64Offset = 0;
	FILE *pFile;
	strncpy(_pThis->path, _pCPath, sizeof(_pThis->path)-1);
	if(0 == access(_pCPath, 0)) 
	{
		s64Offset = getFileSize(_pCPath);
		if(s64Offset >= ULOG_FILE_MAX_SIZE) 
		{
			s64Offset = 0;
			char aucNewFile[128] = "";
			sprintf(aucNewFile, "%s.0", _pCPath);
			if(-1 == rename(_pCPath, aucNewFile)) 
			{
				perror("-1 == rename");
			}
			pFile = fopen(_pCPath, "w"); ///< cannot fseek
		} 
		else 
		{
			pFile = fopen(_pCPath, "a"); ///< cannot fseek
		}
	} 
	else 
	{
		pFile = fopen(_pCPath, "w"); ///< can fseek
	}
	if(NULL == pFile) 
	{
		perror("log_file = NULL");
		return -1;
	}
	_pThis->offset = s64Offset;
	_pThis->file = pFile;
	printf("create log file %s %ld\n", _pCPath, s64Offset);
	return 0;
}

/*******************************************************************************
* Name: 
* Descriptions: 
* Parameter:	
* Return:	
*******************************************************************************/
static void serverLogPrint(T_LogInfo *_pThis, int _u8Level, const char* format, ...)
{
	int s32Ret = 0, u32Len = 0;
	char aucBuffer[1024];
	s32Ret = sprintf(aucBuffer, "%s ", getLogTime());
	va_list list;
	va_start(list, format);
	if(s32Ret < 0) s32Ret = 0;
	u32Len += s32Ret;
	s32Ret = vsnprintf(aucBuffer + u32Len, sizeof(aucBuffer) - u32Len - 1, format, list);
	va_end(list);
	if(s32Ret <= 0) 
		return;
	
	u32Len += s32Ret;
	if('@' < _u8Level && _u8Level < 'G') {_u8Level -= '@';}	//'A'-'G'
	else if('0' < _u8Level && _u8Level < ':') {_u8Level -= '0';} //'0'-'9'
	else {_u8Level = 0xF;}
	
	fprintf(stdout, "%s", aucBuffer);
	if(_u8Level <= LOG_INFO && NULL != _pThis->file) 
	{ 
		if(_pThis->offset + u32Len > ULOG_FILE_MAX_SIZE) //超出当前文件最大长度,新建一个文件
		{
			fclose(_pThis->file);
			char aucNewFile[128] = "";
			sprintf(aucNewFile, "%s.0", _pThis->path);	//直接覆盖

			if(-1 == rename(_pThis->path, aucNewFile)) 
			{
				perror("-1 == rename");
			}
			_pThis->offset = 0;
			_pThis->file = fopen(_pThis->path, "w"); ///< cannot fseek
			if(NULL == _pThis->file) 
			{
				perror("NULL == _pThis->file");
			}
		}
		
		if(_pThis->offset + u32Len <= ULOG_FILE_MAX_SIZE) 
		{
			fprintf(_pThis->file, "%s", aucBuffer);
			/* commend tmpfs */
			fflush(_pThis->file);
			_pThis->offset += u32Len;
		}
	}
}

/*******************************************************************************
* Name: 
* Descriptions: 
* Parameter:	
* Return:	
*******************************************************************************/
int CONSOLE_LogMain(int argc, char *argv[]) 
{
	int localSfd, s32Ret;
	struct sockaddr_un c_addr;
	struct sockaddr_un s_addr;
	char aucBuffer[1024];

	int s32Efd, getEvtCnt, IndexEvt, addrlen = sizeof(struct sockaddr_un);
	int waitEvtTimeout = 1000;
	struct epoll_event ev;
	struct epoll_event events[MAX_EPEVENT_SIZE];
	T_LogInfo *pThis = (T_LogInfo *)malloc(sizeof(T_LogInfo));
	if(NULL == pThis) 
	{
		perror("malloc T_LogInfo fail");
		return -1;
	}
	
	if(-1 == openLogFile(pThis, ULOG_FILE_PATH)) 
	{
		perror("openLogFile create fail");
		return -1;
	}

	if(-1 == (s32Efd = epoll_create(MAX_EPEVENT_SIZE)))
	{
		perror("epoll_create fail");
		return -1;
	}

	localSfd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if(localSfd < 0) 
	{  
		perror("localSfd < 0 socket create fail");
		return -1;
	}

	s_addr.sun_family = AF_UNIX;
	strncpy(s_addr.sun_path, ULOG_UNIX_DOMAIN, sizeof(s_addr.sun_path) - 1);
	unlink(ULOG_UNIX_DOMAIN);

	s32Ret = bind(localSfd, (struct sockaddr*)&s_addr, sizeof(s_addr));
	if(-1 == s32Ret) 
	{  
		perror("s32Ret = bind fail");
		close(localSfd);
		unlink(ULOG_UNIX_DOMAIN);
		return -1;
	}

	ev.data.fd = localSfd, ev.events = (EPOLLIN/*|EPOLLET*/);
	if(epoll_ctl(s32Efd, EPOLL_CTL_ADD, localSfd, &ev) == -1)
	{
		perror("localSfd epoll_ctl fail");
		close(localSfd);
		unlink(ULOG_UNIX_DOMAIN);
		return -1;
	}

	while (1) 
	{
		if((getEvtCnt = epoll_wait(s32Efd, events, MAX_EPEVENT_SIZE, waitEvtTimeout)) == -1)
		{
			perror("epoll_wait error");
			continue;
		}
		for(IndexEvt = 0; IndexEvt < getEvtCnt; ++IndexEvt) 
		{
			if(events[IndexEvt].events & EPOLLIN) 
			{
				if(localSfd == events[IndexEvt].data.fd) 
				{
					s32Ret = recvfrom(events[IndexEvt].data.fd, aucBuffer, sizeof(aucBuffer) - 1, 0, 
									(struct sockaddr *)&c_addr, (socklen_t *)&addrlen); 
					if(s32Ret >= 0) 
					{
						aucBuffer[s32Ret] = '\0';
						serverLogPrint(pThis, aucBuffer[0], "%s", aucBuffer + 1);
					}
				}
			} 
			else 
			{
				printf("events %d", events[IndexEvt].events);
			}
		}
	}
	close(localSfd);
	unlink(ULOG_UNIX_DOMAIN);
	return 0;
}

#endif
/*******************************************************************************
* Name: 
* Descriptions: 
* Parameter:	
* Return:	
*******************************************************************************/
static int clientLogPrint(char *_pData, int _u32Len) 
{	
	int s32Ret, s32Cfd;
	if(NULL == _pData) 
		return -1;
	
	if(_u32Len <= 0) 
		_u32Len = strlen(_pData);

#if defined(ZXY_LOG_USE_UNIX) && ZXY_LOG_USE_UNIX!=0	
	if(0 != pthread_mutex_lock(&g_tLogLock)) 
	{
		perror("log mutex lock");
		return -1;
	}
	s32Cfd = g_tLogClientFd;
	if(s32Cfd < 0) 
	{
    	s32Cfd = socket(PF_UNIX, SOCK_DGRAM, 0);
		if(s32Cfd < 0) 
		{  
        	perror("cfd = socket(PF_UNIX, SOCK_STREAM, 0)");
			pthread_mutex_unlock(&g_tLogLock);
        	return -1;
    	}
    	g_tLogCAddr.sun_family = AF_UNIX;
    	strncpy(g_tLogCAddr.sun_path, ULOG_UNIX_DOMAIN, sizeof(g_tLogCAddr.sun_path) - 1);
		g_tLogClientFd = s32Cfd;
	}

    s32Ret = sendto(s32Cfd, _pData, _u32Len, 0, (struct sockaddr *)&g_tLogCAddr, sizeof(g_tLogCAddr));
	if(s32Ret < 0) 
	{
		fprintf(stdout, "%s %s", getLogTime(), _pData + 1);
	}
	pthread_mutex_unlock(&g_tLogLock);
#elif defined(ZXY_LOG_USE_STM32) && ZXY_LOG_USE_STM32 != 0


#endif	
    return 0;
}



/*******************************************************************************
* Name: 
* Descriptions: 
* Parameter:	
* Return:	
*******************************************************************************/
void CONSOLE_ErrPrint(const char *format, ...) 
{
	int s32Ret = 0, u32Len = 0;
	char aucBuffer[1024];
	
	s32Ret = sprintf(aucBuffer, "%X[%s] ", LOG_ERROR & 0xF, g_tLogNameTable[LOG_ERROR].pucPrefix);
	
	va_list list;
	va_start(list, format);
	if(s32Ret < 0) 
		s32Ret = 0;
	
	u32Len += s32Ret;
	s32Ret = vsnprintf(aucBuffer + u32Len, sizeof(aucBuffer) - u32Len - 1, format, list);
	va_end(list);
	if(s32Ret <= 0) 
		return;
	
	u32Len += s32Ret;
	
	clientLogPrint(aucBuffer, u32Len);
}

void CONSOLE_LogPrint(char _u8Level, const char *format, ...) 
{
#if defined(ZXY_EN_LOG_DEBUG_ON) && ZXY_EN_LOG_DEBUG_ON!=0	
	int s32Ret = 0, u32Len = 0;
	char aucBuffer[1024];

	if (_u8Level > ZXY_LOG_DEBUG_LV) return;
	
	s32Ret = sprintf(aucBuffer, "%X[%s] ", _u8Level & 0xF, g_tLogNameTable[(unsigned char)_u8Level].pucPrefix);
	
	va_list list;
	va_start(list, format);
	if(s32Ret < 0) 
		s32Ret = 0;
	
	u32Len += s32Ret;
	s32Ret = vsnprintf(aucBuffer + u32Len, sizeof(aucBuffer) - u32Len - 1, format, list);
	va_end(list);
	if(s32Ret <= 0) 
		return;
	
	u32Len += s32Ret;
	
	clientLogPrint(aucBuffer, u32Len);
#else
	return;
#endif	
}


#if defined(ZXY_LOG_DEBUG_DYNC_ON) && ZXY_LOG_DEBUG_DYNC_ON!=0
typedef int (*DYNAMIC_LOG)(char *, int);

static int clientLogNull(char *_pData, int _u32Len)
{
	return 0;
}


DYNAMIC_LOG	DyncLogTable[2] = {
	clientLogNull,
	clientLogPrint,
};

void CONSOLE_DyncPrint(char _u8Level, char _u8LocalLevel, char u8flags, const char *format, ...) 
{	
	int s32Ret = 0, u32Len = 0;
	char aucBuffer[1024];

	if (_u8Level > _u8LocalLevel) return;
	s32Ret = sprintf(aucBuffer, "%X[%s] ", _u8Level & 0xF, g_tLogNameTable[(unsigned char)_u8Level].pucPrefix);
	
	va_list list;
	va_start(list, format);
	if(s32Ret < 0) s32Ret = 0;
	
	u32Len += s32Ret;
	s32Ret = vsnprintf(aucBuffer + u32Len, sizeof(aucBuffer) - u32Len - 1, format, list);
	va_end(list);
	if(s32Ret <= 0) return;
	u32Len += s32Ret;
	
	DyncLogTable[u8flags](aucBuffer, u32Len);
}


static T_SubLogInfo	*s_aptSubLogTable[CONSOLE_SUBLOG_MAX] = {0};

static int lookup(T_SubLogInfo *_pInfo)
{
	int i, u32Ret = -1;
	//查找是否有同名的
	for (i = 0; i < CONSOLE_SUBLOG_MAX; i++){
		if (s_aptSubLogTable[i] && s_aptSubLogTable[i] == _pInfo){
			u32Ret = i;
			goto EXIT;
		}
	}
	//查找第一个空位置
	for (i = 0; i < CONSOLE_SUBLOG_MAX; i++){
		if (s_aptSubLogTable[i] == NULL){
			u32Ret = i;
			goto EXIT;
		}
	}
EXIT:	
	return u32Ret;
}

int CONSOLE_SubLogLookup(T_SubLogInfo *_pInfo)
{	
	return lookup(_pInfo);
}

int CONSOLE_SubLogRegister(T_SubLogInfo *_pInfo)
{
	int u32Ret = -1;
	if ((u32Ret = lookup(_pInfo)) != -1){
		s_aptSubLogTable[u32Ret] = _pInfo;
	}
	
	return u32Ret;
}

int CONSOLE_SubLogSetParam(int _u32Index, char _enLogLv ,char _bLogOn)
{
	T_SubLogInfo *pInfo	= s_aptSubLogTable[_u32Index];
	pInfo->m_enLevel	= _enLogLv;
	pInfo->m_bOn		= _bLogOn;
	return 0;
}

T_SubLogInfo *CONSOLE_SubLogGetParam(int _u32Index)
{
	return s_aptSubLogTable[_u32Index];
}

void CONSOLE_SubLogDummy(void)
{
	int i;
	
	printf("statistics registered modules:\r\n");
	printf("     ID      NAME    LEVEL   ON/OFF\r\n");
	printf("    -------------------------------\r\n");
	for (i = 0; i < CONSOLE_SUBLOG_MAX; i++){
		T_SubLogInfo *pInfo = s_aptSubLogTable[i];
		if (pInfo){
			printf("    |%02d|%8s|%8s|%8s|\r\n", i, pInfo->m_strName, g_tLogNameTable[pInfo->m_enLevel].pucPrefix, pInfo->m_bOn?"ON":"OFF");
		}
	} 
	printf("    -------------------------------\r\n");
	
}

#endif















