#ifndef _RPCSERVER_H_
#define _RPCSERVER_H_

#include "SocketListener.h"
#include "Socket.h"
#include "RPCProg.h"
#include "host.h"

#define PROG_NUM 128

class CRPCServer : public ISocketListener {
public:
	CRPCServer();
	virtual ~CRPCServer();
	void Set(int nProg, CRPCProg* pRPCProg);
	void SetLogOn(bool bLogOn);
	void SocketReceived(CSocket* pSocket);
protected:
	CRPCProg* m_pProgTable[PROG_NUM];
	mutex_t*  m_hMutex;
    int Process(int nType, XDRInput* pInStream, XDROutput* pOutStream, const char* pRemoteAddr);
};

#endif
