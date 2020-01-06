#ifndef __ZXY_JSON_H__
#define __ZXY_JSON_H__

#include "typedefs.h"
#include "pool.h"
#include "cstring.h"
#include "config_site.h"
#include "dl_list.h"


#if defined(ZXY_HAS_JSON) && ZXY_HAS_JSON!=0


enum 
{
    JSON_VAL_NULL,
    JSON_VAL_BOOL,
    JSON_VAL_NUMBER,
    JSON_VAL_STRING,
    JSON_VAL_ARRAY,
    JSON_VAL_OBJ
};

typedef struct _tagJsonElem T_JsonElem;

typedef struct _tagJsonList
{
    T_DoubleList	list;
}T_JsonList;

struct _tagJsonElem
{
	T_DoubleList	list;
    str_t			name;
    int				type;
    union
    {
		char		is_true;
		float		num;
		str_t	str;
		T_JsonList	children;
    } value;
};

typedef struct _tagJsonErrInfo
{
    unsigned	line;
    unsigned	col;
    int			err_char;
}T_JsonErrInfo;



typedef int (*JSON_WRITER)(const char *s, unsigned size, void *user_data);

API_DEF(void) 	JSON_ElemSetNull(T_JsonElem *el, str_t *name);
API_DEF(void) 	JSON_ElemSetBool(T_JsonElem *el, str_t *name, char val);
API_DEF(void) 	JSON_ElemSetNumber(T_JsonElem *el, str_t *name, float val);
API_DEF(void) 	JSON_ElemSetString(T_JsonElem *el, str_t *name, str_t *val);
API_DEF(void) 	JSON_ElemSetArray(T_JsonElem *el, str_t *name);
API_DEF(void) 	JSON_ElemSetObj(T_JsonElem *el, str_t *name);
API_DEF(void) 	JSON_ElemAdd(T_JsonElem *el, T_JsonElem *child);
API_DEF(int)	JSON_ElemWrite(const T_JsonElem *elem, char *buffer, unsigned *size);
API_DEF(int)   	JSON_ElemWritef(const T_JsonElem *elem, JSON_WRITER writer, void *user_data);

API_DEF(T_JsonElem*) JSON_Parse(T_PoolInfo *pool, char *buffer, unsigned *size, T_JsonErrInfo *err_info);


#endif

#endif	/* __ZXY_JSON_H__ */
