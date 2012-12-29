/*
 * ThreadBase.h
 *
 *  Created on: 2012-4-4
 *      Author: zh
 */

#ifndef THREADBASE_H_
#define THREADBASE_H_

#include <pthread.h>

namespace NS_ZH_UTIL {

class CThreadBase {
public:
	CThreadBase(const char* name);
	virtual ~CThreadBase();
	int start();
	int stop();
private:
	static void* ThreadRoutine(void * self);
	void run();

	enum EThrState
	{E_THR_UNAVAILABLE,
	E_THR_INITIALIZED,
	E_THR_RUNNING,
	E_THR_STOPPED};
protected:
	pthread_t m_Handle;
	EThrState m_State;
	char m_Name[32];
	virtual int dostop() = 0;
	virtual void dorun() = 0;
};

} /* namespace NS_JOYIT_UTIL */
#endif /* THREADBASE_H_ */
