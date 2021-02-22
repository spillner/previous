#ifndef _DATAGRAMSOCKET_H_
#define _DATAGRAMSOCKET_H_

#include "SocketListener.h"
#include "Socket.h"

class UDPServerSocket
{
public:
	UDPServerSocket(ISocketListener* pListener);
	~UDPServerSocket();
	bool Open(int porgNum, uint16_t nPort = 0);
	void Close(void);
	int GetPort(void);
	void Run(void);

private:
	int              m_nPort;
	int              m_Socket;
	CSocket*         m_pSocket;
	bool             m_bClosed;
	ISocketListener* m_pListener;
};

#endif
