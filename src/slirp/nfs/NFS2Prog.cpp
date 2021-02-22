#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <ftw.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>

#include "NFS2Prog.h"
#include "FileTableNFSD.h"
#include "nfsd.h"

using namespace std;

enum {
	NFS_OK = 0,
	NFSERR_PERM = 1,
	NFSERR_NOENT = 2,
	NFSERR_IO = 5,
	NFSERR_NXIO = 6,
	NFSERR_ACCES = 13,
	NFSERR_EXIST = 17,
	NFSERR_NODEV = 19,
	NFSERR_NOTDIR = 20,
	NFSERR_ISDIR = 21,
	NFSERR_FBIG = 27,
	NFSERR_NOSPC = 28,
	NFSERR_ROFS = 30,
	NFSERR_NAMETOOLONG = 63,
	NFSERR_NOTEMPTY = 66,
	NFSERR_DQUOT = 69,
	NFSERR_STALE = 70,
	NFSERR_WFLUSH = 99,
};

enum {
	NFNON = 0,
	NFREG = 1,
	NFDIR = 2,
	NFBLK = 3,
	NFCHR = 4,
	NFLNK = 5,
};

CNFS2Prog::CNFS2Prog() : CRPCProg(PROG_NFS, 2, "nfsd"), m_defUID(0), m_defGID(0) {
    #define RPC_PROG_CLASS CNFS2Prog
    SetProc(1,  GETATTR);
    SetProc(2,  SETATTR);
    SetProc(4,  LOOKUP);
    SetProc(5,  READLINK);
    SetProc(6,  READ);
    SetProc(7,  WRITECACHE);
    SetProc(8,  WRITE);
    SetProc(9,  CREATE);
    SetProc(10, REMOVE);
    SetProc(11, RENAME);
    SetProc(12, LINK);
    SetProc(13, SYMLINK);
    SetProc(14, MKDIR);
    SetProc(15, RMDIR);
    SetProc(16, READDIR);
    SetProc(17, STATFS);
}

CNFS2Prog::~CNFS2Prog() { }

void CNFS2Prog::SetUserID(unsigned int nUID, unsigned int nGID) {
	m_defUID = nUID;
	m_defGID = nGID;
}

int CNFS2Prog::ProcedureGETATTR(void) {
    string   path;
    
	GetPath(path);
    Log("GETATTR %s", path.c_str());
	if (!(CheckFile(path)))
		return PRC_OK;

	m_out->Write(NFS_OK);
	WriteFileAttributes(path);
    return PRC_OK;
}

static void set_attrs(const string& path, const FileAttrs& fstat) {
    if(FileAttrs::Valid(fstat.mode))
        nfsd_fts[0]->chmod(path, fstat.mode);
    
    timeval times[2];
    timeval now;
    gettimeofday(&now, NULL);
    times[0].tv_sec  = FileAttrs::Valid(fstat.atime_sec)  ? fstat.atime_sec  : now.tv_sec;
    times[0].tv_usec = FileAttrs::Valid(fstat.atime_usec) ? fstat.atime_usec : now.tv_usec;
    times[1].tv_sec  = FileAttrs::Valid(fstat.mtime_sec)  ? fstat.mtime_sec  : now.tv_sec;
    times[1].tv_usec = FileAttrs::Valid(fstat.mtime_usec) ? fstat.mtime_usec : now.tv_usec;
    if(FileAttrs::Valid(fstat.atime_sec) || FileAttrs::Valid(fstat.mtime_sec))
        nfsd_fts[0]->utimes(path, times);
    nfsd_fts[0]->SetFileAttrs(path, fstat);
}

static struct stat read_stat(XDRInput* xin) {
    
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t size;
    uint32_t atime_sec;
    uint32_t atime_usec;
    uint32_t mtime_sec;
    uint32_t mtime_usec;
    
    xin->Read(&mode);
    xin->Read(&uid);
    xin->Read(&gid);
    xin->Read(&size);
    xin->Read(&atime_sec);
    xin->Read(&atime_usec);
    xin->Read(&mtime_sec);
    xin->Read(&mtime_usec);
    
    struct stat result;
    result.st_mode              = mode;
    result.st_uid               = uid;
    result.st_gid               = gid;
    result.st_size              = size;
#if HAVE_STRUCT_STAT_ST_ATIMESPEC
    result.st_atimespec.tv_sec  = atime_sec;
    result.st_atimespec.tv_nsec = atime_usec * 1000;
#else
    result.st_atim.tv_sec       = atime_sec;
    result.st_atim.tv_nsec      = atime_usec * 1000;
#endif
#if HAVE_STRUCT_STAT_ST_MTIMESPEC
    result.st_mtimespec.tv_sec  = mtime_sec;
    result.st_mtimespec.tv_nsec = mtime_usec * 1000;
#else
    result.st_mtim.tv_sec       = mtime_sec;
    result.st_mtim.tv_nsec      = mtime_usec * 1000;
#endif
    result.st_rdev              = FATTR_INVALID;
    return result;
}

int CNFS2Prog::ProcedureSETATTR(void) {
    string   path;
    
	Log("SETATTR");
	GetPath(path);
	if (!(CheckFile(path)))
		return PRC_OK;

    set_attrs(path, FileAttrs(read_stat(m_in)));
    
    m_out->Write(NFS_OK);
	WriteFileAttributes(path);
    return PRC_OK;
}

static void write_handle(XDROutput* xout, uint64_t handle) {
    uint64_t data[4] = {handle,0,0,0};
    xout->Write(data, FHSIZE);
}

int CNFS2Prog::ProcedureLOOKUP(void) {
    string path;

	GetFullPath(path);
	if (!(CheckFile(path)))
		return PRC_OK;

    uint64_t handle = nfsd_fts[0]->GetFileHandle(path);
    if(handle) {
        m_out->Write(NFS_OK);
        Log("LOOKUP %s=%" PRIu64, path.c_str(), handle);
        write_handle(m_out, handle);
        WriteFileAttributes(path);
    } else {
        m_out->Write(NFSERR_NOENT);
    }
    return PRC_OK;
}

int nfs_err(int error) {
    switch (error) {
        case 0:      return NFS_OK;
        case ENOENT: return NFSERR_NOENT;
        case EACCES: return NFSERR_ACCES;
        case EINVAL: return NFSERR_IO;
        default:
            return NFSERR_IO;
    }
}

int CNFS2Prog::ProcedureREADLINK(void) {
    string   path;
    
    GetPath(path);
    Log("READLINK %s", path.c_str());
    if (!(CheckFile(path)))
        return PRC_OK;
    
    string result;
    if(int err = nfsd_fts[0]->readlink(path, result)) {
        m_out->Write(nfs_err(err));
    } else {
        m_out->Write(NFS_OK);
        XDRString data(result);
        m_out->Write(data);
    }
    
    return PRC_OK;
}

int CNFS2Prog::ProcedureREAD(void) {
    string   path;
    uint32_t nOffset;
    uint32_t nCount;
    uint32_t nTotalCount;

	GetPath(path);
    Log("READ %s", path.c_str());
	if (!(CheckFile(path)))
		return PRC_OK;

	m_in->Read(&nOffset);
	m_in->Read(&nCount);
	m_in->Read(&nTotalCount);
    
    XDROpaque buffer(nCount);
    File file(nfsd_fts[0], path, "rb");
    if(file.IsOpen()) {
        nCount = file.Read(nOffset, buffer.m_data, buffer.m_size);
        buffer.SetSize(nCount);
        m_out->Write(NFS_OK);
    } else {
        buffer.SetSize(0);
        m_out->Write(nfs_err(errno));
    }
	WriteFileAttributes(path);
    m_out->Write(buffer);

    return PRC_OK;
}

int CNFS2Prog::ProcedureWRITECACHE(void) {
    m_out->Write(NFS_OK);
    return PRC_OK;
}

int CNFS2Prog::ProcedureWRITE(void) {
    string   path;
    uint32_t nBeginOffset;
    uint32_t nOffset;
    uint32_t nTotalCount;

	GetPath(path);
    Log("WRITE %s", path.c_str());
	if (!(CheckFile(path)))
		return PRC_OK;

	m_in->Read(&nBeginOffset);
	m_in->Read(&nOffset);
	m_in->Read(&nTotalCount);

    XDROpaque buffer;
	m_in->Read(buffer);

    File file(nfsd_fts[0], path, "r+b");
    if(file.IsOpen()) {
        file.Write(nOffset, buffer.m_data, buffer.m_size);
        m_out->Write(NFS_OK);
    } else {
        m_out->Write(nfs_err(errno));
    }
	WriteFileAttributes(path);

    return PRC_OK;
}

int CNFS2Prog::ProcedureCREATE(void) {
    string path;
    
	if(!(GetFullPath(path)))
		return PRC_OK;
    Log("CREATE %s", path.c_str());

    FileAttrs fstat(read_stat(m_in));
    if(!(FileAttrs::Valid(fstat.uid))) fstat.uid = m_defUID;
    if(!(FileAttrs::Valid(fstat.gid))) fstat.gid = m_defGID;
     // touch
    File file(nfsd_fts[0], path, "wb");
    if(file.IsOpen()) {
        set_attrs(path, fstat);
        m_out->Write(NFS_OK);
        write_handle(m_out, nfsd_fts[0]->GetFileHandle(path));
        WriteFileAttributes(path);
    } else {
        nfs_err(errno);
    }
    return PRC_OK;
}

int CNFS2Prog::ProcedureREMOVE(void) {
    string path;

	GetFullPath(path);
    Log("REMOVE %s", path.c_str());
	if (!(CheckFile(path)))
		return PRC_OK;

    int err = nfs_err(nfsd_fts[0]->remove(path));
    m_out->Write(err);
    if(!(err)) nfsd_fts[0]->Remove(path);

    return PRC_OK;
}

int CNFS2Prog::ProcedureRENAME(void) {
    string pathFrom;
    string pathTo;

	GetFullPath(pathFrom);
	if (!(CheckFile(pathFrom)))
		return PRC_OK;
	GetFullPath(pathTo);
    Log("RENAME %s->%s", pathFrom.c_str(), pathTo.c_str());

    int err = nfs_err(nfsd_fts[0]->rename(pathFrom, pathTo));
    m_out->Write(err);
    if(!(err)) nfsd_fts[0]->Move(pathFrom, pathTo);
    
    return PRC_OK;
}

int CNFS2Prog::ProcedureLINK(void) {
    string   to;
    string   from;

    GetPath(to);
    GetFullPath(from);
    Log("LINK %s->%s", from.c_str(), to.c_str());
    
    m_out->Write(nfs_err(nfsd_fts[0]->link(from, to, false)));
    
    return PRC_OK;
}

int CNFS2Prog::ProcedureSYMLINK(void) {
    string to;
    
    GetFullPath(to);
    XDRString from;
    m_in->Read(from);
    Log("SYMLINK %s->%s", from.Get(), to.c_str());
    
    FileAttrs fstat(read_stat(m_in));
    int err = nfsd_fts[0]->link(from.Get(), to, true);
    if(!(err)) set_attrs(to, fstat);
    m_out->Write(nfs_err(err));
    
    return PRC_OK;
}

int CNFS2Prog::ProcedureMKDIR(void) {
    string path;

	Log("MKDIR");
	if(!(GetFullPath(path)))
		return PRC_OK;

    FileAttrs fstat(read_stat(m_in));
    if(int err = nfsd_fts[0]->mkdir(path, DEFAULT_PERM)) {
        nfs_err(err);
    } else {
        set_attrs(path, fstat);
        m_out->Write(NFS_OK);
        write_handle(m_out, nfsd_fts[0]->GetFileHandle(path));
        WriteFileAttributes(path);
    }
    
    return PRC_OK;
}

int CNFS2Prog::ProcedureRMDIR(void) {
    string path;

	Log("RMDIR");
	GetFullPath(path);
	if (!(CheckFile(path)))
		return PRC_OK;
    
    int err = nfs_err(nfsd_fts[0]->nftw(path, FileTable::Remove, 3, FTW_DEPTH | FTW_PHYS));
    m_out->Write(err);
    if(!(err)) nfsd_fts[0]->Remove(path);
    
    return PRC_OK;
}

int CNFS2Prog::ProcedureREADDIR(void) {
    string   path;
    DIR*     handle;
    uint32_t cookie;
    uint32_t count;
    uint64_t fhandle;
    
	GetPath(path, &fhandle);
	if (!(CheckFile(path)))
		return PRC_OK;
    m_in->Read(&cookie);
    m_in->Read(&count);
    
    Log("READDIR %s", path.c_str());
    
    uint32_t eof = 1;
    size_t   ftnlen = strlen(FILE_TABLE_NAME), namelen;
    handle          = nfsd_fts[0]->opendir(path);
	if (handle) {
        m_out->Write(NFS_OK);
        int skip = cookie;
        for(struct dirent* fileinfo = readdir(handle); fileinfo; fileinfo = readdir(handle)) {
#if HAVE_STRUCT_DIRENT_D_NAMELEN
            namelen = fileinfo->d_namlen;
#else
            namelen = strlen(fileinfo->d_name);
#endif
            if(ftnlen != namelen || strncmp(FILE_TABLE_NAME, fileinfo->d_name, namelen)) {
                if(--skip >= 0) continue;
                
                m_out->Write(1);  //value follows
                m_out->Write(nfsd_fts[0]->FileId(fileinfo->d_ino));
                string dname(fileinfo->d_name, namelen);
                XDRString name(dname);
                Log("%d %s %s", cookie, path.c_str(), name.Get());
                m_out->Write(name);
                m_out->Write(cookie+1);
                cookie++;
            }
            if(m_out->GetSize() >= count - 128) { // 128: give some space for XDR data
                eof = 0;
                break;
            }
		};
		closedir(handle);
        m_out->Write(0);  //no value follows
        m_out->Write(eof);
    } else {
        m_out->Write(nfs_err(errno));
    }

    return PRC_OK;
}

static const int BLOCK_SIZE = 4096;

static uint32_t nfs_blocks(const struct statvfs* fsstat, uint32_t fsblocks) {
    uint64_t result = fsblocks;
    result *= (uint64_t)fsstat->f_bsize;
    if(result >= 0x7FFFFFFF) result = 0x7FFFFFFF; // fix size for NS 2GB limit
    result /= BLOCK_SIZE;
    return result;
}

int CNFS2Prog::ProcedureSTATFS(void) {
    string path;
	struct statvfs fsstat;

	Log("STATFS");
	GetPath(path);
	if(!(CheckFile(path)))
		return PRC_OK;

    if(int err = nfsd_fts[0]->statvfs(path, fsstat)) {
        m_out->Write(nfs_err(err));
    } else {
        m_out->Write(NFS_OK);
        m_out->Write(BLOCK_SIZE*2);  //transfer size
        m_out->Write(BLOCK_SIZE);  //block size
        m_out->Write(nfs_blocks(&fsstat, fsstat.f_blocks));  //total blocks
        m_out->Write(nfs_blocks(&fsstat, fsstat.f_bfree));  //free blocks
        m_out->Write(nfs_blocks(&fsstat, fsstat.f_bavail));  //available blocks
    }
    
    return PRC_OK;
}

bool CNFS2Prog::GetPath(string& result, uint64_t* handle) {
    uint64_t data[4];
    m_in->Read((void*)data, FHSIZE);
    if(handle) *handle = data[0];
    return nfsd_fts[0]->GetCanonicalPath(data[0], result);
}

bool CNFS2Prog::GetFullPath(string& result) {
    if(!(GetPath(result)))
        return false;
    
    XDRString path;
    m_in->Read(path);
    if(result[result.length()-1] != '/') result += "/";
    result += path.Get();
    return true;
}

bool CNFS2Prog::CheckFile(const string& path) {
	if (path.length() == 0) {
		m_out->Write(NFSERR_STALE);
		return false;
	}
    
    // links always pass (will be resolved on the client side via readlink)
    struct stat fstat;
    if(nfsd_fts[0]->Stat(path, fstat) == 0 && (fstat.st_mode & S_IFMT) == S_IFLNK)
        return true;
    
    if(nfsd_fts[0]->access(path, F_OK)) {
		m_out->Write(NFSERR_NOENT);
		return false;
	}

    return true;
}

bool CNFS2Prog::WriteFileAttributes(const string& path) {
	struct stat fstat;

	if (nfsd_fts[0]->Stat(path, fstat) != 0)
		return false;

    uint32_t type = NFNON;
    if     (S_ISREG(fstat.st_mode)) type = NFREG;
    else if(S_ISDIR(fstat.st_mode)) type = NFDIR;
    else if(S_ISBLK(fstat.st_mode)) type = NFBLK;
    else if(S_ISCHR(fstat.st_mode)) type = NFCHR;
    else if(S_ISLNK(fstat.st_mode)) type = NFLNK;
            
	m_out->Write(type);  //type
	m_out->Write(fstat.st_mode);  //mode
	m_out->Write(fstat.st_nlink);  //nlink
	m_out->Write(fstat.st_uid);  //uid
	m_out->Write(fstat.st_gid);  //gid
	m_out->Write(fstat.st_size);  //size
	m_out->Write(fstat.st_blksize);  //blocksize
	m_out->Write(fstat.st_rdev);  //rdev
	m_out->Write(fstat.st_blocks);  //blocks
	m_out->Write(fstat.st_dev);  //fsid
        m_out->Write(nfsd_fts[0]->FileId(fstat.st_ino));
#if HAVE_STRUCT_STAT_ST_ATIMESPEC
	m_out->Write(fstat.st_atimespec.tv_sec);  //atime
	m_out->Write(fstat.st_atimespec.tv_nsec / 1000);  //atime
#else
	m_out->Write(fstat.st_atim.tv_sec);  //atime
	m_out->Write(fstat.st_atim.tv_nsec / 1000);  //atime
#endif
#if HAVE_STRUCT_STAT_ST_MTIMESPEC
	m_out->Write(fstat.st_mtimespec.tv_sec);  //mtime
	m_out->Write(fstat.st_mtimespec.tv_nsec / 1000);  //mtime
	m_out->Write(fstat.st_mtimespec.tv_sec);  //ctime -- ignored, we use mtime instead
	m_out->Write(fstat.st_mtimespec.tv_nsec / 1000);  //ctime
#else
	m_out->Write(fstat.st_mtim.tv_sec);  //mtime
	m_out->Write(fstat.st_mtim.tv_nsec / 1000);  //mtime
	m_out->Write(fstat.st_mtim.tv_sec);  //ctime -- ignored, we use mtime instead
	m_out->Write(fstat.st_mtim.tv_nsec / 1000);  //ctime
#endif
	return true;
}
