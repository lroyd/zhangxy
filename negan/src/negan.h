#ifndef __ZXY_NEGAN_H__
#define __ZXY_NEGAN_H__

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

#include "dl_list.h"
#include "cstring.h"


typedef struct _tagNegClient T_NegClient;


/* 协议回调设置 */


typedef struct _tagNegProfile
{
	str_t 			m_strName;	//profile name
	
	/* session +- */
	int (*funcSessionCreate)(void **_p2Out);
	int (*funcSessionDestroy)(void *_pData);
	/* client +- */
	int (*funcClientCreate)(void **_p2Out);
	int (*funcClientDestroy)(void *_pData);	
	/* msg +=- */
	int (*funcRecvMsgParse)(const void *data, int size, void **_p2Out);
	int (*funcSendMsgPack)(void *_pMsg, void *data, int size);
	int (*funcMsgRequest)(T_NegClient *cc, void *_pMsg);
	
	/* select[r-w] */
	int (*funcSelectCheck)(void *_pvData, fd_set *_pRset, fd_set *_pWset, int *_u32Maxfd);
	int (*funcSelectProcess)(void *_pvData);
}T_NegProfile;





typedef struct _tagNegDemo 
{
	T_DoubleList	m_dlSession;	//(down)s->m_dlDemo		
	T_DoubleList	m_dlClient;		//(down-down)client->m_dlClient
#define	NEGAN_TCP_LISTEN_MAX	(128);
	int				m_u32Sockfd;	

	T_NegProfile	*in_pProfile;
	
}T_NegDemo;


typedef struct _tagNegSession
{
	T_DoubleList	m_dlDemo;		//(up)demo->m_dlSession
	T_DoubleList	m_dlSession;	//(down)client->m_dlSession

	str_t 			m_strPath;	//相当于url
	T_NegDemo		*m_ptDemo;
	/******************************/
	void			*m_pvData;	//session数据	
}T_NegSession;


enum{
	SES_CC_STATE_INIT = 0,
	SES_CC_STATE_READY,
	SES_CC_STATE_PLAYING,
	SES_CC_STATE_RECORDING
};


struct _tagNegClient
{
	T_DoubleList	m_dlClient;	//(up-up)d->m_dlClient
	T_DoubleList	m_dlSession;//(up)s->m_dlSession
	
	char			m_enState;
	int 			m_u32Sockfd;
	
	struct in_addr	in_tPeerAddr;
	unsigned long	m_ulSessionId;

	char reqbuf[1024];	//协议请求缓冲
	int  reqlen;

	T_NegDemo		*m_ptDemo;
	T_NegSession	*m_ptSession;
	
	/******************************/
	void			*m_pvData;	//client数据
	
	
	
};


T_NegDemo *NEG_Create(int _u32Port, int _u32Listen, T_NegProfile *_pProfile);
void NEG_Destroy(T_NegDemo *demo);

T_NegSession *NEG_NewSession(T_NegDemo *d, const char *_strPath);	//_strPath必须是const “”
void NEG_DelSession(T_NegSession *s);

int NEG_doEvent(T_NegDemo *demo);	//接收的连接必须是短链接




#ifdef __cplusplus
}
#endif

#endif	/* __ZXY_NEGAN_H__ */

