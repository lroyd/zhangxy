#ifndef __ZXY_PORT_H__
#define __ZXY_PORT_H__

#include "typedefs.h"
#include "amio_dev_config.h"
#include "cstring.h"
#include "frame.h"
#include "dl_list.h"
#include "pool.h"



#define PORT_ARRAY_MAX		(3)		//最大级联等级

enum{
	PORT_FUNC_DATA,			//0
	
	PORT_FUNC_CREATE,		//1
	PORT_FUNC_DESTROY,		//2
	
	PORT_FUNC_START,		//3
	PORT_FUNC_STOP,			//4
	
	PORT_FUNC_CANCEL,		//5
};

//有bug，不支持动态增加,销毁后实体依然存在,可能导致后续无法连接
typedef struct _tagPortHandle
{
	PORT_DATA_CALLBCK	funcProcess;

	PORT_DATA_CALLBCK	funcCreate;
	PORT_DATA_CALLBCK	funcDestroy;

	PORT_DATA_CALLBCK	funcStart;
	PORT_DATA_CALLBCK	funcStop;
	
	PORT_DATA_CALLBCK	funcCancel;
}T_PortHandle;


typedef struct _tagPortInfo
{
	str_t				m_strName;
	void				*m_pUsrData;
	
	T_PortHandle		*m_onFrame;

	struct _tagPortInfo	*in_pMaster;
	int			m_u32Slot;

    struct _tagPortInfo	*in_apPortList[PORT_ARRAY_MAX];
}T_PortInfo;

//名字不可以未空
API_DEF(int)	PORT_Create(T_PoolInfo *_pPool, const char *_strName, void *_pUsrData, T_PortHandle *_pHandle, T_PortInfo **_ppOut);
API_DEF(int)	PORT_Destroy(T_PortInfo *_pPort);
API_DEF(int)	PORT_Connect(T_PortInfo *_pMaster, T_PortInfo *_pSub);
API_DEF(int)	PORT_Dispatch(char _enType, void *_pPort, T_Frame *_pFrame);
API_DEF(int)	PORT_Process(void *_pPort, T_Frame *_pFrame);

API_DEF(char*)	PORT_Dump0(T_PortInfo *_pPort, char *_pBuffer, unsigned _uSize);	//只打印当前一级子端口,简易log










#endif	/* __ZXY_PORT_H__ */

