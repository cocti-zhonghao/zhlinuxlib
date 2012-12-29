/*
 * TcpClient.cpp
 *
 *  Created on: 2012-4-18
 *      Author: zh
 */
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#include "TcpClient.h"
#include "osutil.h"

using namespace NS_ZH_UTIL;

#define LOCK(X) NS_ZH_UTIL::CScopeLock<NS_ZH_UTIL::CSpinLock> lock(X)

CTcpClient::CTcpClient()
:m_maxFd(-1)
,m_IDSeed(1)
{
	::memset(m_Peers, 0, sizeof(m_Peers));
	FD_ZERO(&m_fdset);
	FD_ZERO(&m_connect_fdset);
}

CTcpClient::~CTcpClient()
{
}

unsigned CTcpClient::socket(const char* ip, unsigned short port)
{
	CINETAddr peer_addr(ip, port);
	if(!peer_addr) return 0;
	//
	int sock  = ::socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) return 0;
	if(sock >= FD_SETSIZE)
	{
		TEMP_FAILURE_RETRY(::close(sock)); return 0;
	}
	//set non-block
	int val = 0;
	if((val = ::fcntl(sock, F_GETFL, 0)) >= 0)
	{
		val |= O_NONBLOCK;
		if(::fcntl(sock, F_SETFL, val) < 0) {TEMP_FAILURE_RETRY(::close(sock)); return 0;}
	}
	else {TEMP_FAILURE_RETRY(::close(sock)); return 0;}
	//
	return addSocket(sock, peer_addr);
}

unsigned CTcpClient::addSocket(int sock, NS_ZH_UTIL::CINETAddr& addr)
{
	CServerPeer* pNew = new CServerPeer();
	pNew->m_dataSock = sock;
	pNew->m_peerAddr = addr;
	unsigned ID = pNew->m_ID = allocID();
	//
	unsigned pos = 0;
	{
		LOCK(m_sockVectorLock);
		pos = nextFreeSlot(pNew->m_ID%FD_SETSIZE);
		if(pos > 0)
		{
			m_Peers[pos]= pNew;
			m_maxFd = sock > m_maxFd ? sock : m_maxFd;
			FD_SET(sock, &m_fdset);
		}
	}
	if(0 == pos) {TEMP_FAILURE_RETRY(::close(pNew->m_dataSock)); delete pNew; ID=0;}
	return ID;
}

unsigned CTcpClient::nextFreeSlot(unsigned hint)
{
	unsigned pos = hint;
	do
	{
		if(NULL == m_Peers[pos]) return pos;
		pos = (pos+1) >= FD_SETSIZE ? 0 : pos+1;
	}
	while(pos != hint);
	return 0;
}

unsigned CTcpClient::ID2SlotIndex(unsigned id)
{
	unsigned pos = id%FD_SETSIZE;
	unsigned next= pos;
	do
	{
		if(NULL == m_Peers[next]) return 0;
		else if(m_Peers[next]->m_ID == id) return next;
		next = (next+1) >= FD_SETSIZE ? 0 : next+1;
	}
	while(next != pos);
	return 0;
}

int CTcpClient::closeDataSock(unsigned ID)
{
	LOCK(m_sockVectorLock);
	unsigned pos = ID2SlotIndex(ID);
	if(pos > 0)
	{
		m_Peers[pos]->m_Flag = SOCK_CLOSE_PENDING; //延迟关闭
		return 0;
	}
	return -1;
}

void CTcpClient::closeDataSock(CServerPeer* pPeer)
{
	if(pPeer)
	{
		TEMP_FAILURE_RETRY(::close(pPeer->m_dataSock));
	}
}

int CTcpClient::maxSockFd()
{
	int max = -1;
	for(int ii = 0; ii < FD_SETSIZE; ++ii)
	{
		if(m_Peers[ii] && m_Peers[ii]->m_Flag != SOCK_CLOSE_PENDING && m_Peers[ii]->m_dataSock > max) max = m_Peers[ii]->m_dataSock;
	}
	return max;
}

int CTcpClient::services()
{
	CServerPeer* pNeedClose = 0; int iNeedClose;
	CServerPeer* pNeedConnect = 0;
	CServerPeer* pReconnect = 0;
	//first check if any sock need close or connect
	for(int ii = 0; ii < FD_SETSIZE; ++ii)
	{
		pNeedClose = 0;
		pNeedConnect = 0;
		pReconnect = 0;
		{
			LOCK(m_sockVectorLock);
			if(m_Peers[ii])
			{
				if(m_Peers[ii]->m_Flag == SOCK_CLOSE_PENDING)
				{
					pNeedClose = m_Peers[ii];
					if(m_Peers[ii]->m_dataSock >= 0)
					{
						FD_CLR(m_Peers[ii]->m_dataSock, &m_fdset);
						FD_CLR(m_Peers[ii]->m_dataSock, &m_connect_fdset);
					}
					m_Peers[ii] = 0;
					if(pNeedClose->m_dataSock == m_maxFd)
					{
						m_maxFd = maxSockFd();
					}
				}
				else if(m_Peers[ii]->m_Flag == SOCK_INIT)
				{
					pNeedConnect = m_Peers[ii];
				}
				else if(m_Peers[ii]->m_Flag == SOCK_CONNECT_BREAK)
				{
					pReconnect = m_Peers[ii];
				}
			}
		}
		//
		closeDataSock(pNeedClose);
		connectDataSock(pNeedConnect);
		reconnectDataSock(pReconnect);
	}
	//wait sock ready to read
	struct timeval timeout={0, 100000};
	fd_set fdset; fd_set connect_fdset;
	::memcpy(&fdset, &m_fdset, sizeof(fd_set));
	::memcpy(&connect_fdset, &m_connect_fdset, sizeof(fd_set));
	int fdCnt = ::select(m_maxFd+1, m_maxFd>=0 ? &fdset : 0, m_maxFd>=0 ? &connect_fdset : 0, 0, &timeout);

	if(fdCnt > 0) doNetIO(fdset, connect_fdset, fdCnt);

	return 0;
}

void CTcpClient::connectDataSock(CServerPeer* pPeer)
{
	if(NULL == pPeer) return;
	int iret = TEMP_FAILURE_RETRY(::connect(pPeer->m_dataSock, &pPeer->m_peerAddr.get_sockaddr(), sizeof(sockaddr)));
	{
		LOCK(m_sockVectorLock);
		if(pPeer->m_Flag != SOCK_CLOSE_PENDING)
		{
			if(iret >= 0 || errno == EISCONN )
			{
				pPeer->m_Flag = SOCK_CONNECTED;
				iret = 0;
			}
			else if(errno == EINPROGRESS)
			{
				pPeer->m_Flag = SOCK_CONNECT_PENDING;
				FD_SET(pPeer->m_dataSock, &m_connect_fdset);
			}
			else
			{
				FD_CLR(pPeer->m_dataSock, &m_connect_fdset);
				FD_CLR(pPeer->m_dataSock, &m_fdset);
				pPeer->m_Flag = SOCK_CONNECT_BREAK;
			}
		}
		else iret = -1;
	}
	if(iret >= 0) statNotify(pPeer->m_ID, SOCK_CONNECTED);
}

void CTcpClient::reconnectDataSock(CServerPeer* pPeer)
{
	if(NULL == pPeer) return;
	if(pPeer->m_dataSock >= 0)
	{
		FD_CLR(pPeer->m_dataSock, &m_connect_fdset);
		FD_CLR(pPeer->m_dataSock, &m_fdset);
		TEMP_FAILURE_RETRY(::close(pPeer->m_dataSock));
		pPeer->m_dataSock = -1;
	}
	pPeer->m_dataBuf.reset();
	int sock  = ::socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) return;
	if(sock >= FD_SETSIZE)
	{
		TEMP_FAILURE_RETRY(::close(sock)); return;
	}
	//set non-block
	int val = 0;
	if((val = ::fcntl(sock, F_GETFL, 0)) >= 0)
	{
		val |= O_NONBLOCK;
		if(::fcntl(sock, F_SETFL, val) < 0) {TEMP_FAILURE_RETRY(::close(sock)); return;}
	}
	else {TEMP_FAILURE_RETRY(::close(sock)); return;}
	//
	int iret = TEMP_FAILURE_RETRY(::connect(sock, &pPeer->m_peerAddr.get_sockaddr(), sizeof(sockaddr)));
	{
		{
		//we should wait all the send thread to stop using the old sock for sending data ???
		CScopeLock<CMutex> wait_send_thread(pPeer->m_SendMutex);
		}
		LOCK(m_sockVectorLock);
		//now it is safe to update m_dataSock to new sock, because all the send thread will not used this new sock
		//until this new sock connected. see CTcpClient::send()
		if(pPeer->m_Flag != SOCK_CLOSE_PENDING)
		{
			pPeer->m_dataSock = sock;
			m_maxFd = sock > m_maxFd ? sock : m_maxFd;
			if(iret >= 0)
			{
				pPeer->m_Flag = SOCK_CONNECTED;
				FD_SET(sock, &m_fdset);
			}
			else if(errno == EINPROGRESS)
			{
				pPeer->m_Flag = SOCK_CONNECT_PENDING;
				FD_SET(sock, &m_fdset);
				FD_SET(pPeer->m_dataSock, &m_connect_fdset);
			}
			else
			{
				FD_CLR(pPeer->m_dataSock, &m_connect_fdset);
				FD_CLR(pPeer->m_dataSock, &m_fdset);
				pPeer->m_Flag = SOCK_CONNECT_BREAK;
			}
		}
		else
		{
			TEMP_FAILURE_RETRY(::close(sock)); return;
		}
	}
	if(iret >= 0) statNotify(pPeer->m_ID, SOCK_CONNECTED);
}

void CTcpClient::doNetIO(fd_set& fdset, fd_set& connect_fdset, int fdcnt)
{
	CServerPeer* pNeedRead = 0;
	CServerPeer* pNotifyConnect = 0;

	for(int ii = 0; ii < FD_SETSIZE; ++ii)
	{
		pNeedRead = 0;
		pNotifyConnect = 0;
		{
			LOCK(m_sockVectorLock);
			if(m_Peers[ii])
			{
				if(m_Peers[ii]->m_Flag == SOCK_CONNECT_PENDING)
				{
					if(FD_ISSET(m_Peers[ii]->m_dataSock, &connect_fdset))
					{
						if(FD_ISSET(m_Peers[ii]->m_dataSock, &fdset)) //sock可读并且可写，connect失败
						{
							m_Peers[ii]->m_Flag = SOCK_INIT; 			//允许重连
						}
						else //sock可写，connect成功
						{
							pNotifyConnect = m_Peers[ii];
							m_Peers[ii]->m_Flag = SOCK_CONNECTED;
							FD_CLR(m_Peers[ii]->m_dataSock, &m_connect_fdset);
						}
					}
				}
				else if(m_Peers[ii]->m_Flag == SOCK_CONNECTED)
				{
					if(FD_ISSET(m_Peers[ii]->m_dataSock, &fdset)) pNeedRead = m_Peers[ii];
				}
			}
		}
		//
		if(pNotifyConnect) statNotify(pNotifyConnect->m_ID, SOCK_CONNECTED);
		readDataSock(pNeedRead);
	}
}

void CTcpClient::readDataSock(CServerPeer* pPeer)
{
	if(!pPeer) return;
	int iRead = 0;
	CRingBuffer& buf = pPeer->m_dataBuf;
	unsigned freeSize = buf.getFreeSize();
	int sock = pPeer->m_dataSock;
	if(freeSize > 0)
	{
		unsigned char* pRead = buf.getReadPtr();
		unsigned char* pWrite= buf.getWritePtr();
		if(pWrite < pRead)
		{
			iRead = TEMP_FAILURE_RETRY(::read(sock, pWrite, freeSize));
			if(iRead == 0)//peer has closed
			{
				//change sock state
				{
				LOCK(m_sockVectorLock);
				if(pPeer->m_Flag != SOCK_CLOSE_PENDING) pPeer->m_Flag = SOCK_CONNECT_BREAK;
				}
				//notify connection has broken
				if(pPeer->m_Flag == SOCK_CONNECT_BREAK)
				{
					statNotify(pPeer->m_ID, SOCK_CONNECT_BREAK);
				}
				return;
			}
			else if(iRead < 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK){}
				else			//some error
				{
					//change sock state
					{
					LOCK(m_sockVectorLock);
					if(pPeer->m_Flag != SOCK_CLOSE_PENDING) pPeer->m_Flag = SOCK_CONNECT_BREAK;
					}
					//notify connection has broken
					if(pPeer->m_Flag == SOCK_CONNECT_BREAK)
					{
						statNotify(pPeer->m_ID, SOCK_CONNECT_BREAK);
					}
				}
				return;
			}
			else
			{
				buf.advanceWritePtr(iRead);
			}
		}
		else
		{
			unsigned size_1 = buf.getSizeFromWritePos2End();
			int iRead_1 = TEMP_FAILURE_RETRY(::read(sock, pWrite, size_1));
			if(iRead_1 == 0) //peer has closed
			{
				//change sock state
				{
				LOCK(m_sockVectorLock);
				if(pPeer->m_Flag != SOCK_CLOSE_PENDING) pPeer->m_Flag = SOCK_CONNECT_BREAK;
				}
				//notify connection has broken
				if(pPeer->m_Flag == SOCK_CONNECT_BREAK)
				{
					statNotify(pPeer->m_ID, SOCK_CONNECT_BREAK);
				}
				return;
			}
			else if(iRead_1 < 0)
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK) {}
				else //some error
				{
					//change sock state
					{
					LOCK(m_sockVectorLock);
					if(pPeer->m_Flag != SOCK_CLOSE_PENDING) pPeer->m_Flag = SOCK_CONNECT_BREAK;
					}
					//notify connection has broken
					if(pPeer->m_Flag == SOCK_CONNECT_BREAK)
					{
						statNotify(pPeer->m_ID, SOCK_CONNECT_BREAK);
					}
				}
				return;
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
				if(iRead_2 == 0) //peer has closed
				{
					//change sock state
					{
					LOCK(m_sockVectorLock);
					if(pPeer->m_Flag != SOCK_CLOSE_PENDING) pPeer->m_Flag = SOCK_CONNECT_BREAK;
					}
					//notify connection has broken
					if(pPeer->m_Flag == SOCK_CONNECT_BREAK)
					{
						statNotify(pPeer->m_ID, SOCK_CONNECT_BREAK);
					}
					return;
				}
				if(iRead_2 < 0)
				{
					if(errno == EAGAIN || errno == EWOULDBLOCK) {}
					else //some error
					{
						//change sock state
						{
						LOCK(m_sockVectorLock);
						if(pPeer->m_Flag != SOCK_CLOSE_PENDING) pPeer->m_Flag = SOCK_CONNECT_BREAK;
						}
						//notify connection has broken
						if(pPeer->m_Flag == SOCK_CONNECT_BREAK)
						{
							statNotify(pPeer->m_ID, SOCK_CONNECT_BREAK);
						}
						return;
					}
				}
				else
				{
					buf.advanceWritePtr(iRead_2);
					iRead += iRead_2;
				}
			}
		}
	}
	//
	if(pPeer->m_Flag != SOCK_CLOSE_PENDING) dataNotify(pPeer->m_ID, pPeer->m_dataBuf);
}

int CTcpClient::send(unsigned ID, const void* pData, size_t size)
{
	CServerPeer* pPeer = NULL;
	{
		LOCK(m_sockVectorLock);
		unsigned pos = ID2SlotIndex(ID);
		if(pos > 0 && m_Peers[pos] && m_Peers[pos]->m_Flag == SOCK_CONNECTED)
		{
			pPeer = m_Peers[pos]; pPeer->AddRef(); //hold this
		}
	}
	int iret = 0;
	if(pPeer)
	{
		{
		CScopeLock<CMutex> lock(pPeer->m_SendMutex);
		//after lock, check again
		if(pPeer->m_Flag == SOCK_CONNECTED)
		{
			struct timeval timeout = {0,0};
			iret = NS_ZH_UTIL::send_n(pPeer->m_dataSock, pData, size, timeout);
		}
		}//the lock will be released at here
		 //the Release() function may the delete self, so we should first release the lock
		pPeer->Release();
		return iret == size ? 0 : -1;
	}
	else return -1;
}
