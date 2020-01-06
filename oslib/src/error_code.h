#ifndef __ZXY_ERROR_CODE_H__
#define __ZXY_ERROR_CODE_H__


#define EO_SUCCESS			(0)

#define ERRNO_START_STATUS	(4000)

#define EO_FAILED	    	(ERRNO_START_STATUS + 0)	//调用错误
#define EO_UNKNOWN	    	(ERRNO_START_STATUS + 1)	//未知错误
#define EO_PENDING	    	(ERRNO_START_STATUS + 2)	
#define EO_TOOMANYCONN	    (ERRNO_START_STATUS + 3)	
#define EO_INVAL	    	(ERRNO_START_STATUS + 4)	//参数无效
#define EO_NAMETOOLONG	    (ERRNO_START_STATUS + 5)	
#define EO_NOTFOUND	    	(ERRNO_START_STATUS + 6)	
#define EO_NOMEM	    	(ERRNO_START_STATUS + 7)	//内存不足分配失败
#define EO_BUG             	(ERRNO_START_STATUS + 8)	
#define EO_TIMEDOUT        	(ERRNO_START_STATUS + 9)	
#define EO_TOOMANY         	(ERRNO_START_STATUS + 10)
#define EO_BUSY            	(ERRNO_START_STATUS + 11)
#define EO_NOTSUP	    	(ERRNO_START_STATUS + 12)
#define EO_INVALIDOP	    (ERRNO_START_STATUS + 13)
#define EO_CANCELLED	    (ERRNO_START_STATUS + 14)
#define EO_EXISTS          	(ERRNO_START_STATUS + 15)
#define EO_EOF		    	(ERRNO_START_STATUS + 16)
#define EO_TOOBIG	    	(ERRNO_START_STATUS + 17)
#define EO_RESOLVE	    	(ERRNO_START_STATUS + 18)
#define EO_TOOSMALL	    	(ERRNO_START_STATUS + 19)
#define EO_IGNORED	    	(ERRNO_START_STATUS + 20)
#define EO_IPV6NOTSUP	    (ERRNO_START_STATUS + 21)
#define EO_AFNOTSUP	    	(ERRNO_START_STATUS + 22)
#define EO_GONE	    		(ERRNO_START_STATUS + 23)
#define EO_SOCKETSTOP	    (ERRNO_START_STATUS + 24)
#define EO_INJSON			(ERRNO_START_STATUS + 25)
#define EO_FORMAT			(ERRNO_START_STATUS + 26)	//解析格式错误
#define EO_HARDWARE			(ERRNO_START_STATUS + 27)	//底层硬件错误

#define MEDIA_ERRNO_START_STATUS	(5000)
#define EO_INIPARSE	    	(MEDIA_ERRNO_START_STATUS + 0)	//配置文件解析错误





#endif /* __ZXY_ERROR_CODE_H__ */