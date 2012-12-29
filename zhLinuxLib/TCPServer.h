#ifndef CTCPSERVER_H
#define CTCPSERVER_H

#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>

#include "INETAddr.h"
#include "RingBuffer.h"
#include "ScopeLock.h"
using NS_ZH_UTIL::CScopeLock;
#include "TcpCommonDef.h"
#include "osutil.h"


//template<typename MUTEX>
struct CClientPeer
{
	int m_dataSock;
	NS_ZH_UTIL::CINETAddr m_peerAddr;
	NS_ZH_UTIL::CRingBuffer m_dataBuf;
	unsigned m_Flag;
	//MUTEX m_LocalMutex;
	NS_ZH_UTIL::CMutex m_LocalMutex;
	//
	CClientPeer()
	:m_dataSock(-1)
	,m_dataBuf(1024)
	,m_Flag(SOCK_INIT)
	{}
	//
	int send(const void* pData, size_t size)
	{
		int iret = 0;
		if(m_Flag == SOCK_CONNECTED)
		{
			CScopeLock<NS_ZH_UTIL::CMutex> lock(m_LocalMutex);
			//after lock, check again
			if(m_Flag == SOCK_CONNECTED)
			{
				struct timeval timeout = {0,0};//no timeout, always wait
				iret = NS_ZH_UTIL::send_n(m_dataSock, pData, size, timeout);
			}
		}
		return iret == size ? 0 : -1;
	}
	int close(bool delay=false)
	{
		if(delay)
		{
			if(m_Flag == SOCK_CONNECTED)
			{
				CScopeLock<NS_ZH_UTIL::CMutex> lock(m_LocalMutex);
				if(m_Flag == SOCK_CONNECTED) m_Flag = SOCK_CLOSE_PENDING;	//ÑÓ³Ù¹Ø±Õ
				return 0;
			}
		}
		else
		{
			if(m_Flag == SOCK_CLOSE_PENDING)
			{
				CScopeLock<NS_ZH_UTIL::CMutex> lock(m_LocalMutex);
				if(m_Flag == SOCK_CLOSE_PENDING)
				{
					TEMP_FAILURE_RETRY(::close(m_dataSock));
					m_dataSock= -1;
					m_Flag = SOCK_CONNECT_BREAK;
					return 0;
				}
			}
		}
		return -1;
	}
	int open(int sock, NS_ZH_UTIL::CINETAddr& peeraddr)
	{
		if(m_dataSock == -1)
		{
			CScopeLock<NS_ZH_UTIL::CMutex> lock(m_LocalMutex);
			if(m_dataSock == -1)
			{
				m_dataBuf.reset();
				m_peerAddr = peeraddr;
				m_Flag = SOCK_CONNECTED;
				m_dataSock = sock;
				return 0;
			}
		}
		return -1;
	}
};

//template<typename MUTEX>
class CTCPServer
{
	public:
		CTCPServer();
		virtual ~CTCPServer();
		//
		int Init(const char* srvAddr, unsigned short port);
		int Uninit();
		//
		int close(int fd);
		int send(int fd, const void* pData, size_t size);
		int services(struct timeval& timeout);

	protected:
		NS_ZH_UTIL::CINETAddr m_srvAddr;
		int m_srvSock;
		int m_maxFd;
		fd_set m_fdset;
		CClientPeer m_Peers[FD_SETSIZE];
		//
		bool m_running;
		//
		void doNetIO(fd_set& fdset, int fdcnt);
		void acceptNewConnection();
		void readDataSock(fd_set& fdset, int fdcnt);
		int readDataSock(int sock, NS_ZH_UTIL::CRingBuffer& buf);
		//
		virtual void dataNotify(int size, CClientPeer* peer) {}
		virtual void statNotify(enum ESockStates, CClientPeer* peer) {}
	private:
		void delayClose();

};

#endif // CTCPSERVER_H
