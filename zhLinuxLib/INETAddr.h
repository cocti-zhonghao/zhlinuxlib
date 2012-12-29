#ifndef CINETADDR_H
#define CINETADDR_H

#include <netinet/in.h>
#include <regex.h>


namespace NS_ZH_UTIL {

class CINETAddr
{
	public:
		CINETAddr();
		CINETAddr(const char* pAddr);
		CINETAddr(const char* pIp, unsigned short port);
		~CINETAddr();
		const struct in_addr& get_in_addr() const {return m_addr.sin_addr;}
		const in_addr_t& get_in_addr_t() const {return m_addr.sin_addr.s_addr;}
		const sockaddr_in& get_sockaddr_in() const {return m_addr;}
		const sockaddr& get_sockaddr() const {return *((sockaddr*)&m_addr);}
		const char* get_ip() const {return m_ip;}
		unsigned short get_port() const {return m_port;}
		//
		int setAddress(const char* pAddr);
		int setAddress(const char* pIp, unsigned short port);
		int setAddress(const sockaddr* pAddr);
		//

		operator bool() {return m_validation;}
	protected:
	private:
		unsigned short m_port;
		char m_ip[INET_ADDRSTRLEN];
		sockaddr_in m_addr;
		bool m_validation;
		int validate(const char* pAddr, unsigned short port);
};

class CIPValidator
{
	struct _CIPRegex
	{
		static char* m_pPatternstr;
		regex_t m_reg;
		_CIPRegex()
		{
			::regcomp(&m_reg, m_pPatternstr, REG_EXTENDED);
		}
		~_CIPRegex()
		{
			::regfree(&m_reg);
		}
		bool match(const char* ip)
		{
			return ::regexec(&m_reg, ip, 0, NULL, 0) == 0;
		}
	};
	public:
		CIPValidator(const char* ip);
		operator bool() {return m_validation;}
	private:
		bool m_validation;
		static _CIPRegex m_regex;
};

}

#endif // CINETADDR_H
