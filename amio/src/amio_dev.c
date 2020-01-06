/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#include "amio_dev.h"
#include "amio_drv.h"
#include "amio_dev_config.h"


/************************************************/

#if AMIO_DEV_HAS_NULL
T_AmioDevFactory *AmioNullDevice(T_PoolFactory *_pPool);
#endif

#if AMIO_DEV_HAS_HIAUDIO
T_AmioDevFactory *AmioHiaudioDevice(T_PoolFactory *_pPool);
#endif

#if AMIO_DEV_HAS_HIVIDEO
T_AmioDevFactory *AmioHivideoDevice(T_PoolFactory *_pPool);
#endif

int device_load(T_AmioSubInfo *_pInfo)
{
#if AMIO_DEV_HAS_NULL
    _pInfo->m_atDriver[_pInfo->m_u8DrvTotal++].pCreate = &AmioNullDevice;
#endif	

#if AMIO_DEV_HAS_HIAUDIO
	_pInfo->m_atDriver[_pInfo->m_u8DrvTotal++].pCreate = &AmioHiaudioDevice;
#endif

#if AMIO_DEV_HAS_HIVIDEO
	_pInfo->m_atDriver[_pInfo->m_u8DrvTotal++].pCreate = &AmioHivideoDevice;
#endif

    return _pInfo->m_u8DrvTotal;
}











