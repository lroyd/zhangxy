#ifndef __ZXY_AKI_BUFFER_H__
#define __ZXY_AKI_BUFFER_H__

#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************/
//#define AKI_USE_OSLIB
//#define AKI_USE_LOCK  
//#define AKI_USE_TASK 
/*************************************************************************/

//��֧�ֶ�̬���䣬��Ϊ���ü�¼�ͷ�
enum{
	AKI_BUF_STATUS_FREE,
	AKI_BUF_STATUS_UNLINKED,
	AKI_BUF_STATUS_QUEUED
};

#define AKI_POOL_PUBLIC         	(0)
#define AKI_POOL_RESTRICTED     	(1)

#define AKI_BUFFER_POOLS_TOTAL_MAX	(6)
#define AKI_ALIGN_SIZE(s) 			((((s) + 3)/sizeof(int))* sizeof(int))

#define AKI_BUFFER_HDR_SIZE			(sizeof(T_AkiPackHdr))
#define AKI_BUFFER_PADDING_SIZE		(sizeof(T_AkiPackHdr) + sizeof(int))
#define AKI_BUFFER_SIZE_MAX			((unsigned short)0xFFFF - AKI_BUFFER_PADDING_SIZE)

  
/**************************************************************************/
typedef struct _tagBufferHdr {
	struct _tagBufferHdr	*m_pNext;	//��һ����
    char m_u8PoolId;		//��ǰpool id
    char m_u8TaskId;	//task id������ʹ��
    char m_u8Status;		//��ǰbuffer״̬
    char m_u8Type;		//δʹ��	
}T_AkiPackHdr;


typedef struct _tagFreeq
{
    T_AkiPackHdr *in_pFirst;	//��¼��ǰ��һ�����ã����ƶ�
    T_AkiPackHdr *in_pLast;

    unsigned short    m_u16PackSize;		//ÿ������ʹ�ô�С
    unsigned short    m_u16PackTotal;		//���ĸ���
    unsigned short    m_u16PackCurCnt;	//��ʹ�ð��ĸ���
    unsigned short    m_u16PackMaxCnt;	//��ʷʹ����������

}T_AkiFreeq;

typedef struct _tagAkiBuffer
{
    T_AkiFreeq		in_atFreeq[AKI_BUFFER_POOLS_TOTAL_MAX];

    char			*m_apPoolStart[AKI_BUFFER_POOLS_TOTAL_MAX];
    char			*m_apPoolEnd[AKI_BUFFER_POOLS_TOTAL_MAX];
    unsigned short	m_ausPoolSize[AKI_BUFFER_POOLS_TOTAL_MAX];	//��ͷ�����65535��65k

    unsigned short	m_u16PoolAccessMask;
    char			m_aucPoolList[AKI_BUFFER_POOLS_TOTAL_MAX];	//����Ȩ��ʹ��
	
    char			m_u8CurrTotalPools;
	pthread_mutex_t m_tLock;
	
}T_AkiBuffer;


/**************************************************************************/

void AKI_BufferInit(T_AkiBuffer *_pAkiBuffer);

//id:0+
int AKI_BufferCreatePool(T_AkiBuffer *_pAki, void *_pUAddress, unsigned short _u16USize, unsigned short _u16Size, char *_pOut);

void *AKI_BufferDestroyPool(T_AkiBuffer *_pAki, char _u8Pid);

unsigned short AKI_PoolGetTotal(T_AkiBuffer *_pAki, char _u8Pid);

unsigned short AKI_PoolGetFreeTotal(T_AkiBuffer *_pAki, char _u8Pid);


void *AKI_GetPackage(T_AkiBuffer *_pAki, unsigned short _u16Size);
void AKI_PutPackage(T_AkiBuffer *_pAki, void *_pPack);

void *AKI_CheckStartAddr(T_AkiBuffer *_pAki, void *_pUsrAddr);
void *AKI_PoolGetPackage(T_AkiBuffer *_pAki, char _u8Pid);

//debug
char *AKI_BufferPrint(T_AkiBuffer *_pAki, char *_pBuf, int _u32Len);
char *AKI_BufferPrintUsage(T_AkiBuffer *_pAki, char *_pBuf, int _u32Len);
char *AKI_PoolStatistics(T_AkiBuffer *_pAki, char _u8Pid, char *_pBuf, int _u32Len);


#ifdef __cplusplus
}
#endif


#endif


