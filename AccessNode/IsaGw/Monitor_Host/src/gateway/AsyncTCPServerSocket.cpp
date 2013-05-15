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

//added by Cristian.Guef
#ifdef __CYGWIN
#define __USE_W32_SOCKETS
#endif

#ifdef __USE_W32_SOCKETS
#include <winsock2.h> //hack [nicu.dascalu] - should be included before asio
#endif
#include <boost/asio.hpp>

//   
 enum LogLevel   
 {   
         LL_ERROR = 1,   
         LL_WARN,   
         LL_INFO,   
         LL_DEBUG,   
         LL_MAX_LEVEL   
 };   
    
    
 bool InitLogEnv(const char *pszIniFile);   
 bool IsLogEnabled(enum LogLevel);   
    
 void mhlog(enum LogLevel debugLevel, const std::ostream& message ) ;   
 void mhlog(enum LogLevel debugLevel, const char* message) ;   
    
 #define LOG_DEBUG(message) \   
         mhlog(LL_DEBUG, std::stringstream().flush() <<message)   
    
 #define LOG_INFO(message) \   
         mhlog(LL_INFO, std::stringstream().flush() <<message)   
    
 #define LOG_WARN(message) \   
         mhlog(LL_WARN, std::stringstream().flush() <<message)   
    
 #define LOG_ERROR(message) \   
         mhlog(LL_ERROR, std::stringstream().flush() <<message)   
    
 #define LOG_DEF(name) inline void __NOOP(){}   
 #define LOG_INFO_ENABLED() IsLogEnabled(LL_INFO)   
 #define LOG_DEBUG_ENABLED() IsLogEnabled(LL_DEBUG)   
 // 


#include "AsyncTCPServerSocket.h"

namespace nisa100 {
namespace gateway {

const int RECONNECT_TIMEOUT = 3; //seconds

class asio_serversocket
{
public:
	asio_serversocket() :
		socket(ioservice), acceptor(ioservice), reconnecttimer(ioservice)
	{
	}
	boost::asio::io_service ioservice;
	boost::asio::ip::tcp::socket socket;
	boost::asio::ip::tcp::acceptor acceptor;
	boost::asio::deadline_timer reconnecttimer;
};

AsyncTCPServerSocket::AsyncTCPServerSocket(int port_) :
	port(port_)
{
	asiosocket.reset(new asio_serversocket());

	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	asiosocket->acceptor.open(endpoint.protocol());
	asiosocket->acceptor.bind(endpoint);
	asiosocket->acceptor.listen();
}

AsyncTCPServerSocket::~AsyncTCPServerSocket()
{
	asiosocket->acceptor.close();
}

std::string AsyncTCPServerSocket::Host()
{
	return connectedHost;
}

int AsyncTCPServerSocket::Port()
{
	return connectedPort;
}


void AsyncTCPServerSocket::RunIOService(int usec)
{
	asiosocket->ioservice.run();
}

void AsyncTCPServerSocket::StartIOService()
{
	this->port = port;

	reconnect(0);
}

void AsyncTCPServerSocket::StopIOService()
{
	//ioservice-> post(boost::bind(&boost::asio::ip::tcp::socket::close, socket.get()));
	asiosocket->ioservice.stop(); //CHECKME [nicu.dascalu] - should be called in ioservice thread ???
}

void AsyncTCPServerSocket::SendBytes(const char* buffer, std::size_t count)
{
	boost::asio::write(asiosocket->socket, boost::asio::buffer(buffer, count));
}

void AsyncTCPServerSocket::reconnect(int when)
{
	try
	{
		ConnectionStatus(false); // notify disconnected
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("server_reconnect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
	}

	asiosocket->socket.close();
//	asiosocket->acceptor.close();

	// wait X seconds before retry a reconnection
	asiosocket->reconnecttimer.expires_from_now(boost::posix_time::seconds(when));
	asiosocket->reconnecttimer.async_wait(boost::bind(&AsyncTCPServerSocket::async_wait_connect, this,
	    boost::asio::placeholders::error));
}

void AsyncTCPServerSocket::async_wait_connect(const boost::system::error_code& error)
{
	LOG_INFO("async_server_wait_connect: listening on port=" << port << " wait_error=" << error);

	asiosocket->acceptor.async_accept(asiosocket->socket, boost::bind(&AsyncTCPServerSocket::handle_connect, this,
	    boost::asio::placeholders::error));
}

void AsyncTCPServerSocket::handle_connect(const boost::system::error_code& error)
{
	if (!error)
	{
		try
		{
			connectedHost = asiosocket->socket.remote_endpoint().address().to_string();
			connectedPort = asiosocket->socket.remote_endpoint().port();
			ConnectionStatus(true); // notify connected
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("async_server_wait_connect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
		}
		async_read();
	}
	else
	{
		LOG_INFO("async_server_wait_connect: connection failed! error_code=" << error);
		reconnect(RECONNECT_TIMEOUT);
	}
}

void AsyncTCPServerSocket::async_read()
{
	boost::asio::async_read(asiosocket->socket, boost::asio::buffer(readBuffer, sizeof(readBuffer)),
	    boost::asio::transfer_at_least(1), boost::bind(&AsyncTCPServerSocket::handle_read, this,
	        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void AsyncTCPServerSocket::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (!error)
	{
		LOG_DEBUG("server_handle_read: received " << bytes_transferred << " bytes");
		try
		{
			ReceiveBytes(readBuffer, bytes_transferred);
		}
		catch (...)
		{
			LOG_ERROR("server_handle_read: Unhandled exception when notifying ReceiveBytes!");
		}

		async_read(); //schedule next read
	}
	else if (error != boost::asio::error::operation_aborted)
	{
		LOG_ERROR("server_handle_read: connection lost! error=" << error.message());
		reconnect(RECONNECT_TIMEOUT);
	}
	else // exit requested
	{

	}
}

} //namespace gateway
} // namespace nisa100
