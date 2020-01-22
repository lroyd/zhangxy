

#include "negan_adapt.h"




T_NegSession *NEG_APT_AllocSession(T_NegDemo *d)
{
	T_NegSession *s = (T_NegSession *)calloc(1, sizeof(T_NegSession));
	if (NULL == s) {
		printf("alloc memory for rtsp_session failed\n");
		return NULL;
	}
	s->m_ptDemo = d;
	DL_ListInit(&s->m_dlDemo);
	DL_ListInit(&s->m_dlSession);
	DL_ListAddTail(&d->m_dlSession, &s->m_dlDemo);		
	
	/* 设置协议相关的session数据 */
	if (d->in_pProfile->funcSessionCreate){
		if (d->in_pProfile->funcSessionCreate(&s->m_pvData)){
			DL_ListAddDelete(&s->m_dlDemo);
			free(s);
			s = NULL;
		}		
	}
	
	return s;
}

void NEG_APT_FreeSession(T_NegSession *s)
{
	if (s){
		T_NegDemo *d = s->m_ptDemo;
		DL_ListAddDelete(&s->m_dlDemo);	//删除d->m_dlSession下的s->m_dlDemo
		
		/* 释放协议相关的session数据 */
		if (d->in_pProfile->funcSessionDestroy)
			d->in_pProfile->funcSessionDestroy(s->m_pvData);
		free(s);
	}
}


T_NegClient *NEG_APT_AllocClientConnection(T_NegDemo *d)
{
	T_NegClient *cc = (T_NegClient *)calloc(1, sizeof(T_NegClient));
	if (NULL == cc) {
		printf("alloc memory for rtsp_session failed\n");
		return NULL;
	}	
	cc->m_ptDemo		= d;
	DL_ListInit(&cc->m_dlClient);	//d->client
	DL_ListInit(&cc->m_dlSession);	//s->m_dlSession

	DL_ListAddTail(&d->m_dlClient, &cc->m_dlClient);	

	/* 分配client数据 */
	if (d->in_pProfile->funcClientCreate){
		if (d->in_pProfile->funcClientCreate(&cc->m_pvData)){
			DL_ListAddDelete(&cc->m_dlClient);
			free(cc);
			cc = NULL;
		}
	}
	
	return cc;
}

void NEG_APT_FreeClientConnection(T_NegClient *cc)
{
	if (cc){
		T_NegDemo *d = cc->m_ptDemo;
		DL_ListAddDelete(&cc->m_dlClient);	//删除d->m_dlClient下对应的cc->m_dlClient
		
		/* 释放client数据 */
		if (d->in_pProfile->funcClientDestroy)
			d->in_pProfile->funcClientDestroy(cc->m_pvData);	//rtsp还需要在里面释放rtp
		free(cc);	
	}
}


int NEG_APT_RecvMsg(T_NegClient *cc, void **out)
{
	int ret;
	if (sizeof(cc->reqbuf) - cc->reqlen - 1 > 0){
		ret = recv(cc->m_u32Sockfd, cc->reqbuf + cc->reqlen, sizeof(cc->reqbuf) - cc->reqlen - 1, MSG_DONTWAIT);
		if (ret == 0){
			printf("peer closed\n");
			return -1;
		}
		if (ret == -1){
			if (errno != EAGAIN && errno != EINTR) {
				printf("recv data failed: %d\n", errno);
				return -1;
			}
			ret = 0;
		}
		cc->reqlen += ret;
		cc->reqbuf[cc->reqlen] = 0;
	}

	if (cc->reqlen == 0 || ret == 0){
		return 0;
	}

	/* 协议解析 */
	T_NegDemo *d = cc->m_ptDemo;
	if (d->in_pProfile->funcRecvMsgParse){
		ret = d->in_pProfile->funcRecvMsgParse(cc->reqbuf, cc->reqlen, out);
		if (ret < 0) {
			printf("Invalid frame\n");
			return -1;
		}
		if (ret == 0) {
			return 0;
		}
	}

	memmove(cc->reqbuf, cc->reqbuf + ret, cc->reqlen - ret);
	cc->reqlen -= ret;
	printf("recv %d bytes message from %s, data = [%s][%d]\n", ret, inet_ntoa(cc->in_tPeerAddr), cc->reqbuf, cc->reqlen);
	return ret;
}

int NEG_APT_SendMsg(T_NegClient *cc, void *msg) 
{
	char szbuf[1024] = "";
	int ret;
	/* 协议打包 */
	T_NegDemo *d = cc->m_ptDemo;
	if (d->in_pProfile->funcSendMsgPack){
		ret = d->in_pProfile->funcSendMsgPack(msg, szbuf, sizeof(szbuf));
		if (ret < 0) {
			printf("msg_build_to_array failed\n");
			return -1;
		}
	}
	
	if (ret){
		/* 非阻塞发送 */
		ret = send(cc->m_u32Sockfd, szbuf, ret, 0);
		if (ret == -1) {
			printf("response send failed\n");
			return -1;
		}

		printf("sent %d bytes message to %s\n", ret, inet_ntoa(cc->in_tPeerAddr));		
	}
	
	return ret;
}







