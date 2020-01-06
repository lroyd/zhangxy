#ifndef __ZXY_CONFIG_H__
#define __ZXY_CONFIG_H__
/*******************************************************************************
	> File Name: .h
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
*******************************************************************************/
////////////////////////////////////////////////////////////////////
#define ZXY_HAS_CTYPE_H					(0)		//使用ctype.h，默认不使用
#define ZXY_HAS_LIMITS_H				(0)		//使用limits.h，默认不使用

#define ZXY_HAS_JSON					(1)		//默认使用






//基础库thread(info)/mutex(debug)
#define ZXY_EN_THREAD_CHECK_STACK		(1)		//+++配置项
#define ZXY_EN_THREAD_SET_STACK_SIZE	(1)		//+++配置项,会失败？
#define ZXY_EN_THREAD_ALLOCATE_STACK	(0)		//使用pool分配线程栈

#define ZXY_SET_THREAD_DEF_STACK_SIZE	(8192)

#define ZXY_EN_MUTEX_DEBUG	(0)	//mutex的详细log,一般不开放

#define ZXY_HAS_ATOMIC		(1)	

////////////////////////////////////////////////////////////////////
//timestamp
#define ZXY_HAS_TIME_LITTLE_ENDIAN		(1)		//默认不需要设置

#define	ZXY_HAS_TIME_INT64				(1)		//与ZXY_HAS_TIME_FLOATING互斥
#define ZXY_HAS_TIME_FLOATING			(0)		//与ZXY_HAS_TIME_INT64互斥

//timer
#define ZXY_EN_TIMER_DEBUG				(0)		//dump输出file，line
////////////////////////////////////////////////////////////////////
//log
#define ZXY_EN_LOG_COLOUR	(1)			//开启颜色
#define ZXY_EN_LOG_DEBUG_ON	(1)			//[k]开启log输出和动态开关，默认只输出error



////////////////////////////////////////////////////////////////////
//sock

#define ZXY_HAS_SOCK_INET_ATON	    	(1)		//默认1，使用inet_aton，而不是inet_addr(会认为255.255.255.255无效)
#define ZXY_HAS_SOCK_INET_PTON	    	(1)

#define	ZXY_HAS_SOCK_IPV6				(0)		//默认0
#define	ZXY_HAS_SOCK_IPV6_V6ONLY		(0)		//默认0

#define ZXY_HAS_SOCK_GETADDRINFO		(0)		//gethostbyname和gethostbyaddr这两个函数仅仅支持IPv4，使用新的函数getaddrinfo



////////////////////////////////////////////////////////////////////

#endif