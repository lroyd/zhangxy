#include "json.h"


static char json_doc1[] =
"{\
    \"Object\": {\
       \"Integer\":  800,\
       \"Negative\":  -12,\
       \"Float\": -7.2,\
       \"String\":  \"A\\tString with tab\",\
       \"Object2\": {\
           \"True\": true,\
           \"False\": false,\
           \"Null\": null\
       },\
       \"Array1\": [116, false, \"string\", {}],\
       \"Array2\": [\
	    {\
        	   \"Float\": 123.,\
	    },\
	    {\
		   \"Float\": 123.,\
	    }\
       ]\
     },\
   \"Integer\":  800,\
   \"Array1\": [116, false, \"string\"]\
}\
";

static char json_doc2[] = "{\"Integer\":  800}";
#if 0
"{\
   \"Integer\":  800\
}";
#endif
static int json_verify_1()
{
	THREAD_Init();
    T_PoolInfo *pool;
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);	
    pool = POOL_Create(&cp.in_pFactory, "json", 1000, 1000, NULL);		
	
    T_JsonElem *elem;
    char *out_buf;
    unsigned size;
    T_JsonErrInfo err;

    size = (unsigned)strlen(json_doc2);
    elem = JSON_Parse(pool, json_doc2, &size, &err);
    if (!elem){
		printf("  Error: json_verify_1() parse error\r\n");
		
		printf("=== %d, %d, %c\r\n", err.line, err.col, err.err_char);
		
		goto on_error;
    }
	printf("xxxx %d\r\n", elem->type);
	//解析单个元素
	switch(elem->type){
		case JSON_VAL_STRING:
		{
			printf("[%s] = %s\r\n", elem->name.ptr, elem->value.str.ptr);
			break;
		}
		case JSON_VAL_NUMBER:
		{
			//printf("[%s] = %f\r\n", c_strbuf(elem->name), elem->value.num);
			break;
		}		
		case JSON_VAL_BOOL:
		{
			//printf("[%s] = %d\r\n", c_strbuf(elem->name), elem->value.is_true);
			break;
		}		
		case JSON_VAL_NULL:
		{
			
			
			break;
		}		
		case JSON_VAL_ARRAY:
		{
			
			
			break;
		}		
		case JSON_VAL_OBJ:
		{
			printf("xxxxxxxxwwww\r\n");
			T_JsonElem *e = DL_ListGetNext(&elem->value.children.list, T_JsonElem, list);
			if (e){
				printf("qqqqqqq %d, %f\r\n", e->type, e->value.num);
			}
			
			
			break;
		}			
	}
	
	
	T_JsonElem a, obj;
	JSON_ElemSetObj(&obj, NULL);
	
	
	str_t key = c_str("test");
	str_t val = c_str("1234");
	JSON_ElemSetString(&a, &key, &val);
	
	JSON_ElemAdd(&obj, &a);
	
	size = 64;
    char * outbuf = POOL_Alloc(pool, size);

    if (JSON_ElemWrite(&obj, outbuf, &size)){
		printf("  Error: json_verify_1() write error\r\n");
		goto on_error;
    }

    printf("Json document:\n%s\r\n", outbuf);	
	
	
	
	
#if 0	
    size = (unsigned)strlen(json_doc2) * 2;
    out_buf = POOL_Alloc(pool, size);

    if (JSON_ElemWrite(elem, out_buf, &size)){
		printf("  Error: json_verify_1() write error\r\n");
		goto on_error;
    }

    printf("Json document:\n%s\r\n", out_buf);
#endif	
    POOL_Destroy(pool);
    return 0;

on_error:
    POOL_Destroy(pool);
    return 10;
}


void json_test(void)
{
    int rc;
    rc = json_verify_1();
    if (rc)
		printf("======= %d\r\n", rc);

    return;
}



