//
//  FileTableNFSD.hpp
//  Previous
//
//  Created by Simon Schubiger on 04.03.19.
//

#ifndef FileTableNFSD_hpp
#define FileTableNFSD_hpp

#include "../../ditool/FileTable.h"
#include "XDRStream.h"
#include "host.h"

class FileTableNFSD : public FileTable {
    atomic_int                      doRun;
    thread_t*                       thread;
    mutex_t*                        mutex;
    
    std::map<std::string, uint32_t> blockDevices;
    std::map<std::string, uint32_t> characterDevices;

    static int  ThreadProc(void *lpParameter);
    
    bool        IsBlockDevice(const std::string& fname);
    bool        IsCharDevice(const std::string& fname);
    bool        IsDevice    (const std::string & path, std::string& fname);
    void        Write       (void);
    FileAttrDB* GetDB       (uint64_t handle);
    FileAttrs*  GetFileAttrs(const std::string& path);
public:
    FileTableNFSD(const std::string& basePath, const std::string& basePathAlias);
    virtual ~FileTableNFSD(void);
    
    int         Stat           (const std::string& path, struct stat& stat);
    bool        GetCanonicalPath(uint64_t fhandle, std::string& result);
    void        Move           (const std::string& pathFrom, const std::string& pathTo);
    void        Remove         (const std::string& path);
    uint64_t    GetFileHandle  (const std::string& path);
    void        SetFileAttrs   (const std::string& path, const FileAttrs& fstat);
    
    void Run(void);
};

#endif /* FileTableNFSD_hpp */
