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


#include "AsyncTCPSocket.h"
#include "../log/Log.h"

#include "GChannel.h"

namespace tunnel {
namespace gateway {

const int RECONNECT_TIMEOUT = 3; //seconds


AsyncTCPSocket::AsyncTCPSocket(const std::string& host_, int port_) :
	host(host_), port(port_)
{
	m_pTCPSocket = new CTcpSocket;
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

void AsyncTCPSocket::SetGChannel(GChannel *pGChannel)
{
	m_pGChannel = pGChannel;
}

void AsyncTCPSocket::RunIOService()
{
	bool disconnected = false;
	
	//connection
	if (!m_pTCPSocket->IsValid())
	{
		disconnected = true;
		reconnect(RECONNECT_TIMEOUT);
		if (!m_pTCPSocket->IsValid())
			return;
	}
	if (disconnected == true)
		handle_connect();
	
	//input_data
	int nReadLen = sizeof(readBuffer); 
	if (m_pTCPSocket->Recv(10000/*usec*/, (void*)readBuffer, nReadLen))
		handle_read(nReadLen);
}

void AsyncTCPSocket::StartIOService()
{
	this->host = host;
	this->port = port;

	try
	{
		m_pGChannel->ChangeState(false); // notify disconnected
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("reconnect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
	}
}

void AsyncTCPSocket::StopIOService()
{
}

void AsyncTCPSocket::SendBytes(const char* buffer, std::size_t count)
{
	if (!m_pTCPSocket->Send((const void*)buffer, count))
	{
		m_pGChannel->ChangeState(false); // notify disconnected
		std::exception ex;
		throw ex;
	}
}

void AsyncTCPSocket::reconnect(int when)
{
	try
	{
		m_pGChannel->ChangeState(false); // notify disconnected
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("reconnect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
	}

	m_pTCPSocket->Create();
	m_pTCPSocket->Connect(this->host.c_str(), this->port, when);
}

void AsyncTCPSocket::handle_connect()
{
	try
	{
		m_pGChannel->ChangeState(true); // notify connected
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("async_wait_connect: Unhandled exception when notifying ConnectionStatus!" << ex.what());
	}
}


void AsyncTCPSocket::handle_read(std::size_t bytes_transferred)
{
	//LOG_DEBUG("handle_read: received " << bytes_transferred << " bytes");
	try
	{
		m_pGChannel->ReceiveBytes(readBuffer, bytes_transferred);
	}
	catch (...)
	{
		LOG_ERROR("handle_read: Unhandled exception when notifying ReceiveBytes!");
	}

}

} //namespace gateway
} // namespace tunnel
