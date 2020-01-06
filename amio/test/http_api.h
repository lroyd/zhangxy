#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__



#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_SER_PORT			(8001)

#define HTTP_RECV_DATA_LEN_MAX			(8*1024)

#define HTTP_CLIENT_MAX		(8)


int http_server_main(int argc, char* argv[]);
int http_parse_path(int fd, const char *recv_data);
void http_client_init(void);
void http_send_jpeg(char *data, unsigned long len);

int http_client_del(int fd);



#ifdef __cplusplus
}
#endif

#endif
