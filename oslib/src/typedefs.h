#ifndef __ZXY_TYPEDEFS_H__
#define __ZXY_TYPEDEFS_H__

#include <stdio.h>
#include <assert.h>

#if defined(__cplusplus)
	#define API_DECL(type)	type
#else
	#define API_DECL(type)	extern type
#endif


#define API_DEF(type)	type

//定义额外检查
#define ZXY_ENABLE_EXTRA_CHECK	(1)		//系统控制宏


#if defined(ZXY_ENABLE_EXTRA_CHECK) && ZXY_ENABLE_EXTRA_CHECK != 0
	#define ASSERT_RETURN(expr, retval)    do {if (!(expr)) {return retval;}} while (0)
#else
	#define ASSERT_RETURN(expr, retval)    assert(expr)
#endif


#if defined(ZXY_ENABLE_EXTRA_CHECK) && ZXY_ENABLE_EXTRA_CHECK != 0
#   define ASSERT_ON_FAIL(expr, exec_on_fail)   do {assert(expr); if (!(expr)) exec_on_fail; } while (0)
#else
#   define ASSERT_ON_FAIL(expr, exec_on_fail)   assert(expr)
#endif

#define ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))





#endif /* __ZXY_TYPEDEFS_H__ */