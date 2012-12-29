/*
 * TcpCommondef.h
 *
 *  Created on: 2012-11-5
 *      Author: lankey
 */

#ifndef TCPCOMMONDEF_H_
#define TCPCOMMONDEF_H_

enum ESockStates
{
	SOCK_INIT,
	SOCK_CONNECT_PENDING,
	SOCK_CONNECTED,
	SOCK_CLOSE_PENDING,
	SOCK_CONNECT_BREAK
};


#endif /* TCPCOMMONDEF_H_ */
