/*
 * TcpClient.h
 *
 *  Created on: 2012-4-18
 *      Author: zh
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_
#include <sys/select.h>

#include "INETAddr.h"
#include "RingBuffer.h"
#include "ScopeLock.h"
#include "TcpCommonDef.h"

struct CServerPeer : public NS_ZH_UTIL::CRefCountBase
{
	int m_dataSock;
	unsigned m_ID;
	unsigned m_Flag;
	NS_ZH_UTIL::CINETAddr m_peerAddr;
	NS_ZH_UTIL::CRingBuffer m_dataBuf;
	NS_ZH_UTIL::CMutex m_SendMutex;
	//
	CServerPeer()
	:m_dataSock(-1)
	,m_ID(0)
	,m_Flag(SOCK_INIT)
	,m_dataBuf(1024)
	{}
};

class CTcpClient
{
public:
	CTcpClient();
	virtual ~CTcpClient();
	unsigned socket(const char* ip, unsigned short port);
	int closeDataSock(unsigned ID);
	int send(unsigned ID, const void* pData, size_t size);
	int services();
protected:
	virtual void dataNotify(unsigned ID, NS_ZH_UTIL::CRingBuffer& buffer) {}
	virtual void statNotify(unsigned ID, enum ESockStates) {}
	int m_maxFd;
	fd_set m_fdset;
	fd_set m_connect_fdset;
	CServerPeer* m_Peers[FD_SETSIZE];
private:
	unsigned addSocket(int sock, NS_ZH_UTIL::CINETAddr& addr);
	void closeDataSock(CServerPeer* pPeer);
	unsigned allocID() {return __sync_fetch_and_add(&m_IDSeed, 2U);}
	void connectDataSock(CServerPeer* pPeer);
	void reconnectDataSock(CServerPeer* pPeer);
	void doNetIO(fd_set& fdset, fd_set& connect_fdset, int fdcnt);
	void readDataSock(CServerPeer* pPeer);
	//
	unsigned nextFreeSlot(unsigned hint);
	unsigned ID2SlotIndex(unsigned id);
	int maxSockFd();
private:
	NS_ZH_UTIL::CSpinLock m_sockVectorLock;
	unsigned m_IDSeed;
};

#endif /* TCPCLIENT_H_ */
