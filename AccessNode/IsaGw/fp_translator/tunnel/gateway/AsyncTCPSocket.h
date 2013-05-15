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

#ifndef ASYNCTCPSOCKET_H_
#define ASYNCTCPSOCKET_H_

#include "ISocket.h"

//#include <boost/bind.hpp>
#include "../../../../Shared/TcpSocket.h"

namespace tunnel {
namespace gateway {


class GChannel;

class AsyncTCPSocket
	: public ISocket
{
	
public:
	AsyncTCPSocket(const std::string& host, int port);
	virtual ~AsyncTCPSocket();

	virtual void RunIOService();
	virtual void StartIOService();
	virtual void StopIOService();
		
	virtual void SendBytes(const char* buffer, std::size_t count);
	virtual std::string Host();
	virtual int Port(); 

public: 
	void SetGChannel(GChannel *pGChannel);
private:
	GChannel *m_pGChannel;

private:
	void reconnect(int when);
	
	void handle_connect(void);
	
	void handle_read(std::size_t bytes_transferred);
	
	CTcpSocket *m_pTCPSocket;

	std::string host;
	int port;
	
	char readBuffer[64*1024];
	//bool bRunService;
};

} //namespace gateway
} //namespace tunnel

#endif /*ASYNCTCPSOCKET_H_*/
