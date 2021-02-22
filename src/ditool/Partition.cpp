//
//  Partition.cpp
//  Previous
//
//  Created by Simon Schubiger on 03.03.19.
//

#include <string.h>

#include "Partition.h"
#include "fs.h"
#include "UFS.h"

using namespace std;

static disk_partition DUMMY = {};

Partition::Partition(void) : partNo(-1), partIdx(-1), im(NULL), dl(NULL), part(DUMMY), fsOff(0) {}

Partition::Partition(int partNo, int partIdx, DiskImage* im, const disk_label* dl, disk_partition& part) :
partNo(partNo),
partIdx(partIdx),
im(im),
dl(dl),
part(part),
fsOff(fsv(part.p_base)) {
    if(partNo == 0) fsOff += fsv(im->dl.dl_dt.d_front);
}

Partition::~Partition(void) {}

bool Partition::isUFS(void) const {
    if(strncmp(&part.p_type[1], "4.3BSD", 6))
        return false;
    
    uint8_t sectors[8*im->sectorSize];
    if(readSectors(8, 8, sectors))
        return false;
    
    struct ufs_super_block* fs = (struct ufs_super_block*)sectors;
    return fsv(fs->fs_magic) == FS_MAGIC;
}

int Partition::readSectors(uint32_t sector, uint32_t count, uint8_t* dst) const {
    uint64_t offset = sector;
    offset += fsOff;
    offset *= im->sectorSize;
    streamsize size = count;
    size *= im->sectorSize;
    ios_base::io_state result = im->read(offset, size, dst);
    if     (result == ios_base::goodbit) return ERR_NO;
    else if(result &  ios_base::eofbit)  return ERR_EOF;
    else                                 return ERR_FAIL;
}

ostream& operator<< (ostream& os, const Partition& part) {
    double size = fsv(part.part.p_size);
    size *= part.im->sectorSize;
    size /= FACT_MB;
    os << "Partition #" << part.partIdx << ": " << &part.part.p_type[1] << " " << size << " MBytes";
    if(part.isUFS())
        os << UFS(part);
    return os;
}
