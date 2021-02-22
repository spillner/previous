#ifndef _RPCPROG_H_
#define _RPCPROG_H_

#include <stdint.h>
#include <stddef.h>

/* The maximum number of bytes in a pathname argument. */
#define MAXPATHLEN 1024

/* The maximum number of bytes in a file name argument. */
#define MAXNAMELEN 255

/* The maximum number of remote procedures per program. */
#define MAX_NUM_PROCS 32

/* The size in bytes of the opaque file handle. */
#define FHSIZE      32
#define FHSIZE_NFS3 64

enum
{
	PRC_OK,
	PRC_FAIL,
	PRC_NOTIMP
};

typedef struct
{
	uint32_t    version;
	uint32_t    proc;
    int         sockType;
	const char *remoteAddr;
} ProcessParam;

#ifdef __cplusplus

#include "XDRStream.h"

class CRPCProg;

typedef int (CRPCProg::*PPROC)(void);

class CRPCProg
{
public:
    CRPCProg(int progNum, int version, const char* name);
    virtual     ~CRPCProg();
    
    void         Init(uint16_t portTCP, uint16_t portUDP);
    void         Setup(XDRInput* xin, XDROutput* xout, ProcessParam* param);
	virtual int  Process(void);
	virtual void SetLogOn(bool bLogOn);
    int          GetProgNum(void) const;
    int          GetVersion(void) const;
    const char*  GetName(void)    const;
    uint16_t     GetPortTCP(void) const;
    uint16_t     GetPortUDP(void) const;
    
    int          ProcedureNULL(void);
    int          ProcedureNOTIMPL(void);
    
protected:
    PPROC          m_procs[MAX_NUM_PROCS];
    const char*    m_procNames[MAX_NUM_PROCS];
	bool           m_bLogOn;
    int            m_progNum;
    int            m_version;
    const char*    m_name;
    uint16_t       m_portTCP;
    uint16_t       m_portUDP;
    ProcessParam*  m_param;
    XDRInput*      m_in;
    XDROutput*     m_out;

    void           _SetProc(int procNum, const char* name, PPROC proc);
	size_t         Log(const char *format, ...) const;
};

#define SetProc(num, proc) _SetProc(num, #proc, (PPROC)&RPC_PROG_CLASS::Procedure##proc)

#endif

#endif
