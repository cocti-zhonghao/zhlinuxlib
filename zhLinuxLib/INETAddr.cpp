#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>

#include "INETAddr.h"

namespace NS_ZH_UTIL {

CINETAddr::CINETAddr()
:m_port(0)
,m_validation(false)
{
	memset(m_ip, 0, sizeof(m_ip));
	memset(&m_addr, 0, sizeof(sockaddr_in));
	m_addr.sin_family = AF_INET;
}

CINETAddr::CINETAddr(const char* pAddr)
:m_port(0)
,m_validation(false)
{
	memset(m_ip, 0, sizeof(m_ip));
	memset(&m_addr, 0, sizeof(sockaddr_in));
	m_addr.sin_family = AF_INET;
	//
	const char* p = pAddr; while(*p != '\0' && *p!=':') ++p;
	if(*p == ':')
	{
		size_t iplen = p-pAddr;
		char ip[iplen+1];
		memcpy(ip, pAddr, iplen); ip[iplen] = '\0';
		++p;
		long port = ::strtol(p, 0, 10);
		errno = 0;
		if(0 == port && errno != 0)
		{
			m_validation = false;
			return;
		}
		validate(ip, port);
	}
	else
	{
		p = pAddr; while(*p != '\0' && *p != '.') ++p;
		if(*p == '.') validate(pAddr, 0);
		else validate(NULL, ::strtol(pAddr, 0, 10));
	}
}

CINETAddr::CINETAddr(const char* pAddr, unsigned short port)
:m_port(port)
,m_validation(false)
{
	memset(m_ip, 0, sizeof(m_ip));
	memset(&m_addr, 0, sizeof(sockaddr_in));
	m_addr.sin_family = AF_INET;
	//
	validate(pAddr, port);
}

int CINETAddr::setAddress(const char* pAddr)
{
	const char* p = pAddr; while(*p != '\0' && *p!=':') ++p;
	if(*p == ':')
	{
		size_t iplen = p-pAddr;
		char ip[iplen+1];
		memcpy(ip, pAddr, iplen); ip[iplen] = '\0';
		++p;
		long port = ::strtol(p, 0, 10);
		errno = 0;
		if(0 == port && errno != 0)
		{
			return -1;
		}
		return validate(ip, port);
	}
	else
	{
		p = pAddr; while(*p != '\0' && *p != '.') ++p;
		if(*p == '.') return validate(pAddr, 0);
		else return validate(NULL, ::strtol(pAddr, 0, 10));
	}
	return -1;
}

int CINETAddr::setAddress(const char* pIp, unsigned short port)
{
	return validate(pIp, port);
}

int CINETAddr::setAddress(const sockaddr* pAddr)
{
	::memcpy(&m_addr, pAddr, sizeof(sockaddr_in));
	//
	m_port = ::ntohs(m_addr.sin_port);
	::memset(m_ip, 0, sizeof(m_ip));
	::inet_ntop(AF_INET, &m_addr.sin_addr, m_ip, sizeof(m_ip));
	m_validation = true;
}

int CINETAddr::validate(const char* pAddr, unsigned short port)
{
	if(NULL == pAddr || pAddr[0] == '\0')
	{
		m_port = port;
		m_addr.sin_addr.s_addr=::htonl(INADDR_ANY);
		m_addr.sin_port = ::htons(m_port);
		::memset(m_ip, 0, sizeof(m_ip));
		::strcpy(m_ip, "0.0.0.0");
		m_validation = true;
		return 0;
	}
	else
	{
		if(::inet_pton(AF_INET, pAddr, &m_addr.sin_addr) == 1)
		{
			m_port = port;
			m_addr.sin_port = ::htons(m_port);
			::memset(m_ip, 0, sizeof(m_ip));
			::strncpy(m_ip, pAddr, sizeof(m_ip) -1);
			m_validation = true;
			return 0;
		}
	}
	return -1;
}

CINETAddr::~CINETAddr()
{
}

char* CIPValidator::_CIPRegex::m_pPatternstr="^(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])$";
CIPValidator::_CIPRegex CIPValidator::m_regex;
CIPValidator::CIPValidator(const char* ip)
{
	m_validation = NULL == ip ? false : m_regex.match(ip);
}

}
