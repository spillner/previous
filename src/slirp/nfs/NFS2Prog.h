#ifndef _NFS2PROG_H_
#define _NFS2PROG_H_

#include <string>

#include "RPCProg.h"

class CNFS2Prog : public CRPCProg
{
public:
	CNFS2Prog();
	~CNFS2Prog();
	void SetUserID(unsigned int nUID, unsigned int nGID);

protected:
	unsigned int m_defUID, m_defGID;

	int ProcedureGETATTR(void);
	int ProcedureSETATTR(void);
	int ProcedureLOOKUP(void);
    int ProcedureREADLINK(void);
	int ProcedureREAD(void);
    int ProcedureWRITECACHE(void);
	int ProcedureWRITE(void);
	int ProcedureCREATE(void);
	int ProcedureREMOVE(void);
	int ProcedureRENAME(void);
    int ProcedureLINK(void);
    int ProcedureSYMLINK(void);
	int ProcedureMKDIR(void);
	int ProcedureRMDIR(void);
	int ProcedureREADDIR(void);
	int ProcedureSTATFS(void);

private:
    bool GetPath(std::string& result, uint64_t* handle = NULL);
    bool GetFullPath(std::string& result);
	bool CheckFile(const std::string& path);
    bool WriteFileAttributes(const std::string& path);
};

#endif
