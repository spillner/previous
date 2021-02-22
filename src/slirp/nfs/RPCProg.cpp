#include <stdarg.h>
#include <stdio.h>

#include "RPCProg.h"
#include "Socket.h"

CRPCProg::CRPCProg(int progNum, int version, const char* name) : m_bLogOn(true), m_progNum(progNum), m_version(version), m_name(name), m_portTCP(0), m_portUDP(0) {
    #define RPC_PROG_CLASS CRPCProg
    SetProc(0, NULL);
    for(int p = 1; p < MAX_NUM_PROCS; p++)
        SetProc(p, NOTIMPL);
}

CRPCProg::~CRPCProg() {}

void CRPCProg::Init(uint16_t portTCP, uint16_t portUDP) {
    m_portTCP = portTCP;
    m_portUDP = portUDP;
    Log(" init tcp:%d->%d udp%d->%d",
        GetPortTCP(), ntohs(CSocket::map_and_htons(SOCK_STREAM, GetPortTCP())),
        GetPortUDP(), ntohs(CSocket::map_and_htons(SOCK_DGRAM,  GetPortUDP())));
}

void CRPCProg::Setup(XDRInput* xin, XDROutput* xout, ProcessParam* param) {
    m_in     = xin;
    m_out    = xout;
    m_param = param;
}


int CRPCProg::Process(void) {
    PPROC       proc = &CRPCProg::ProcedureNOTIMPL;
    const char* name = "NOTIMPL";
    if(m_param->proc < MAX_NUM_PROCS) {
        proc = m_procs[m_param->proc];
        name = m_procNames[m_param->proc];
    }
    int result = (this->*proc)();
    if(result == PRC_NOTIMP) Log(" %d(...) = %d", m_param->proc, result);
    else                     Log(" %s(...) = %d", name,          result);
    return result;
}

void CRPCProg::_SetProc(int procNum, const char* name, PPROC proc) {
    if(procNum < 0 || procNum >= MAX_NUM_PROCS) {
        Log("%s procNum out of bounds: %d", name, procNum);
        return;
    }
    m_procs    [procNum] = proc;
    m_procNames[procNum] = name;
}

void CRPCProg::SetLogOn(bool bLogOn) {
	m_bLogOn = bLogOn;
}

int CRPCProg::GetProgNum(void) const {
    return m_progNum;
}

int CRPCProg::GetVersion(void) const {
    return m_version;
}

const char* CRPCProg::GetName(void) const {
    return m_name;
}

uint16_t CRPCProg::GetPortTCP(void) const {
    return m_portTCP;
}

uint16_t CRPCProg::GetPortUDP(void) const {
    return m_portUDP;
}

int CRPCProg::ProcedureNULL(void) {
    return PRC_OK;
}

int CRPCProg::ProcedureNOTIMPL(void) {
    return PRC_NOTIMP;
}

size_t CRPCProg::Log(const char *format, ...) const {
	va_list vargs;
	int nResult;

	nResult = 0;
	if (m_bLogOn)
	{
		va_start(vargs, format);
        printf("[NFSD:%s:%d] ", m_name, GetProgNum());
		nResult = vprintf(format, vargs);
        printf("\n");
		va_end(vargs);
	}
	return nResult;
}
