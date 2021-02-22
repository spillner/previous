#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include "XDRStream.h"

using namespace std;

#define MAXDATA (1024 * 1024)

XDROpaque::XDROpaque() : m_size(0), m_data(NULL), m_deleteData(false) {}

XDROpaque::XDROpaque(size_t size) : m_size(size), m_data(new uint8_t[size]), m_deleteData(true) {}

XDROpaque::XDROpaque(const void* data, size_t size) : m_size(0), m_data(NULL), m_deleteData(false) {
    Set(data, size);
}

XDROpaque::~XDROpaque() {
    if(m_deleteData) delete[] m_data;
}

void XDROpaque::Set(const XDROpaque& src) {
    Set(src.m_data, src.m_size);
}

void XDROpaque::SetSize(size_t size) {
    Set(m_data, size);
}

void XDROpaque::Set(const void* data, size_t size) {
    if(data == m_data && size <= m_size) {
        m_size = size;
    } else {
        uint8_t* toDelete = m_deleteData ? m_data : NULL;
        m_deleteData = true;
        m_size       = size;
        m_data       = new uint8_t[size];
        if(data) memcpy(m_data, data, size);
        delete[] toDelete;
    }
}

XDRString::XDRString(string& str) : XDROpaque(str.c_str(), str.size()), m_str(NULL) {}

XDRString::XDRString(void) : XDROpaque(), m_str(NULL) {}

const char* XDRString::Get(void) {
    if(!(m_str)) {
        m_str = new char[m_size+1];
        memcpy(m_str, m_data, m_size);
        m_str[m_size] = '\0';
    }
    return m_str;
}

void XDRString::Set(const void* data, size_t size) {
    XDROpaque::Set(data, size);
    if(m_str) delete[ ] m_str;
    m_str = NULL;
}

void XDRString::Set(const char* str) {
    Set((uint8_t*)str, strlen(str) + 1);
}

XDRString::~XDRString() {
    if(m_str) delete[] m_str;
}

XDRStream::XDRStream(bool deleteBuffer, uint8_t* buffer, size_t capacity, size_t size) :
m_deleteBuffer(deleteBuffer),
m_buffer (buffer),
m_capacity(capacity),
m_size (size),
m_index (0) {}

size_t XDRStream::AlignIndex(void) {
    m_index = (m_index + 3) & 0xFFFFFFFC;
    if (m_index > m_size) m_size = m_index;
    return m_index;
}
uint8_t* XDRStream::GetBuffer(void) {
    return m_buffer;
}

void XDRStream::SetSize(size_t nSize) {
    m_index = 0;  //seek to the beginning of the input buffer
    m_size = nSize;
}

size_t XDRStream::GetCapacity(void) {return m_capacity;}

size_t XDRStream::GetPosition(void) {return m_index;}

size_t XDRStream::GetSize(void) {return m_size;}

void XDRStream::Reset(void) {m_index = m_size = 0;}

XDRInput::XDRInput()                           : XDRStream(true,  new uint8_t[MAXDATA], MAXDATA,       0)             {}
XDRInput::XDRInput(XDROpaque& opaque)          : XDRStream(false, opaque.m_data,        opaque.m_size, opaque.m_size) {}
XDRInput::XDRInput(uint8_t* data, size_t size) : XDRStream(false, data,                 size,          size)          {}

XDRInput::~XDRInput() {
    if(m_deleteBuffer)  delete[] m_buffer;
}

size_t XDRInput::Read(void *pData, size_t nSize) {
	if (nSize > m_size - m_index)  //over the number of bytes of data in the input buffer
		nSize = m_size - m_index;
	memcpy(pData, m_buffer + m_index, nSize);
	m_index += nSize;
	return nSize;
}

size_t XDRInput::Read(uint32_t* val) {
    uint32_t hval;
    size_t count = Read(&hval, sizeof(uint32_t));
    *val         = ntohl(hval);
    return count;
}

size_t XDRInput::Read(XDROpaque& opaque) {
    Read((uint32_t*)&opaque.m_size);
    if(opaque.m_deleteData) delete [] opaque.m_data;
    opaque.m_data = &m_buffer[m_index];
    if(Skip(opaque.m_size) < opaque.m_size)
        throw __LINE__;
    AlignIndex();
    return 4 + opaque.m_size;
}

size_t XDRInput::Skip(ssize_t nSize)
{
	if (nSize > (ssize_t)(m_size - m_index))  //over the number of bytes of data in the input buffer
		nSize = m_size - m_index;
	m_index += nSize;
	return nSize;
}

XDROutput::XDROutput() : XDRStream(true,  new uint8_t[MAXDATA], MAXDATA, 0) {}

XDROutput::~XDROutput() {
    if(m_deleteBuffer) delete[] m_buffer;
}

void XDROutput::Write(void *pData, size_t nSize) {
	if (m_index + nSize > m_capacity)  //over the size of output buffer
		nSize = m_capacity - m_index;
	memcpy(m_buffer + m_index, pData, nSize);
	m_index += nSize;
	if (m_index > m_size)
		m_size = m_index;
}

void XDROutput::Write(XDROpaque& opaque) {
    Write(opaque.m_size);
    Write(opaque.m_data, opaque.m_size);
    AlignIndex();
}

void XDROutput::Write(XDRString& string) {
    Write((XDROpaque&)string);
}

void XDROutput::Write(uint32_t val) {
    uint32_t nval= htonl(val);
	Write(&nval, sizeof(uint32_t));
}

void XDROutput::Seek(int nOffset, int nFrom) {
	if      (nFrom == SEEK_SET) m_index = nOffset;
	else if (nFrom == SEEK_CUR) m_index += nOffset;
	else if (nFrom == SEEK_END) m_index = m_size + nOffset;
}

void XDROutput::Write(size_t maxLen, const char* format, ...) {
    va_list vargs;
    
    va_start(vargs, format);
    size_t len = vsnprintf((char*)&m_buffer[m_index+4], maxLen, format, vargs);
    Write(len);
    va_end(vargs);
    m_index += len;
    AlignIndex();
}

