#include <fstream>
#include <sstream>

#include "MountProg.h"
#include "FileTableNFSD.h"
#include "nfsd.h"
#include "compat.h"

using namespace std;

enum
{
	MNT_OK = 0,
    MNTERR_PERM = 1,
    MNTERR_NOENT = 2,
	MNTERR_IO = 5,
    MNTERR_ACCESS = 13,
	MNTERR_NOTDIR = 20,
    MNTERR_INVAL = 22
};

CMountProg::CMountProg() : CRPCProg(PROG_MOUNT, 3, "mountd"), m_nMountNum(0) {
	m_exportPath[0] = '\0';
	memset(m_clientAddr, 0, sizeof(m_clientAddr));
    #define RPC_PROG_CLASS CMountProg
    SetProc(1, MNT);
    SetProc(3, UMNT);
    SetProc(5, EXPORT);
}

CMountProg::~CMountProg() {
	for (int i = 0; i < MOUNT_NUM_MAX; i++) delete[] m_clientAddr[i];
}

int CMountProg::GetMountNumber(void) const {
	return m_nMountNum;  //the number of clients mounted
}

const char* CMountProg::GetClientAddr(int nIndex) const {
	if (nIndex < 0 || nIndex >= m_nMountNum)
		return NULL;
	for (int i = 0; i < MOUNT_NUM_MAX; i++) {
        if (m_clientAddr[i]) {
            if (nIndex == 0) {
				return m_clientAddr[i];  //client address
            } else {
				--nIndex;
            }
        }
	}
	return NULL;
}

int CMountProg::ProcedureMNT(void) {
    XDRString path;
    int i, addr_len;

    m_in->Read(path);
    Log("MNT from %s for '%s'\n", m_param->remoteAddr, path.Get());
    
    uint64_t handle = nfsd_fts[0]->GetFileHandle(path.Get());
    if(handle) {
        m_out->Write(MNT_OK); //OK
        
        uint64_t data[8] = {handle, 0, 0, 0, 0, 0, 0, 0};
        
        if (m_param->version == 1) {
            m_out->Write(data, FHSIZE);
        } else {
            m_out->Write(FHSIZE_NFS3);
            m_out->Write(data, FHSIZE_NFS3);
            m_out->Write(0);  //flavor
        }
        
        ++m_nMountNum;
	addr_len = strlen(m_param->remoteAddr);
        
        for (i = 0; i < MOUNT_NUM_MAX; i++) {
            if (m_clientAddr[i] == NULL) { //search an empty space
                m_clientAddr[i] = new char[addr_len + 1];
                strncpy(m_clientAddr[i], m_param->remoteAddr, addr_len + 1);  //remember the client address
                break;
            }
        }
    } else {
        m_out->Write(MNTERR_ACCESS);  //permission denied
    }
    
    return PRC_OK;
}

int CMountProg::ProcedureUMNT(void) {
    XDRString path;
    m_in->Read(path);
    Log("UNMT from %s for '%s'", m_param->remoteAddr, path.Get());
    
    for (int i = 0; i < MOUNT_NUM_MAX; i++) {
        if (m_clientAddr[i] != NULL) {
            if (strcmp(m_param->remoteAddr, m_clientAddr[i]) == 0) { //address match
                delete[] m_clientAddr[i];  //remove this address
                m_clientAddr[i] = NULL;
                --m_nMountNum;
                break;
            }
        }
    }
    
    return PRC_OK;
}

int CMountProg::ProcedureEXPORT(void) {
    Log("EXPORT");
    
    string path = nfsd_fts[0]->GetBasePathAlias();
    // dirpath
    m_out->Write(1);
    m_out->Write(MAXPATHLEN, path.c_str());
    // groups
    m_out->Write(1);
    m_out->Write(1);
    m_out->Write((void*)"*...", 4);
    m_out->Write(0);
    
    m_out->Write(0);
    m_out->Write(0);
    
    return PRC_OK;
}

char *CMountProg::FormatPath(const char *pPath, pathFormats format) {
    size_t len = strlen(pPath);
    
    //Remove head spaces
    while (*pPath == ' ') {
        ++pPath;
        len--;
    }
    
    //Remove tail spaces
    while (len > 0 && *(pPath + len - 1) == ' ') {
        len--;
    }
    
    //Remove windows tail slashes (except when its only a drive letter)
    while (len > 0 && *(pPath + len - 2) != ':' && *(pPath + len - 1) == '\\') {
        len--;
    }
    
    //Remove unix tail slashes
    while (len > 1 && *(pPath + len - 1) == '/') {
        len--;
    }
    
    //Is comment?
    if (*pPath == '#') {
        return NULL;
    }
    
    //Remove head "
    if (*pPath == '"') {
        ++pPath;
        len--;
    }
    
    //Remove tail "
    if (len > 0 && *(pPath + len - 1) == '"') {
        len--;
    }
    
    if (len < 1) {
        return NULL;
    }
    
    char *result = (char *)malloc(len + 1);
    strncpy_s(result, len + 1, pPath, len);
    
    //Check for right path format
    if (format == FORMAT_PATH) {
        if (result[0] == '.') {
            static char path1[MAXPATHLEN];
            getcwd(path1, MAXPATHLEN);
            
            if (result[1] == '\0') {
                len = strlen(path1);
                result = (char *)realloc(result, len + 1);
                strcpy_s(result, len + 1, path1);
            } else if (result[1] == '/') {
                strcat_s(path1, result + 1);
                len = strlen(path1);
                result = (char *)realloc(result, len + 1);
                strcpy_s(result, len + 1, path1);
            }
            
        }
        if (len >= 2 && result[1] == ':' && ((result[0] >= 'A' && result[0] <= 'Z') || (result[0] >= 'a' && result[0] <= 'z'))) { //check path format
            char tempPath[MAXPATHLEN] = "\\\\?\\";
            strcat_s(tempPath, result);
            len = strlen(tempPath);
            result = (char *)realloc(result, len + 1);
            strcpy_s(result, len + 1, tempPath);
        }
        
        if (len < 6 || (result[0] != '/' && (result[5] != ':' || !((result[4] >= 'A' && result[4] <= 'Z') || (result[4] >= 'a' && result[4] <= 'z'))))) { //check path format
            Log("Path %s format is incorrect.", pPath);
            Log("Please use a full path such as C:\\previous or \\\\?\\C:\\previous or /home/foo/previous");
            free(result);
            return NULL;
        }
        
        for (size_t i = 0; i < len; i++)
            if (result[i] == '\\')
                result[i] = '/';
    } else if (format == FORMAT_PATHALIAS) {
        if (pPath[1] == ':' && ((pPath[0] >= 'A' && pPath[0] <= 'Z') || (pPath[0] >= 'a' && pPath[0] <= 'z'))) {
            strncpy_s(result, len + 1, pPath, len);
            //transform Windows format to mount path d:\work => /d/work
            result[1] = result[0];
            result[0] = '/';
            for (size_t i = 2; i < strlen(result); i++) {
                if (result[i] == '\\') {
                    result[i] = '/';
                }
            }
        } else if (pPath[0] != '/') { //check path alias format
            Log("Path alias format is incorrect.\n");
            Log("Please use a path like /exports\n");
            free(result);
            return NULL;
        }
    }
    
    return result;
}
