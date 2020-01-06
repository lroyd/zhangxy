/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include "uepoll.h"


/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
int UEPOLL_Create(int *_pOutEfd)
{
	*_pOutEfd = -1;
	*_pOutEfd = epoll_create(EPOLL_EVENT_NUM_MAX); 
	if(*_pOutEfd < 0)
	{
		printf( "epoll_create error");
		return -1;		
	}	

	return 0;
}

/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
int UEPOLL_Add(int _pEfd, int _pFd)
{
	struct epoll_event ev;
	ev.data.fd = _pFd;
	ev.events = (EPOLLIN);
	
	if(epoll_ctl(_pEfd, EPOLL_CTL_ADD, _pFd, &ev) == -1)
	{
		printf("epoll_ctl add error");
		return -1;
	}
	
	return 0;
}
/************************************************************************
* Name: 		
* Descriptions:
* Parameter:		
* Return:     
* **********************************************************************/
int UEPOLL_Del(int _pEfd, int _pFd) 
{
	if(epoll_ctl(_pEfd, EPOLL_CTL_DEL, _pFd, NULL) == -1)
	{
		printf("epoll_ctl del error");
		return -1;
	}
	
	return 0;
}


