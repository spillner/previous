//
//  DiskImage.cpp
//  Previous
//
//  Created by Simon Schubiger on 03.03.19.
//

#include "DiskImage.h"

#include <iostream>
#include <cstring>

/* Pull in ntohs()/ntohl()/htons()/htonl() declarations... shotgun approach */
#if defined(linux)
    /* netinet/in.h doesn't have proper extern "C" declarations for these... may also apply to other Unices */
    extern "C" uint32_t ntohl(uint32_t);
    extern "C" uint16_t ntohs(uint16_t);
    extern "C" uint32_t htonl(uint32_t);
    extern "C" uint16_t htons(uint16_t);
#else
    #if HAVE_ARPA_INET_H
        #include <arpa/inet.h>
    #endif
    #if HAVE_NETINET_IN_H
        #include <netinet/in.h>
    #endif
    #if HAVE_WINSOCK_H
        #include <winsock.h>
    #endif
#endif

using namespace std;

uint32_t fsv(uint32_t v) { return ntohl(v); }
uint16_t fsv(uint16_t v) { return ntohs(v); }

int16_t  fsv(int16_t v)  { return ntohs(v); }
int32_t  fsv(int32_t v)  { return ntohl(v); }

DiskImage::DiskImage(const string& path, ifstream& imf) : imf(imf), path(path) {
    read(0, sizeof(dl), &dl);
    if(
        strncmp(dl.dl_version, "NeXT", 4) &&
        strncmp(dl.dl_version, "dlV2", 4) &&
        strncmp(dl.dl_version, "dlV3", 4)
        ) {
        cout << "Unknown version: " << dl.dl_version << endl;
        return;
    }
    sectorSize = fsv(dl.dl_dt.d_secsize);
    if(sectorSize != 0x400) {
        cout << "Unsupported sector size: " << fsv(dl.dl_size);
        return;
    }
    for(int p = 0; p < NPART; p++) {
        if(fsv(dl.dl_dt.d_partitions[p].p_bsize) == 0 || fsv(dl.dl_dt.d_partitions[p].p_bsize) == ~0)
            continue;
        parts.push_back(Partition(p, parts.size(), this, &dl, dl.dl_dt.d_partitions[p]));
    }
}

ios_base::iostate DiskImage::read(streampos offset, streamsize size, void* data) {
    imf.seekg(offset, ios::beg);
    imf.read((char*)data, size);
    ios_base::iostate result = imf.rdstate();
    if(result != ios_base::goodbit)
        cout << "Can't read " << size << " bytes at offset " << offset << endl;
    return result;
}

DiskImage::~DiskImage(void) {
    
}

ostream& operator<< (ostream& os, const DiskImage& im) {
    double size = im.sectorSize;
    size *= fsv(im.dl.dl_dt.d_ntracks);
    size *= fsv(im.dl.dl_dt.d_nsectors);
    size *= fsv(im.dl.dl_dt.d_ncylinders);
    size /= FACT_MB;
    os << "Disk '" << im.dl.dl_dt.d_name << "' '" << im.dl.dl_label << "' '" << im.dl.dl_dt.d_type << "' " << size << " MBytes" << endl;
    os << "  Sector size: " << im.sectorSize << " Bytes" << endl << endl;
    for(size_t p = 0; p < im.parts.size(); p++)
        os << "  " << im.parts[p] << endl;
    return os;
}

