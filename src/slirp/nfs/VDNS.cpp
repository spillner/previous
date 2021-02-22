//
//  VDNS.c
//  Previous
//
//  Created by Simon Schubiger on 22.02.19.
//

#include <string.h>

#include "VDNS.h"
#include "nfsd.h"

#include "compat.h"

vdns_record VDNS::s_dns_db[32];
size_t      VDNS::s_dns_db_sz = 0;

static size_t from_dot(uint8_t* dst, const char* src) {
    size_t   result = strlen(src) + 2;
    uint8_t* len    = dst++;
    *len            = 0;
    while(*src) {
        if(*src == '.') {
            len = dst;
            *len = 0;
            src++;
            continue;
        }
        *dst++ = tolower(*src++);
        *len = *len + 1;
    }
    *dst++ = '\0';
    return result;
}

void VDNS::AddRecord(uint32_t addr, const char* _name) {
    
    size_t size = strlen(_name);
    char name[size + 1];
    for(size_t i = 0; i < size; i++)
        name[i] = tolower(_name[i]);
    name[size] = '\0';
    
    vdns_record* rec = &s_dns_db[s_dns_db_sz++];
    rec->type   = REC_A;
    rec->inaddr = addr;
    sprintf(rec->key,  "%d.%d.%d.%d.", 0xFF&(addr >> 24), 0xFF&(addr >> 16), 0xFF&(addr >> 8),  0xFF&(addr));
    uint32_t inaddr = htonl(addr);
    rec->size = 4;
    memcpy(rec->data, &inaddr, rec->size);
    
    if(_name) {
        vdns_record* rec = &s_dns_db[s_dns_db_sz++];
        rec->type   = REC_A;
        rec->inaddr = addr;
        sprintf(rec->key,  "%s.", name);
        uint32_t inaddr = htonl(addr);
        rec->size = 4;
        memcpy(rec->data, &inaddr, rec->size);
        
        rec         = &s_dns_db[s_dns_db_sz++];
        rec->type   = REC_PTR;
        rec->inaddr = addr;
        sprintf(rec->key,  "%d.%d.%d.%d.in-addr.arpa.", 0xFF&(addr), 0xFF&(addr >> 8),  0xFF&(addr >> 16), 0xFF&(addr >> 24));
        rec->size = from_dot(rec->data , name);
    }
}

VDNS::VDNS(void)  : m_hMutex(host_mutex_create()) {
    m_udp = new UDPServerSocket(this);
    m_udp->Open(PROG_VDNS, PORT_DNS);
    
    char hostname[_SC_HOST_NAME_MAX];
    hostname[0] = '\0';
    gethostname(hostname, sizeof(hostname));
    
    AddRecord(ntohl(special_addr.s_addr) | CTL_ALIAS, hostname);
    AddRecord(ntohl(special_addr.s_addr) | CTL_HOST,  NAME_HOST);
    AddRecord(ntohl(special_addr.s_addr) | CTL_DNS,   NAME_DNS);
    AddRecord(ntohl(special_addr.s_addr) | CTL_NFSD,  NAME_NFSD);
    AddRecord(0x7F000001,                             "localhost");
}

VDNS::~VDNS(void) {
    m_udp->Close();
    delete m_udp;
    host_mutex_destroy(m_hMutex);
}

static vdns_rec_type to_dot(char* dst, const uint8_t* src, size_t size) {
    const uint8_t* end   = &src[size];
    uint8_t        count = 0;
    int            result = REC_UNKNOWN;
    while(*src) {
        if(src >= end) goto error;
        count = *src++;
        if(count > 63) goto error;
        for(int j = 0; j < count; j++) {
            if(src >= end) goto error;
            *dst++ = tolower(*src++);
        }
        *dst++ = '.';
    }
    src++;
    result = *src++;
    result <<= 8;
    result |= *src;
error:
    *dst = '\0';
    return (vdns_rec_type)result;
}

vdns_record* VDNS::Query(uint8_t* data, size_t size) {
    char  qname[_SC_HOST_NAME_MAX];
    vdns_rec_type qtype = to_dot(qname, data, size);
    printf("[VDNS] query(%d) '%s'\n", qtype, qname);
    
    if(qtype < 0) return NULL;
    
    for(size_t n = 0; n < s_dns_db_sz; n++)
        if(!(strcmp(qname, s_dns_db[n].key)) && s_dns_db[n].type == qtype)
            return &s_dns_db[n];
    
    return NULL;
}

extern "C" int nfsd_vdns_match(struct mbuf *m) {
    if(m->m_hdr.mh_len <= 40) return false;
    return VDNS::Query((uint8_t*)&m->m_data[40], m->m_hdr.mh_len-40) != NULL;
}

void VDNS::SocketReceived(CSocket* pSocket) {
    NFSDLock lock(m_hMutex);
    
    XDRInput*  in    = pSocket->GetInputStream();
    XDROutput* out   = pSocket->GetOutputStream();
    uint8_t*   msg   = &in->GetBuffer()[in->GetPosition()];
    int        n     = in->GetSize();

    // SameId
    msg[2]=0x81;msg[3]=0x80;
    // Change Opcode and flags
    msg[8]=0;msg[9]=0; // NSCOUNT
    msg[10]=0;msg[11]=0; // ARCOUNT
    // Keep request in message and add answer
    size_t off = 12;
    msg[n++]=0xC0; msg[n++]=off; // Offset to the domain name
    vdns_record* rec = Query(&msg[off], in->GetSize()-(in->GetPosition()+off));

    msg[n++]=0x00;
    msg[n++]=rec->type;  // Type
    
    msg[n++]=0x00;msg[n++]=0x01; // Class 1
    msg[n++]=0x00;msg[n++]=0x00;msg[n++]=0x00;msg[n++]=0x3c; // TTL

    if(rec) {
        msg[6]=0;msg[7] = 1; // Num answers
        uint32_t inaddr = rec->inaddr;
        printf("[VDNS] reply '%s' -> %d.%d.%d.%d\n", rec->key, 0xFF&(inaddr >> 24), 0xFF&(inaddr >> 16), 0xFF&(inaddr >> 8), 0xFF&(inaddr));
        switch(rec->type) {
            case REC_A:
            case REC_PTR:
                msg[n++]=0x00;msg[n++]=rec->size;
                memcpy(&msg[n], rec->data, rec->size);
                n += rec->size;
                break;
            default:
                printf("[VDNS] unknown query:%d ('%s')\n", rec->type, rec->key);
                break;
        }
    } else {
        msg[6]=0;msg[7] = 0; // Num answers
        printf("[VDNS] no record found.\n");
    }
    // Send the answer
    
    out->Write(msg, n);
    pSocket->Send();  //send response
}
