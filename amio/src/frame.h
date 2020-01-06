#ifndef __ZXY_FRAME_H__
#define __ZXY_FRAME_H__

enum
{
    FRAME_TYPE_NONE,
    FRAME_TYPE_AUDIO,
    FRAME_TYPE_VIDEO,
	FRAME_TYPE_COMM,
	FRAME_TYPE_EXTENDED,
};

typedef struct _tagFrame
{
    char				m_u8Type;	//帧类型（比如264中p帧i帧）
	int					m_u32fd;	//真实物理通道fd
    void				*m_vpData;
    unsigned			m_uSize;
    unsigned long long	m_ullTs;
    int					bit_info;
}T_Frame;


#pragma pack(1)
typedef struct _tagFrameExt{
    T_Frame			base;
    //uint16_t     samples_cnt;
    //uint16_t     subframe_cnt;
}T_FrameExt;
#pragma pack()



typedef int (*PORT_DATA_CALLBCK)(void *_pUsrData, T_Frame *_pFrame);



#endif
