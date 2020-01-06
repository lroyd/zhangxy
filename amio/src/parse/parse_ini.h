/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_PARSE_INI_H__
#define __ZXY_PARSE_INI_H__

#define INI_FIELD_NAME_LEN_MAX		(12)		//field 长度



#define PARSER_CONFIG(key, pool, path, name, field, out)	INI_##key##_ParserConfig(pool, path, name, field, out)


#endif
