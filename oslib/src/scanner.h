#ifndef __ZXY_SCANNER_H__
#define __ZXY_SCANNER_H__

#include "typedefs.h"
#include "cstring.h"


typedef uint32_t cis_elem_t;

#define CIS_MAX_INDEX   (sizeof(cis_elem_t) << 3)

typedef struct _tag_cis_buf
{
    cis_elem_t    cis_buf[256];
    cis_elem_t    use_mask;
}cis_buf_t;

typedef struct _tagcis
{
    cis_elem_t	*cis_buf;
    int			cis_id;
}cis_t;

#define CIS_SET(cis,c)   ((cis)->cis_buf[(int)(c)] |= (1 << (cis)->cis_id))
#define CIS_CLR(cis,c)   ((cis)->cis_buf[(int)c] &= ~(1 << (cis)->cis_id))
#define CIS_ISSET(cis,c) ((cis)->cis_buf[(int)c] & (1 << (cis)->cis_id))

enum
{
    SCAN_AUTOSKIP_WS = 1,
    SCAN_AUTOSKIP_WS_HEADER = 3,
    SCAN_AUTOSKIP_NEWLINE = 4
};


API_DEF(void)	cis_buf_init(cis_buf_t *cs_buf);
API_DEF(int)	cis_init(cis_buf_t *cs_buf, cis_t *cis);
API_DEF(int)	cis_dup(cis_t *new_cis, cis_t *existing);
API_DEF(void) 	cis_add_range(cis_t *cis, int cstart, int cend);
API_DEF(void) 	cis_add_alpha(cis_t *cis);
API_DEF(void) 	cis_add_num(cis_t *cis);
API_DEF(void) 	cis_add_str(cis_t *cis, const char *str);
API_DEF(void) 	cis_add_cis(cis_t *cis, const cis_t *rhs);
API_DEF(void) 	cis_del_range(cis_t *cis, int cstart, int cend);
API_DEF(void) 	cis_del_str(cis_t *cis, const char *str);
API_DEF(void) 	cis_invert(cis_t *cis);

#define cis_match(a, b) CIS_ISSET(a, b)

struct _tag_scanner;

typedef void (*SCAN_ERR_CALLBCK)(struct _tag_scanner *scanner);

typedef struct _tag_scanner
{
    char *begin;
    char *end;
    char *curptr;
    int   line;
    char *start_line;
    int   skip_ws;
    SCAN_ERR_CALLBCK callback;
}scanner_t;

typedef struct _tag_scan_state
{
    char *curptr;
    int   line;
    char *start_line;
}scan_state;


API_DEF(void) 	scan_init(scanner_t *scanner, char *bufstart, size_t buflen, unsigned options, SCAN_ERR_CALLBCK callback);
API_DEF(void) 	scan_fini(scanner_t *scanner);
API_DEF(int) 	scan_is_eof(const scanner_t *scanner);
API_DEF(int) 	scan_peek(scanner_t *scanner, const cis_t *spec, str_t *out);
API_DEF(int) 	scan_peek_n(scanner_t *scanner, size_t len, str_t *out);
API_DEF(int) 	scan_peek_until(scanner_t *scanner, const cis_t *spec, str_t *out);
API_DEF(void) 	scan_get(scanner_t *scanner, const cis_t *spec, str_t *out);
API_DEF(void) 	scan_get_unescape(scanner_t *scanner, const cis_t *spec, str_t *out);
API_DEF(void) 	scan_get_quote(scanner_t *scanner, int begin_quote, int end_quote, str_t *out);
API_DEF(void) 	scan_get_quotes(scanner_t *scanner, const char *begin_quotes, const char *end_quotes, int qsize, str_t *out);
API_DEF(void) 	scan_get_n(scanner_t *scanner, unsigned N, str_t *out);
API_DEF(int) 	scan_get_char(scanner_t *scanner);		//取出一个字符
API_DEF(void) 	scan_get_until(scanner_t *scanner, const cis_t *spec, str_t *out);
API_DEF(void) 	scan_get_until_ch(scanner_t *scanner, int until_char, str_t *out);	//当前位置开始，到指定字符，拷贝到out
API_DEF(void) 	scan_get_until_chr(scanner_t *scanner, const char *until_spec, str_t *out);
API_DEF(void) 	scan_advance_n(scanner_t *scanner, unsigned N, char skip);
API_DEF(int) 	scan_strcmp(scanner_t *scanner, const char *s, int len);
API_DEF(int) 	scan_stricmp(scanner_t *scanner, const char *s, int len);
API_DEF(int) 	scan_stricmp_alnum(scanner_t *scanner, const char *s, int len);
API_DEF(void) 	scan_get_newline(scanner_t *scanner);
API_DEF(void) 	scan_skip_whitespace(scanner_t *scanner);
API_DEF(void) 	scan_skip_line(scanner_t *scanner);
API_DEF(void) 	scan_save_state(const scanner_t *scanner, scan_state *state);
API_DEF(void) 	scan_restore_state(scanner_t *scanner, scan_state *state);
API_DEF(int) 	scan_get_col(const scanner_t *scanner);


#endif

