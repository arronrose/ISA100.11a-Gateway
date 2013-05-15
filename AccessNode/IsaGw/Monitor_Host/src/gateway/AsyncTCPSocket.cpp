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

/*commented by Cristian.Guef
#ifdef __USE_W32_SOCKETS
#	include <winsock2.h> //hack [nicu.dascalu] - should be included before asio
#endif
#include <boost/asio.hpp>
*/


#include "AsyncTCPSocket.h"

namespace nisa100 {
namespace gateway {

const int RECONNECT_TIMEOUT = 3; //seconds

/*commented by Cristian.Guef
class asio_socket
{
public:
	asio_socket() :
		socket(ioservice), reconnecttimer(ioservice)
	{
	}
	boost::asio::io_service ioservice;
	boost::asio::ip::tcp::socket socket;
	boost::asio::deadline_timer reconnecttimer;
};
*/

AsyncTCPSocket::AsyncTCPSocket(const std::string& host_, int port_) :
	host(host_), port(port_)
{
	/* commented by Cristian.Guef
	asiosocket.reset(new asio_socket());
	*/
	//added by Cristian.Guef
	m_pTCPSocket = new CTcpSocket;
}

AsyncTCPSocket::AsyncTCPSocket(const std::string& host_, int port_, const std::string &sslCertifFile, 
			   const std::string &sslKeyFile, const std::string &sslCAFile):host(host_), port(port_)
{
	LOG_INFO("host=" << host << "port=" << port);
	LOG_INFO("sslKeyFile=" << sslKeyFile << " sslCertifFile=" << sslCertifFile << " sslCertifFile=" << sslCAFile);
	m_pTCPSocket = new CTcpSecureSocket();
	((CTcpSecureSocket*)m_pTCPSocket)->InitSSL(CTcpSecureSocket::CLIENT_SIDE, 
		sslKeyFile.c_str(), sslCertifFile.c_str(), sslCAFile.c_str() );
}

AsyncTCPSocket::~AsyncTCPSocket()
{
	delete m_pTCPSocket;
}

std::string AsyncTCPSocket::Host()
{
	return host;
}

int AsyncTCPSocket::Port()
{
	return port;
}


void AsyncTCPSocket::RunIOService(int usec)
{
	/* commented by Cristian.Guef
	asiosocket->ioservice.run();
	*/
	//added by Cristian.Guef
	bool disconnected = false;
	if (!m_pTCPSocket->IsValid())
	{
		disconnected = true;
		reconnect(RECONNECT_TIMEOUT);
		if (!m_pTCPSocket->IsValid())
			return;
	}
	if (disconnected == true)
		handle_connect();
	int nReadLen = sizeof(readBuffer); 
	if (m_pTCPSocket->Recv(usec, (void*)readBuffer, nReadLen))
		handle_read(nReadLen);

}

void AsyncTCPSocket::StartIOService()
{
	this->host = host;
	this->port = port;

	/*commented by Cristian.Guef
	reconnect(0);
	*/

	//added by Cristian.Guef
	try
	{
		ConnectionStatus(false); // notify disconnected
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("reconnect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
	}
}

void AsyncTCPSocket::StopIOService()
{
	/* commented by Cristian.Guef
	//ioservice-> post(boost::bind(&boost::asio::ip::tcp::socket::close, socket.get()));
	asiosocket->ioservice.stop(); //CHECKME [nicu.dascalu] - should be called in ioservice thread ???
	*/
}

void AsyncTCPSocket::SendBytes(const char* buffer, std::size_t count)
{
	/* commented by Cristian.Guef
	boost::asio::write(asiosocket->socket, boost::asio::buffer(buffer, count));
	*/
	//added by Cristian.Guef
	if (!m_pTCPSocket->Send((const void*)buffer, count))
	{
		ConnectionStatus(false); // notify disconnected
		std::exception ex;
		throw ex;
	}
}

void AsyncTCPSocket::reconnect(int when)
{
	/* commented by Cristian.Guef
	try
	{
		ConnectionStatus(false); // notify disconnected
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("reconnect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
	}

	asiosocket->socket.close();

	// wait X seconds before retry a reconnection
	asiosocket->reconnecttimer.expires_from_now(boost::posix_time::seconds(when));
	asiosocket->reconnecttimer.async_wait(boost::bind(&AsyncTCPSocket::async_wait_connect, this,
	    boost::asio::placeholders::error));
		*/
	//added by Cristian.Guef
	try
	{
		ConnectionStatus(false); // notify disconnected
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("reconnect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
	}

	m_pTCPSocket->Create();
	m_pTCPSocket->Connect(this->host.c_str(), this->port, when);
}

/* commented by Cristian.Guef
void AsyncTCPSocket::async_wait_connect(const boost::system::error_code& error)
{
	LOG_INFO("async_wait_connect: trying to connect to host=" << host << " port=" << port << " wait_error=" << error);

	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);
	asiosocket->socket.async_connect(endpoint, boost::bind(&AsyncTCPSocket::handle_connect, this,
	    boost::asio::placeholders::error));
}

void AsyncTCPSocket::handle_connect(const boost::system::error_code& error)
{
	if (!error)
	{
		try
		{
			ConnectionStatus(true); // notify connected
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("async_wait_connect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
		}
		async_read();
	}
	else
	{
		LOG_INFO("async_wait_connect: connection failed! error_code=" << error);
		reconnect(RECONNECT_TIMEOUT);
	}
}
*/
//added by Cristian.Guef
void AsyncTCPSocket::handle_connect()
{
	try
	{
		ConnectionStatus(true); // notify connected
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("async_wait_connect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
	}
}

/* commented by Cristian.Guef
void AsyncTCPSocket::async_read()
{
	boost::asio::async_read(asiosocket->socket, boost::asio::buffer(readBuffer, sizeof(readBuffer)),
	    boost::asio::transfer_at_least(1), boost::bind(&AsyncTCPSocket::handle_read, this,
	        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void AsyncTCPSocket::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (!error)
	{
		LOG_DEBUG("handle_read: received " << bytes_transferred << " bytes");
		try
		{
			ReceiveBytes(readBuffer, bytes_transferred);
		}
		catch (...)
		{
			LOG_ERROR("handle_read: Unhandled exception when notifying ReceiveBytes!");
		}

		async_read(); //schedule next read
	}
	else if (error != boost::asio::error::operation_aborted)
	{
		LOG_ERROR("handle_read: connection lost! error=" << error.message());
		reconnect(RECONNECT_TIMEOUT);
	}
	else // exit requested
	{

	}
}
*/

//added by Cristian.uef
void AsyncTCPSocket::handle_read(std::size_t bytes_transferred)
{
	//LOG_DEBUG("handle_read: received " << bytes_transferred << " bytes");
	try
	{
		ReceiveBytes(readBuffer, bytes_transferred);
	}
	catch (...)
	{
		LOG_ERROR("handle_read: Unhandled exception when notifying ReceiveBytes!");
	}

}


} //namespace gateway
} // namespace nisa100
