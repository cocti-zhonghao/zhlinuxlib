#ifndef OSUTIL_H_INCLUDED
#define OSUTIL_H_INCLUDED

#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>

namespace NS_ZH_UTIL {

typedef struct SCPU_Load
{
	unsigned long _user;
	unsigned long _nice;
	unsigned long _system;
	unsigned long _idle;
	unsigned long _irq;
	unsigned long _iowait;
	unsigned long _softirq;
	SCPU_Load() {memset(this, 0, sizeof(SCPU_Load));}
	unsigned long total() {return _user+_nice+_system+_idle+_irq+_iowait+_softirq;}
	unsigned long idle() {return _idle;}
	unsigned long used() {return total()-_idle;}
} SCPU_Load;

typedef struct SIP_Info
{
	char _ipaddr[32];
	char _mask[32];
	char _gateway[32];
	char _phyaddr[32];
	SIP_Info() {memset(this, 0, sizeof(SIP_Info));}
} SIP_Info;
int CaculateMemLoad(); //
int GetCPULoad(SCPU_Load &load);
int CaculateCPULoad(SCPU_Load &load1, SCPU_Load &load2);
int CaculateDiskLoad();
void GetIP(const char *ifName, const SIP_Info& ip);
int SetIP(const char* ifName, const SIP_Info& ip);
int Reboot();
int send_n(int sock, const void* data, size_t len, struct timeval timeout/*={0,0}*/);
long operator-(struct timespec& t1, struct timespec& t2);
void delay(int ms);

//
extern unsigned char BitMaskBit1[256];
extern unsigned char BitMaskFirstBit0[256];
extern unsigned char BitMaskFirstBit1[256];
extern unsigned char BitMaskLastBit0[256];
extern unsigned char BitMaskLastBit1[256];
extern unsigned char BitMaskAll1Mask[8][8];
extern unsigned char BitMaskContinuousBit1[8][256];
extern unsigned char BitMaskContinuousBit0[8][256];

template<unsigned N>
struct TBitMask_N
{
	TBitMask_N(){::memset(m_bitsvector, 0, sizeof(m_bitsvector));}
	void set(int index){assert(index>=0 && index<N); m_bitsvector[index/8]|=(0x01<<(index&0x07));}//index&0x07<=>index%8
	void set() {::memset(m_bitsvector, 0xFF, sizeof(m_bitsvector));}
	void set(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<N && numbits>=0);
		numbits = (numbits > N-startPos) ? N-startPos : numbits;
		//
		int len = 0;
		while(numbits > 0)
		{
			len = (8-(startPos&0x07)); len = len > numbits ? numbits : len;
			m_bitsvector[startPos/8]|=BitMaskAll1Mask[startPos&0x07][len-1];
			numbits -= len;
			startPos += len;
		}
	}
	void clr(int index){assert(index>=0 && index<N); m_bitsvector[index/8]&=~(0x01<<(index&0x07));}
	void clr() {::memset(m_bitsvector, 0, sizeof(m_bitsvector));}
	void clr(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<N);
		numbits = (numbits > N-startPos) ? N-startPos : numbits;
		//
		int len = 0;
		while(numbits > 0)
		{
			len = (8-(startPos&0x07)); len = len > numbits ? numbits : len;
			m_bitsvector[startPos/8]&=~BitMaskAll1Mask[startPos&0x07][len-1];
			numbits -= len;
			startPos += len;
		}
	}
	bool isset(int index){assert(index>=0 && index<N); return (m_bitsvector[index/8]&(0x01<<(index&0x07))) > 0;}
	bool isset(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<N);
		numbits = (numbits > N-startPos) ? N-startPos : numbits;
		int len = 0;
		while(numbits > 0)
		{
			len = (8-(startPos&0x07)); len = len > numbits ? numbits : len;
			if((m_bitsvector[startPos/8]&BitMaskAll1Mask[startPos&0x07][len-1]) > 0) return true;
			numbits -= len;
			startPos += len;

		}
		return false;
	}
	int bit1()
	{
		int r=0; for(int ii = 0; ii<(N+7)/8; ++ii) r+=BitMaskBit1[m_bitsvector[ii]]; return r;
		//another algorithm
		//const unsigned int M1 = 0x55555555;
		//const unsigned int M2 = 0x33333333;
		//const unsigned int M4 = 0x0f0f0f0f;
		////const unsigned int M8 = 0x00ff00ff;
		////const unsigned int M16 = 0x0000ffff;
		//unsigned char byte;
		//for(int ii = 0; ii < (N+7)/8; ++ii)
		//{
		//	byte = m_bitsvector[ii];
		//	byte = (byte & M1) + ((byte >> 1) & M1);
		//	byte = (byte & M2) + ((byte >> 2) & M2);
		//	byte = (byte & M4) + ((byte >> 4) & M4);
		//	r+=byte;
		//}
		//return r;
	}
	int bit0() {return N-bit1();}
	int bit1(int startPos, int endPos)
	{
		assert(startPos>=0 && startPos<N && endPos>=startPos);
		endPos = endPos < N ? endPos : N-1;
		int numbits = endPos-startPos+1;
		int len = 0;
		int r = 0;
		while(numbits > 0)
		{
			len = (8-(startPos&0x07)); len = len > numbits ? numbits : len;
			r += BitMaskBit1[m_bitsvector[startPos/8]&BitMaskAll1Mask[startPos&0x07][len-1]];
			numbits -= len;
			startPos += len;
		}
		return r;
	}
	int bit0(int startPos)
	{
		return bit0(startPos, N-1);
	}
	int bit0(int startPos, int endPos)
	{
		return (endPos-startPos+1)-bit1(startPos, endPos);
	}
	int continuousBit1(int startPos)
	{
		assert(startPos>=0 && startPos<N);
		int r = 0;
		int ii = startPos/8; startPos = startPos&0x07;
		int bit1 = 0;
		while(ii<(N+7)/8)
		{
			bit1 = BitMaskContinuousBit1[startPos][m_bitsvector[ii]];
			if(0==bit1) break;
			r+=bit1;
			if(bit1<(8-startPos)) break;
			++ii;
			startPos=0;
		}
		return r;
	}
	int continuousBit0(int startPos)
	{
		assert(startPos>=0 && startPos<N);
		int r = 0;
		int ii = startPos/8; startPos = startPos&0x07;
		int bit0 = 0;
		while(ii<(N+7)/8)
		{
			bit0 = BitMaskContinuousBit0[startPos][m_bitsvector[ii]];
			if(0==bit0) break;
			r+=bit0;
			if(bit0<(8-startPos)) break;
			++ii;
			startPos=0;
		}
		return r;
	}
	int first_bit0()
	{
		for(int ii = 0; ii < (N+7)/8; ++ii) if(BitMaskFirstBit0[m_bitsvector[ii]]!=0xff) return BitMaskFirstBit0[m_bitsvector[ii]]+ii*8;
		return -1;
	}
	int first_bit0(int startPos)
	{
		if(startPos < 0 || startPos >= N) return -1;
		int ii = startPos/8; startPos = startPos&0x07;
		if(startPos > 0)
		{
			do{if((m_bitsvector[ii]&(0x01<<startPos)) == 0) return ii*8+startPos;}while(++startPos < 8);
			++ii;
		}
		//
		for(; ii < (N+7)/8; ++ii) if(BitMaskFirstBit0[m_bitsvector[ii]]!=0xff) return BitMaskFirstBit0[m_bitsvector[ii]]+ii*8;
		return -1;
	}
	int last_bit0()
	{
		static const int c = (N+7)/8;
		for(int ii = c-1; ii >= 0; --ii) if(BitMaskLastBit0[m_bitsvector[ii]]!=0xff) return BitMaskLastBit0[m_bitsvector[ii]]+ii*8;
		return -1;
	}
	int first_bit1()
	{
		static const int c = (N+7)/8;
		for(int ii = 0; ii < c; ++ii) if(BitMaskFirstBit1[m_bitsvector[ii]]!=0xff) return BitMaskFirstBit1[m_bitsvector[ii]]+ii*8;
		return -1;
	}
	int first_bit1(int startPos)
	{
		if(startPos < 0 || startPos >= N) return -1;
		int ii = startPos/8; startPos = startPos&0x07;
		if(startPos > 0)
		{
			do{if((m_bitsvector[ii]&(0x01<<startPos)) > 0) return ii*8+startPos;}while(++startPos < 8);
			++ii;
		}
		//
		for(; ii < (N+7)/8; ++ii) if(BitMaskFirstBit1[m_bitsvector[ii]]!=0xff) return BitMaskFirstBit1[m_bitsvector[ii]]+ii*8;
		return -1;
	}
	int last_bit1()
	{
		for(int ii = (N+7)/8-1; ii >= 0; --ii) if(BitMaskLastBit1[m_bitsvector[ii]]!=0xff) return BitMaskLastBit1[m_bitsvector[ii]]+ii*8;
		return -1;
	}
	friend TBitMask_N<N> operator|(const TBitMask_N<N>& op1, const TBitMask_N<N>& op2)
	{
		TBitMask_N<N> temp;
		for(int ii =0; ii<(N+7)/8; ++ii) temp.m_bitsvector[ii]=op1.m_bitsvector[ii]|op2.m_bitsvector[ii];
		return temp;
	}
	friend TBitMask_N<N> operator&(const TBitMask_N<N>& op1, const TBitMask_N<N>& op2)
	{
		TBitMask_N<N> temp;
		for(int ii =0; ii<(N+7)/8; ++ii) temp.m_bitsvector[ii]=op1.m_bitsvector[ii]&op2.m_bitsvector[ii];
		return temp;
	}
	friend TBitMask_N<N> operator^(const TBitMask_N<N>& op1, const TBitMask_N<N>& op2)
	{
		TBitMask_N<N> temp;
		for(int ii =0; ii<(N+7)/8; ++ii) temp.m_bitsvector[ii]=op1.m_bitsvector[ii]^op2.m_bitsvector[ii];
		return temp;
	}
	friend TBitMask_N<N> operator~(const TBitMask_N<N>& op)
	{
		TBitMask_N<N> temp;
		for(int ii =0; ii<(N+7)/8; ++ii) temp.m_bitsvector[ii]=~op.m_bitsvector[ii];
		return temp;
	}
	friend bool operator==(const TBitMask_N<N>& op1, const TBitMask_N<N>& op2)
	{
		return ::memcmp(op1.m_bitsvector, op2.m_bitsvector, (N+7)/8)==0;
	}
private:
	unsigned char m_bitsvector[(N+7)/8];
};

template<>
struct TBitMask_N<8>
{
	TBitMask_N():m_8bits(0){}
	void set(int index){assert(index>=0 && index<8); m_8bits|=(0x01<<index);}
	void set() {m_8bits=0xff;}
	void set(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<8 && numbits>=0);
		if(numbits==0) return;
		if(8-startPos<numbits) numbits=8-startPos;
		m_8bits |= BitMaskAll1Mask[startPos][numbits-1];
	}
	void clr(int index){assert(index>=0 && index<8); m_8bits &=~(0x01<<index);}
	void clr() {m_8bits=0;}
	void clr(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<8 && numbits>=0);
		if(numbits==0) return;
		if(8-startPos<numbits) numbits=8-startPos;
		m_8bits &=~ BitMaskAll1Mask[startPos][numbits-1];
	}
	bool isset(int index)
	{
		assert(index>=0 && index<8); return (m_8bits&(0x01<<index)) > 0;
	}
	bool isset(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<8 && numbits >= 0);
		if(numbits == 0) return false;
		if(startPos + numbits > 8) numbits = 8 - startPos;
		if((m_8bits&BitMaskAll1Mask[startPos][numbits-1]) > 0) return true;//@zh BUG FIXED -> ��������ȼ�
		return false;
	}
	int bit1()
	{
		return BitMaskBit1[m_8bits];
	}
	int bit0() {return 8-bit1();}
	int bit1(int startPos, int endPos)
	{
		assert(startPos>=0 && startPos<8 && endPos>=startPos);
		endPos = endPos < 8 ? endPos : 7;
		return BitMaskBit1[m_8bits&BitMaskAll1Mask[startPos][endPos-startPos]];
	}
	int bit0(int startPos)
	{
		return bit0(startPos, 7);
	}
	int bit0(int startPos, int endPos)
	{
		return (endPos-startPos+1)-bit1(startPos, endPos);
	}
	int continuousBit1(int startPos)
	{
		assert(startPos>=0 && startPos<8);
		return BitMaskContinuousBit1[startPos][m_8bits];
	}
	int continuousBit0(int startPos)
	{
		assert(startPos>=0 && startPos<8);
		return BitMaskContinuousBit0[startPos][m_8bits];
	}
	int first_bit0()
	{
		int iRet = BitMaskFirstBit0[m_8bits];
		return iRet == 0xff ? -1 : iRet;
	}
	int first_bit0(int startPos)
	{
		if(startPos < 0 || startPos >= 8) return -1;
		if(startPos == 0) return first_bit0();
		int iRet = BitMaskFirstBit0[BitMaskAll1Mask[0][startPos-1] | m_8bits];
		return iRet == 0xff ? -1 : iRet;
	}
	int last_bit0()
	{
		int iRet = BitMaskLastBit0[m_8bits];
		return iRet == 0xff ? -1 : iRet;
	}
	int first_bit1()
	{
		int iRet = BitMaskFirstBit1[m_8bits];
		return iRet == 0xff ? -1 : iRet;
	}
	int first_bit1(int startPos)
	{
		if(startPos < 0 || startPos >= 8) return -1;
		if(startPos == 0) return first_bit1();
		int iRet = BitMaskFirstBit1[(~BitMaskAll1Mask[0][startPos-1]) & m_8bits];
		return iRet == 0xff ? -1 : iRet;
	}
	int last_bit1()
	{
		int iRet = BitMaskLastBit1[m_8bits];
		return iRet == 0xff ? -1 : iRet;
	}
	friend TBitMask_N<8> operator|(const TBitMask_N<8>& op1, const TBitMask_N<8>& op2)
	{
		TBitMask_N<8> temp;
		temp.m_8bits = op1.m_8bits | op2.m_8bits;
		return temp;
	}
	friend TBitMask_N<8> operator&(const TBitMask_N<8>& op1, const TBitMask_N<8>& op2)
	{
		TBitMask_N<8> temp;
		temp.m_8bits = op1.m_8bits & op2.m_8bits;
		return temp;
	}
	friend TBitMask_N<8> operator^(const TBitMask_N<8>& op1, const TBitMask_N<8>& op2)
	{
		TBitMask_N<8> temp;
		temp.m_8bits = op1.m_8bits ^ op2.m_8bits;
		return temp;
	}
	friend TBitMask_N<8> operator~(const TBitMask_N<8>& op)
	{
		TBitMask_N<8> temp;
		temp.m_8bits = ~op.m_8bits;
		return temp;
	}
	friend bool operator==(const TBitMask_N<8>& op1, const TBitMask_N<8>& op2)
	{
		return op1.m_8bits == op2.m_8bits;
	}

	//
private:
	unsigned char m_8bits;
};

template<>
struct TBitMask_N<32>
{
	TBitMask_N(){*((int*)&m_32bits[0])=0;}
	void set(int index)
	{
		assert(index>=0 && index<32);
		m_32bits[index/8]|=(0x01<<(index&0x07));
	}
	void set() {*((int*)&m_32bits[0])=0xffffffff;}
	void set(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<32 && numbits>=0);
		numbits = (numbits > 32-startPos) ? 32-startPos : numbits;
		//
		int len = 0;
		while(numbits > 0)
		{
			len = (8-(startPos&0x07)); len = len > numbits ? numbits : len;
			m_32bits[startPos/8]|=BitMaskAll1Mask[startPos&0x07][len-1];
			numbits -= len;
			startPos += len;
		}
	}
	void clr(int index)
	{
		assert(index>=0 && index<32);
		m_32bits[index/8]&=~(0x01<<(index&0x07));
	}
	void clr() {*((int*)&m_32bits[0])=0;}
	void clr(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<32 && numbits>=0);
		numbits = (numbits > 32-startPos) ? 32-startPos : numbits;
		//
		int len = 0;
		while(numbits > 0)
		{
			len = (8-(startPos&0x07)); len = len > numbits ? numbits : len;
			m_32bits[startPos/8]&=~BitMaskAll1Mask[startPos&0x07][len-1];
			numbits -= len;
			startPos += len;
		}
	}
	bool isset(int index)
	{
		assert(index>=0 && index<32);
		return (m_32bits[index/8]&(0x01<<(index&0x07))) > 0;
	}
	bool isset(int startPos, int numbits)
	{
		assert(startPos>=0 && startPos<32);
		numbits = (numbits > 32-startPos) ? 32-startPos : numbits;
		int len = 0;
		while(numbits > 0)
		{
			len = (8-(startPos&0x07)); len = len > numbits ? numbits : len;
			if((m_32bits[startPos/8]&BitMaskAll1Mask[startPos&0x07][len-1]) > 0) return true;
			numbits -= len;
			startPos += len;

		}
		return false;
	}
	int bit1()
	{
		return BitMaskBit1[m_32bits[0]]+BitMaskBit1[m_32bits[1]]+BitMaskBit1[m_32bits[2]]+BitMaskBit1[m_32bits[3]];
	}
	int bit0() {return 32-bit1();}
	int bit1(int startPos, int endPos)
	{
		assert(startPos>=0 && startPos<32 && endPos>=startPos);
		endPos = endPos < 32 ? endPos : 32-1;
		int numbits = endPos-startPos+1;
		int len = 0;
		int r = 0;
		while(numbits > 0)
		{
			len = (8-(startPos&0x07)); len = len > numbits ? numbits : len;
			r += BitMaskBit1[m_32bits[startPos/8]&BitMaskAll1Mask[startPos&0x07][len-1]];
			numbits -= len;
			startPos += len;
		}
		return r;
	}
	int bit0(int startPos)
	{
		return bit0(startPos, 32-1);
	}
	int bit0(int startPos, int endPos)
	{
		return (endPos-startPos+1)-bit1(startPos, endPos);
	}
	int continuousBit1(int startPos)
	{
		assert(startPos>=0 && startPos<32);
		int r = 0;
		int ii = startPos/8; startPos = startPos&0x07;
		int bit1 = 0;
		while(ii<4)
		{
			bit1 = BitMaskContinuousBit1[startPos][m_32bits[ii]];
			if(0==bit1) break;
			r+=bit1;
			if(bit1<(8-startPos)) break;
			++ii;
			startPos=0;
		}
		return r;
	}
	int continuousBit0(int startPos)
	{
		assert(startPos>=0 && startPos<32);
		int r = 0;
		int ii = startPos/8; startPos = startPos&0x07;
		int bit0 = 0;
		while(ii<4)
		{
			bit0 = BitMaskContinuousBit0[startPos][m_32bits[ii]];
			if(0==bit0) break;
			r+=bit0;
			if(bit0<(8-startPos)) break;
			++ii;
			startPos=0;
		}
		return r;
	}
	int first_bit0()
	{
		int iRet = BitMaskFirstBit0[m_32bits[0]];
		if(iRet != 0xff) return iRet;
		iRet = BitMaskFirstBit0[m_32bits[1]];
		if(iRet != 0xff) return iRet+8;
		iRet = BitMaskFirstBit0[m_32bits[2]];
		if(iRet != 0xff) return iRet+16;
		iRet = BitMaskFirstBit0[m_32bits[3]];
		if(iRet != 0xff) return iRet+24;
		return -1;
	}
	int first_bit0(int startPos)
	{
		if(startPos < 0 || startPos >= 32) return -1;
		int ii = startPos/8; startPos = startPos&0x07;
		if(startPos > 0)
		{
			int iRet = BitMaskFirstBit0[BitMaskAll1Mask[0][startPos-1] | m_32bits[ii]];
			if(iRet != 0xff) return ii*8+iRet;
			else ++ii;
		}
		//
		for(; ii < 4; ++ii) if(BitMaskFirstBit0[m_32bits[ii]]!=0xff) return BitMaskFirstBit0[m_32bits[ii]]+ii*8;
		return -1;
	}
	int last_bit0()
	{
		int iRet = BitMaskLastBit0[m_32bits[3]];
		if(iRet != 0xff) return iRet+24;
		iRet = BitMaskLastBit0[m_32bits[2]];
		if(iRet != 0xff) return iRet+16;
		iRet = BitMaskLastBit0[m_32bits[1]];
		if(iRet != 0xff) return iRet+8;
		iRet = BitMaskLastBit0[m_32bits[0]];
		if(iRet != 0xff) return iRet;
		return -1;
	}
	int first_bit1()
	{
		int iRet = BitMaskFirstBit1[m_32bits[0]];
		if(iRet != 0xff) return iRet;
		iRet = BitMaskFirstBit1[m_32bits[1]];
		if(iRet != 0xff) return iRet+8;
		iRet = BitMaskFirstBit1[m_32bits[2]];
		if(iRet != 0xff) return iRet+16;
		iRet = BitMaskFirstBit1[m_32bits[3]];
		if(iRet != 0xff) return iRet+24;
		return -1;
	}
	int first_bit1(int startPos)
	{
		if(startPos < 0 || startPos >= 32) return -1;
		int ii = startPos/8; startPos = startPos&0x07;
		if(startPos > 0)
		{
			int iRet = BitMaskFirstBit1[(~BitMaskAll1Mask[0][startPos-1]) & m_32bits[ii]];
			if(iRet != 0xff) return ii*8+iRet;
			else ++ii;
		}
		//
		for(; ii < 4; ++ii) if(BitMaskFirstBit1[m_32bits[ii]]!=0xff) return BitMaskFirstBit1[m_32bits[ii]]+ii*8;
		return -1;
	}
	int last_bit1()
	{
		int iRet = BitMaskLastBit1[m_32bits[3]];
		if(iRet != 0xff) return iRet+24;
		iRet = BitMaskLastBit1[m_32bits[2]];
		if(iRet != 0xff) return iRet+16;
		iRet = BitMaskLastBit1[m_32bits[1]];
		if(iRet != 0xff) return iRet+8;
		iRet = BitMaskLastBit1[m_32bits[0]];
		if(iRet != 0xff) return iRet;
		return -1;
	}
	friend TBitMask_N<32> operator|(const TBitMask_N<32>& op1, const TBitMask_N<32>& op2)
	{
		TBitMask_N<32> temp;
		*((int*)(&temp.m_32bits[0])) = *((int*)(&op1.m_32bits[0])) | *((int*)(&op2.m_32bits[0]));
		return temp;
	}
	friend TBitMask_N<32> operator&(const TBitMask_N<32>& op1, const TBitMask_N<32>& op2)
	{
		TBitMask_N<32> temp;
		*((int*)(&temp.m_32bits[0])) = *((int*)(&op1.m_32bits[0])) & *((int*)(&op2.m_32bits[0]));
		return temp;
	}
	friend TBitMask_N<32> operator^(const TBitMask_N<32>& op1, const TBitMask_N<32>& op2)
	{
		TBitMask_N<32> temp;
		*((int*)(&temp.m_32bits[0])) = *((int*)(&op1.m_32bits[0])) ^ *((int*)(&op2.m_32bits[0]));
		return temp;
	}
	friend TBitMask_N<32> operator~(const TBitMask_N<32>& op)
	{
		TBitMask_N<32> temp;
		*((int*)(&temp.m_32bits[0])) = ~(*((int*)(&op.m_32bits[0])));
		return temp;
	}
	friend bool operator==(const TBitMask_N<32>& op1, const TBitMask_N<32>& op2)
	{
		return *((int*)(&op1.m_32bits[0])) == *((int*)(&op2.m_32bits[0]));
	}
private:
	unsigned char m_32bits[4];
};

} // namespace
#endif // OSUTIL_H_INCLUDED
