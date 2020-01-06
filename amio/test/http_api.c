#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/wait.h>


#include "unet.h"
#include "uepoll.h"

#include "http_api.h"


enum http_stream_type {
    STREAM_H264,
    STREAM_JPEG,
    STREAM_MJPEG,
    STREAM_MP4
};

struct http_client_info {
    int socket_fd;
    enum http_stream_type type;
};

struct http_client_info client_fds[HTTP_CLIENT_MAX];
pthread_mutex_t client_fds_mutex;


const char *RESPONSE_200_OK_FULL_IMAGE = "HTTP/1.1 200 OK\r\n"
                         "Connection: Close\r\n"
                         "Content-length: 106\r\n"
                         "Content-type: text/html\r\n"
                         "\r\n"
                         "<html><body marginheight='0' marginwidth='0'><img width='100%' height='100%' src='./video.mjpg'></body></html>";

const char *RESPONSE_200_OK_HEADER = "HTTP/1.1 200 OK\r\n"
                         "Connection: Keep-Alive\r\n"
                         "Content-Type: multipart/x-mixed-replace;boundary=--dcmjpeg\r\n"
                         "Expires: 0\r\n"
                         "Vary: *\r\n"
                         "Cache-Control: no-store, must-revalidate\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "Access-Control-Allow-Methods: GET\r\n"
                         "\r\n";

const char *RESPONSE_200_OK = "HTTP/1.1 200 OK\r\n"
                         "Connection: Close\r\n"
                         "Content-length: 83\r\n"
                         "Content-type: text/html\r\n"
                         "\r\n"
                         "<html><body marginheight='0' marginwidth='0'><img src='./video.mjpg'></body></html>";
const char *RESPONSE_ERROR = "HTTP/1.1 404 Not Found\r\n"
                         "Connection: Close\r\n"
                         "Content-length: 171\r\n"
                         "Content-type: text/html\r\n"
                         "\r\n"
                         "<html><head><title>Document Error: Site or Page Not Found</title></head>\r\n"
                         "\t\t<body><h2>Access Error: Site or Page Not Found</h2>\r\n"
                         "\t\t<p>Cannot open URL</p></body></html>\r\n"
                         "\r\n";

void http_send_jpeg(char *buf, unsigned long size) 
{
	int i;
    char prefix_buf[128] = {0}, suffix_buf[32] = {0};
	unsigned long prefix_size = sprintf(prefix_buf, "--dcmjpeg\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", size);
    //unsigned long suffix_size = sprintf(suffix_buf, "\r\n--" "boundarydonotcross" "\r\n");

    pthread_mutex_lock(&client_fds_mutex);
    for (i = 0; i < HTTP_CLIENT_MAX; ++i) {
        if (client_fds[i].socket_fd < 0) continue;
        if (send_to_client(i, prefix_buf, prefix_size) < 0) continue; 
        if (send_to_client(i, buf, size) < 0) continue; 
		
		//if (send_to_client(i, suffix_buf, suffix_size) < 0) continue;
		
    }
    pthread_mutex_unlock(&client_fds_mutex);
}




void close_socket_fd(int socket_fd) 
{
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
}

static void free_client(int i) 
{
    if (client_fds[i].socket_fd < 0) 
		return;
    close_socket_fd(client_fds[i].socket_fd);
    client_fds[i].socket_fd = -1;
}

static int send_to_fd(int client_fd, char* buf, unsigned long size) 
{
    unsigned int sent = 0, len = 0;
    if (client_fd < 0) return -1;
    while (sent < size) {
        len = send(client_fd, buf + sent, size - sent, MSG_NOSIGNAL);
        if (len < 0) {
			return -1;
		}
        sent += len;
    }
    return 0;
}

int send_to_client(int i, char* buf, unsigned long size) 
{
    if (send_to_fd(client_fds[i].socket_fd, buf, size) < 0) { 
		free_client(i); 
		return -1; 
	}
    return 0;
}


int http_client_del(int fd)
{
	int ret = -1, i;
    pthread_mutex_lock(&client_fds_mutex);
    for (i = 0; i < HTTP_CLIENT_MAX; ++i) {
        if (client_fds[i].socket_fd == fd){
			free_client(i);
			ret = 0;
			break;
		}
    }
    pthread_mutex_unlock(&client_fds_mutex);	
	return ret;
}

int http_parse_path(int fd, const char *recv_data)
{
	int ret = -1, i, find = 0;
	if (strncmp(recv_data, "GET", 3) == 0) {
        if (strstr(recv_data, "video.mjpg") != NULL) {
			send_to_fd(fd, RESPONSE_200_OK_HEADER, strlen(RESPONSE_200_OK_HEADER));
            pthread_mutex_lock(&client_fds_mutex);

			for (i = 0; i < HTTP_CLIENT_MAX; ++i){
				if (client_fds[i].socket_fd == fd){
					find = 1;
					break;
				}
			}
			
			if (find == 0){
				for (i = 0; i < HTTP_CLIENT_MAX; ++i){
					if (client_fds[i].socket_fd < 0) {
						client_fds[i].socket_fd = fd; 
						client_fds[i].type = STREAM_MJPEG; 
						printf("client fd = %d ->add\r\n", fd);
						break; 
					}				
				}
				
				if (i == HTTP_CLIENT_MAX){
					close(fd);
				}
			}
			
            pthread_mutex_unlock(&client_fds_mutex);			
			ret = 0;
        }
		else if (strstr(recv_data, "mjpeg.html") != NULL) {
            if (send(fd, RESPONSE_200_OK, strlen(RESPONSE_200_OK), 0) == -1)
                printf("send failed\n");
            printf("mjpeg.html lcfd=%d\n", fd);

        } 
		else if (strstr(recv_data, "mjpegFullScreen.html") != NULL) {
            int written = write(fd, RESPONSE_200_OK_FULL_IMAGE, strlen(RESPONSE_200_OK_FULL_IMAGE));
            if (written < 0) {
                printf("write 200 failed\n");
            }
			else if (written < strlen(RESPONSE_200_OK_FULL_IMAGE)) {
                printf("not all data write ok 200\n");
            }
        } 
		else {
            printf("no found page! fd=%d\n", fd);
            if (send(fd, RESPONSE_ERROR, strlen(RESPONSE_ERROR), 0) == -1)
                printf("send failed\n");
        }
    }

	return ret;
}


void http_client_init(void)
{
	int i;
    for (i = 0; i < HTTP_CLIENT_MAX; ++i) {
		client_fds[i].socket_fd = -1; 
		client_fds[i].type = -1; 
	}
	pthread_mutex_init(&client_fds_mutex, NULL);
}


int http_server_main(int argc, char* argv[])
{
	int ret = -1, nfds, i,j , efd = -1;
	int http_fd = -1, cfd = -1;
	static unsigned int count = 0;
	struct epoll_event	events[EPOLL_EVENT_NUM_MAX];  
	pthread_t client;
	UEPOLL_Create(&efd);	
	NETTCP_ServerOpen(HTTP_SER_PORT, &http_fd);
	
	if (efd == -1 || http_fd == -1){
		printf("open efd = %d, http = %d error!!!\r\n", efd, http_fd);
		return -1;
	}	
	UEPOLL_Add(efd, http_fd);
	
	http_client_init();
	
	while(1){
		nfds = epoll_wait(efd, events, EPOLL_EVENT_NUM_MAX, 2000);  //1000ms = 1s
		if (nfds <= 0){
			if (count++ >= 7200){
				return 0;
			}
		}
		else{
			for (i = 0; i < nfds; i++){
				if (events[i].data.fd == http_fd){
					struct sockaddr_in client_addr;
					socklen_t length = sizeof(client_addr);
					cfd = accept(http_fd, (struct sockaddr*)&client_addr, &length);
					if (UEPOLL_Add(efd, cfd)){
						printf("conn_fd = %d, epoll_add error\r\n", cfd);
						close(cfd);
						continue;						
					}					
				}	
				else{
					char recv_buffer[HTTP_RECV_DATA_LEN_MAX] = {0};
					//memset(recv_buffer, 0, HTTP_RECV_DATA_LEN_MAX);
					ret = read(events[i].data.fd, recv_buffer, HTTP_RECV_DATA_LEN_MAX); 
					if(ret <= 0){
						UEPOLL_Del(efd, events[i].data.fd);
						printf("client fd = %d ->close\r\n", events[i].data.fd);
						close(events[i].data.fd);
					}
					else{
						if(http_parse_path(events[i].data.fd, recv_buffer)) { 

						};						
					}					
				}
				
			}
		}			
	}
	return 0;
}






