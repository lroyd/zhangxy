/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include <stdio.h>
#include "port.h"

struct arg_info
{
	char *a;
	int b;
	int cancel;
};

int port_process(void *arg, T_Frame *frame)
{
	T_PortInfo *port = (T_PortInfo *)arg;
	struct arg_info *info = (struct arg_info *)port->m_pUsrData;
	printf("%s [%s] %d\r\n",__FUNCTION__, c_strbuf(&port->m_strName), info->b);
	
	return 0;
}

int port_create(void *arg, T_Frame *frame)
{
	T_PortInfo *port = (T_PortInfo *)arg;
	struct arg_info *info = (struct arg_info *)port->m_pUsrData;
	printf("%s [%s] %d\r\n",__FUNCTION__, c_strbuf(&port->m_strName), info->b);
	
	return 0;
}

int port_destroy(void *arg, T_Frame *frame)
{
	T_PortInfo *port = (T_PortInfo *)arg;
	struct arg_info *info = (struct arg_info *)port->m_pUsrData;
	printf("%s [%s] %d\r\n",__FUNCTION__, c_strbuf(&port->m_strName), info->b);
	
	return 0;
}

int port_start(void *arg, T_Frame *frame)
{
	T_PortInfo *port = (T_PortInfo *)arg;
	struct arg_info *info = (struct arg_info *)port->m_pUsrData;
	printf("%s [%s] %d\r\n",__FUNCTION__, c_strbuf(&port->m_strName), info->b);
	
	return 0;
}

int port_stop(void *arg, T_Frame *frame)
{
	T_PortInfo *port = (T_PortInfo *)arg;
	struct arg_info *info = (struct arg_info *)port->m_pUsrData;
	printf("%s [%s] %d\r\n",__FUNCTION__, c_strbuf(&port->m_strName), info->b);
	
	return 0;
}

int port_cancel(void *arg, T_Frame *frame)
{
	T_PortInfo *port = (T_PortInfo *)arg;
	struct arg_info *info = (struct arg_info *)port->m_pUsrData;
	printf("%s [%s] %d\r\n",__FUNCTION__, c_strbuf(&port->m_strName), info->b);
	
	return 0;
}

T_PortHandle port_handle={
	port_process,
	
	port_create,
	port_destroy,
	
	port_start,
	port_stop,
	
	port_cancel
};


int port_test(void) 
{
	THREAD_Init();
	T_PoolCaching cp;
	T_PoolInfo *pool;
	POOL_CachingInit(&cp, NULL, 0);
	pool = POOL_Create(&cp.in_pFactory, "port", 1000, 1000, NULL);	
	
	//创建
	enum{PORT_MAX = 5};
	struct arg_info arg[PORT_MAX] = {0};
	T_PortInfo *port[PORT_MAX];
	int i, ret = 0;char buf[32] = {0};
	for (i = 0; i < PORT_MAX; i++){
		sprintf(buf, "port%d", i);
		arg[i].b = i + 10;
		ret = PORT_Create(pool, buf, &arg[i], &port_handle, &port[i]);
		if (ret){
			printf("PORT_Create error %s\r\n", buf);
		}
	}
	
	//连接
	PORT_Connect(port[0], port[1]);
	PORT_Connect(port[1], port[3]);
	PORT_Connect(port[0], port[4]);
	PORT_Connect(port[0], port[2]);
	
	//打印
	char buff[64] = {0};
	printf("%s\r\n", PORT_Dump0(port[0], buff, 64));
	
	//执行
	printf("==========================================================\r\n");
	PORT_Dispatch(PORT_FUNC_DATA, port[0], NULL);
	
	printf("==========================================================\r\n");
	PORT_Dispatch(PORT_FUNC_CREATE, port[0], NULL);
	printf("==========================================================\r\n");
	PORT_Dispatch(PORT_FUNC_DESTROY, port[0], NULL);
	printf("==========================================================\r\n");
	PORT_Dispatch(PORT_FUNC_START, port[0], NULL);
	printf("==========================================================\r\n");
	PORT_Dispatch(PORT_FUNC_STOP, port[0], NULL);
	PORT_Destroy(port[3]);
	printf("==========================================================\r\n");
	PORT_Dispatch(PORT_FUNC_CANCEL, port[0], NULL);	
	
    return 0;
}









