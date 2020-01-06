#ifndef __NET_TCP__
#define __NET_TCP__



int NETTCP_ServerOpen(int , int *);

int NETTCP_ClientSend(char *, int , char *, unsigned int );

int NETUDP_ServerOpen(int , int *);

int NETTCP_ClientConnect(char *, int , int *);

int NETTCP_ClientWrite(int , char *, unsigned int );

#endif


