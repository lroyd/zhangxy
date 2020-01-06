#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/un.h>
#include <errno.h>


int select_version(int *fd) {
    int c_fd = *fd;
    fd_set rset, wset;
    struct timeval tval;
    FD_ZERO(&rset);
    FD_SET(c_fd, &rset);
    wset = rset;
    tval.tv_sec = 0;
    tval.tv_usec = 300 * 1000; //300毫秒
    int ready_n;
    if ((ready_n = select(c_fd + 1, &rset, &wset, 0, &tval)) == 0) {
        close(c_fd); /* timeout */
        errno = ETIMEDOUT;
        perror("select timeout.\n");
        return (-1);
    }
    if (FD_ISSET(c_fd, &rset)) {
        int error;
        socklen_t len = sizeof (error);
        if (getsockopt(c_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			printf("getsockopt error.\r\n");
            return -1;
        }
		printf("in fire. 111 error = %d\r\n", error);
    }
    if (FD_ISSET(c_fd, &wset)) {
        int error;
        socklen_t len = sizeof (error);
        if (getsockopt(c_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            printf("getsockopt error.\r\n");
            return -1;
        }
        printf("in fire. error = %d\r\n", error);
    }
    return 0;
}

int main()
{
    int ssock = -1, csock0 = -1, csock1 = -1, enable = 1;
	ssock = socket(AF_INET,SOCK_STREAM, 0);
	//setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR,(void *)&enable, sizeof (enable));
	
    char *ip = "192.168.1.164";
	//char *ip = "0.0.0.0";
    int port = 60001;	
	
#if 0	
    enable = 1;
    if ((ioctl(ssock, FIONBIO, (unsigned long)&enable))) 
    {
		printf("pj_ioqueue_register_sock error: ioctl\r\n");
		return -1;
    }
#else	
	enable = fcntl(ssock, F_GETFL, 0);
	fcntl(ssock, F_SETFL, enable | O_NONBLOCK);	
	
#endif

#if 0
int rc;
	printf("222222222\r\n");
	rc = send(ssock, "111111", 6, 0);
	printf("rc = %d\r\n", rc);
	if (rc < 0){
		printf("send error %d, %d(%d)\r\n", ssock, errno, EAGAIN);
		if (errno == EAGAIN){
			printf("send PENDING.....\r\n");
		}
		else{
			printf("send xxxxxxxxxxxxxx\r\n");
		}		
	}
#endif

#if 0
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);	//inet_addr("0.0.0.0");
    addr.sin_port = htons(60001);
    if(bind(ssock, (struct sockaddr *)&addr, sizeof(addr))==-1)
    {
        perror("bind error");
        return -1;
    }	
	
	printf("local ip = %s, port = %d\r\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	
	
	listen(ssock, 5);
	char ip[24] = {0};
	struct sockaddr_in remote;
	int remote_len = sizeof(remote);	
	if ((csock0 = accept(ssock, (struct sockaddr *)&remote, (socklen_t *)&remote_len)) >= 0){
		//strcpy(ip, inet_ntoa(remote.sin_addr));
		printf("accept new fd = %d, ip = %s, port = %d\r\n", csock0, inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
	}
	else{
		printf("accept error %d, %d(%d)\r\n", csock0, errno, EAGAIN);
		if (errno == EAGAIN){
			printf("accept PENDING.....\r\n");
		}
		else{
			printf("accept xxxxxxxxxxxxxx\r\n");
		}
		
	}
#endif


#if 0
        int error, status;
        socklen_t len = sizeof (error);

	
	struct sockaddr_in client_addr;
	
#if 1	

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(ip);
    client_addr.sin_port = htons(port);
    if(connect(ssock, (struct sockaddr *)&client_addr, sizeof(client_addr)))
    {
        perror("connect error");
		if (errno == EINPROGRESS){
			printf("EINPROGRESS \r\n");
		}
    }
	
	status = getsockopt(ssock, SOL_SOCKET, SO_ERROR, &error, &len);
	printf("xxxxxxxxxxxxx status = %d, error = %d\r\n", status, error);
	
#else
	
    status = connect(ssock, (struct sockaddr*) &client_addr, sizeof(client_addr));
    while (status < 0) {
        if (errno == EINPROGRESS) {
            break;
        } else {
            perror("connect remote server fail.\n");
            printf("%d\n", errno);
            exit(0);
        }
    }	
	select_version(&ssock);
#endif	
	while(1)
	{
		sleep(1);
	}
#endif
	
	struct sockaddr_in sockaddr; 
	sockaddr.sin_family = AF_INET; 
	sockaddr.sin_port = htons(port); 
	sockaddr.sin_addr.s_addr = inet_addr(ip); 
	socklen_t length = sizeof(sockaddr);
	int ret = connect(ssock, (struct sockaddr*)&sockaddr, length);
	if (ret == -1)
	{	
		printf("xxxxxxxxxxxxxxxxx %d\r\n",errno);
		if (errno == EINPROGRESS)
		{
			//正在连接
			fd_set wset;
			int optval;
			socklen_t optlen = sizeof(optval);
			struct timeval tv;
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			
			/* wait to be available in writing */
			FD_ZERO(&wset);
			FD_SET(ssock, &wset);
			ret = select(ssock + 1, NULL, &wset, NULL, &tv);
			if (ret <= 0) 
			{
				/* timeout or fail */
				printf("net client connect timeout error\r\n");
				close(ssock);
				return -1;
			}
			/* the connection is established if SO_ERROR and optval are set to 0 */
			ret = getsockopt(ssock, SOL_SOCKET, SO_ERROR, (void *)&optval, &optlen);
			if (ret == 0 && optval == 0) 
			{
				printf("net tcp client create ssock %d\r\n",ssock);
				
				while(1){
					sleep(1);
				}
				return ssock;
			} 
			else 
			{
				errno = ECONNREFUSED;
				printf("net client connect refused error\r\n");
				close(ssock);
				return -1;
			}
		}
		else
		{
			printf("net client connect sock error\r\n");
			close(ssock);
			return -1;
		}

	}
	else
	{
		printf("net client create ssock %d\r\n",ssock);
	}		
	
	
	
	return 0;
}