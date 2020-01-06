#include <stdio.h>
#include <string.h>

#include "ctype.h"
#include "climits.h"
#include "cstring.h"

#define CHECK_STACK()			//不使用CHECK_STACK

//只能是const类型的
const str_t *c_cstr(str_t *str, const char *s)
{
    str->ptr = (char*)s;
    str->slen = s ? (ssize_t)strlen(s) : 0;
    return str;
}

str_t *c_strset(str_t *str, char *ptr, size_t length)
{
    str->ptr = ptr;
    str->slen = (ssize_t)length;
    return str;
}

str_t *c_strset2(str_t *str, char *src)
{
    str->ptr = src;
    str->slen = src ? (ssize_t)strlen(src) : 0;
    return str;
}

str_t *c_strset3(str_t *str, char *begin, char *end)
{
    str->ptr = begin;
    str->slen = (ssize_t)(end-begin);
    return str;
}

size_t c_strlen(const str_t *str )
{
    return str->slen;
}

const char *c_strbuf(const str_t *str)
{
    return str->ptr;
}

ssize_t c_strspn(const str_t *str, const str_t *set_char)
{
    ssize_t i, j, count = 0;
    for (i = 0; i < str->slen; i++) {
		if (count != i) 
			break;

		for (j = 0; j < set_char->slen; j++) {
			if (str->ptr[i] == set_char->ptr[j])
				count++;
		}
    }
    return count;
}

ssize_t c_strspn2(const str_t *str, const char *set_char)
{
    ssize_t i, j, count = 0;
    for (i = 0; i < str->slen; i++) {
		if (count != i)
			break;

		for (j = 0; set_char[j] != 0; j++) {
			if (str->ptr[i] == set_char[j])
			count++;
		}
    }
    return count;
}

ssize_t c_strcspn(const str_t *str, const str_t *set_char)
{
    ssize_t i, j;
    for (i = 0; i < str->slen; i++) {
		for (j = 0; j < set_char->slen; j++) {
			if (str->ptr[i] == set_char->ptr[j])
			return i;
		}
    }
    return i;
}

ssize_t c_strcspn2(const str_t *str, const char *set_char)
{
    ssize_t i, j;
    for (i = 0; i < str->slen; i++) {
		for (j = 0; set_char[j] != 0; j++) {
			if (str->ptr[i] == set_char[j])
			return i;
		}
    }
    return i;
}

ssize_t c_strtok(const str_t *str, const str_t *delim, str_t *tok, size_t start_idx)
{    
    ssize_t str_idx;

    tok->slen = 0;
    if ((str->slen == 0) || ((size_t)str->slen < start_idx)){
		return str->slen;
    }
    
    tok->ptr = str->ptr + start_idx;
    tok->slen = str->slen - start_idx;

    str_idx = c_strspn(tok, delim);
    if (start_idx + str_idx == (size_t)str->slen){
		return str->slen;
    }    
    tok->ptr += str_idx;
    tok->slen -= str_idx;

    tok->slen = c_strcspn(tok, delim);
    return start_idx + str_idx;
}

ssize_t c_strtok2(const str_t *str, const char *delim, str_t *tok, size_t start_idx)
{
    ssize_t str_idx;

    tok->slen = 0;
    if ((str->slen == 0) || ((size_t)str->slen < start_idx)) {
		return str->slen;
    }
    tok->ptr = str->ptr + start_idx;
    tok->slen = str->slen - start_idx;
    str_idx = c_strspn2(tok, delim);
    if (start_idx + str_idx == (size_t)str->slen) {
		return str->slen;
    }
    tok->ptr += str_idx;
    tok->slen -= str_idx;

    tok->slen = c_strcspn2(tok, delim);
    return start_idx + str_idx;
}

int stricmp(const char *s1, const char *s2)
{
    while ((*s1==*s2) || (c_tolower(*s1)==c_tolower(*s2))) {
		if (!*s1++)
			return 0;
		++s2;
    }
    return (c_tolower(*s1) < c_tolower(*s2)) ? -1 : 1;
}

int strnicmp(const char *s1, const char *s2, int len)
{
    if (!len) return 0;

    while ((*s1==*s2) || (c_tolower(*s1)==c_tolower(*s2))) {
		if (!*s1++ || --len <= 0)
			return 0;
		++s2;
    }
    return (c_tolower(*s1) < c_tolower(*s2)) ? -1 : 1;
}

char *c_strstr(const str_t *str, const str_t *substr)
{
    const char *s, *ends;
    if (substr->slen == 0) {
		return (char*)str->ptr;
    }
    s = str->ptr;
    ends = str->ptr + str->slen - substr->slen;
    for (; s <= ends; ++s) {
		if (strncmp(s, substr->ptr, substr->slen)==0)
			return (char*)s;
    }
    return NULL;
}


char *c_stristr(const str_t *str, const str_t *substr)
{
    const char *s, *ends;
    if (substr->slen == 0) {
		return (char*)str->ptr;
    }

    s = str->ptr;
    ends = str->ptr + str->slen - substr->slen;
    for (; s<=ends; ++s) {
		if (strnicmp(s, substr->ptr, substr->slen)==0)
			return (char*)s;
    }
    return NULL;
}

//移除字符串左侧的空白字符
str_t *c_strltrim(str_t *str)
{
    char *end = str->ptr + str->slen;
    register char *p = str->ptr;
    while (p < end && c_isspace(*p))++p;
    str->slen -= (p - str->ptr);
    str->ptr = p;
    return str;
}

str_t *c_strrtrim(str_t *str)
{
    char *end = str->ptr + str->slen;
    register char *p = end - 1;
    while (p >= str->ptr && c_isspace(*p))--p;
    str->slen -= ((end - p) - 1);
    return str;
}

char *c_create_random_string(char *str, size_t len)
{
    unsigned i;
    char *p = str;
    CHECK_STACK();
    for (i = 0; i < len/8; ++i) {
		uint32_t val = rand();
		c_val_to_hex_digit( (val & 0xFF000000) >> 24, p+0 );
		c_val_to_hex_digit( (val & 0x00FF0000) >> 16, p+2 );
		c_val_to_hex_digit( (val & 0x0000FF00) >>  8, p+4 );
		c_val_to_hex_digit( (val & 0x000000FF) >>  0, p+6 );
		p += 8;
    }
    for (i = i * 8; i < len; ++i) {
		*p++ = c_hex_digits[ rand() & 0x0F ];
    }
    return str;
}

long c_strtol(const str_t *str)
{
    CHECK_STACK();
    if (str->slen > 0 && (str->ptr[0] == '+' || str->ptr[0] == '-')) {
        str_t s;
        s.ptr = str->ptr + 1;
        s.slen = str->slen - 1;
        return (str->ptr[0] == '-'? -(long)c_strtoul(&s) : c_strtoul(&s));
    } else
        return c_strtoul(str);
}

int c_strtol2(const str_t *str, long *value)
{
    str_t s;
    unsigned long retval = 0;
    char is_negative = 0;
    int rc = 0;
    CHECK_STACK();
    if (!str || !value) {
        return EO_INVAL;
    }

    s = *str;
    c_strltrim(&s);

    if (s.slen == 0)
        return EO_INVAL;

    if (s.ptr[0] == '+' || s.ptr[0] == '-') {
        is_negative = (s.ptr[0] == '-');
        s.ptr += 1;
        s.slen -= 1;
    }

    rc = c_strtoul3(&s, &retval, 10);
    if (rc == EO_INVAL) {
        return rc;
    } else if (rc != EO_SUCCESS) {
        *value = is_negative ? C_MINLONG : C_MAXLONG;
        return is_negative ? EO_TOOSMALL : EO_TOOBIG;
    }

    if (retval > C_MAXLONG && !is_negative) {
        *value = C_MAXLONG;
        return EO_TOOBIG;
    }

    if (retval > (C_MAXLONG + 1UL) && is_negative) {
        *value = C_MINLONG;
        return EO_TOOSMALL;
    }

    *value = is_negative ? -(long)retval : retval;

    return EO_SUCCESS;
}

unsigned long c_strtoul(const str_t *str)
{
    unsigned long value;
    unsigned i;
    CHECK_STACK();
    value = 0;
    for (i=0; i<(unsigned)str->slen; ++i) {
		if (!c_isdigit(str->ptr[i]))
			break;
		value = value * 10 + (str->ptr[i] - '0');
    }
    return value;
}

unsigned long c_strtoul2(const str_t *str, str_t *endptr, unsigned base)
{
    unsigned long value;
    unsigned i;

    CHECK_STACK();

    value = 0;
    if (base <= 10) {
		for (i=0; i<(unsigned)str->slen; ++i) {
			unsigned c = (str->ptr[i] - '0');
			if (c >= base)
			break;
			value = value * base + c;
		}
    } 
	else if (base == 16) {
		for (i=0; i<(unsigned)str->slen; ++i) {
			if (!c_isxdigit(str->ptr[i]))
			break;
			value = value * 16 + c_hex_digit_to_val(str->ptr[i]);
		}
    } 
	else {
		assert(!"unsupported base");
		i = 0;
		value = 0xFFFFFFFFUL;
    }

    if (endptr) {
		endptr->ptr = str->ptr + i;
		endptr->slen = str->slen - i;
    }

    return value;
}

int c_strtoul3(const str_t *str, unsigned long *value, unsigned base)
{
    str_t s;
    unsigned i;
    CHECK_STACK();
    if (!str || !value) {
        return EO_INVAL;
    }

    s = *str;
    c_strltrim(&s);

    if (s.slen == 0 || s.ptr[0] < '0' ||
	(base <= 10 && (unsigned)s.ptr[0] > ('0' - 1) + base) ||
	(base == 16 && !c_isxdigit(s.ptr[0])))
    {
        return EO_INVAL;
    }

    *value = 0;
    if (base <= 10) {
		for (i=0; i<(unsigned)s.slen; ++i) {
			unsigned c = s.ptr[i] - '0';
			if (s.ptr[i] < '0' || (unsigned)s.ptr[i] > ('0' - 1) + base) {
				break;
			}
			if (*value > C_MAXULONG / base) {
				*value = C_MAXULONG;
				return EO_TOOBIG;
			}

			*value *= base;
			if ((C_MAXULONG - *value) < c) {
				*value = C_MAXULONG;
				return EO_TOOBIG;
			}
			*value += c;
		}
    } 
	else if (base == 16) {
		for (i=0; i<(unsigned)s.slen; ++i) {
			unsigned c = c_hex_digit_to_val(s.ptr[i]);
			if (!c_isxdigit(s.ptr[i]))
				break;

			if (*value > C_MAXULONG / base) {
				*value = C_MAXULONG;
				return EO_TOOBIG;
			}
			*value *= base;
			if ((C_MAXULONG - *value) < c) {
				*value = C_MAXULONG;
				return EO_TOOBIG;
			}
			*value += c;
		}
    }
	else {
		assert(!"unsupported base");
		return EO_INVAL;
    }
    return EO_SUCCESS;
}

float c_strtof(const str_t *str)
{
    str_t part;
    char *pdot;
    float val;

    if (str->slen == 0)
		return 0;

    pdot = (char *)memchr(str->ptr, '.', str->slen);
    part.ptr = str->ptr;
    part.slen = pdot ? pdot - str->ptr : str->slen;

    if (part.slen)
		val = (float)c_strtol(&part);
    else
		val = 0;

    if (pdot) {
		part.ptr = pdot + 1;
		part.slen = (str->ptr + str->slen - pdot - 1);
		if (part.slen) {
			str_t endptr;
			float fpart, fdiv;
			int i;
			fpart = (float)c_strtoul2(&part, &endptr, 10);
			fdiv = 1.0;
			for (i=0; i<(part.slen - endptr.slen); ++i)
				fdiv = fdiv * 10;
			if (val >= 0)
				val += (fpart / fdiv);
			else
				val -= (fpart / fdiv);
		}
    }
    return val;
}

int c_utoa(unsigned long val, char *buf)
{
    return c_utoa_pad(val, buf, 0, 0);
}

int c_utoa_pad( unsigned long val, char *buf, int min_dig, int pad)
{
    char *p;
    int len;

    CHECK_STACK();

    p = buf;
    do {
        unsigned long digval = (unsigned long) (val % 10);
        val /= 10;
        *p++ = (char) (digval + '0');
    } while (val > 0);

    len = (int)(p-buf);
    while (len < min_dig) {
		*p++ = (char)pad;
		++len;
    }
    *p-- = '\0';

    do {
        char temp = *p;
        *p = *buf;
        *buf = temp;
        --p;
        ++buf;
    } while (buf < p);

    return len;
}

char *c_strchr(const str_t *str, int chr)
{
    if (str->slen == 0)
		return NULL;
    return (char*) memchr((char*)str->ptr, chr, str->slen);
}


str_t c_str(char *str)
{
    str_t dst;
    dst.ptr = str;
    dst.slen = str ? strlen(str) : 0;
    return dst;
}
/****************************************************************/
//复制
str_t *c_strdup(T_PoolInfo *pool, str_t *dst, const str_t *src)
{
    if (dst == src) return dst;
    if (src->slen) {
		dst->ptr = (char*)POOL_Alloc(pool, src->slen);
		memcpy(dst->ptr, src->ptr, src->slen);
    }
    dst->slen = src->slen;
    return dst;
}

str_t *c_strdup_with_null(T_PoolInfo *pool, str_t *dst, const str_t *src)
{
    dst->ptr = (char*)POOL_Alloc(pool, src->slen+1);
    if (src->slen) {
		memcpy(dst->ptr, src->ptr, src->slen);
    }
    dst->slen = src->slen;
    dst->ptr[dst->slen] = '\0';
    return dst;
}

str_t *c_strdup2(T_PoolInfo *pool, str_t *dst, const char *src)
{
    dst->slen = src ? strlen(src) : 0;
    if (dst->slen){
		dst->ptr = (char*)POOL_Alloc(pool, dst->slen);
		memcpy(dst->ptr, src, dst->slen);
    } 
	else{
		dst->ptr = NULL;
    }
    return dst;
}

str_t *c_strdup2_with_null(T_PoolInfo *pool, str_t *dst, const char *src)
{
    dst->slen = src ? strlen(src) : 0;
    dst->ptr = (char*)POOL_Alloc(pool, dst->slen+1);
    if (dst->slen) {
		memcpy(dst->ptr, src, dst->slen);
    }
    dst->ptr[dst->slen] = '\0';
    return dst;
}

str_t c_strdup3(T_PoolInfo *pool, const char *src)
{
    str_t temp;
    c_strdup2(pool, &temp, src);
    return temp;
}
//复制src->dst
str_t *c_strassign(str_t *dst, str_t *src)
{
    dst->ptr = src->ptr;
    dst->slen = src->slen;
    return dst;
}
/****************************************************************/
//字符串拷贝
str_t *c_strcpy(str_t *dst, const str_t *src)
{
    dst->slen = src->slen;
    if (src->slen > 0)
		memcpy(dst->ptr, src->ptr, src->slen);
    return dst;
}

str_t * c_strcpy2(str_t *dst, const char *src)
{
    dst->slen = src ? strlen(src) : 0;
    if (dst->slen > 0)
		memcpy(dst->ptr, src, dst->slen);
    return dst;
}

str_t *c_strncpy(str_t *dst, const str_t *src, ssize_t max)
{
    assert(max >= 0);
    if (max > src->slen) max = src->slen;
    if (max > 0)
		memcpy(dst->ptr, src->ptr, max);
    dst->slen = max;
    return dst;
}

str_t *c_strncpy_with_null(str_t *dst, const str_t *src, ssize_t max)
{
    assert(max > 0);
    if (max <= src->slen)
		max = max-1;
    else
		max = src->slen;
    memcpy(dst->ptr, src->ptr, max);
    dst->ptr[max] = '\0';
    dst->slen = max;
    return dst;
}

/****************************************************************/
//字符串比较，按照长度小的走
int c_strcmp(const str_t *str1, const str_t *str2)
{
    if (str1->slen == 0) {
		return str2->slen==0 ? 0 : -1;
    } 
	else if (str2->slen == 0) {
		return 1;
    }
	else {
		size_t min = (str1->slen < str2->slen)? str1->slen : str2->slen;
		int res = memcmp(str1->ptr, str2->ptr, min);
		if (res == 0) {
			return (str1->slen < str2->slen) ? -1 :(str1->slen == str2->slen ? 0 : 1);
		} 
		else {
			return res;
		}
    }
}

int c_strncmp(const str_t *str1, const str_t *str2, size_t len)
{
    str_t copy1, copy2;
    if (len < (unsigned)str1->slen) {
		copy1.ptr = str1->ptr;
		copy1.slen = len;
		str1 = &copy1;
    }
    if (len < (unsigned)str2->slen) {
		copy2.ptr = str2->ptr;
		copy2.slen = len;
		str2 = &copy2;
    }
    return c_strcmp(str1, str2);
}

int c_strncmp2(const str_t *str1, const char *str2, size_t len)
{
    str_t copy2;
    if (str2) {
		copy2.ptr = (char*)str2;
		copy2.slen = strlen(str2);
    } 
	else {
		copy2.slen = 0;
    }
    return c_strncmp(str1, &copy2, len);
}

int c_strcmp2(const str_t *str1, const char *str2)
{
    str_t copy2;
    if (str2) {
		copy2.ptr = (char*)str2;
		copy2.slen = strlen(str2);
    } else {
		copy2.ptr = NULL;
		copy2.slen = 0;
    }
    return c_strcmp(str1, &copy2);
}
/****************************************************************/
//比较字典序
//长度：小的那个
//-1:0:1
int c_stricmp(const str_t *str1, const str_t *str2)
{
    if (str1->slen == 0) {
		return str2->slen == 0 ? 0 : -1;
    } 
	else if (str2->slen == 0) {
		return 1;
    } 
	else {
		size_t min = (str1->slen < str2->slen)? str1->slen : str2->slen;
		int res = strnicmp(str1->ptr, str2->ptr, min);
		if (res == 0) {
			return (str1->slen < str2->slen) ? -1 : (str1->slen == str2->slen ? 0 : 1);
		}
		else {
			return res;
		}
    }
}

int c_stricmp2(const str_t *str1, const char *str2)
{
    str_t copy2;
    if (str2){
		copy2.ptr = (char*)str2;
		copy2.slen = strlen(str2);
    } 
	else{
		copy2.ptr = NULL;
		copy2.slen = 0;
    }
    return c_stricmp(str1, &copy2);
}
//比较str1和str2的前n个字符字典序，str_t类型
int c_strnicmp(const str_t *str1, const str_t *str2, size_t len)
{
    str_t copy1, copy2;
    if (len < (unsigned)str1->slen) {
		copy1.ptr = str1->ptr;
		copy1.slen = len;
		str1 = &copy1;
    }

    if (len < (unsigned)str2->slen) {
		copy2.ptr = str2->ptr;
		copy2.slen = len;
		str2 = &copy2;
    }
    return c_stricmp(str1, str2);
}

int c_strnicmp2(const str_t *str1, const char *str2, size_t len)
{
    str_t copy2;

    if (str2) {
		copy2.ptr = (char*)str2;
		copy2.slen = strlen(str2);
    } 
	else {
		copy2.slen = 0;
    }

    return c_strnicmp(str1, &copy2, len);
}
/****************************************************************/
//字符串拼接 dst + src
void c_strcat(str_t *dst, const str_t *src)
{
    if (src->slen) {
		memcpy(dst->ptr + dst->slen, src->ptr, src->slen);
		dst->slen += src->slen;
    }
}

//字符串拼接2 dst + str（字符串）
void c_strcat2(str_t *dst, const char *str)
{
    size_t len = str? strlen(str) : 0;
    if (len) {
		memcpy(dst->ptr + dst->slen, str, len);
		dst->slen += len;
    }
}

//去除空格
str_t *c_strtrim(str_t *str)
{
    c_strltrim(str);
    c_strrtrim(str);
    return str;
}


//str中必须全是数字
int c_parse_number(char *str, match_t *table, int *out)
{
	char *comma, *head;
	int i;
	if (!str || !table) return 0;
	head = str;
	i = 0;
	comma = NULL;

	for(i = 0; i < table->num; ++i){
		comma = strchr(head, table->pattern[i]);
		if (!comma){
            *(out + i) = atoi(head);
			break;
		}

		*comma = '\0';
        *(out+ i) = atoi(head);
		head = comma + 1;	
	}

	return i + 1;
}
