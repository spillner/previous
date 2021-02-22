#ifndef _XDRSTREAM_H_
#define _XDRSTREAM_H_

#include <string>
#include <stdio.h>
#include <stdint.h>

class XDROpaque {
public:
    size_t   m_size;
    uint8_t* m_data;
    bool     m_deleteData;
    
    XDROpaque();
    XDROpaque(size_t size);
    XDROpaque(const void* data, size_t size);
    virtual ~XDROpaque();
    
    void Set(const XDROpaque& src);
    void SetSize(size_t size);
    virtual void Set(const void* dataWillBeCopied, size_t size);
};

class XDRString : public XDROpaque {
    char* m_str;
public:
    XDRString(void);
    XDRString(std::string& str);
    virtual ~XDRString();
    
    const char*  Get(void);
    virtual void Set(const void* dataWillBeCopied, size_t size);
    void         Set(const char* str);
};

class XDRStream {
protected:
    bool     m_deleteBuffer;
    uint8_t* m_buffer;
    size_t   m_capacity;
    size_t   m_size;
    size_t   m_index;
    
    XDRStream(bool deleteBuffer, uint8_t* buffer, size_t capacity, size_t size);
    size_t   AlignIndex(void);
public:
    size_t   GetSize(void);
    uint8_t* GetBuffer(void);
    void     SetSize(size_t nSize);
    size_t   GetCapacity(void);
    size_t   GetPosition(void);
    void     Reset(void);
};

class XDROutput : public XDRStream {
public:
    XDROutput();
    ~XDROutput();
    void     Write(void *pData, size_t nSize);
    void     Write(uint32_t nValue);
    void     Seek(int nOffset, int nFrom);
    void     Write(XDROpaque& opaque);
    void     Write(XDRString& string);
    void     Write(size_t maxLen, const char* format, ...);
};

class XDRInput : public XDRStream {
public:
    XDRInput();
    XDRInput(XDROpaque& opaque);
    XDRInput(uint8_t* data, size_t size);
    ~XDRInput();
    size_t   Read(void *pData, size_t nSize);
    size_t   Read(uint32_t* pnValue);
    size_t   Read(XDROpaque& opaque);
    size_t   Skip(ssize_t nSize);
};

#endif
