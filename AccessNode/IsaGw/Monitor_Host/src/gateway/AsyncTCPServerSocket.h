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

#ifndef ASYNCTCPSERVERSOCKET_H_
#define ASYNCTCPSERVERSOCKET_H_

#include "ISocket.h"

#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
//#include "../Log.h"


namespace nisa100 {
namespace gateway {

class asio_serversocket; //forward decl

/**
 * @brief Decouple from asio async tcp socket.
 */ 
class AsyncTCPServerSocket
	: public ISocket
{
	/* commented by Ctistian.Guef
	LOG_DEF("nisa100.gateway.AsyncTCPServerSocket");
	*/

public:
	AsyncTCPServerSocket(int port);
	virtual ~AsyncTCPServerSocket();

	virtual void RunIOService(int usec);
	virtual void StartIOService();
	virtual void StopIOService();
		
	virtual void SendBytes(const char* buffer, std::size_t count);
	virtual std::string Host();
	virtual int Port(); 
	
private:
	void reconnect(int when);
	
	void async_wait_connect(const boost::system::error_code& error);
	void handle_connect(const boost::system::error_code& error);
	
	void async_read();
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
	
	void sync_write();
	
	boost::scoped_ptr<asio_serversocket> asiosocket; 

	int port;
	
	std::string connectedHost;
	int connectedPort;
	
	char readBuffer[64*1024];
};
	
} //namespace gateway
} //namespace nisa100

#endif /*ASYNCTCPSERVERSOCKET_H_*/
