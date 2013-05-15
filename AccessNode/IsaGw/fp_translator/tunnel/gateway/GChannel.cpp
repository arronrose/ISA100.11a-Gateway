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

#include <iomanip>
#include <sstream>
//#include <boost/bind.hpp> //for binding to callback function
#include "GChannel.h"

#include "../util/serialization/Serialization.h"
#include "../util/serialization/NetworkOrder.h"

#include "Crc32c.h"
#include <arpa/inet.h>

#include "../log/Log.h"

#include "../flow/TrackingManager.h"

namespace tunnel {
namespace gateway {

GChannel::GChannel(const std::string& host, int port)
{
	m_socket = new AsyncTCPSocket(host, port);
	
	m_isOpen = false;
	((AsyncTCPSocket*)m_socket)->SetGChannel(this);
}

GChannel::~GChannel()
{
}

void GChannel::Start()
{
	m_socket->StartIOService();
}

void GChannel::Stop()
{
	m_socket->StopIOService();
}

void GChannel::Run()
{
	m_socket->RunIOService();
}

void GChannel::SetTrackManager(tunnel::comm::TrackingManager *pTrackManager)
{
	m_pTrackManager = pTrackManager;
}

void GChannel::SendPacket(GeneralPacket& packet)
{
	if (!m_isOpen)
	{
		THROW_EXCEPTION1(ChannelException, "Channel not open!");
	}

	const util::NetworkOrder& network = util::NetworkOrder::Instance();

	packet.version = 3;  //we support version 3

	std::basic_ostringstream<boost::uint8_t> buffer;
	util::binary_write(buffer, (boost::uint8_t) packet.version, network);
	util::binary_write(buffer, (boost::uint8_t) packet.serviceType, network);

	if(packet.version == 3)
	{
		util::binary_write(buffer, (boost::uint32_t) packet.sessionID, network);
	}
	else
	{
		THROW_EXCEPTION1(ChannelException, "Invalid packet version (!= 3)!");
	}

	util::binary_write(buffer, packet.trackingID, network);
	util::binary_write(buffer, (boost::uint32_t) packet.dataSize, network);
	
	//CRC32C
	{
		std::basic_string<boost::uint8_t> my_string(buffer.str());
		packet.headerCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
		packet.headerCRC = htonl(packet.headerCRC);
		buffer.write((boost::uint8_t*)&packet.headerCRC, sizeof(unsigned int));
	}

	if (packet.dataSize > 0)
		buffer.write(packet.data.c_str(), packet.dataSize);

	//
	std::basic_string<boost::uint8_t> string(buffer.str());
	try
	{
		m_socket->SendBytes((const char*) string.c_str(), string.size());

		if (LOG_DEBUG_ENABLED())
		{
			std::ostringstream dataBytes;
			dataBytes << std::hex << std::uppercase << std::setfill('0');
			for (int i = 0; i < (int) string.size(); i++)
			{
				dataBytes << std::setw(2) << (int) string.at(i) << ' ';
			}
			LOG_DEBUG("SendPacket:packet=" << packet.ToString() << ",  bytes=<" << dataBytes.str() << ">");
		}
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("SendPacket: Failed to write on socket. error=" << ex.what());
		THROW_EXCEPTION1(ChannelException, "Failed to write on channel!");
	}
	catch (...)
	{
		LOG_ERROR("SendPacket: Failed to write on socket. unknown error!");
		THROW_EXCEPTION1(ChannelException, "Failed to write on channel!");
	}
}

void GChannel::ChangeState(bool open)
{
	try
	{
		if (m_isOpen != open)
		{
			m_isOpen = open;
			if (!m_isOpen)
			{
				LOG_WARN("ChangeState: Channel close.");
				// notify of disconnected event
				for (std::vector<IGWTrigger* >::iterator it =
						m_disconnected.begin(); it != m_disconnected.end(); it++)
				{
					try
					{
						(*it)->GWDisconnected();
					}
					catch (std::exception& ex)
					{
						LOG_ERROR("Fire event: gateway connected: failed! error=" << ex.what());
					}
					catch (...)
					{
						LOG_ERROR("Fire event: gateway disconnected. failed! unknown error!");
					}
				}
			}
			else
			{
				LOG_INFO("ChangeState: Channel open.");
				m_currentReadState = ReadingHeaderVersion;
				for (std::vector<IGWTrigger* >::iterator it =
						m_connected.begin(); it != m_connected.end(); it++)
				{
					try
					{
						(*it)->GWConnected(m_socket->Host(), m_socket->Port());
					}
					catch (std::exception& ex)
					{
						LOG_ERROR("Fire event: gateway connected: failed! error=" << ex.what());
					}
					catch (...)
					{
						LOG_ERROR("Fire event: gateway connected: failed! unknown error!");
					}
				}
			}
		}
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("ChangeState: Unhandled exception when notifying Channel::ChangeState! " << ex.what());
	}
}

void GChannel::ReceiveBytes(const char* buffer, std::size_t count)
{
	std::ostringstream dataBytes;
	dataBytes << std::hex << std::uppercase << std::setfill('0');

	for (int i = 0; i < (int) count; i++)
	{
		dataBytes << std::setw(2) << (unsigned int) *((const unsigned char *) buffer + i) << ' ';
	}

	//LOG_DEBUG("Received bytes=<" << dataBytes.str() << ">.");

	m_remainingBytes.insert(m_remainingBytes.end(), buffer, buffer + count);

	const util::NetworkOrder& network = util::NetworkOrder::Instance();

	while (true) // will break if not enough bytes available
	{
		if (m_currentReadState == ReadingHeaderVersion)
		{
			if (m_remainingBytes.size() > 0)
			{
				m_packetInProgress.version = util::binary_read_seq<boost::uint8_t>(m_remainingBytes, network);

				m_currentReadState = ReadingHeader;
			}
			else
			{
				break; /* not enough bytes*/
			}
		}

		if (m_currentReadState == ReadingHeader)
		{
			int expectedPacketSize;
			
			if (m_packetInProgress.version == 3)
			{
				expectedPacketSize = GeneralPacket::GeneralPacket_SIZE_VERSION3;
			}
			else
			{
				LOG_ERROR("Invalid packet version");
				m_currentReadState == ReadingHeader;
				m_packetInProgress = GeneralPacket();
				break;
			}

			if ((int) m_remainingBytes.size() >= expectedPacketSize)
			{
				m_packetInProgress.serviceType = (GeneralPacket::GServiceTypes) util::binary_read_seq<boost::uint8_t>(
						m_remainingBytes, network);

				m_packetInProgress.sessionID = util::binary_read_seq<boost::uint32_t>(m_remainingBytes, network);
				
				m_packetInProgress.trackingID = util::binary_read_seq<boost::int32_t>(m_remainingBytes, network);

				m_packetInProgress.dataSize = util::binary_read_seq<boost::uint32_t>(m_remainingBytes, network);
				
				m_packetInProgress.headerCRC = util::binary_read_seq<boost::uint32_t>(m_remainingBytes, network);

				m_currentReadState = ReadingData;
			}
			else
			{
				break; /* not enough bytes*/
			}
		}

		if (m_currentReadState == ReadingData)
		{
			if (m_remainingBytes.size() >= m_packetInProgress.dataSize)
			{
				m_packetInProgress.data.insert(m_packetInProgress.data.end(), m_remainingBytes.begin(), m_remainingBytes.begin()
						+ m_packetInProgress.dataSize);
				m_remainingBytes.erase(m_remainingBytes.begin(), m_remainingBytes.begin() + m_packetInProgress.dataSize);

				m_currentReadState = DataReadCompleted;
			}
			else
			{
				break;/* not enough bytes*/
			}
		}

		if (m_currentReadState == DataReadCompleted)
		{

			try
			{
				LOG_DEBUG("ReceivePacket: " << m_packetInProgress.ToString());
				m_pTrackManager->ReceivePacket(m_packetInProgress);
			}
			catch (std::exception& ex)
			{
				LOG_ERROR("ReceiveBytes: unhandled error notifying ReceivePacket. error=" << ex.what());
			}

			m_packetInProgress = GeneralPacket();
			m_currentReadState = ReadingHeaderVersion;
		}

	} /*while(true)*/
}

} //namespace gateway
} //namespace tunnel
