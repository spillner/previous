//
//  BootparamProg.cpp
//  Previous
//
//  Created by Simon Schubiger on 18.01.19.
//

#include <string>
#include <string.h>
#if HAVE_SYS_SYSLIMITS_H
    #include <sys/syslimits.h>
#endif
#if HAVE_LIMITS_H
    #include <limits.h>
#endif
#include <netinet/in.h>

#include "nfsd.h"
#include "BootparamProg.h"
#include "FileTableNFSD.h"

using namespace std;

CBootparamProg::CBootparamProg() : CRPCProg(PROG_BOOTPARAM, 1, "bootparamd") {
    #define RPC_PROG_CLASS CBootparamProg
    SetProc(1, WHOAMI);
    SetProc(2, GETFILE);
}

CBootparamProg::~CBootparamProg() {}

const int IP_ADDR_TYPE    = 1;

static void WriteInAddr(XDROutput* out, uint32_t inAddr) {
    out->Write(IP_ADDR_TYPE);
    out->Write(0xFF&(inAddr >> 24));
    out->Write(0xFF&(inAddr >> 16));
    out->Write(0xFF&(inAddr >> 8));
    out->Write(0xFF&(inAddr));
}

extern "C" struct in_addr special_addr;

int CBootparamProg::ProcedureWHOAMI(void) {
    uint32_t address_type;
    uint32_t address;
    
    m_in->Read(&address_type);
    switch (address_type) {
        case IP_ADDR_TYPE:
            m_in->Read(&address);
            break;
        default:
            return PRC_FAIL;
    }
    char hostname[_SC_HOST_NAME_MAX];
    strcpy(hostname, NAME_HOST);
    char* client = hostname;
    char* domain = hostname;
    while(*++domain) {
        if(*domain == '.') {
            *domain = '\0';
            domain++;
            break;
        }
    }
    m_out->Write(_SC_HOST_NAME_MAX, client);
    m_out->Write(_SC_HOST_NAME_MAX, domain);
    WriteInAddr(m_out, ntohl(special_addr.s_addr) | CTL_GATEWAY);
    return PRC_OK;
}

int CBootparamProg::ProcedureGETFILE(void) {
    XDRString client;
    XDRString key;
    m_in->Read(client);
    m_in->Read(key);
    string path = nfsd_fts[0]->GetBasePathAlias();
    /*if(strcmp("private", key.Get()) == 0) {
        if(path != "/") path += "/";
        path += "clients/previous";
    } else */if(strcmp("root", key.Get())) {
        if(path != "/") path += "/";
            path += key.Get();
    }

    if(path.size()) {
        m_out->Write(_SC_HOST_NAME_MAX, NAME_NFSD);
        WriteInAddr(m_out, ntohl(special_addr.s_addr) | CTL_NFSD);
        m_out->Write(PATH_MAX, path.c_str());
        return PRC_OK;
    } else {
        Log("Unknown key: %s", key.Get());
        return PRC_FAIL;
    }
}
