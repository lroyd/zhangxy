/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_COMM_INFO_H__
#define __ZXY_COMM_INFO_H__

#include "cstring.h"


typedef struct _tagCommChannel
{
	str_t			m_strName;
	int				m_u32ChnlId;	
	int				m_u32ChnlFd;
	char			m_enType;
	char			m_enDir;
	
	char			m_enCap;
	void			*m_vpStream;

}T_CommChannel;









#endif	
