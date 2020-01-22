#include "http_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

static http_parser *parser;

int on_message_begin(http_parser* p) 
{

	printf("\n***MESSAGE BEGIN***\n\n");
	return 0;
}

int on_headers_complete(http_parser* p) 
{
	int a = http_should_keep_alive(p);
	
	printf("\n***HEADERS COMPLETE : [%d]***\n\n", a);
	return 0;
}

int on_message_complete(http_parser* _) 
{
	(void)_;
	printf("\n***MESSAGE COMPLETE***\n\n");
	return 0;
}

int on_url(http_parser* _, const char* at, size_t length) 
{
	(void)_;
	printf("Url: %.*s\n", (int)length, at);
	return 0;
}

int on_header_field(http_parser* _, const char* at, size_t length) 
{
	(void)_;
	printf("Header field [%.*s : ", (int)length, at);
	return 0;
}

int on_header_value(http_parser* _, const char* at, size_t length) 
{
	(void)_;
	printf("%.*s]\n", (int)length, at);
	return 0;
}

int on_body(http_parser* _, const char* at, size_t length) 
{
	(void)_;
	printf("Body: %.*s\n", (int)length, at);
	return 0;
}

static http_parser_settings settings_null =  //http_parser的回调函数，需要获取HEADER后者BODY信息，可以在这里面处理。
{
	.on_message_begin	= on_message_begin,
	.on_header_field	= on_header_field,
	.on_header_value	= on_header_value,
	.on_url				= on_url,
	.on_status			= 0,
	.on_body			= on_body,
	.on_headers_complete = on_headers_complete,
	.on_message_complete = on_message_complete,
};

int main (void)
{
	const char *buf;
	int i;
	float start, end;
	size_t parsed;

	parser = malloc(sizeof(http_parser));  //分配一个http_parser
	printf("============================================================================\r\n");
	buf = "GET http://admin.omsg.cn/uploadpic/2016121034000012.png HTTP/1.1\r\nHost: admin.omsg.cn\r\nAccept: */*\r\nConnection: Keep-Alive\r\n\r\n";
	printf("%s\r\n", buf);
	printf("============================================================================\r\n");
	start = (float)clock()/CLOCKS_PER_SEC;
	http_parser_init(parser, HTTP_REQUEST);
	parsed = http_parser_execute(parser, &settings_null, buf, strlen(buf));
	end = (float)clock()/CLOCKS_PER_SEC;
	
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
	buf="HTTP/1.1 200 OK\r\n"
		"Date: Tue, 04 Aug 2009 07:59:32 GMT\r\n"
		"Server: Apache\r\n"
		"X-Powered-By: Servlet/2.5 JSP/2.1\r\n"
		"Content-Type: text/xml; charset=utf-8\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">\n"
		"  <SOAP-ENV:Body>\n"
		"    <SOAP-ENV:Fault>\n"
		"       <faultcode>SOAP-ENV:Client</faultcode>\n"
		"       <faultstring>Client Error</faultstring>\n"
		"    </SOAP-ENV:Fault>\n"
		"  </SOAP-ENV:Body>\n"
		"</SOAP-ENV:Envelope>";
	printf("%s\r\n", buf);
	printf("============================================================================\r\n");
	http_parser_init(parser, HTTP_RESPONSE);  //初始化parser为Response类型
	parsed = http_parser_execute(parser, &settings_null, buf, strlen(buf));  //执行解析过程

	free(parser);
	parser = NULL;

	printf("Elapsed %f seconds.\n", (end - start));

	return 0;
}