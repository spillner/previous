#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "FileTable.h"
#include "compat.h"

using namespace std;

#ifdef _XDRSTREAM_H_
FileAttrs::FileAttrs(XDRInput* xin) : rdev(INVALID) {
    xin->Read(&mode);
    xin->Read(&uid);
    xin->Read(&gid);
    xin->Read(&size);
    xin->Read(&atime_sec);
    xin->Read(&atime_usec);
    xin->Read(&mtime_sec);
    xin->Read(&mtime_usec);
    
}
#endif

FileAttrs::FileAttrs(const struct stat& stat) :
mode      (stat.st_mode),
uid       (stat.st_uid),
gid       (stat.st_gid),
size      (stat.st_size),
#if HAVE_STRUCT_STAT_ST_ATIMESPEC
    atime_sec (stat.st_atimespec.tv_sec),
    atime_usec(stat.st_atimespec.tv_nsec / 1000),
#else
    atime_sec (stat.st_atim.tv_sec),
    atime_usec(stat.st_atim.tv_nsec / 1000),
#endif
#if HAVE_STRUCT_STAT_ST_MTIMESPEC
    mtime_sec (stat.st_mtimespec.tv_sec),
    mtime_usec(stat.st_mtimespec.tv_nsec / 1000),
#else
    mtime_sec (stat.st_mtim.tv_sec),
    mtime_usec(stat.st_mtim.tv_nsec / 1000),
#endif
rdev      (stat.st_rdev)
{}

FileAttrs::FileAttrs(const FileAttrs& attrs) :
mode      (FATTR_INVALID),
uid       (FATTR_INVALID),
gid       (FATTR_INVALID),
size      (FATTR_INVALID),
atime_sec (FATTR_INVALID),
atime_usec(FATTR_INVALID),
mtime_sec (FATTR_INVALID),
mtime_usec(FATTR_INVALID),
rdev      (FATTR_INVALID)
{Update(attrs);}

void FileAttrs::Update(const FileAttrs& attrs) {
#define UPDATE_ATTR(a) a = Valid(attrs. a) ? attrs. a : a
    UPDATE_ATTR(mode);
    UPDATE_ATTR(uid);
    UPDATE_ATTR(gid);
    UPDATE_ATTR(size);
    UPDATE_ATTR(atime_sec);
    UPDATE_ATTR(atime_usec);
    UPDATE_ATTR(mtime_sec);
    UPDATE_ATTR(mtime_usec);
    UPDATE_ATTR(rdev);
}

FileAttrs::FileAttrs(File& fin, string& name) {
    char cname[MAXNAMELEN];
    if(fscanf(fin.file, "0%o:%d:%d:%d:%s\n", &mode, &uid, &gid, &rdev, cname) == 5)
        name = cname;
    else
        name.resize(0);
}

void FileAttrs::Write(File& fout, const string& name) {
    fprintf(fout.file, "0%o:%d:%d:%d:%s\n", mode, uid, gid, rdev, name.c_str());
}

bool FileAttrs::Valid(uint32_t statval) {return statval != 0xFFFFFFFF;}

//----- file table

FileTable::FileTable(const string& _basePath, const string& _basePathAlias) {
    basePathAlias = _basePathAlias;
    basePath      = _basePath;
    if(basePath[basePath.length()-1] == '/') basePath.resize(basePath.size()-1);
}

FileTable::~FileTable() {
    for(map<uint64_t,FileAttrDB*>::iterator it = handle2db.begin(); it != handle2db.end(); it++)
        delete it->second;
    handle2db.clear();
}

string FileTable::basename(const string& path) {
    string result;
    char tmp[MAXPATHLEN];
    strncpy(tmp, path.c_str(), MAXPATHLEN-1);
    result = ::basename(tmp);
    return result;
}

string FileTable::basename_helper(string const & path) {
	return basename((const string &) path);
}

string FileTable::dirname(const string& path) {
    string result;
    char tmp[MAXPATHLEN];
    strncpy(tmp, path.c_str(), MAXPATHLEN-1);
    result = ::dirname(tmp);
    return result;
}

string FileTable::GetBasePath(void) {return basePath;}
string FileTable::GetBasePathAlias(void) {return basePathAlias;}

/*
static const char* memrchr(const char* s, char c, size_t n) {
    if (n > 0) {
        const char*  p = s;
        const char*  q = p + n;
        while (1) {
            q--; if (q < p || q[0] == c) break;
            q--; if (q < p || q[0] == c) break;
            q--; if (q < p || q[0] == c) break;
            q--; if (q < p || q[0] == c) break;
        }
        if (q >= p)
            return q;
    }
    return NULL;
}
*/
 
string FileTable::ResolveAlias(const string& path) {
    string result = path;
    if(basePathAlias != "/" && path.find(basePathAlias) == 0)
        result.erase(0, basePathAlias.length());
    if(result.length()==0)
        result = "/";
    return result;
}

string FileTable::Canonicalize(const string& _path) {
#if 1
    string path = basePath;
    path += ResolveAlias(_path);
    string dir  = dirname(path);
    string base = basename(path);
    
    string result;
    char* resolved = ::realpath(dir.c_str(), NULL);
    if(resolved) {
        result = resolved;
        result += "/";
        result += base;
        free(resolved);
        if(basePath != "/" && path.find(basePath) == 0) {
            result.erase(0, basePath.length());
            if(result.length()==0)
                result = "/";
            return result;
        }
    }
    return _path;
#else
    char pwd[] = "/";
    char res[MAXPATHLEN];
    const char* src = _path.c_str();
    size_t res_len;
    size_t src_len = strlen(src);
    
    const char * ptr = src;
    const char * end = &src[src_len];
    const char * next;
    
    if (src_len == 0 || src[0] != '/') {
        // relative path
        size_t pwd_len;
        
        pwd_len = strlen(pwd);
        memcpy(res, pwd, pwd_len);
        res_len = pwd_len;
    } else {
        res_len = 0;
    }
    
    for (ptr = src; ptr < end; ptr=next+1) {
        size_t len;
        next = (char*)memchr(ptr, '/', end-ptr);
        if (next == NULL)
            next = end;
        len = next-ptr;
        switch(len) {
            case 2:
                if (ptr[0] == '.' && ptr[1] == '.') {
                    const char * slash = (char*)memrchr(res, '/', res_len);
                    if (slash != NULL)
                        res_len = slash - res;
                    continue;
                }
                break;
            case 1:
                if (ptr[0] == '.') continue;
                else               break;
            case 0:                continue;
        }
        
        if (res_len != 1)
            res[res_len++] = '/';
        
        memcpy(&res[res_len], ptr, len);
        res_len += len;
    }
    
    if (res_len == 0) res[res_len++] = '/';
    res[res_len] = '\0';
    
    return res;
#endif
}

int FileTable::Stat(const string& _path, struct stat& fstat) {
    string     path   = ResolveAlias(_path);
    int        result = stat(path, fstat);
    FileAttrs* attrs  = GetFileAttrs(path);
    
    if(attrs) {
        if(FileAttrs::Valid(attrs->mode)) {
            uint32_t mode = fstat.st_mode; // copy format & permissions from actual file in the file system
            mode &= ~(S_IWUSR  | S_IRUSR);
            mode |= attrs->mode & (S_IWUSR | S_IRUSR); // copy user R/W permissions from attributes
            if(S_ISREG(fstat.st_mode) && fstat.st_size == 0 && (attrs->mode & S_IFMT)) {
                // mode heursitics: if file is empty we map it to the various special formats (CHAR, BLOCK, FIFO, etc.) from stored attributes
                mode &= ~S_IFMT;                // clear format
                mode |= (attrs->mode & S_IFMT); // copy format from attributes
            }
            fstat.st_mode = mode;
        }
        fstat.st_uid  = FileAttrs::Valid(attrs->uid)  ? attrs->uid  : fstat.st_uid;
        fstat.st_gid  = FileAttrs::Valid(attrs->gid)  ? attrs->gid  : fstat.st_gid;
        fstat.st_rdev = FileAttrs::Valid(attrs->rdev) ? attrs->rdev : fstat.st_rdev;
    }
    
    return result;
}

static uint64_t rotl(uint64_t x, uint64_t n) {return (x<<n) | (x>>(64LL-n));}

string FileTable::MakePath(const string& directory, const string& file) {
    string result = directory;
    if(directory[directory.length()-1] != '/')
        result += "/";
    result += file;
    return result;
}

int FileTable::Remove(const char* fpath, const struct stat* /*sb*/, int /*typeflag*/, struct FTW* /*ftwbuf*/) {
    fchmodat(AT_FDCWD, fpath, ACCESSPERMS, AT_SYMLINK_NOFOLLOW);
    ::remove(fpath);
    return 0;
}

static uint64_t make_file_handle(const struct stat& fstat) {
    uint64_t result = fstat.st_dev;
    result = rotl(result, 32) ^ fstat.st_ino;
    if(result == 0) result = ~result;
    return result;
}

uint64_t FileTable::GetFileHandle(const string& _path) {

    string path = ResolveAlias(_path);
    
    uint64_t result = 0;
    struct stat fstat;
    if(stat(path, fstat) == 0) {
        result = make_file_handle(fstat);
        handle2path[result] = Canonicalize(path);
    } else {
        printf("No file handle for %s\n", _path.c_str());
    }
    return result;
}

bool FileTable::GetCanonicalPath(uint64_t handle, string& result) {
    map<uint64_t, string>::iterator iter = handle2path.find(handle);
    if(iter != handle2path.end()) {
        result = iter->second;
        return true;
    }
    return false;
}

void FileTable::Move(const string& pathFrom, const string&) {
    uint64_t handle = GetFileHandle(pathFrom);
    if(handle) {
        map<uint64_t, string>::iterator iter = handle2path.find(handle);
        if(iter != handle2path.end())
            handle2path.erase(iter);
    }
}

void FileTable::Remove(const string& path) {
    uint64_t handle = GetFileHandle(path);
    if(handle) {
        map<uint64_t, string>::iterator iter = handle2path.find(handle);
        if(iter != handle2path.end()) {
            handle2path.erase(iter);
            GetDB(handle)->Remove(path);
        }
    }
}

FileAttrDB* FileTable::GetDB(uint64_t handle) {
    string   path      = handle2path[handle];
    string   dbdir     = dirname(path);
    uint64_t dirhandle = GetFileHandle(dbdir);
    
    if(dirhandle) {
        FileAttrDB* result = NULL;
        map<uint64_t, FileAttrDB*>::iterator iter = handle2db.find(dirhandle);
        if(iter != handle2db.end())
            result = iter->second;
        else {
            result               = new FileAttrDB(*this, dbdir);
            handle2db[dirhandle] = result;
        }
        return result;
    }
    return NULL;
}

void FileTable::SetFileAttrs(const std::string& path, const FileAttrs& fstat) {
    uint64_t handle = GetFileHandle(path);
    if(handle) GetDB(handle)->SetFileAttrs(path, fstat);
}

FileAttrs* FileTable::GetFileAttrs(const std::string& path) {
    uint64_t handle = GetFileHandle(path);
    return handle ? GetDB(handle)->GetFileAttrs(path) : NULL;
}

void FileTable::Write(void) {
    for(set<FileAttrDB*>::iterator it = dirty.begin(); it != dirty.end(); it++)
        (*it)->Write();
    
    dirty.clear();
}

void FileTable::Dirty(FileAttrDB* db) {
    dirty.insert(db);
}

uint32_t FileTable::FileId(uint64_t ino) {
    uint32_t result = ino;
    return (result ^ (ino >> 32LL)) & 0x7FFFFFFF;
}

//----- file io

File::File(FileTable* ft, const std::string& path, const std::string& mode) : ft(ft), path(path), restoreStat(false), file(NULL) {
    file = ::fopen(ft->ToHostPath(path).c_str(), mode.c_str());
    if(!(file) && errno == EACCES) {
        restoreStat = true;
        ft->stat(path, fstat);
        ft->chmod(path, fstat.st_mode);
        file = ::fopen(ft->ToHostPath(path).c_str(), mode.c_str());
    }
}

File::~File(void) {
    if(restoreStat) {
        ft->chmod(path, fstat.st_mode);
        struct timeval times[2];
#if HAVE_STRUCT_STAT_ST_ATIMESPEC
        times[0].tv_sec  = fstat.st_atimespec.tv_sec;
        times[0].tv_usec = fstat.st_atimespec.tv_nsec / 1000;
#else
        times[0].tv_sec  = fstat.st_atim.tv_sec;
        times[0].tv_usec = fstat.st_atim.tv_nsec / 1000;
#endif
#if HAVE_STRUCT_STAT_ST_MTIMESPEC
        times[1].tv_sec  = fstat.st_mtimespec.tv_sec;
        times[1].tv_usec = fstat.st_mtimespec.tv_nsec / 1000;
#else
        times[1].tv_sec  = fstat.st_mtim.tv_sec;
        times[1].tv_usec = fstat.st_mtim.tv_nsec / 1000;
#endif
        ft->utimes(path, times);
    }
    if(file) fclose(file);
}

int File::Read(size_t fileOffset, void* dst, size_t count) {
    ::fseek(file, fileOffset, SEEK_SET);
    return ::fread(dst, sizeof(uint8_t), count, file);
}

int File::Write(size_t fileOffset, void* src, size_t count) {
    ::fseek(file, fileOffset, SEEK_SET);
    return ::fwrite(src, sizeof(uint8_t), count, file);
}

bool File::IsOpen(void) {
    return file != NULL;
}

string FileTable::ToHostPath(const string& path) {
    if(path[0] != '/')
        throw __LINE__;
    
    string result = basePath;
    result += path;
    return result;
}

static int get_error(int result) {
    return result < 0 ? errno : result;
}

int FileTable::chmod(const string& path, mode_t mode) {
    return get_error(::fchmodat(AT_FDCWD, ToHostPath(path).c_str(), mode | S_IWUSR  | S_IRUSR, AT_SYMLINK_NOFOLLOW));
}

int FileTable::access(const string& path, int mode) {
    return get_error(::access(ToHostPath(path).c_str(), mode));
}

DIR* FileTable::opendir(const string& path) {
    return ::opendir(ToHostPath(path).c_str());
}

int FileTable::remove(const string& path) {
    return get_error(::remove(ToHostPath(path).c_str()));
}

int FileTable::rename(const string& from, const string& to) {
    return get_error(::rename(ToHostPath(from).c_str(), ToHostPath(to).c_str()));
}

int FileTable::readlink(const string& _path, string& result) {
    string path = ToHostPath(_path);
    
    struct stat sb;
    ssize_t nbytes, bufsiz;
    
    if (lstat(path.c_str(), &sb) == -1)
        return errno;
    
    /* Add one to the link size, so that we can determine whether
     the buffer returned by readlink() was truncated. */
    
    bufsiz = sb.st_size + 1;
    
    /* Some magic symlinks under (for example) /proc and /sys
     report 'st_size' as zero. In that case, take PATH_MAX as
     a "good enough" estimate. */
    
    if (sb.st_size == 0)
        bufsiz = MAXPATHLEN;
    
    char buf[bufsiz];
    
    nbytes = ::readlink(path.c_str(), buf, bufsiz);
    if (nbytes == -1)
        return errno;
    
    buf[nbytes] = '\0';
    result = buf;
    
    return 0;
}

int FileTable::link(const string& _from, const string& _to, bool soft) {
    string from = soft ? _from : ToHostPath(_from);
    string to   = ToHostPath(_to);
    
    if(soft) return get_error(::symlink(from.c_str(), to.c_str()));
    else     return get_error(::link   (to.c_str(), from.c_str()));
}

int FileTable::mkdir(const string& path, mode_t mode) {
    return get_error(::mkdir(ToHostPath(path).c_str(), mode));
}

int FileTable::nftw(const string& path, int (*fn)(const char *, const struct stat *ptr, int flag, struct FTW *), int depth, int flags) {
    return get_error(::nftw(ToHostPath(path).c_str(), fn, depth, flags));
}

int FileTable::statvfs(const string& path, struct statvfs& fsstat) {
    return get_error(::statvfs(ToHostPath(path).c_str(), &fsstat));
}

int FileTable::stat(const string& path, struct stat& fstat) {
    return get_error(::lstat(ToHostPath(path).c_str(), &fstat));
}

int FileTable::utimes(const string& path, const struct timeval times[2]) {
    return get_error(::lutimes(ToHostPath(path).c_str(), times));
}

//----- file attribute database

FileAttrDB::FileAttrDB(FileTable& ft, const string& directory) : ft(ft) {
    path = FileTable::MakePath(directory, FILE_TABLE_NAME);
    File file(&ft, path, "r");
    if(file.IsOpen()) {
        for(;;) {
            string fname;
            FileAttrs* fattrs = new FileAttrs(file, fname);
            if(fname.length() == 0) {
                delete fattrs;
                break;
            }
            attrs[fname] = fattrs;
        }
    }
}

FileAttrDB::~FileAttrDB() {
    for(map<string, FileAttrs*>::iterator it = attrs.begin(); it != attrs.end(); it++)
        delete it->second;
}

FileAttrs* FileAttrDB::GetFileAttrs(const std::string& path) {
    string fname = FileTable::basename(path);
    
    map<string, FileAttrs*>::iterator iter = attrs.find(fname);
    return iter == attrs.end() ? NULL : iter->second;
}

void FileAttrDB::SetFileAttrs(const std::string& path, const FileAttrs& fattrs) {
    string fname = FileTable::basename(path);
    if("." == fname || ".." == fname) return;
    
    map<string, FileAttrs*>::iterator iter = attrs.find(fname);
    if(iter == attrs.end()) attrs[fname] = new FileAttrs(fattrs);
    else                    attrs[fname]->Update(fattrs);
    
    ft.Dirty(this);
}

void FileAttrDB::Remove(const std::string& path) {
    string fname = FileTable::basename(path);
    
    map<string, FileAttrs*>::iterator iter = attrs.find(fname);
    if(iter != attrs.end()) {
        delete iter->second;
        attrs.erase(iter);
        ft.Dirty(this);
    }
}

void FileAttrDB::Write(void) {
    if(attrs.size() > 0) {
        File file(&ft, path, "w");
        
        if(file.IsOpen()) {
            for(map<string, FileAttrs*>::iterator it = attrs.begin(); it != attrs.end(); it++)
                it->second->Write(file, it->first);
        }
    }
}
