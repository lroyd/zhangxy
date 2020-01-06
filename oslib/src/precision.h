#ifndef __ZXY_HIGH_PRECISION_H__
#define __ZXY_HIGH_PRECISION_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>




/************************************************************/
#if defined(ZXY_HAS_TIME_FLOATING) && ZXY_HAS_TIME_FLOATING != 0
	//使用浮点运算
	#include <math.h>
	typedef double highprec_t;

	#define HIGHPREC_VALUE_IS_ZERO(a)     (a==0)
	#define HIGHPREC_MOD(a,b)             (a=fmod(a,b))

#elif defined(ZXY_HAS_TIME_INT64) && ZXY_HAS_TIME_INT64 != 0
	//使用64bit
	typedef int64_t highprec_t;

#else
	#warning "high precision math is not available"
	//回退使用32bit
    typedef int32_t highprec_t;

#endif


#ifndef HIGHPREC_MUL
#   define HIGHPREC_MUL(a1,a2)   (a1 = a1 * a2)
#endif

#ifndef HIGHPREC_DIV
#   define HIGHPREC_DIV(a1,a2)   (a1 = a1 / a2)
#endif

#ifndef HIGHPREC_MOD
#   define HIGHPREC_MOD(a1,a2)   (a1 = a1 % a2)
#endif


#ifndef HIGHPREC_VALUE_IS_ZERO
#   define HIGHPREC_VALUE_IS_ZERO(a)     (a==0)
#endif


#endif

