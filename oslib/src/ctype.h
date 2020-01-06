#ifndef __ZXY_CTYPE_H__
#define __ZXY_CTYPE_H__

#include "config_site.h"

#if defined(ZXY_HAS_CTYPE_H) && ZXY_HAS_CTYPE_H != 0
#  include <ctype.h>
#else
#  define isalnum(c)	    (isalpha(c) || isdigit(c))
#  define isalpha(c)	    (islower(c) || isupper(c))
#  define isascii(c)	    (((unsigned char)(c))<=0x7f)
#  define isdigit(c)	    ((c)>='0' && (c)<='9')
#  define isspace(c)	    ((c)==' '  || (c)=='\t' || (c)=='\n' || (c)=='\r' || (c)=='\v')
#  define islower(c)	    ((c)>='a' && (c)<='z')
#  define isupper(c)	    ((c)>='A' && (c)<='Z')
#  define isxdigit(c)	    (isdigit(c) || (tolower(c)>='a'&&tolower(c)<='f'))
#  define tolower(c)	    (((c) >= 'A' && (c) <= 'Z') ? (c)+('a'-'A') : (c))
#  define toupper(c)	    (((c) >= 'a' && (c) <= 'z') ? (c)-('a'-'A') : (c))
#endif


static inline int c_isalnum(unsigned char c) { return isalnum(c); }
static inline int c_isalpha(unsigned char c) { return isalpha(c); }
static inline int c_isascii(unsigned char c) { return c<128; }
static inline int c_isdigit(unsigned char c) { return isdigit(c); }
static inline int c_isspace(unsigned char c) { return isspace(c); }
static inline int c_islower(unsigned char c) { return islower(c); }
static inline int c_isupper(unsigned char c) { return isupper(c); }
static inline int c_isblank(unsigned char c) { return (c==' ' || c=='\t'); }
static inline int c_tolower(unsigned char c) { return tolower(c); }
static inline int c_toupper(unsigned char c) { return toupper(c); }
static inline int c_isxdigit(unsigned char c){ return isxdigit(c); }



#define c_hex_digits	"0123456789abcdef"


static inline void c_val_to_hex_digit(unsigned value, char *p)
{
    *p++ = c_hex_digits[ (value & 0xF0) >> 4 ];
    *p   = c_hex_digits[ (value & 0x0F) ];
}


static inline unsigned c_hex_digit_to_val(unsigned char c)
{
    if (c <= '9')
		return (c-'0') & 0x0F;
    else if (c <= 'F')
		return  (c-'A'+10) & 0x0F;
    else
		return (c-'a'+10) & 0x0F;
}



#endif	/* __ZXY_CTYPE_H__ */

