#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#include "TCPServer.h"

using namespace NS_ZH_UTIL;

//
CTCPServer::CTCPServer()
:m_srvSock(-1)
,m_running(false)
,m_maxFd(-1)
{
	FD_ZERO(&m_fdset);
}

CTCPServer::~CTCPServer()
{
	Uninit();
}

int CTCPServer::Init(const char* srvAddr, unsigned short port)
{
	if(m_srvAddr.setAddress(srvAddr, port) < 0)
	{
		return -1;
	}
	m_srvSock = ::socket(AF_INET, SOCK_STREAM, 0);
	if(m_srvSock < 0)
	{
		return -1;
	}
	//setsockopt
	int val = 1;
	if(::setsockopt(m_srvSock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0)
	{
		return -1;
	}
	//non block
	if((val = ::fcntl(m_srvSock, F_GETFL, 0)) >= 0)
	{
		val |= O_NONBLOCK;
		if(::fcntl(m_srvSock, F_SETFL, val) < 0)
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
	//bind
	if(::bind(m_srvSock, &m_srvAddr.get_sockaddr(), sizeof(struct sockaddr)) < 0)
	{
		return -1;
	}
	//listen
	if(::listen(m_srvSock, 0) < 0)
	{
		return -1;
	}
	//
	//
	FD_SET(m_srvSock, &m_fdset);
	m_maxFd = m_srvSock;
	//
	return 0;
}

int CTCPServer::Uninit()
{
	//close all sock
	for(int ii = 0; ii < FD_SETSIZE; ++ii)
	{
		TEMP_FAILURE_RETRY(::close(m_Peers[ii].m_dataSock));
		m_Peers[ii].m_dataSock = -1;
	}
	//
	TEMP_FAILURE_RETRY(::close(m_srvSock));
	FD_ZERO(&m_fdset);
	m_maxFd = -1;
	return 0;
}

int CTCPServer::close(int fd)
{
	if(fd >= 0 && fd < FD_SETSIZE) return m_Peers[fd].close(true);
	return -1;
}

int CTCPServer::send(int fd, const void* pData, size_t size)
{
	if(fd >= 0 && fd < FD_SETSIZE) return m_Peers[fd].send(pData, size);
	return -1;
}

int CTCPServer::services(struct timeval& timeout)
{
	//struct timeval timeout;
	//timeout.tv_sec = 1; timeout.tv_usec = 0;
	fd_set fdset;
	::memcpy(&fdset, &m_fdset, sizeof(fd_set));

	int fdCnt = ::select(m_maxFd+1, &fdset, 0, 0, &timeout);
	if(fdCnt > 0) doNetIO(fdset, fdCnt);
	delayClose();
	return fdCnt;
}

void CTCPServer::doNetIO(fd_set& fdset, int fdcnt)
{
	if(FD_ISSET(m_srvSock, &fdset) != 0)
	{
		fdcnt--;
		acceptNewConnection();
	}
	//
	readDataSock(fdset, fdcnt);
}

void CTCPServer::acceptNewConnection()
{
	struct sockaddr addr_in; socklen_t len = sizeof(struct sockaddr);
	int sock = TEMP_FAILURE_RETRY(::accept(m_srvSock, &addr_in, &len));
	//
	if(sock < 0) return;
	//
	CINETAddr addr; addr.setAddress(&addr_in);
	if(sock >= FD_SETSIZE)
	{
		TEMP_FAILURE_RETRY(::close(sock));
		return;
	}
	//
	//set non-block mode
	int val = 0;
	if((val = ::fcntl(sock, F_GETFL, 0)) >= 0)
	{
		val |= O_NONBLOCK;
		if(::fcntl(sock, F_SETFL, val) < 0)
		{
			TEMP_FAILURE_RETRY(::close(sock));
			return;
		}
	}
	else
	{
		TEMP_FAILURE_RETRY(::close(sock));
		return;
	}
	//
	if(m_Peers[sock].open(sock, addr) >= 0)
	{
		FD_SET(sock, &m_fdset); m_maxFd = sock > m_maxFd ? sock : m_maxFd;
		statNotify(SOCK_CONNECTED, &m_Peers[sock]);
		return;
	}
	else
	{
		TEMP_FAILURE_RETRY(::close(sock));
	}
	return;
}

void CTCPServer::delayClose()
{
	for(int ii = 0; ii < FD_SETSIZE ; ++ii)
	{
		int iSock = m_Peers[ii].m_dataSock;
		if(m_Peers[ii].close(false) >= 0)
		{
			FD_CLR(iSock, &m_fdset);
			if(iSock == m_maxFd)
			{
				m_maxFd = -1;
				for(int ij = iSock-1; ij >= 0; --ij)
				{
					if(m_Peers[ij].m_dataSock >= 0)
					{
						m_maxFd = m_Peers[ij].m_dataSock;
						break;
					}
				}
				m_maxFd = m_maxFd == -1 ? m_srvSock : m_maxFd;
			}
		}
	}
}

void CTCPServer::readDataSock(fd_set& fdset, int fdcnt)
{
	int iSock = -1;
	for(int ii = 0; ii < FD_SETSIZE ; ++ii)
	{
		if(m_Peers[ii].m_Flag == SOCK_CONNECTED &&
		   m_Peers[ii].m_dataSock >= 0 &&
		   FD_ISSET(m_Peers[ii].m_dataSock, &fdset))
		{

			--fdcnt;
			int iRead = readDataSock(m_Peers[ii].m_dataSock, m_Peers[ii].m_dataBuf);
			if(iRead < 0)
			{
				//read error, we should close this sock
				statNotify(SOCK_CONNECT_BREAK, &m_Peers[ii]);
				m_Peers[ii].close(true);
				continue;
			}
			else if(iRead > 0)
			{
				dataNotify(iRead, &m_Peers[ii]);
			}
			else // iRead == 0 ???
			{
				//
			}
		}
	}
}

int CTCPServer::readDataSock(int sock, CRingBuffer& buf)
{
	int iRead = 0;
	unsigned freeSize = buf.getFreeSize();
	if(freeSize > 0)
	{
		unsigned char* pRead = buf.getReadPtr();
		unsigned char* pWrite= buf.getWritePtr();
		if(pWrite < pRead)
		{
			iRead = TEMP_FAILURE_RETRY(::read(sock, pWrite, freeSize));
			if(iRead == 0) return -1; 	//peer has closed
			else if(iRead < 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK) return 0;
				else return -1;			//some error
			}
			else
			{
				buf.advanceWritePtr(iRead);
				return iRead;
			}
		}
		else
		{
			unsigned size_1 = buf.getSizeFromWritePos2End();
			int iRead_1 = TEMP_FAILURE_RETRY(::read(sock, pWrite, size_1));
			if(iRead_1 == 0) return -1; //peer has closed
			else if(iRead_1 < 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK) return 0;
				else return -1;		//some error
			}
			else
			{
				buf.advanceWritePtr(iRead_1);
				iRead += iRead_1;
			}
			//
			unsigned size_2 = freeSize-size_1;
			if(iRead_1==size_1 && size_2 > 0)
			{
				pWrite= buf.getWritePtr();
				int iRead_2 = TEMP_FAILURE_RETRY(::read(sock, pWrite, size_2));
				if(iRead_2 == 0) return -1; //peer has closed
				if(iRead_2 < 0)
				{
					if(errno == EAGAIN || errno == EWOULDBLOCK) return 0;
					else return -1;		//some error
				}
				else
				{
					buf.advanceWritePtr(iRead_2);
					iRead += iRead_2;
				}
			}
			return iRead;
		}
	}
	return 0;
}

