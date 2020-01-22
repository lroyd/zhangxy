#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "negan.h"
#include "negan_adapt.h"






static int set_client_socket (int sockfd)
{
	int ret;
	int sndbufsiz = 1024 * 512;	//512k
	int keepalive = 1;
	int keepidle = 60;		//tcp空闲时间（无数据交互）
	int keepinterval = 3;	//两次探测间隔时间
	int keepcount = 5;		//判定断开的次数
	struct linger ling;

	ret = fcntl(sockfd, F_GETFL, 0);
	if (ret < 0) {
		printf("fcntl F_GETFL failed\n");
	} else {
		ret |= O_NONBLOCK;
		ret = fcntl(sockfd, F_SETFL, ret);
		if (ret < 0) {
			printf("fcntl F_SETFL failed\n");
		}
	}

	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&sndbufsiz, sizeof(sndbufsiz));
	if (ret == -1) {
		printf("setsockopt SO_SNDBUF failed\n");
	}

	ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepalive, sizeof(keepalive));
	if (ret == -1) {
		printf("setsockopt SO_KEEPALIVE failed\n");
	}

	ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (const char*)&keepidle, sizeof(keepidle));
	if (ret == -1) {
		printf("setsockopt TCP_KEEPIDLE failed\n");
	}

	ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (const char*)&keepinterval, sizeof(keepinterval));
	if (ret == -1) {
		printf("setsockopt TCP_KEEPINTVL failed\n");
	}

	ret = setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (const char*)&keepcount, sizeof(keepcount));
	if (ret == -1) {
		printf("setsockopt TCP_KEEPCNT failed\n");
	}

	memset(&ling, 0, sizeof(ling));
	ling.l_onoff = 1;
	ling.l_linger = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char*)&ling, sizeof(ling)); //resolve too many TCP CLOSE_WAIT
	if (ret == -1) {
		printf("setsockopt SO_LINGER failed\n");
	}

	return 0;
}



static T_NegDemo *alloc_demo(void)
{
	T_NegDemo *d = (T_NegDemo *)calloc(1, sizeof(T_NegDemo));
	if (NULL == d){
		printf("alloc memory for rtsp_demo failed\n");
		return NULL;
	}
	DL_ListInit(&d->m_dlSession);
	DL_ListInit(&d->m_dlClient);
	return d;
}

static void free_demo(T_NegDemo *d)
{
	if (d){
		free(d);
	}
}










T_NegDemo *NEG_Create(int _u32Port, int _u32Listen, T_NegProfile *_pProfile)
{
	T_NegDemo *d = NULL;
	struct sockaddr_in inaddr;
	int sockfd, enable = 1;
    int ret;
	
	//1.创建实体
	d = alloc_demo();
	if (NULL == d) return NULL;

	//2.初始化连接和会话队列
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1){
		printf("create tcp socket failed\n");
		free_demo(d);
		return NULL;
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(void *)&enable, sizeof (enable));
	
	if (_u32Port <= 0) _u32Port = 554;

	memset(&inaddr, 0, sizeof(inaddr));
	inaddr.sin_family = AF_INET;
	inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inaddr.sin_port = htons(_u32Port);
	ret = bind(sockfd, (struct sockaddr*)&inaddr, sizeof(inaddr));
	if (ret == -1){
		printf("bind socket to address failed\n");
		close(sockfd);
		free_demo(d);
		return NULL;
	}
	
	if (_u32Listen <= 0) _u32Listen = NEGAN_TCP_LISTEN_MAX;
	ret = listen(sockfd, _u32Listen);
	if (ret == -1){
		printf("listen socket failed\n");
		close(sockfd);
		free_demo(d);
		return NULL;
	}

	d->m_u32Sockfd	= sockfd;
	d->in_pProfile	= _pProfile;
	
	printf("negan starting on port %d\n", _u32Port);
	return d;	
}









T_NegSession *NEG_NewSession(T_NegDemo *_pInfo, const char *_strPath)
{
	T_NegDemo *d = (T_NegDemo *)_pInfo;
	T_NegSession *s = NULL;

	if (!d || !_strPath || strlen(_strPath) == 0) {
		printf("param invalid\n");
		goto fail;
	}
	
	//1.检查会话是否已经存在
	DL_ListForeach(s, &d->m_dlSession, T_NegSession, m_dlDemo){		
		if (!c_strcmp2(&s->m_strPath, _strPath)){
			printf("path:%s (%s) is exist!!!\n", c_strbuf(&s->m_strPath), _strPath);
			goto fail;
		}		
	}

	//2.创建会话
	s = NEG_APT_AllocSession(d);
	
	//5.复制路径
	c_cstr(&s->m_strPath, _strPath);
	printf("add session path: %s\n", c_strbuf(&s->m_strPath));
	
	return s;
fail:
	return NULL;
}




static void __client_connection_bind_session (T_NegClient *cc, T_NegSession *s)
{
	if (cc->m_ptSession == NULL){
		cc->m_ptSession = s;
		DL_ListAddTail(&s->m_dlSession, &cc->m_dlSession);
	}
}

static void __client_connection_unbind_session (T_NegClient *cc)
{
	T_NegSession *s = cc->m_ptSession;
	if (s){
		DL_ListAddDelete(&cc->m_dlSession);
		cc->m_ptSession = NULL;
	}
}


static T_NegClient *new_client_connection(T_NegDemo *d)
{
	T_NegClient *cc = NULL;
	struct sockaddr_in inaddr;
	int sockfd;
	socklen_t addrlen = sizeof(inaddr);

	//1.接收客户端连接
	sockfd = accept(d->m_u32Sockfd, (struct sockaddr*)&inaddr, &addrlen);
	if (sockfd == -1) 
	{
		printf("accept failed\n");
		return NULL;
	}
	//2.设置tcp属性
	set_client_socket(sockfd);//XXX DEBUG

	printf("new client %s:%u comming\n", inet_ntoa(inaddr.sin_addr), ntohs(inaddr.sin_port));

	//3.创建cli实体
	cc = NEG_APT_AllocClientConnection(d);
	
	cc->m_enState		= SES_CC_STATE_INIT;
	cc->m_u32Sockfd		= sockfd;
	cc->in_tPeerAddr	= inaddr.sin_addr;

	return cc;
}


static void del_client_connection(T_NegClient *cc)
{
	if (cc) {
		printf("delete client %d from %s\n", cc->m_u32Sockfd, inet_ntoa(cc->in_tPeerAddr));
		__client_connection_unbind_session(cc);

		close(cc->m_u32Sockfd);
		NEG_APT_FreeClientConnection(cc);
	}
}

void NEG_DelSession(T_NegSession *session)
{
	T_NegSession *s = (T_NegSession *)session;
	if (s){
		T_NegClient *cc = NULL, *tmp = NULL;
		
		//删除s->m_dlSession下的所有cc->m_dlSession
		DL_ListForeachSafe(cc, tmp, &s->m_dlSession, T_NegClient, m_dlSession){
			del_client_connection(cc);
		}
		
		printf("del session path: %s\n", c_strbuf(&s->m_strPath));

		NEG_APT_FreeSession(s);
	}
}


//删除rtsp实体
void NEG_Destroy(T_NegDemo *demo)
{
	T_NegDemo *d = (T_NegDemo *)demo;
	T_NegClient *cc = NULL, *tmp = NULL;
	T_NegSession *s = NULL, *n = NULL;
	if (d) {
		//删除d->m_dlClient下的所有cc->m_dlClient
		DL_ListForeachSafe(cc, tmp, &d->m_dlClient, T_NegClient, m_dlClient){
			del_client_connection(cc);
		}
		
		//删除d->m_dlSession下的所有cc->m_dlClient
		DL_ListForeachSafe(s, n, &d->m_dlSession, T_NegSession, m_dlDemo){
			NEG_DelSession(s);
		}

		close(d->m_u32Sockfd);
		free_demo(d);
	}
}



int NEG_doEvent(T_NegDemo *demo)
{
	T_NegDemo *d = demo;
	T_NegClient *cc = NULL, *cc1 = NULL;
	T_NegSession *s = NULL;
	struct timeval tv;
	fd_set rfds;
	fd_set wfds;
	int maxfd;
	int ret;

	if (NULL == d){
		return -1;
	}

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

	FD_SET(d->m_u32Sockfd, &rfds);

	maxfd = d->m_u32Sockfd;
	
	DL_ListForeach(cc, &d->m_dlClient, T_NegClient, m_dlClient){
		s = cc->m_ptSession;
		FD_SET(cc->m_u32Sockfd, &rfds);
		if (cc->m_u32Sockfd > maxfd) maxfd = cc->m_u32Sockfd;		
		
		if (cc->m_enState != SES_CC_STATE_PLAYING)
			continue;
		
		/* 检查设置相关输出fd */
		if (d->in_pProfile->funcSelectCheck){
			d->in_pProfile->funcSelectCheck(cc->m_pvData, &rfds, &wfds, &maxfd);
		}
	}
	
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	ret = select(maxfd + 1, &rfds, &wfds, NULL, &tv);
	if (ret < 0){
		printf("select failed\n");
		return -1;
	}
	
	if (ret == 0){
		return 0;	//超时返回
	}

	if (FD_ISSET(d->m_u32Sockfd, &rfds)) 	//有新链接进来
	{
		new_client_connection(d);
	}

	cc = DL_ListGetNext(&d->m_dlClient, T_NegClient, m_dlClient);
	while(cc)
	{
		cc1 = cc;
		s = cc1->m_ptSession;
		cc = DL_ListGetNext(&cc1->m_dlClient, T_NegClient, m_dlClient);	//当前连接的下一个
		if (cc == (T_NegClient *)&d->m_dlClient){
			cc = NULL;
		}
		
		if (FD_ISSET(cc1->m_u32Sockfd, &rfds)) 
		{
			do 	//必须是短链接
			{
				void *pReqMsg = NULL;
				ret = NEG_APT_RecvMsg(cc1, &pReqMsg);
				if (ret == 0){
					//无数据
					break;
				} 
					
				if (ret < 0){
					del_client_connection(cc1); //错误
					cc1 = NULL;
					break;
				}
				
				if (d->in_pProfile->funcMsgRequest){
					d->in_pProfile->funcMsgRequest(cc1, pReqMsg);
				}				

			} while (cc1);

			if (cc1 == NULL)
				continue;
		}

		if (cc1->m_enState != SES_CC_STATE_PLAYING)
			continue;
		
		if (d->in_pProfile->funcSelectProcess){
			d->in_pProfile->funcSelectProcess(cc1->m_pvData);
		}		
		
	}

	return 1;
}


