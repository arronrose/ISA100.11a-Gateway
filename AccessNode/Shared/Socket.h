/*
* Copyright (C) 2013 Nivis LLC.
* Email:   opensource@nivis.com
* Website: http://www.nivis.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
* 
* Redistribution and use in source and binary forms must retain this
* copyright notice.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

 /********************************************************************
	created:	10:10:2003
	file:		Socket.h
	author:		Claudiu Hobeanu, claudiu.hobeanu@nivis.com
	
	purpose:	 a wrapper class for bsd sockets
*********************************************************************/


#if !defined(AFX_SOCKET_H__6A87450F_D9E9_4D92_A92F_6B537E76B534__INCLUDED_)
#define AFX_SOCKET_H__6A87450F_D9E9_4D92_A92F_6B537E76B534__INCLUDED_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1


class  CWrapSocket  
{
public:
	CWrapSocket();
	
	virtual ~CWrapSocket();

public:

	virtual bool Create( int p_nType, int p_nFamily = AF_INET);
	bool Bind( unsigned short p_nPort, const char* p_szLocalAddress = NULL );
	virtual void Close();
	
	//time is in usec
	virtual bool CheckRecv( unsigned int  p_nTimeout = 10000 ) ;

	/// Check if can send to socket. Time is in usec
	virtual bool CanSend( void ) ;
	
	virtual void Attach(int p_nSock) {	Close(); m_socket = p_nSock; }

	
	bool IsValid(){	return (m_socket != INVALID_SOCKET);}
	operator int() const { return m_socket; }	
	void SetPort( unsigned short p_nPort ) { m_nPort = htons(p_nPort);}
	unsigned short GetPort( void ) { return  ntohs(m_nPort); };

	static unsigned long GetLocalIp();

protected:
	int			m_socket;
	unsigned short	m_nPort;	//net order
	int			m_nSockFamily;
	int			m_nSockType;
private:
	CWrapSocket( const CWrapSocket& p_rSock) 
	{
		//just to remove compiling warning
		m_socket = p_rSock.m_socket ; 
		m_nPort = p_rSock.m_nPort; 
	}
	int	m_nOnline;
	
};

#endif // !defined(AFX_SOCKET_H__6A87450F_D9E9_4D92_A92F_6B537E76B534__INCLUDED_)
