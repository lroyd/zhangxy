#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>


int select_version2(int *fd) {
    int c_fd = *fd, ready_n, error, cnt = 1;
	struct timeval timeout;
	socklen_t len = sizeof (error);
    fd_set wset;
	
	if (getsockopt(c_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		printf("getsockopt error.\r\n");
		return -1;
	}
	printf("111 write error = %d\r\n", error);
  
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	
	FD_ZERO(&wset);
	FD_SET(c_fd, &wset);
	
	ready_n = select(c_fd + 1, NULL, &wset, 0, &timeout);
	if (ready_n <= 0) 
	{
		printf("select time out (%d)\r\n", errno);
		return -1;
	}

	if (getsockopt(c_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		printf("getsockopt error.\r\n");
		return -1;
	}
	printf("222 write error = %d\r\n", error);
	
    return 0;
}


int select_version(int *fd) {
    int c_fd = *fd;
    fd_set rset, wset;
    struct timeval tval;
    FD_ZERO(&rset);
    FD_SET(c_fd, &rset);
    wset = rset;
    tval.tv_sec = 0;
    tval.tv_usec = 300 * 1000; //300ºÁÃë
    int ready_n;
    if ((ready_n = select(c_fd + 1, &rset, &wset, 0, NULL)) == 0) {
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
		printf("read error = %d\r\n", error);
    }
    if (FD_ISSET(c_fd, &wset)) {
        int error;
        socklen_t len = sizeof (error);
        if (getsockopt(c_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            printf("getsockopt error.\r\n");
            return -1;
        }
        printf("write error = %d\r\n", error);
    }
    return 0;
}

int epoll_version(int *fd) {
    int c_fd = *fd, i;
    int ep = epoll_create(1024);
    struct epoll_event event;
    event.events = (EPOLLIN | EPOLLOUT | EPOLLET);

    //event.data.ptr = (void*) &_data;
	event.data.fd = c_fd;
    epoll_ctl(ep, EPOLL_CTL_ADD, c_fd, &event);
	
    struct epoll_event eventArr[1000];
    int status, err;
    socklen_t len;
    err = 0;
    len = sizeof (err);
    int n = epoll_wait(ep, eventArr, 20, 300);
    for (i = 0; i < n; i++) {
        struct epoll_event ev = eventArr[i];
        int events = ev.events;
        if (events & EPOLLERR) {
            status = getsockopt(c_fd, SOL_SOCKET, SO_ERROR, &err, &len);
            printf("EPOLLERR error = %d\r\n", err);
        }
        if (events & EPOLLIN) {
            status = getsockopt(c_fd, SOL_SOCKET, SO_ERROR, &err, &len);
            printf("EPOLLIN error = %d\r\n", err);
        }
        if (events & EPOLLOUT) {
			printf("EPOLLOUT\r\n");
        }
    }

}

int main(int argc, char** argv) {
    char *ip = "192.168.1.164";
	//char *ip = "0.0.0.0";
    int port = 60001;
    int c_fd, flags, ret;
    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof (s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = inet_addr(ip);

	c_fd = socket(AF_INET, SOCK_STREAM, 0);
#if 1
	flags = fcntl(c_fd, F_GETFL, 0);
	fcntl(c_fd, F_SETFL, flags | O_NONBLOCK);
#endif

    ret = connect(c_fd, (struct sockaddr*) &s_addr, sizeof (struct sockaddr));
    if (ret < 0) {
		printf("connect error = %d", errno);
        if (errno == EINPROGRESS) {
			printf(" EINPROGRESS\r\n");
        } else {
            printf(" FUCK~~~\r\n"); 
        }
    }
    //select_version(&c_fd);
	select_version2(&c_fd);
	//epoll_version(&c_fd);
    return 0;
}