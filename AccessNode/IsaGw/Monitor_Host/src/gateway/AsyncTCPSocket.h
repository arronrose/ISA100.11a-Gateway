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

/*commented by Cristian.Guef
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
*/

//added by Cristian.Guef
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <Shared/TcpSocket.h>
#include <Shared/TcpSecureSocket.h>

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../Log.h"


namespace nisa100 {
namespace gateway {

class asio_socket; //forward decl

/**
 * @brief Decouple from asio async tcp socket.
 */ 
class AsyncTCPSocket
	: public ISocket
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.gateway.AsyncTCPSocket");
	*/

public:
	AsyncTCPSocket(const std::string& host, int port);

	AsyncTCPSocket(const std::string& host, int port, const std::string &sslCertifFile, const std::string &sslKeyFile, const std::string &sslCAFile);

	virtual ~AsyncTCPSocket();

	virtual void RunIOService(int usec);
	virtual void StartIOService();
	virtual void StopIOService();
		
	virtual void SendBytes(const char* buffer, std::size_t count);
	virtual std::string Host();
	virtual int Port(); 
	
private:
	void reconnect(int when);
	
	/*commented by Cristian.Guef
	void async_wait_connect(const boost::system::error_code& error);
	void handle_connect(const boost::system::error_code& error);
	*/
	//added by Cristian.Guef
	void handle_connect(void);

	/*commented by Cristian.Guef
	void async_read();
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
	*/
	//added by Cristian.Guef
	void handle_read(std::size_t bytes_transferred);

	/*commented by Cristian.Guef
	void sync_write();
	boost::scoped_ptr<asio_socket> asiosocket; 
	*/
	//added by Cristian.Guef
	CTcpSocket *m_pTCPSocket;

	std::string host;
	int port;
	
	char readBuffer[64*1024];
};

} //namespace gateway
} //namespace nisa100

#endif /*ASYNCTCPSOCKET_H_*/
