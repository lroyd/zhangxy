/*************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: 
	> Created Time: 
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "os.h"

#include "rtmp.h"   


#define RTMP_HEAD_SIZE		(sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)
#define RTMP_VIDEO_FPS		(30)	//30:25根据gop调正


typedef struct tag_meta_data  
{    
	unsigned int    tick;  
	unsigned int    gop;  
   
	unsigned int    sps_len;  
	unsigned char   *sps;
	unsigned int    pps_len;  
	unsigned char   *pps;   
	
	RTMPPacket		*head;
	
	RTMPPacket		*data;
}meta_data_t;

RTMP *rtmp = NULL;  
meta_data_t meta = {0};

static int tick = 0;

static int send_data(unsigned char key, char *data, unsigned int size, unsigned int pts)  
{
	int ret = -1, i = 0;
	RTMPPacket *packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + size + 9);
	if (packet == NULL) return ret;
	char *body	= NULL;
	memset(packet, 0, RTMP_HEAD_SIZE);
	
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (char *)packet->m_body;
	body[i++] = key ? 0x17 : 0x27;	//1:Iframe  7:AVC   2:Pframe  7:AVC   
	body[i++] = 0x01;
	body[i++] = 0x00;
	body[i++] = 0x00; 
	body[i++] = 0x00;
	body[i++] = size>>24 & 0xff;  
	body[i++] = size>>16 & 0xff;  
	body[i++] = size>>8 & 0xff;  
	body[i++] = size & 0xff;
	
	memcpy(&body[i], data, size);
	
	packet->m_nBodySize			= size + i;
	packet->m_hasAbsTimestamp	= 0;
	packet->m_packetType		= RTMP_PACKET_TYPE_VIDEO; /*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2		= rtmp->m_stream_id;
	packet->m_nChannel			= 0x04;
	packet->m_nTimeStamp		= pts;
	packet->m_headerType		= RTMP_PACKET_SIZE_LARGE;
	if (RTMP_IsConnected(rtmp)){
		if (key) RTMP_SendPacket(rtmp, meta.head, TRUE);
		ret = RTMP_SendPacket(rtmp, packet, TRUE); /*TRUE 为放进发送队列,FALSE 是不放进发送队列,直接发送*/
	}
	else{
		printf("what fuck?\r\n");
	}
	free(packet);
	return !ret;  
}  


static int create_key_head(char *pps, unsigned int pps_len, char *sps, unsigned int sps_len)
{
	int ret = -1, i = 0;
	char *body	= NULL;

	if (!meta.pps && pps){
		meta.pps_len = pps_len;
		meta.pps = (char*)malloc(pps_len);
		memcpy(meta.pps, pps, pps_len);
		printf("rtmp set pps ok\r\n");
	}
	
	if (!meta.sps &&sps){
		meta.sps_len = sps_len;
		meta.sps = (char*)malloc(sps_len);
		memcpy(meta.sps, sps, sps_len);	
		printf("rtmp set sps ok\r\n");
	}
	
	if (meta.head || !meta.sps || !meta.pps) return -1;

	printf("rtmp set head ok\r\n");
	meta.head = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + 128);
	RTMPPacket *packet = meta.head;
	memset(packet, 0, RTMP_HEAD_SIZE + 128);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (char *)packet->m_body;
	body[i++] = 0x17;
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	body[i++] = 0x01;
	body[i++] = meta.sps[1];
	body[i++] = meta.sps[2];
	body[i++] = meta.sps[3];
	body[i++] = 0xff;

	body[i++]   = 0xe1;
	body[i++] = (meta.sps_len >> 8) & 0xff;
	body[i++] = meta.sps_len & 0xff;
	memcpy(&body[i], meta.sps, meta.sps_len);
	i +=  meta.sps_len;
	
	body[i++]   = 0x01;
	body[i++] = (meta.pps_len >> 8) & 0xff;
	body[i++] = (meta.pps_len) & 0xff;
	memcpy(&body[i], meta.pps, meta.pps_len);
	i +=  meta.pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = rtmp->m_stream_id;

	return ret;
}

//创建全局变量
void rtmp_init(void)
{
	signal(SIGPIPE, SIG_IGN);
	if (rtmp) {
		RTMP_Free(rtmp);
	}
	rtmp = RTMP_Alloc();
	RTMP_Init(rtmp);

	rtmp->Link.timeout = 10;
}

//连接服务器
int rtmp_create(void *arg)
{
	char *url = (char *)arg;
	
	if (RTMP_SetupURL(rtmp, (char*)url) == FALSE) return -1;
	
	RTMP_EnableWrite(rtmp);
	
	if (RTMP_Connect(rtmp, NULL) == FALSE) return -1;
	
	if (RTMP_ConnectStream(rtmp, 0) == FALSE){
		RTMP_Close(rtmp);
		return -1;
	}	

	meta_data_t *p = &meta;
	p->tick = 0;
	p->gop = 1000/RTMP_VIDEO_FPS;
	
	return 0;
}



int rtmp_callbck(int type, char *p_data, int len, unsigned long long pts)
{
	meta_data_t *p = &meta;
	int key = 0, ret = -1;
	char *data = p_data + 4;
	int size = len - 4;
	switch(type){
		case 0x05:{
			key = 1;
		}
		case 0x01:{
			ret = send_data(key, data, size, p->tick);
			p->tick += p->gop;				
			break;
		}
		case 0x06:{
			ret = 0;				
			break;
		}		
		case 0x07:{
			create_key_head(NULL, 0, data, size);
			ret = 0;
			break;
		}
		case 0x08:{
			create_key_head(data, size, NULL, 0);
			ret = 0;
			break;
		}
	}

	return ret;
}

int rtmp_refresh(void *arg)
{
	rtmp_create(arg);
	return 0;
}