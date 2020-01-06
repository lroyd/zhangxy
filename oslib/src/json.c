#include "ctype.h"

#if defined(ZXY_HAS_JSON) && ZXY_HAS_JSON!=0

#include "json.h"
#include "scanner.h"

#define CHECK_STACK()


#define EL_INIT(p_el, nm, typ)	do { \
				    if (nm) { \
					p_el->name = *nm; \
				    } else { \
					p_el->name.ptr = (char*)""; \
					p_el->name.slen = 0; \
				    } \
				    p_el->type = typ; \
				} while (0)

struct write_state;
struct parse_state
{
    T_PoolInfo		*pool;
    scanner_t		 scanner;
    T_JsonErrInfo 	*err_info;
    cis_t		 float_spec;	/* numbers with dot! */
};

#define NO_NAME	1

static int elem_write(const T_JsonElem *_pElem, struct write_state *st, unsigned flags);
static T_JsonElem* parse_elem_throw(struct parse_state *st, T_JsonElem *_pElem);

void JSON_ElemSetNull(T_JsonElem *_pElem, str_t *_strName)
{
    EL_INIT(_pElem, _strName, JSON_VAL_NULL);
}

void JSON_ElemSetBool(T_JsonElem *_pElem, str_t *_strName, char _bVal)
{
    EL_INIT(_pElem, _strName, JSON_VAL_BOOL);
    _pElem->value.is_true = _bVal;
}

void JSON_ElemSetNumber(T_JsonElem *_pElem, str_t *_strName, float _fVal)
{
    EL_INIT(_pElem, _strName, JSON_VAL_NUMBER);
    _pElem->value.num = _fVal;
}

void JSON_ElemSetString(T_JsonElem *_pElem, str_t *_strName, str_t *_pValue)
{
    EL_INIT(_pElem, _strName, JSON_VAL_STRING);
    _pElem->value.str = *_pValue;
}

void JSON_ElemSetArray(T_JsonElem *_pElem, str_t *_strName)
{
    EL_INIT(_pElem, _strName, JSON_VAL_ARRAY);
	DL_ListInit(&_pElem->value.children.list);
}

void JSON_ElemSetObj(T_JsonElem *_pElem, str_t *_strName)
{
    EL_INIT(_pElem, _strName, JSON_VAL_OBJ);
	DL_ListInit(&_pElem->value.children.list);
}

void JSON_ElemAdd(T_JsonElem *_pElem, T_JsonElem *_pChild)
{
    assert(_pElem->type == JSON_VAL_OBJ || _pElem->type == JSON_VAL_ARRAY);
	DL_ListAddTail(&_pElem->value.children.list, &_pChild->list);
}


static int parse_children(struct parse_state *_pSt, T_JsonElem *_pParent)
{
    char end_quote = (_pParent->type == JSON_VAL_ARRAY)? ']' : '}';
    scan_get_char(&_pSt->scanner);
    while (*_pSt->scanner.curptr != end_quote){
		T_JsonElem *pChild;
		while (*_pSt->scanner.curptr == ',')
			scan_get_char(&_pSt->scanner);

		if (*_pSt->scanner.curptr == end_quote)
			break;

		pChild = parse_elem_throw(_pSt, NULL);
		if (!pChild) return EO_INJSON;
		JSON_ElemAdd(_pParent, pChild);
    }
    scan_get_char(&_pSt->scanner);
    return EO_SUCCESS;
}

static unsigned parse_quoted_string(struct parse_state *_pSt, str_t *_pOut)
{
    str_t token;
    char *op, *ip, *iend;
    scan_get_quote(&_pSt->scanner, '"', '"', &token);
    token.ptr++;
    token.slen-=2;
    if (c_strchr(&token, '\\') == NULL){
		*_pOut = token;
		return 0;
    }
    _pOut->ptr = op = POOL_Alloc(_pSt->pool, token.slen);
    ip = token.ptr;
    iend = token.ptr + token.slen;
    while (ip != iend){
		if (*ip == '\\'){
			++ip;
			if (ip == iend) goto on_error;
			
			if (*ip == 'u'){
				ip++;
				if (iend - ip < 4){
					ip = iend -1;
					goto on_error;
				}
				*op++ = (char)(c_hex_digit_to_val(ip[2]) * 16 + c_hex_digit_to_val(ip[3]));
				ip += 4;
			} else if (*ip=='"' || *ip=='\\' || *ip=='/'){
				*op++ = *ip++;
			} else if (*ip=='b'){
				*op++ = '\b';
				ip++;
			} else if (*ip=='f'){
				*op++ = '\f';
				ip++;
			} else if (*ip=='n'){
				*op++ = '\n';
				ip++;
			} else if (*ip=='r'){
				*op++ = '\r';
				ip++;
			} else if (*ip=='t'){
				*op++ = '\t';
				ip++;
			} 
			else
				goto on_error;
		}
		else
			*op++ = *ip++;
    }

    _pOut->slen = op - _pOut->ptr;
    return 0;

on_error:
    _pOut->slen = op - _pOut->ptr;
    return (unsigned)(ip - token.ptr);
}

static T_JsonElem* parse_elem_throw(struct parse_state *_pSt, T_JsonElem *_pElem)
{
    str_t strName = {NULL, 0}, value = {NULL, 0};
    str_t token;

    if (!_pElem)
		_pElem = POOL_Alloc(_pSt->pool, sizeof(*_pElem));

    if (*_pSt->scanner.curptr == '"'){
		scan_get_char(&_pSt->scanner);	//偏移过"
		scan_get_until_ch(&_pSt->scanner, '"', &token);
		scan_get_char(&_pSt->scanner);	//偏移过"

		if (*_pSt->scanner.curptr == ':'){	//key
			scan_get_char(&_pSt->scanner);
			strName = token;
		} 
		else
			value = token;
    }

    if (value.slen) {
		JSON_ElemSetString(_pElem, &strName, &value);
		return _pElem;
    }

    if (cis_match(&_pSt->float_spec, *_pSt->scanner.curptr) || *_pSt->scanner.curptr == '-'){
		float fVal;
		char neg = 0;

		if (*_pSt->scanner.curptr == '-'){
			scan_get_char(&_pSt->scanner);
			neg = 1;
		}

		scan_get(&_pSt->scanner, &_pSt->float_spec, &token);
		fVal = c_strtof(&token);
		if (neg) fVal = -fVal;

		JSON_ElemSetNumber(_pElem, &strName, fVal);

    } else if (*_pSt->scanner.curptr == '"'){
		unsigned err;
		char *start = _pSt->scanner.curptr;

		err = parse_quoted_string(_pSt, &token);
		if (err) {
			_pSt->scanner.curptr = start + err;
			return NULL;
		}
		JSON_ElemSetString(_pElem, &strName, &token);
    } else if (c_isalpha(*_pSt->scanner.curptr)){
		if (scan_strcmp(&_pSt->scanner, "false", 5)==0){
			JSON_ElemSetBool(_pElem, &strName, 0);
			scan_advance_n(&_pSt->scanner, 5, 1);
		} 
		else if (scan_strcmp(&_pSt->scanner, "true", 4)==0){
			JSON_ElemSetBool(_pElem, &strName, 1);
			scan_advance_n(&_pSt->scanner, 4, 1);
		} 
		else if (scan_strcmp(&_pSt->scanner, "null", 4)==0){
			JSON_ElemSetNull(_pElem, &strName);
			scan_advance_n(&_pSt->scanner, 4, 1);
		} 
		else
			return NULL;

    } else if (*_pSt->scanner.curptr == '['){
		JSON_ElemSetArray(_pElem, &strName);
		if (parse_children(_pSt, _pElem) != EO_SUCCESS)
			return NULL;
    } 
	else if (*_pSt->scanner.curptr == '{'){
		JSON_ElemSetObj(_pElem, &strName);
		if (parse_children(_pSt, _pElem) != EO_SUCCESS)
			return NULL;
    } 
	else
		return NULL;

    return _pElem;
}

static void on_syntax_error(scanner_t *scanner)
{
	printf("error@@@@@@@@@@@@@@\r\n");
}

T_JsonElem* JSON_Parse(T_PoolInfo *pool, char *_strJson, unsigned *size, T_JsonErrInfo *_pErrInfo)
{
    cis_buf_t cis_buf;
    struct parse_state st;
    T_JsonElem *root;
    ASSERT_RETURN(pool && _strJson && size, NULL);

    if (!*size) return NULL;
    memset(&st, 0, sizeof(st));
    st.pool = pool;
    st.err_info = _pErrInfo;
    scan_init(&st.scanner, _strJson, *size, SCAN_AUTOSKIP_WS | SCAN_AUTOSKIP_NEWLINE, &on_syntax_error);
    cis_buf_init(&cis_buf);
    cis_init(&cis_buf, &st.float_spec);
    cis_add_str(&st.float_spec, ".0123456789");
	
	root = parse_elem_throw(&st, NULL);
    if (!root && _pErrInfo) {
		_pErrInfo->line = st.scanner.line;
		_pErrInfo->col = scan_get_col(&st.scanner) + 1;
		_pErrInfo->err_char = *st.scanner.curptr;
    }

    *size = (unsigned)((_strJson + *size) - st.scanner.curptr);
    scan_fini(&st.scanner);
    return root;
}

struct buf_writer_data
{
    char		*pos;
    unsigned	size;
};

static int buf_writer(const char *s, unsigned size, void *user_data)
{
    struct buf_writer_data *buf_data = (struct buf_writer_data*)user_data;
    if (size+1 >= buf_data->size)
		return EO_TOOBIG;

    memcpy(buf_data->pos, s, size);
    buf_data->pos += size;
    buf_data->size -= size;

    return EO_SUCCESS;
}

int JSON_ElemWrite(const T_JsonElem *_pElem, char *buffer, unsigned *size)
{
    struct buf_writer_data buf_data;
    int status;

    ASSERT_RETURN(_pElem && buffer && size, EO_INVAL);

    buf_data.pos = buffer;
    buf_data.size = *size;

    status = JSON_ElemWritef(_pElem, &buf_writer, &buf_data);
    if (status != EO_SUCCESS)
		return status;

    *buf_data.pos = '\0';
    *size = (unsigned)(buf_data.pos - buffer);
    return EO_SUCCESS;
}

#define MAX_INDENT			(100)
#define JSON_NAME_MIN_LEN	(20)
#define ESC_BUF_LEN			(64)
#define JSON_INDENT_SIZE	(3)

struct write_state
{
    JSON_WRITER	 writer;
    void 		*user_data;
    char		 indent_buf[MAX_INDENT];
    int			 indent;
    char		 space[JSON_NAME_MIN_LEN];
};

#define CHECK(expr) do { status=expr; if (status!=EO_SUCCESS) return status; } while (0)

static int write_string_escaped(const str_t *value, struct write_state *st)
{
    const char *ip = value->ptr;
    const char *iend = value->ptr + value->slen;
    char buf[ESC_BUF_LEN];
    char *op = buf;
    char *oend = buf + ESC_BUF_LEN;
    int status;

    while (ip != iend){
		while (ip != iend && op != oend){
			if (oend - op < 2) break;

			if (*ip == '"'){
				*op++ = '\\';
				*op++ = '"';
				ip++;
			} else if (*ip == '\\'){
				*op++ = '\\';
				*op++ = '\\';
				ip++;
			} else if (*ip == '/'){
				*op++ = '\\';
				*op++ = '/';
				ip++;
			} else if (*ip == '\b'){
				*op++ = '\\';
				*op++ = 'b';
				ip++;
			} else if (*ip == '\f'){
				*op++ = '\\';
				*op++ = 'f';
				ip++;
			} else if (*ip == '\n'){
				*op++ = '\\';
				*op++ = 'n';
				ip++;
			} else if (*ip == '\r'){
				*op++ = '\\';
				*op++ = 'r';
				ip++;
			} else if (*ip == '\t'){
				*op++ = '\\';
				*op++ = 't';
				ip++;
			} else if ((*ip >= 32 && *ip < 127)){
				*op++ = *ip++;
			} 
			else{
				if (oend - op < 6) break;
				*op++ = '\\';
				*op++ = 'u';
				*op++ = '0';
				*op++ = '0';
				c_val_to_hex_digit(*ip, op);
				op+=2;
				ip++;
			}
		}

		CHECK( st->writer( buf, (unsigned)(op-buf), st->user_data) );
		op = buf;
    }

    return EO_SUCCESS;
}

static int write_children(const T_JsonList *list, const char quotes[2], struct write_state *st)
{
    unsigned flags = (quotes[0]=='[') ? NO_NAME : 0;
    int status;

    //CHECK( st->writer( st->indent_buf, st->indent, st->user_data) );
    CHECK(st->writer( &quotes[0], 1, st->user_data));
    CHECK(st->writer( " ", 1, st->user_data));

	if(!DL_ListIsEmpty(&list->list)){	
		char indent_added = 0;
		T_JsonElem *pChild = DL_ListGetNext(&list->list, T_JsonElem, list);	//list->next;

		if (pChild->name.slen == 0){
			while (pChild != (T_JsonElem*)list){
				status = elem_write(pChild, st, flags);
				if (status != EO_SUCCESS)
					return status;
				
				T_JsonElem *tmp = DL_ListGetNext(&pChild->list, T_JsonElem, list);
				if (tmp != (T_JsonElem*)list)
					CHECK(st->writer( ", ", 2, st->user_data));
				pChild = tmp;
			}
		} 
		else{
			if (st->indent < sizeof(st->indent_buf)){
				st->indent += JSON_INDENT_SIZE;
				indent_added = 1;
			}
			CHECK( st->writer( "\n", 1, st->user_data) );
			while (pChild != (T_JsonElem*)list){
				status = elem_write(pChild, st, flags);
				if (status != EO_SUCCESS)
					return status;
				T_JsonElem *tmp = DL_ListGetNext(&pChild->list, T_JsonElem, list);
				if (tmp != (T_JsonElem*)list)
					CHECK(st->writer( ",\n", 2, st->user_data));
				else
					CHECK(st->writer( "\n", 1, st->user_data));
				pChild = tmp;
			}
			if (indent_added){
				st->indent -= JSON_INDENT_SIZE;
			}
			CHECK(st->writer(st->indent_buf, st->indent, st->user_data));
		}
    }
    CHECK(st->writer(&quotes[1], 1, st->user_data));
    return EO_SUCCESS;
}

static int elem_write(const T_JsonElem *_pElem, struct write_state *st, unsigned flags)
{
    int status;
    if (_pElem->name.slen){
		CHECK( st->writer( st->indent_buf, st->indent, st->user_data) );
		if ((flags & NO_NAME)==0){
			CHECK( st->writer( "\"", 1, st->user_data) );
			CHECK( write_string_escaped(&_pElem->name, st) );
			CHECK( st->writer( "\": ", 3, st->user_data) );
			if (_pElem->name.slen < JSON_NAME_MIN_LEN /*&&
			_pElem->type != JSON_VAL_OBJ &&
			_pElem->type != JSON_VAL_ARRAY*/)
			{
				CHECK(st->writer(st->space,(unsigned)(JSON_NAME_MIN_LEN - _pElem->name.slen), st->user_data));
			}
		}
    }

    switch (_pElem->type){
		case JSON_VAL_NULL:
			CHECK( st->writer( "null", 4, st->user_data) );
			break;
		case JSON_VAL_BOOL:
			if (_pElem->value.is_true)
				CHECK( st->writer( "true", 4, st->user_data) );
			else
				CHECK( st->writer( "false", 5, st->user_data) );
			break;
		case JSON_VAL_NUMBER:
			{
				char num_buf[65];
				int len;
				if (_pElem->value.num == (int)_pElem->value.num)
					len = snprintf(num_buf, sizeof(num_buf), "%d",(int)_pElem->value.num);
				else
					len = snprintf(num_buf, sizeof(num_buf), "%f", _pElem->value.num);

				if (len < 0 || len >= sizeof(num_buf))
					return EO_TOOBIG;
				CHECK(st->writer( num_buf, len, st->user_data));
			}
			break;
		case JSON_VAL_STRING:
			CHECK( st->writer( "\"", 1, st->user_data) );
			CHECK( write_string_escaped( &_pElem->value.str, st) );
			CHECK( st->writer( "\"", 1, st->user_data) );
			break;
		case JSON_VAL_ARRAY:
			CHECK( write_children(&_pElem->value.children, "[]", st) );
			break;
		case JSON_VAL_OBJ:
			CHECK( write_children(&_pElem->value.children, "{}", st) );
			break;
		default:
			assert(!"Unhandled value type");
    }
    return EO_SUCCESS;
}

#undef CHECK

int JSON_ElemWritef(const T_JsonElem *_pElem, JSON_WRITER _funcWriter, void *user_data)
{
    struct write_state st;
    ASSERT_RETURN(_pElem && _funcWriter, EO_INVAL);
    st.writer 		= _funcWriter;
    st.user_data	= user_data,
    st.indent 		= 0;
    memset(st.indent_buf, ' ', MAX_INDENT);
    memset(st.space, ' ', JSON_NAME_MIN_LEN);
    return elem_write(_pElem, &st, 0);
}

#endif