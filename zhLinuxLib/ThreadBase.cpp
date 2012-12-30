/*
 * ThreadBase.cpp
 *
 *  Created on: 2012-4-4
 *      Author: zh
 */

#include <string.h>
#include <sys/prctl.h>
#include "ThreadBase.h"

namespace NS_ZH_UTIL {

CThreadBase::CThreadBase(const char* name)
:m_State(E_THR_UNAVAILABLE)
{
	::memset(&m_Handle, 0, sizeof(pthread_t));
	::memset(&m_Name, 0, sizeof(m_Name));
	if(name) ::strncpy(m_Name, name, sizeof(m_Name) - 1);
}

CThreadBase::~CThreadBase()
{
}

void CThreadBase::setName(const char* name)
{
	if(name)
	{
		::memset(&m_Name, 0, sizeof(m_Name));
		::strncpy(m_Name, name, sizeof(m_Name) - 1);
	}
}

void* CThreadBase::ThreadRoutine(void * self)
{
	static_cast<CThreadBase*>(self)->run();
	return 0;
}

int CThreadBase::start()
{
	if(::pthread_create(&m_Handle, 0, CThreadBase::ThreadRoutine, this) == 0)
	{
		m_State = E_THR_INITIALIZED;
		return 0;
	}
	return -1;
}

int CThreadBase::stop()
{
	if(m_State == E_THR_UNAVAILABLE) return -1;
	else if(m_State == E_THR_STOPPED) return 0;
	else
	{
		dostop();
		::pthread_join(m_Handle, 0);
		m_State = E_THR_STOPPED;
		return 0;
	}
}

void CThreadBase::run()
{
	if(m_Name[0] != '\0') ::prctl(PR_SET_NAME, m_Name,0,0,0); // linux kernel 2.6.9
	while(m_State != E_THR_INITIALIZED);
	m_State = E_THR_RUNNING;
	dorun();
}

} /* namespace NS_JOYIT_UTIL */

