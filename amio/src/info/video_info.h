/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __ZXY_VIDEO_INFO_H__
#define __ZXY_VIDEO_INFO_H__

#include "cstring.h"

//视频尺寸属性
enum{
	VI_PIX_D1		= 4,
	VI_PIX_QVGA		= 6,	//320 * 240
	VI_PIX_VGA		= 7,	//640 * 480
	VI_PIX_XGA		= 8,	//1024 * 768
	VI_PIX_HD720	= 16,	//1280 * 720
	VI_PIX_HD1080	= 17	//1920 * 1080
};


typedef struct _tagVideoCorp
{
	//起始坐标
	int				x;	
	int				y;
	//裁剪大小
	int				w;
	int				h;	
}T_VideoCorp;

//旋转属性
enum{
	VI_ROTATE_0	= 0,
	VI_ROTATE_90,
	VI_ROTATE_180,
	VI_ROTATE_270,
	VI_ROTATE_BUTT
};

//格式属性
enum{
	VI_FMT_H264		= 96,
	VI_FMT_H265 	= 265,
	VI_FMT_MJPEG	= 1002
};

//264profile
enum{
	H264_PRO_BASELINE = 0,
	H264_PRO_MP,
	H264_PRO_HP,
};

//压缩属性
enum{
	COMP_MODE_NONE = 0,
	COMP_MODE_SEG,
	COMP_MODE_SEG128,
};

typedef struct _tagVideoChannel
{
	str_t			m_strName;
	int				m_u32ChnlId;	
	int				m_u32ChnlFd;	//vpss和venc通道号是一致的
	char			m_enType;
	char			m_enDir;
	
	char			m_enCap;
	void			*m_vpStream;

	/* 视频参数 */
	//全局属性
	char			m_enGroupSize;
	char			m_enRotate;	//旋转
	char			m_u8fps;	//帧率	
	
	T_VideoCorp		*in_pCorp;	//裁剪,

	//通道属性	
	char 			m_u8BlkSize;
	char			m_enCapPixSize;		//采集大小
	char			m_enVencPixSize;	//编码大小
	int				m_enChnnlFmt;		//1002必须使用int
	char			m_enProfile;		//目前只有264格式支持
	char			m_enComp;			//压缩类型		

}T_VideoChannel;




#endif
