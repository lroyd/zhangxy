#ifndef __ZXY_LIMITS_H__
#define __ZXY_LIMITS_H__

#include "config_site.h"


#if defined(ZXY_HAS_LIMITS_H) && ZXY_HAS_LIMITS_H != 0
#	include <limits.h>
#else

#ifdef _MSC_VER
	#pragma message("limits.h is not found or not supported. LONG_MIN and LONG_MAX will be defined by the library in limits.h and overridable in config_site.h")
#else
	#warning "limits.h is not found or not supported. LONG_MIN and LONG_MAX will be defined by the library in climits.h and overridable in config_site.h"
#endif


#  ifndef LONG_MAX
#    if __WORDSIZE == 64
#      define LONG_MAX		(9223372036854775807L)
#    else
#      define LONG_MAX		(2147483647L)
#    endif
#  endif

#  ifndef LONG_MIN
#    define LONG_MIN		(-LONG_MAX - 1L)
#  endif


#  ifndef ULONG_MAX
#    if __WORDSIZE == 64
#      define ULONG_MAX		(18446744073709551615UL)
#    else    
#      define ULONG_MAX		(4294967295UL)
#    endif
#  endif
#endif


#define C_MAXINT32			(0x7fffffff)
#define C_MININT32			(0x80000000)
#define C_MAXUINT16			(0xffff)
#define C_MAXUINT8			(0xff)
#define C_MAXLONG			LONG_MAX
#define C_MINLONG			LONG_MIN
#define C_MAXULONG			ULONG_MAX


#endif /* __ZXY_LIMITS_H__ */