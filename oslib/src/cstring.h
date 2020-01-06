#ifndef __ZXY_CSTRING_H__
#define __ZXY_CSTRING_H__

#include "typedefs.h"
#include "pool.h"


typedef struct _tag_str_t
{
    char       *ptr;
    ssize_t		slen;
}str_t;


API_DECL(str_t) c_str(char *str);	//栈分配，一般都是临时使用，永久使用需要使用c_strdup
API_DECL(const str_t*) c_cstr(str_t *str, const char *s);
API_DECL(str_t*) c_strset(str_t *str, char *ptr, size_t length);
API_DECL(str_t*) c_strset2(str_t *str, char *src);
API_DECL(str_t*) c_strset3(str_t *str, char *begin, char *end);

API_DECL(str_t*) c_strassign(str_t *dst, str_t *src );
API_DECL(str_t*) c_strcpy(str_t *dst, const str_t *src);
API_DECL(str_t*) c_strcpy2(str_t *dst, const char *src);
API_DECL(str_t*) c_strncpy(str_t *dst, const str_t *src, ssize_t max);
API_DECL(str_t*) c_strncpy_with_null(str_t *dst, const str_t *src, ssize_t max);
API_DECL(str_t*) c_strdup(T_PoolInfo *pool, str_t *dst, const str_t *src);
API_DECL(str_t*) c_strdup_with_null(T_PoolInfo *pool, str_t *dst, const str_t *src);
API_DECL(str_t*) c_strdup2(T_PoolInfo *pool, str_t *dst, const char *src);
API_DECL(str_t*) c_strdup2_with_null(T_PoolInfo *pool, str_t *dst, const char *src);
API_DECL(str_t) c_strdup3(T_PoolInfo *pool, const char *src);

API_DECL(size_t) c_strlen(const str_t *str);
API_DECL(const char*) c_strbuf(const str_t *str);

API_DECL(int) c_strcmp(const str_t *str1, const str_t *str2);
API_DECL(int) c_strcmp2(const str_t *str1, const char *str2);
API_DECL(int) c_strncmp(const str_t *str1, const str_t *str2, size_t len);
API_DECL(int) c_strncmp2(const str_t *str1, const char *str2, size_t len);
API_DECL(int) c_stricmp(const str_t *str1, const str_t *str2);


#define strnicmp_alnum	strnicmp
#define c_stricmp_alnum    c_stricmp



API_DECL(int) c_stricmp2(const str_t *str1, const char *str2);
API_DECL(int) c_strnicmp(const str_t *str1, const str_t *str2, size_t len);
API_DECL(int) c_strnicmp2(const str_t *str1, const char *str2, size_t len);

API_DECL(void) c_strcat(str_t *dst, const str_t *src);
API_DECL(void) c_strcat2(str_t *dst, const char *src);


API_DECL(char*) c_strchr(const str_t *str, int chr);

API_DECL(ssize_t) c_strspn(const str_t *str, const str_t *set_char);
API_DECL(ssize_t) c_strspn2(const str_t *str, const char *set_char);
API_DECL(ssize_t) c_strcspn(const str_t *str, const str_t *set_char);
API_DECL(ssize_t) c_strcspn2(const str_t *str, const char *set_char);
API_DECL(ssize_t) c_strtok(const str_t *str, const str_t *delim, str_t *tok, size_t start_idx);
API_DECL(ssize_t) c_strtok2(const str_t *str, const char *delim, str_t *tok, size_t start_idx);

API_DECL(char*) c_strstr(const str_t *str, const str_t *substr);
API_DECL(char*) c_stristr(const str_t *str, const str_t *substr);
API_DECL(str_t*) c_strltrim( str_t *str );
API_DECL(str_t*) c_strrtrim( str_t *str );
API_DECL(str_t*) c_strtrim( str_t *str );

API_DECL(char*) c_create_random_string(char *str, size_t length);
API_DECL(long) c_strtol(const str_t *str);
API_DECL(int) c_strtol2(const str_t *str, long *value);
API_DECL(unsigned long) c_strtoul(const str_t *str);
API_DECL(unsigned long) c_strtoul2(const str_t *str, str_t *endptr, unsigned base);
API_DECL(int) c_strtoul3(const str_t *str, unsigned long *value, unsigned base);
API_DECL(float) c_strtof(const str_t *str);
API_DECL(int) c_utoa(unsigned long val, char *buf);
API_DECL(int) c_utoa_pad( unsigned long val, char *buf, int min_dig, int pad);

typedef struct _tag_match_t
{
    char	*pattern;	//待匹配的模板
    char	num;		//匹配模板中的个数
}match_t;

API_DECL(int) c_parse_number(char *str, match_t *table, int *out);

#endif	/* __ZXY_CSTRING_H__ */

