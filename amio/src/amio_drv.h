/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_AMIO_DRV_H__
#define __ZXY_AMIO_DRV_H__

#include "amio.h"





typedef struct _tagDriver
{
    AMIO_DEV_FUNC_CREATE	pCreate;	//此函数，只创建底层dev的pool，若失败直接返回空，所以无需销毁函数
    T_AmioDevFactory		*in_pFactory;
	
    int						m_u32ChnlCnt;
	int						m_u32StartIdx;
	int						m_u32DefaultIdx;	//当前设备的默认channel 0+
}T_AmioDriver;


typedef struct _tagSubInfo
{
    unsigned char		m_u8InitCnt;	//多次初始化直接返回
    T_PoolFactory    	*pf;

    unsigned char		m_u8DrvTotal;	//设备驱动总数(包括失败个数)
	
    T_AmioDriver		m_atDriver[AMIO_DEVICE_MAX];

	unsigned char		m_u8ChnlToal;	//通道的总个数
    unsigned int	    m_au32ChnlList[AMIO_DEVICE_SUB_MAX];//有效chnl数组

}T_AmioSubInfo;



T_AmioSubInfo *AmioGetSubInfo(void);



#endif
