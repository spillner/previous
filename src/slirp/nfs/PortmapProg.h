#ifndef _PORTMAPPROG_H_
#define _PORTMAPPROG_H_

#include "RPCProg.h"
#include "RPCProg.h"

#define PORT_NUM 128

class CPortmapProg : public CRPCProg
{
public:
    CPortmapProg();
    virtual ~CPortmapProg();
    void Add(CRPCProg* prog);
    
protected:
    
    const CRPCProg* GetProg(int progNum);
    int ProcedureNOIMP(void);
    int ProcedureNULL(void);
    int ProcedureSET(void);
    int ProcedureUNSET(void);
    int ProcedureGETPORT(void);
    int ProcedureDUMP(void);
    int ProcedureCALLIT(void);
    
private:
    void Write(const CRPCProg* prog);
    const CRPCProg* m_nProgTable[PORT_NUM];
};

#endif
