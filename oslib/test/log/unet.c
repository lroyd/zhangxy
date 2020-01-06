#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "unet.h"

//server肯定是本地的不需要ip地址
int NETTCP_ServerOpen(int _u32Port, int *_pOutFd)
{
	int u32Sock = -1, u32Enable = 1;
	u32Sock = socket(AF_INET,SOCK_STREAM, 0);
	setsockopt(u32Sock, SOL_SOCKET, SO_REUSEADDR,(void *)&u32Enable, sizeof (u32Enable));
	
    struct sockaddr_in tServAddr;
    tServAddr.sin_family = AF_INET;
    tServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tServAddr.sin_port = htons(_u32Port);
 
    if(bind(u32Sock, (struct sockaddr *)&tServAddr, sizeof(tServAddr))==-1)
    {
        perror("bind error");
        return -1;
    }
 
    if(listen(u32Sock, 5) == -1)
    {
        perror("listen error");
        return -1;
    }
	*_pOutFd = u32Sock;
	
	return 0;
}



int NETTCP_ClientConnect(char *_pDestAddr, int _pDestPort, int *_pOutFd)
{
    int u32Sock = socket(AF_INET,SOCK_STREAM, 0);

    struct sockaddr_in tDestAddr;
    memset(&tDestAddr, 0, sizeof(tDestAddr));
    tDestAddr.sin_family = AF_INET;
    tDestAddr.sin_addr.s_addr = inet_addr(_pDestAddr);
    tDestAddr.sin_port = htons(_pDestPort); 

    if (connect(u32Sock, (struct sockaddr *)&tDestAddr, sizeof(tDestAddr)) < 0)
    {
        perror("connect error");
		close(u32Sock);
		return -1;
    }
	*_pOutFd = u32Sock;
	return 0;
}
//长连接发送
int NETTCP_ClientWrite(int _i32Sock, char *_pData, unsigned int _iLen)
{
	int iTotal = 0;
	iTotal = send(_i32Sock, _pData, strlen(_pData),0);
	return iTotal;
}


//单次发送
int NETTCP_ClientSend(char *_pDestAddr, int _pDestPort, char *_pData, unsigned int _iLen)
{
    int iRet, iTotal = 0;

    int u32Sock = socket(AF_INET,SOCK_STREAM, 0);

    struct sockaddr_in tDestAddr;
    memset(&tDestAddr, 0, sizeof(tDestAddr));
    tDestAddr.sin_family = AF_INET;
    tDestAddr.sin_addr.s_addr = inet_addr(_pDestAddr);
    tDestAddr.sin_port = htons(_pDestPort); 

    if (connect(u32Sock, (struct sockaddr *)&tDestAddr, sizeof(tDestAddr)) < 0)
    {
        perror("connect error");
		close(u32Sock);
		return -1;
    }
	
	iTotal = send(u32Sock, _pData, strlen(_pData),0);
	
	printf("tcp client to %s:%d, %d, %s\r\n", _pDestAddr, _pDestPort, iTotal, _pData);
	
	close(u32Sock);
	
    return iTotal;
}


int NETUDP_ServerOpen(int _u32Port, int *_pOutFd)
{
	int u32Sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	int enable = 1;
	setsockopt(u32Sock, SOL_SOCKET, SO_REUSEADDR,(void *)&enable, sizeof (enable));

    struct sockaddr_in tServAddr;
    tServAddr.sin_family = AF_INET;
    tServAddr.sin_addr.s_addr = htonl(INADDR_ANY);	//server 不判定path，默认0.0.0.0
    tServAddr.sin_port = htons(_u32Port);

    if(bind(u32Sock, (struct sockaddr *)&tServAddr, sizeof(tServAddr))== -1)
    {
		printf("echo server bind error");
        return -1;
    }		
	
	*_pOutFd = u32Sock;
	return 0;
}



