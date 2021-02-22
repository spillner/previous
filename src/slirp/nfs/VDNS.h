//
//  VDNS.h
//  Previous
//
//  Created by Simon Schubiger on 22.02.19.
//

#ifndef VDNS_h
#define VDNS_h

#ifdef __cplusplus

#include <stdio.h>

#include "UDPServerSocket.h"
#include "host.h"
#include "mbuf.h"

typedef enum {
    REC_A     = 1,  // Host address
    REC_CNAME = 5,  // Canonical name for an alias
    REC_MX    = 15, // Mail eXchange
    REC_NS    = 2,  // Name Server
    REC_PTR   = 12, // Pointer
    REC_SOA   = 6,  // Start Of Authority
    REC_SRV   = 33, // location of service
    REC_TXT   = 16, // Descriptive text

    REC_UNKNOWN = -1,
} vdns_rec_type;

typedef struct {
    vdns_rec_type type;
    char          key[_SC_HOST_NAME_MAX];
    uint8_t       data[1024];
    size_t        size;
    uint32_t      inaddr;
} vdns_record;

class VDNS : public ISocketListener {
    static vdns_record s_dns_db[32];
    static size_t      s_dns_db_sz;
    mutex_t*           m_hMutex;
    UDPServerSocket*   m_udp;
    
    void AddRecord(uint32_t addr, const char* name = NULL);
public:
    VDNS(void);
    ~VDNS(void);
    
    static vdns_record* Query(uint8_t* data, size_t size);
    void   SocketReceived(CSocket* pSocket);
};

extern "C" int nfsd_vdns_match(struct mbuf *m);
#else
int nfsd_vdns_match(struct mbuf *m);

#endif /* __cplusplus */

#endif /* VDNS_h */
