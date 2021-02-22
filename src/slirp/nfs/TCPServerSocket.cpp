#include <assert.h>

#include "TCPServerSocket.h"
#include "nfsd.h"

static const int BACKLOG = 16;

static int ThreadProc(void *lpParameter) {
	((TCPServerSocket *)lpParameter)->Run();
	return 0;
}

TCPServerSocket::TCPServerSocket(ISocketListener *pListener) : m_nPort(0), m_ServerSocket(0), m_bClosed(true), m_pListener(pListener), m_hThread(NULL), m_pSockets(NULL) {}

TCPServerSocket::~TCPServerSocket() {
	Close();
}

bool TCPServerSocket::Open(int progNum, uint16_t nPort) {
	struct sockaddr_in localAddr;
	int i;

	Close();

    m_ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_ServerSocket == INVALID_SOCKET)
        return false;
    
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = CSocket::map_and_htons(SOCK_STREAM, nPort);
    localAddr.sin_addr = loopback_addr;
    if (bind(m_ServerSocket, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        close(m_ServerSocket);
        return false;
    }
    
    socklen_t size = sizeof(localAddr);
    if(getsockname(m_ServerSocket,  (struct sockaddr *)&localAddr, &size) < 0) {
        close(m_ServerSocket);
        return false;
    }
    m_nPort = nPort == 0 ? ntohs(localAddr.sin_port) : nPort;
    CSocket::map_port(SOCK_STREAM, progNum, ntohs(localAddr.sin_port));

    if (listen(m_ServerSocket, BACKLOG) < 0) {
        close(m_ServerSocket);
        return false;
    }
    
	m_pSockets = new CSocket* [BACKLOG];
	for (i = 0; i < BACKLOG; i++)
		m_pSockets[i] = new CSocket(SOCK_STREAM);
	
	m_bClosed = false;
    m_hThread = host_thread_create(ThreadProc, "ServerSocket", this);
	return true;
}

void TCPServerSocket::Close(void)
{
	int i;

	if (m_bClosed)
		return;
	m_bClosed = true;

	close(m_ServerSocket);

    if      (m_nPort == PORT_DNS)             nfsd_ports.tcp.dns     = 0;
    else if (m_nPort == PORT_PORTMAP)         nfsd_ports.tcp.portmap = 0;
    else if (m_nPort == nfsd_ports.tcp.mount) nfsd_ports.tcp.mount   = 0;
    else if (m_nPort == PORT_NFS)             nfsd_ports.tcp.nfs     = 0;
    
	if (m_hThread != NULL)
	{
		host_thread_wait(m_hThread);
	}

	if (m_pSockets != NULL)
	{
		for (i = 0; i < BACKLOG; i++)
			delete m_pSockets[i];
		delete[] m_pSockets;
		m_pSockets = NULL;
	}
}

int TCPServerSocket::GetPort(void) {
	return m_nPort;
}

void TCPServerSocket::Run(void) {
    int i;
    socklen_t nSize;
	struct sockaddr_in remoteAddr;
	int socket;

	nSize = sizeof(remoteAddr);
	while (!m_bClosed) {
        socket = accept(m_ServerSocket, (struct sockaddr *)&remoteAddr, &nSize);  //accept connection
		if (socket != INVALID_SOCKET) {
			for (i = 0; i < BACKLOG; i++)
				if (!m_pSockets[i]->Active()) {   //find an inactive CSocket
					m_pSockets[i]->Open(socket, m_pListener, &remoteAddr);  //receive input data
					break;
				}
		}
	}
}
