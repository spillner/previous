//
//  FileTableNFSD.cpp
//  Previous
//
//  Created by Simon Schubiger on 04.03.19.
//

#include "FileTableNFSD.h"
#include "devices.h"
#include "compat.h"

using namespace std;

FileTableNFSD::FileTableNFSD(const string& basePath, const string& basePathAlias) : FileTable(basePath, basePathAlias), mutex(host_mutex_create()) {
    for(int d = 0; DEVICES[d][0]; d++) {
        const char* perm  = DEVICES[d][0];
        uint32_t    major = atoi(DEVICES[d][1]);
        uint32_t    minor = atoi(DEVICES[d][2]);
        const char* name  = DEVICES[d][3];
        uint32_t    rdev  = major << 8 | minor;
        if(     perm[0] == 'b') blockDevices[name]     = rdev;
        else if(perm[0] == 'c') characterDevices[name] = rdev;
    }
    
    host_atomic_set(&doRun, 1);
    thread = host_thread_create(&ThreadProc, "FileTable", this);
}

FileTableNFSD::~FileTableNFSD(void) {
    host_atomic_set(&doRun, 0);
    host_thread_wait(thread);
    {
        NFSDLock lock(mutex);
        
        for(map<uint64_t,FileAttrDB*>::iterator it = handle2db.begin(); it != handle2db.end(); it++)
            delete it->second;
        handle2db.clear();
    }
    host_mutex_destroy(mutex);
}

int FileTableNFSD::ThreadProc(void *lpParameter) {
    ((FileTableNFSD*)lpParameter)->Run();
    return 0;
}

void FileTableNFSD::Run(void) {
    const int POLL = 10;
    for(int count = POLL; host_atomic_get(&doRun); count--) {
        host_sleep_sec(1);
        {
            NFSDLock lock(mutex);
            if(count <= 0 && dirty.size()) {
                Write();
                count = POLL;
            }
        }
    }
}

bool FileTableNFSD::IsBlockDevice(const string& fname) {
    return blockDevices.find(fname) != blockDevices.end();
}

bool FileTableNFSD::IsCharDevice(const string& fname) {
    return characterDevices.find(fname) != characterDevices.end();
}

bool FileTableNFSD::IsDevice(const string & path, string& fname) {
    string   directory = FileTable::dirname(path);
    fname              = FileTable::basename_helper((std::string const &) path);
    
    const size_t len = directory.size();
    return
    len >= 4 &&
    directory[len-4] == '/' &&
    directory[len-3] == 'd' &&
    directory[len-2] == 'e' &&
    directory[len-1] == 'v' &&
    (IsBlockDevice(fname) || IsCharDevice(fname));
}

int FileTableNFSD::Stat(const std::string& path, struct stat& fstat) {
    NFSDLock lock(mutex);
    
    int result = FileTable::Stat(path, fstat);
    
    string fname;
    if(fstat.st_rdev == (unsigned long) FATTR_INVALID) {
        if(IsDevice(path, fname)) {
            map<string,uint32_t>::iterator iter = blockDevices.find(fname);
            if(iter != blockDevices.end()) {
                fstat.st_mode &= ~S_IFMT;
                fstat.st_mode |= S_IFBLK;
                fstat.st_rdev = iter->second;
            }
            iter = characterDevices.find(fname);
            if(iter != characterDevices.end()) {
                fstat.st_mode &= ~S_IFMT;
                fstat.st_mode |= S_IFCHR;
                fstat.st_rdev = iter->second;
            }
        }
    }
    
    return result;
}
bool FileTableNFSD::GetCanonicalPath(uint64_t fhandle, std::string& result) {
    NFSDLock lock(mutex);
    return FileTable::GetCanonicalPath(fhandle, result);
}
void FileTableNFSD::Move(const std::string& pathFrom, const std::string& pathTo) {
    NFSDLock lock(mutex);
    return FileTable::Move(pathFrom, pathTo);
}
void  FileTableNFSD::Remove(const std::string& path) {
    NFSDLock lock(mutex);
    return FileTable::Remove(path);
}
uint64_t FileTableNFSD::GetFileHandle(const std::string& path) {
    NFSDLock lock(mutex);
    return FileTable::GetFileHandle(path);
}
void FileTableNFSD::SetFileAttrs(const std::string& path, const FileAttrs& fstat) {
    NFSDLock lock(mutex);
    return FileTable::SetFileAttrs(path, fstat);
}
void FileTableNFSD::Write(void) {
    NFSDLock lock(mutex);
    FileTable::Write();
}

FileAttrDB* FileTableNFSD::GetDB(uint64_t handle) {
    NFSDLock lock(mutex);
    return FileTable::GetDB(handle);
}

FileAttrs* FileTableNFSD::GetFileAttrs(const std::string& path) {
    NFSDLock lock(mutex);
    return FileTable::GetFileAttrs(path);
}
