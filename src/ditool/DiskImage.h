//
//  DiskImage.hpp
//  Previous
//
//  Created by Simon Schubiger on 03.03.19.
//
//  portion of it Copyright (c) 1982, 1986 Regents of the University of California

#ifndef DiskImage_h
#define DiskImage_h

#include <fstream>
#include <vector>
#include <stdint.h>

#include "Partition.h"

uint32_t fsv(uint32_t v);
uint16_t fsv(uint16_t v);

int32_t fsv(int32_t v);
int16_t fsv(int16_t v);

#pragma pack(push, 1)

struct disk_partition {
    int32_t    p_base;            /* base sector# of partition */
    int32_t    p_size;            /* #sectors in partition */
    int16_t    p_bsize;           /* block size in bytes */
    int16_t    p_fsize;           /* frag size in bytes */
    uint16_t   p_opt;             /* 's'pace/'t'ime optimization pref */
    int16_t    p_cpg;             /* cylinders per group */
    int16_t    p_density;         /* bytes per inode density */
    uint8_t    p_minfree;         /* minfree (%) */
    uint8_t    p_newfs;           /* run newfs during init */
#define    MAXMPTLEN    16
    char       p_mountpt[MAXMPTLEN]; /* mount point */
#define    MAXFSTLEN    10
    char       p_type[MAXFSTLEN]; /* file system type p_type[0] is automount flag */
};

struct    disktab {
#define    MAXDNMLEN    24
    char       d_name[MAXDNMLEN];      /* drive name */
#define    MAXTYPLEN    24
    uint8_t    d_type[MAXTYPLEN];      /* drive type */
    int32_t    d_secsize;              /* sector size in bytes */
    int32_t    d_ntracks;              /* # tracks/cylinder */
    int32_t    d_nsectors;             /* # sectors/track */
    int32_t    d_ncylinders;           /* # cylinders */
    int32_t    d_rpm;                  /* revolutions/minute */
    int16_t    d_front;                /* size of front porch (sectors) */
    int16_t    d_back;                 /* size of back porch (sectors) */
    int16_t    d_ngroups;              /* number of alt groups */
    int16_t    d_ag_size;              /* alt group size (sectors) */
    int16_t    d_ag_alts;              /* alternate sectors / alt group */
    int16_t    d_ag_off;               /* sector offset to first alternate */
#define    NBOOTS    2
    int32_t        d_boot0_blkno[NBOOTS];    /* "blk 0" boot locations */
#define    MAXBFLEN 24
    char    d_bootfile[MAXBFLEN];     /* default bootfile */
#define    MAXHNLEN 32
    char    d_hostname[MAXHNLEN];     /* host name */
    char    d_rootpartition;          /* root partition e.g. 'a' */
    char    d_rwpartition;            /* r/w partition e.g. 'b' */
#define    NPART    8
    struct  disk_partition d_partitions[NPART];
};

struct disk_label {
    char                    dl_version[4];            /* label version number */
    int32_t                 dl_label_blkno;           /* block # where this label is */
    int32_t                 dl_size;                  /* size of media area (sectors) */
#define    MAXLBLLEN    24
    char                    dl_label[MAXLBLLEN];      /* media label */
    uint32_t                dl_flags;                 /* flags */
#define    DL_UNINIT    0x80000000                    /* label is uninitialized */
    uint32_t                dl_tag;                   /* volume tag */
    struct    disktab       dl_dt;                    /* common info in disktab */
};

#pragma pack(pop)

const double FACT_MB = 1 << 20;

class Partition;

class DiskImage {
    std::ifstream& imf;
public:
    struct disk_label      dl;
    std::vector<Partition> parts;
    uint64_t               sectorSize;
    const std::string      path;
    
    DiskImage(const std::string& path, std::ifstream& imf);
    ~DiskImage(void);
    
    std::ios_base::iostate read(std::streampos offset, std::streamsize size, void* data);
};

std::ostream& operator<< (std::ostream& stream, const DiskImage& im);

#endif /* DiskImage_hpp */
