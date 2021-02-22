#include <arpa/inet.h>

#include "Socket.h"
#include "nfsd.h"

static int ThreadProc(void *lpParameter)
{
	CSocket *pSocket;

	pSocket = (CSocket *)lpParameter;
	pSocket->Run();
	return 0;
}

CSocket::CSocket(int nType) : m_nType(nType), m_Socket(-1), m_pListener(NULL), m_bActive(false), m_hThread(NULL)  {
	memset(&m_RemoteAddr, 0, sizeof(m_RemoteAddr));
}

CSocket::~CSocket() {
	Close();
}

int CSocket::GetType(void) {
	return m_nType;  //socket type
}

void CSocket::Open(int socket, ISocketListener *pListener, struct sockaddr_in *pRemoteAddr) {
	Close();

	m_Socket = socket;  //socket
	m_pListener = pListener;  //listener
	if (pRemoteAddr != NULL)
		m_RemoteAddr = *pRemoteAddr;  //remote address
	if (m_Socket != INVALID_SOCKET)
	{
		m_bActive = true;
		m_hThread = host_thread_create(ThreadProc, "CSocket", this);  //begin thread
	}
}

void CSocket::Close(void) {
	if (m_Socket != INVALID_SOCKET) {
		close(m_Socket);
		m_Socket = INVALID_SOCKET;
	}

    m_hThread = NULL;
}

void CSocket::Send(void) {
	if (m_Socket == INVALID_SOCKET)
		return;

    ssize_t nBytes = 0;
	if (m_nType == SOCK_STREAM)
		nBytes = send(m_Socket, (const char *)m_Output.GetBuffer(), m_Output.GetSize(), 0);
	else if (m_nType == SOCK_DGRAM)
		nBytes = sendto(m_Socket, (const char *)m_Output.GetBuffer(), m_Output.GetSize(), 0, (struct sockaddr *)&m_RemoteAddr, sizeof(struct sockaddr));
    if(nBytes < 0)
        perror("[NFSD]Socket send");
    m_Output.Reset();  //clear output buffer
}

bool CSocket::Active(void) {
	return m_bActive;  //thread is active or not
}

char *CSocket::GetRemoteAddress(void) {
    return inet_ntoa(m_RemoteAddr.sin_addr);
}

int CSocket::GetRemotePort(void) {
    return htons(m_RemoteAddr.sin_port);
}

XDRInput* CSocket::GetInputStream(void) {
	return &m_Input;
}

XDROutput* CSocket::GetOutputStream(void) {
	return &m_Output;
}

void CSocket::Run(void) {
    socklen_t nSize;

    ssize_t nBytes = 0;
	for (;;) {
		if (m_nType == SOCK_STREAM)
			nBytes = recv(m_Socket, m_Input.GetBuffer(), m_Input.GetCapacity(), 0);
        else if (m_nType == SOCK_DGRAM) {
            nSize = sizeof(m_RemoteAddr);
			nBytes = recvfrom(m_Socket, m_Input.GetBuffer(), m_Input.GetCapacity(), 0, (struct sockaddr *)&m_RemoteAddr, &nSize);
        }
		if (nBytes > 0) {
			m_Input.SetSize(nBytes);  //bytes received
			if (m_pListener != NULL)
				m_pListener->SocketReceived(this);  //notify listener
        } else {
            perror("[NFSD] Socket recv");
            break;
        }
	}
	m_bActive = false;
}

void CSocket::map_port(int type, int progNum, uint16_t port) {
    switch(type) {
        case SOCK_DGRAM:
            switch(progNum) {
                case PROG_VDNS:    nfsd_ports.udp.dns     = port; break;
                case PROG_PORTMAP: nfsd_ports.udp.portmap = port; break;
                case PROG_MOUNT:   nfsd_ports.udp.mount   = port; break;
                case PROG_NFS:     nfsd_ports.udp.nfs     = port; break;
            }
            break;
        case SOCK_STREAM:
            switch(progNum) {
                case PROG_VDNS:    nfsd_ports.tcp.dns     = port; break;
                case PROG_PORTMAP: nfsd_ports.tcp.portmap = port; break;
                case PROG_MOUNT:   nfsd_ports.tcp.mount   = port; break;
                case PROG_NFS:     nfsd_ports.tcp.nfs     = port; break;
            }
            break;
    }
}

uint16_t CSocket::map_and_htons(int sockType, uint16_t port) {
    if(sockType == SOCK_STREAM) {
        switch (port) {
            case PORT_DNS:     return htons(nfsd_ports.tcp.dns);
            case PORT_PORTMAP: return htons(nfsd_ports.tcp.portmap);
            case PORT_NFS:     return htons(nfsd_ports.tcp.nfs);
        }
    } else {
        switch (port) {
            case PORT_DNS:     return htons(nfsd_ports.udp.dns);
            case PORT_PORTMAP: return htons(nfsd_ports.udp.portmap);
            case PORT_NFS:     return htons(nfsd_ports.udp.nfs);
        }
    }
    return htons(port);
}
