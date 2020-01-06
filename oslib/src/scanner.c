#include "scanner.h"
#include "ctype.h"


#define SCAN_IS_SPACE(c)			((c)==' ' || (c)=='\t')
#define SCAN_IS_NEWLINE(c)			((c)=='\r' || (c)=='\n')
#define SCAN_IS_PROBABLY_SPACE(c)	((c) <= 32)
#define SCAN_CHECK_EOF(s)			(s != scanner->end)



static void scan_syntax_err(scanner_t *scanner)
{
    (*scanner->callback)(scanner);
}


void cis_buf_init(cis_buf_t *cis_buf)
{
    memset(cis_buf->cis_buf, 0, sizeof(cis_buf->cis_buf));
    cis_buf->use_mask = 0;
}

int cis_init(cis_buf_t *cis_buf, cis_t *cis)
{
    unsigned i;
    cis->cis_buf = cis_buf->cis_buf;
    for (i = 0; i < CIS_MAX_INDEX; ++i){
        if ((cis_buf->use_mask & (1 << i)) == 0){
            cis->cis_id = i;
			cis_buf->use_mask |= (1 << i);
            return EO_SUCCESS;
        }
    }
    cis->cis_id = CIS_MAX_INDEX;
    return EO_TOOMANY;
}

int cis_dup(cis_t *new_cis, cis_t *existing)
{
    int status;
    unsigned i;
    status = cis_init((cis_buf_t*)existing->cis_buf, new_cis);
    if (status != EO_SUCCESS) return status;
    for (i=0; i<256; ++i){
        if (CIS_ISSET(existing, i))
            CIS_SET(new_cis, i);
        else
            CIS_CLR(new_cis, i);
    }
    return EO_SUCCESS;
}

void cis_add_range(cis_t *cis, int cstart, int cend)
{
    assert(cstart > 0);
    while (cstart != cend){
        CIS_SET(cis, cstart);
		++cstart;
    }
}

void cis_add_alpha(cis_t *cis)
{
    cis_add_range(cis, 'a', 'z'+1);
    cis_add_range(cis, 'A', 'Z'+1);
}

void cis_add_num(cis_t *cis)
{
    cis_add_range(cis, '0', '9'+1);
}

void cis_add_str(cis_t *cis, const char *str)
{
    while (*str){
        CIS_SET(cis, *str);
		++str;
    }
}

void cis_add_cis(cis_t *cis, const cis_t *rhs)
{
    int i;
    for (i = 0; i < 256; ++i){
		if (CIS_ISSET(rhs, i))
			CIS_SET(cis, i);
    }
}

void cis_del_range(cis_t *cis, int cstart, int cend)
{
    while (cstart != cend){
        CIS_CLR(cis, cstart);
        cstart++;
    }
}

void cis_del_str( cis_t *cis, const char *str)
{
    while (*str){
        CIS_CLR(cis, *str);
		++str;
    }
}

void cis_invert( cis_t *cis )
{
    unsigned i;
    for (i = 1; i < 256; ++i){
		if (CIS_ISSET(cis,i))
            CIS_CLR(cis,i);
        else
            CIS_SET(cis,i);
    }
}


void scan_init(scanner_t *scanner, char *bufstart, size_t buflen, unsigned options, SCAN_ERR_CALLBCK callback )
{
    scanner->begin = scanner->curptr = bufstart;
    scanner->end = bufstart + buflen;
    scanner->line = 1;
    scanner->start_line = scanner->begin;
    scanner->callback = callback;
    scanner->skip_ws = options;

    if (scanner->skip_ws) 
		scan_skip_whitespace(scanner);
}

void scan_fini(scanner_t *scanner)
{
	
}

int scan_is_eof(const scanner_t *scanner)
{
    return scanner->curptr >= scanner->end;
}

void scan_skip_whitespace(scanner_t *scanner)
{
    register char *s = scanner->curptr;
    while (SCAN_IS_SPACE(*s)) ++s;
    if (SCAN_IS_NEWLINE(*s) && (scanner->skip_ws & SCAN_AUTOSKIP_NEWLINE)){
		for (;;){
			if (*s == '\r'){
				++s;
				if (*s == '\n') ++s;
				++scanner->line;
				scanner->curptr = scanner->start_line = s;
			} else if (*s == '\n'){
				++s;
				++scanner->line;
				scanner->curptr = scanner->start_line = s;
			} else if (SCAN_IS_SPACE(*s)){
				do {
					++s;
				}while(SCAN_IS_SPACE(*s));
			} else{
				break;
			}
		}
    }

    if (SCAN_IS_NEWLINE(*s) && (scanner->skip_ws & SCAN_AUTOSKIP_WS_HEADER)==SCAN_AUTOSKIP_WS_HEADER){
		/* Check for header continuation. */
		scanner->curptr = s;
		if (*s == '\r') ++s;
		if (*s == '\n') ++s;

		scanner->start_line = s;
		if (SCAN_IS_SPACE(*s)){
			register char *t = s;
			do {
				++t;
			}while(SCAN_IS_SPACE(*t));

			++scanner->line;
			scanner->curptr = t;
		}
    } 
	else
		scanner->curptr = s;
}

void scan_skip_line(scanner_t *scanner)
{
    char *s = strchr(scanner->curptr, '\n');
    if (!s){
		scanner->curptr = scanner->end;
    } else{
		scanner->curptr = scanner->start_line = s+1;
		scanner->line++;
   }
}

int scan_peek( scanner_t *scanner, const cis_t *spec, str_t *out)
{
    register char *s = scanner->curptr;
    if (s >= scanner->end){
		scan_syntax_err(scanner);
		return -1;
    }
    while (cis_match(spec, *s)) ++s;
    c_strset3(out, scanner->curptr, s);
    return *s;
}

int scan_peek_n(scanner_t *scanner, size_t len, str_t *out)
{
    char *endpos = scanner->curptr + len;
    if (endpos > scanner->end){
		scan_syntax_err(scanner);
		return -1;
    }
    c_strset(out, scanner->curptr, len);
    return *endpos;
}

int scan_peek_until(scanner_t *scanner, const cis_t *spec, str_t *out)
{
    register char *s = scanner->curptr;
    if (s >= scanner->end){
		scan_syntax_err(scanner);
		return -1;
    }
    while(SCAN_CHECK_EOF(s) && !cis_match(spec, *s)) ++s;
    c_strset3(out, scanner->curptr, s);
    return *s;
}

void scan_get(scanner_t *scanner, const cis_t *spec, str_t *out)
{
    register char *s = scanner->curptr;
    assert(cis_match(spec,0)==0);
    if (!cis_match(spec, *s)){
		scan_syntax_err(scanner);
		return;
    }

    do {
		++s;
    }while(cis_match(spec, *s));
    c_strset3(out, scanner->curptr, s);
    scanner->curptr = s;
    if (SCAN_IS_PROBABLY_SPACE(*s) && scanner->skip_ws)
		scan_skip_whitespace(scanner);    
}

void scan_get_unescape(scanner_t *scanner, const cis_t *spec, str_t *out)
{
    register char *s = scanner->curptr;
    char *dst = s;
    assert(cis_match(spec,0)==0);
    assert(cis_match(spec,'%')==0);
    if (!cis_match(spec, *s) && *s != '%'){
		scan_syntax_err(scanner);
		return;
    }

    out->ptr = s;
    do {
		if (*s == '%'){
			if (s+3 <= scanner->end && c_isxdigit(*(s+1)) && c_isxdigit(*(s+2))){
				*dst = (uint8_t) ((c_hex_digit_to_val(*(s+1)) << 4) + c_hex_digit_to_val(*(s+2)));
				++dst;
				s += 3;
			}else{
				*dst++ = *s++;
				*dst++ = *s++;
				break;
			}
		}
		
		if (cis_match(spec, *s)){
			char *start = s;
			do {
				++s;
			} while (cis_match(spec, *s));

			if (dst != start) memmove(dst, start, s-start);
			dst += (s-start);
		}
    }while(*s == '%');
    scanner->curptr = s;
    out->slen = (dst - out->ptr);
    if (SCAN_IS_PROBABLY_SPACE(*s) && scanner->skip_ws)
		scan_skip_whitespace(scanner);    
}

void scan_get_quote(scanner_t *scanner, int begin_quote, int end_quote, str_t *out)
{
    char beg = (char)begin_quote;
    char end = (char)end_quote;
    scan_get_quotes(scanner, &beg, &end, 1, out);
}

void scan_get_quotes(scanner_t *scanner, const char *begin_quote, const char *end_quote, int qsize, str_t *out)
{
    register char *s = scanner->curptr;
    int qpair = -1;
    int i;
    assert(qsize > 0);
    for (i = 0; i < qsize; ++i){
		if (*s == begin_quote[i]){
			qpair = i;
			break;
		}
    }
    if (qpair == -1){
		scan_syntax_err(scanner);
		return;
    }
    ++s;
    do {
		while (SCAN_CHECK_EOF(s) && *s != '\n' && *s != end_quote[qpair]) ++s;
		if (*s == end_quote[qpair]){
			if (*(s-1) == '\\'){
				char *q = s-2;
				char *r = s-2;
				while (r != scanner->begin && *r == '\\') --r;
				if (((unsigned)(q-r) & 0x01) == 1){
					break;
				}
				++s;
			} else {
				break;
			}
		} else {
			break;
		}
    } while (1);

    if (*s != end_quote[qpair]){
		scan_syntax_err(scanner);
		return;
    }
    ++s;
    c_strset3(out, scanner->curptr, s);
    scanner->curptr = s;
    if (SCAN_IS_PROBABLY_SPACE(*s) && scanner->skip_ws) 
		scan_skip_whitespace(scanner);
}

void scan_get_n(scanner_t *scanner, unsigned N, str_t *out)
{
    if (scanner->curptr + N > scanner->end) {
		scan_syntax_err(scanner);
		return;
    }
    c_strset(out, scanner->curptr, N);
    scanner->curptr += N;
    if (SCAN_IS_PROBABLY_SPACE(*scanner->curptr) && scanner->skip_ws)
		scan_skip_whitespace(scanner);
}

int scan_get_char(scanner_t *scanner)
{
    int chr = *scanner->curptr;
    if (!chr) {
		scan_syntax_err(scanner);
		return 0;
    }
    ++scanner->curptr;
    if (SCAN_IS_PROBABLY_SPACE(*scanner->curptr) && scanner->skip_ws)
		scan_skip_whitespace(scanner);
    return chr;
}

void scan_get_newline(scanner_t *scanner)
{
    if (!SCAN_IS_NEWLINE(*scanner->curptr)){
		scan_syntax_err(scanner);
		return;
    }

    if (*scanner->curptr == '\r'){
		++scanner->curptr;
    }
    if (*scanner->curptr == '\n'){
		++scanner->curptr;
    }
    ++scanner->line;
    scanner->start_line = scanner->curptr;
}

void scan_get_until(scanner_t *scanner, const cis_t *spec, str_t *out)
{
    register char *s = scanner->curptr;

    if (s >= scanner->end){
		scan_syntax_err(scanner);
		return;
    }

    while (SCAN_CHECK_EOF(s) && !cis_match(spec, *s)) ++s;
    c_strset3(out, scanner->curptr, s);
    scanner->curptr = s;
    if (SCAN_IS_PROBABLY_SPACE(*s) && scanner->skip_ws)
		scan_skip_whitespace(scanner);
}

void scan_get_until_ch(scanner_t *scanner, int until_char, str_t *out)
{
    register char *s = scanner->curptr;
    if (s >= scanner->end){
		scan_syntax_err(scanner);
		return;
    }

    while (SCAN_CHECK_EOF(s) && *s != until_char) ++s;

    c_strset3(out, scanner->curptr, s);
    scanner->curptr = s;
    if (SCAN_IS_PROBABLY_SPACE(*s) && scanner->skip_ws)
		scan_skip_whitespace(scanner);
}

void scan_get_until_chr(scanner_t *scanner, const char *until_spec, str_t *out)
{
    register char *s = scanner->curptr;
    size_t speclen;
    if (s >= scanner->end) {
		scan_syntax_err(scanner);
		return;
    }

    speclen = strlen(until_spec);
    while (SCAN_CHECK_EOF(s) && !memchr(until_spec, *s, speclen)) ++s;
    c_strset3(out, scanner->curptr, s);
    scanner->curptr = s;
    if (SCAN_IS_PROBABLY_SPACE(*s) && scanner->skip_ws)
		scan_skip_whitespace(scanner);
}

void scan_advance_n(scanner_t *scanner, unsigned N, char skip_ws)
{
    if (scanner->curptr + N > scanner->end){
		scan_syntax_err(scanner);
		return;
    }
    scanner->curptr += N;
    if (SCAN_IS_PROBABLY_SPACE(*scanner->curptr) && skip_ws)
		scan_skip_whitespace(scanner);
}

int scan_strcmp(scanner_t *scanner, const char *s, int len)
{
    if (scanner->curptr + len > scanner->end){
		scan_syntax_err(scanner);
		return -1;
    }
    return strncmp(scanner->curptr, s, len);
}

int scan_stricmp(scanner_t *scanner, const char *s, int len)
{
    if (scanner->curptr + len > scanner->end){
		scan_syntax_err(scanner);
		return -1;
    }
    return strnicmp(scanner->curptr, s, len);
}

int scan_stricmp_alnum(scanner_t *scanner, const char *s, int len)
{
    if (scanner->curptr + len > scanner->end){
		scan_syntax_err(scanner);
		return -1;
    }
    return strnicmp_alnum(scanner->curptr, s, len);
}

void scan_save_state(const scanner_t *scanner, scan_state *state)
{
    state->curptr = scanner->curptr;
    state->line = scanner->line;
    state->start_line = scanner->start_line;
}

void scan_restore_state(scanner_t *scanner, scan_state *state)
{
    scanner->curptr = state->curptr;
    scanner->line = state->line;
    scanner->start_line = state->start_line;
}

int scan_get_col(const scanner_t *scanner)
{
    return (int)(scanner->curptr - scanner->start_line);
}
