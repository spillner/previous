#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "SocketListener.h"
#include "XDRStream.h"
#include "host.h"
#include "slirpmain.h"

class CSocket
{
public:
	CSocket(int nType);
	virtual       ~CSocket();
	int            GetType(void);
	void           Open(int socket, ISocketListener *pListener, struct sockaddr_in *pRemoteAddr = NULL);
	void           Close(void);
	void           Send(void);
	bool           Active(void);
	char*          GetRemoteAddress(void);
	int            GetRemotePort(void);
	XDRInput*      GetInputStream(void);
	XDROutput*     GetOutputStream(void);
	void           Run(void);
    
    static uint16_t map_and_htons(int type, uint16_t port);
    static void     map_port(int type, int progNum, uint16_t port);
    
private:
	int                m_nType;
	int                m_Socket;
	struct sockaddr_in m_RemoteAddr;
    ISocketListener*   m_pListener;
	bool               m_bActive;
	thread_t*          m_hThread;
    XDRInput           m_Input;
    XDROutput          m_Output;
};

const int INVALID_SOCKET = -1;

#endif
