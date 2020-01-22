#ifndef __ZXY_NEGAN_APT_H__
#define __ZXY_NEGAN_APT_H__

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

#include "negan.h"


T_NegSession *NEG_APT_AllocSession(T_NegDemo *d);
void NEG_APT_FreeSession(T_NegSession *s);

T_NegClient *NEG_APT_AllocClientConnection(T_NegDemo *d);
void NEG_APT_FreeClientConnection(T_NegClient *cc);

int NEG_APT_RecvMsg(T_NegClient *cc, void **out);
int NEG_APT_SendMsg(T_NegClient *cc, void *msg);



#ifdef __cplusplus
}
#endif

#endif	/* __ZXY_NEGAN_APT_H__ */

