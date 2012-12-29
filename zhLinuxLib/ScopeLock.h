/*
 * ScopeLock.h
 *
 *  Created on: 2012-4-15
 *      Author: zh
 */

#ifndef SCOPELOCK_H_
#define SCOPELOCK_H_

#include <pthread.h>

namespace NS_ZH_UTIL {

class CMutex
{
public:
	CMutex() {::pthread_mutex_init(&m_mutex, 0);}
	~CMutex(){::pthread_mutex_destroy(&m_mutex);}
	void acquire(){::pthread_mutex_lock(&m_mutex);}
	void release(){::pthread_mutex_unlock(&m_mutex);}
private:
	pthread_mutex_t m_mutex;
};

class CSpinLock
{
public:
	CSpinLock() {::pthread_spin_init(&m_spin, 0);}
	~CSpinLock(){::pthread_spin_destroy(&m_spin);}
	void acquire(){::pthread_spin_lock(&m_spin);}
	void release(){::pthread_spin_unlock(&m_spin);}
private:
	pthread_spinlock_t m_spin;
};

class CNullLock
{
public:
	void acquire(){}
	void release(){}
};

template<typename MUTEX>
class CScopeLock {
public:
	CScopeLock(MUTEX& mutex)
	:m_mutex(mutex)
	{
		m_mutex.acquire();
	}
	virtual ~CScopeLock(){m_mutex.release();}
private:
	MUTEX& m_mutex;
};

//用自旋锁模拟原子操作
struct CAtomOP
{
	template<typename T>
	static T __sync_fetch_and_add (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); T t(*ptr); *ptr += value; return t;}
	template<typename T>
	static T __sync_fetch_and_sub (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); T t = *ptr; *ptr -= value; return t;}
	template<typename T>
	static T __sync_fetch_and_or (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); T t = *ptr; *ptr |= value; return t;}
	template<typename T>
	static T __sync_fetch_and_and (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); T t = *ptr; *ptr &= value; return t;}
	template<typename T>
	static T __sync_fetch_and_xor (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); T t = *ptr; *ptr ^= value; return t;}
	template<typename T>
	static T __sync_fetch_and_nand (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); T t = *ptr; *ptr = ~(*ptr&value); return t;}


	template<typename T>
	static T __sync_add_and_fetch (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); *ptr = ~(*ptr&value); return *ptr;}
	template<typename T>
	static T __sync_sub_and_fetch (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); *ptr = ~(*ptr&value); return *ptr;}
	template<typename T>
	static T __sync_or_and_fetch (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); *ptr = ~(*ptr&value); return *ptr;}
	template<typename T>
	static T __sync_and_and_fetch (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); *ptr = ~(*ptr&value); return *ptr;}
	template<typename T>
	static T __sync_xor_and_fetch (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); *ptr = ~(*ptr&value); return *ptr;}
	template<typename T>
	static T __sync_nand_and_fetch (T *ptr, T value)
	{CScopeLock<CSpinLock> lock(m_Mutex); *ptr = ~(*ptr&value); return *ptr;}

private:
	CAtomOP(){};
	static CSpinLock m_Mutex;
};

class CRefCountBase
{
public:
	CRefCountBase()
	:m_Count(1){}
	int AddRef()
	{
		int refCount;
#ifdef NO_BUILTIN_ATOM_OP
		refCount = NS_ZH_UTIL::CAtomOP::__sync_add_and_fetch(&m_Count, 1);
#else
		refCount = __sync_add_and_fetch(&m_Count, 1);
#endif
		return refCount;
	}
	int Release()
	{
		int refCount;
#ifdef NO_BUILTIN_ATOM_OP
		refCount = NS_ZH_UTIL::CAtomOP::__sync_sub_and_fetch(&m_Count, 1);
#else
		refCount = __sync_sub_and_fetch(&m_Count, 1);
#endif
		if(0 == refCount) delete this;
		return refCount;
	}
private:
	int m_Count;
};

//
#ifdef NO_BUILTIN_ATOM_OP
#define __sync_fetch_and_add(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_fetch_and_add((P),(V))
#define __sync_fetch_and_sub(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_fetch_and_sub((P),(V))
#define __sync_fetch_and_or(P, V)  NS_JOYIT_UTIL::CAtomOP::__sync_fetch_and_or((P),(V))
#define __sync_fetch_and_and(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_fetch_and_and((P),(V))
#define __sync_fetch_and_xor(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_fetch_and_xor((P),(V))
#define __sync_fetch_and_nand(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_fetch_and_nand((P),(V))

#define __sync_add_and_fetch(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_add_and_fetch((P),(V))
#define __sync_sub_and_fetch(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_sub_and_fetch((P),(V))
#define __sync_or_and_fetch(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_or_and_fetch((P),(V))
#define __sync_and_and_fetch(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_and_and_fetch((P),(V))
#define __sync_xor_and_fetch(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_xor_and_fetch((P),(V))
#define __sync_nand_and_fetch(P, V) NS_JOYIT_UTIL::CAtomOP::__sync_nand_and_fetch((P),(V))
#endif
//

} /* namespace NS_JOYIT_UTIL */

#endif /* SCOPELOCK_H_ */
