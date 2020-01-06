#ifndef __USR_EPOLL_H__
#define __USR_EPOLL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/epoll.h>


#define EPOLL_EVENT_NUM_MAX		(16)



int UEPOLL_Create(int *_pOutEfd);

int UEPOLL_Add(int _iEfd, int _iFd);

int UEPOLL_Del(int _iEfd, int _iFd);





#ifdef __cplusplus
}
#endif

#endif
