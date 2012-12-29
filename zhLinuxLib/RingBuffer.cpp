/*
 * RingBuffer.cpp
 *
 *  Created on: 2012-4-18
 *      Author: zh
 */

#include <string.h>
#include "RingBuffer.h"

namespace NS_ZH_UTIL {

void CRingBuffer::expand(unsigned size)
{
	if(size > 0)
	{
		unsigned char* pBuf = new unsigned char[m_dataBufSize+size];
		//
		unsigned usedSize = m_dataBufSize-m_freeBufSize;
		if(usedSize > 0)
		{
			if(usedSize <= m_dataBufSize - m_readPos)
			{
				::memcpy(pBuf, m_pDataBuf+m_readPos, usedSize);
			}
			else
			{
				unsigned usedSize_1 = m_dataBufSize - m_readPos;
				::memcpy(pBuf, m_pDataBuf+m_readPos, usedSize_1);
				usedSize -= usedSize_1;
				::memcpy(pBuf+usedSize_1, m_pDataBuf, usedSize);
			}
			m_readPos = 0;
			m_dataBufSize += size;
			m_freeBufSize += size;
			m_writePos = usedSize%m_dataBufSize;
		}
		else
		{
			m_readPos = 0;
			m_dataBufSize += size;
			m_freeBufSize += size;
			m_writePos = 0;
		}
		//
		delete [] m_pDataBuf; m_pDataBuf = pBuf;
	}
}

int CRingBuffer::memcpy(void* dest, unsigned len)
{
	if(getDataSize() < len) return -1;
	if(len <= m_dataBufSize - m_readPos)
	{
		::memcpy(dest, m_pDataBuf+m_readPos, len);
		m_readPos = (m_readPos+len)%m_dataBufSize;
		m_freeBufSize += len;
	}
	else
	{
		unsigned len_1 = m_dataBufSize - m_readPos;
		::memcpy(dest, m_pDataBuf+m_readPos, len_1);
		len -= len_1;
		::memcpy((unsigned char*)dest + len_1, m_pDataBuf, len);
		m_freeBufSize += len_1 + len;
		m_readPos = len;
	}
	//
	//
	return 0;
}

int CRingBuffer::peek(void* dest, unsigned len)
{
	if(getDataSize() < len) return -1;
	if(len <= m_dataBufSize - m_readPos)
	{
		::memcpy(dest, m_pDataBuf+m_readPos, len);
	}
	else
	{
		unsigned len_1 = m_dataBufSize - m_readPos;
		::memcpy(dest, m_pDataBuf+m_readPos, len_1);
		len -= len_1;
		::memcpy((unsigned char*)dest + len_1, m_pDataBuf, len);
	}
	return 0;
}

} /* namespace NS_JOYIT_UTIL */
