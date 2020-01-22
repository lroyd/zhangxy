
#include "negan.h"


typedef struct _sess_data{
	int s0;
	int s1;
}sess_data_t;

typedef struct _client_data{
	int c0;
	int c1;
}client_data_t;

/*****************************************************************************/
int sess_create(void **out)
{
	sess_data_t *s = (sess_data_t *)malloc(sizeof(sess_data_t));
	s->s0 = 1;
	s->s1 = 2;
	*out = (void *)s;
	return 0;
}

int sess_free(void *msg)
{
	free(msg);
	return 0;
}
/*****************************************************************************/
int client_create(void **out)
{
	client_data_t *c = (client_data_t *)malloc(sizeof(client_data_t));
	c->c0 = 3;
	c->c1 = 4;
	*out = (void *)c;
	return 0;
}

int client_free(void *msg)
{
	free(msg);
	return 0;
}
/*****************************************************************************/
int recv_msg(const void *data, int size, void **out)
{
	char *msg = (char *)malloc(size);
	memcpy(msg, data, size);
	*out = msg;
	return size;
}

int send_msg(void *msg, void *data, int size)
{
	int len = strlen(msg);
	memcpy(data, msg, len);
	
	return len;
}

int process_msg(T_NegClient *cc, void *msg)
{
	char *data = (char *)msg;
	printf("usr process data %s\r\n", data);
	
	close(cc->m_u32Sockfd);
	//NEG_APT_SendMsg(cc, "fuck you!!\r\n");

	return 0;
}
/*****************************************************************************/

T_NegProfile profile = {
	"test",5,
	sess_create,
	sess_free,
	
	client_create,
	client_free,
	
	recv_msg,
	send_msg,
	process_msg,
	
	NULL,
	NULL
};



T_NegDemo *demo;
T_NegSession *session;

int main(int argc, const char *argv[])
{
	demo = NEG_Create(60000, 0, &profile);
	if (demo == NULL){
		printf("error demo....\r\n");
		return 0;
	}

	session = NEG_NewSession(demo, "/live/sdp");
	if (session == NULL){
		printf("error session....\r\n");
		return 0;
	}	
	
	while(1){
		NEG_doEvent(demo);
		
		usleep(20*1000);
		//printf("000000\r\n");
		
	}


	return 0;
}

