/*
 * RingBuffer.h
 *
 *  Created on: 2012-4-18
 *      Author: zh
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_
#include <string.h>

namespace NS_ZH_UTIL {

struct CRingBuffer
{
	unsigned char *m_pDataBuf;
	unsigned m_dataBufSize;
	unsigned m_freeBufSize;
	unsigned m_writePos;
	unsigned m_readPos;
	//
	CRingBuffer() {::memset(this, 0, sizeof(*this));}
	CRingBuffer(unsigned size)
	{
		::memset(this, 0, sizeof(*this));
		if(size > 0) {m_dataBufSize = m_freeBufSize = size; m_pDataBuf = new unsigned char[m_dataBufSize];}
	}
	~CRingBuffer() {if(m_pDataBuf) {delete [] m_pDataBuf; m_pDataBuf = 0;}}
	//
	unsigned char* getReadPtr() {return m_pDataBuf+m_readPos;}
	unsigned char* getWritePtr() {return m_pDataBuf+m_writePos;}
	unsigned getDataSize() {return m_dataBufSize-m_freeBufSize;}
	unsigned getFreeSize() {return m_freeBufSize;}
	unsigned getSize()     {return m_dataBufSize;}
	unsigned getSizeFromReadPos2End() {return m_dataBufSize-m_readPos;}
	unsigned getSizeFromWritePos2End() {return m_dataBufSize-m_writePos;}
	void advanceWritePtr(unsigned size) {m_writePos=(m_writePos+size)%m_dataBufSize;m_freeBufSize-=size;}
	void advanceReadPtr(unsigned size) {m_readPos=(m_readPos+size)%m_dataBufSize;m_freeBufSize+=size;}
	void reset() {m_writePos = m_readPos = 0; m_freeBufSize = m_dataBufSize;}
	int memcpy(void* dest, unsigned len);
	int peek(void* dest, unsigned len);
	void expand(unsigned size);
};

} /* namespace NS_JOYIT_UTIL */
#endif /* RINGBUFFER_H_ */
