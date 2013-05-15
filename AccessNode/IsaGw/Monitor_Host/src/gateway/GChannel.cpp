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
#include <boost/bind.hpp> //for binding to callback function
#include "GChannel.h"

#include "../util/serialization/Serialization.h"
#include "../util/serialization/NetworkOrder.h"

//added by Cristian.Guef
#include "crc32c.h"
#include <arpa/inet.h>

namespace nisa100 {
namespace gateway {

GChannel::GChannel(const ConfigApp& configApp_) :
	configApp(configApp_)
{
	if (configApp.GatewayListenMode())
	{
		LOG_ERROR("ERROR: Listen mode DISABLED");
		throw ChannelException("ERROR: Listen mode DISABLED");
#if 0	//disable listen mode. DO NOT DELETE, we may want to re-activate in the future
		try
		{
			socket.reset(new AsyncTCPServerSocket(configApp.GatewayPort()));
			//LOG_INFO("Chnnel is open in listenning mode. listenning on port=" << configApp.GatewayPort());
		}
		catch (std::exception& ex)
		{
			throw ChannelException("Unable to listen to port...");
		}
#endif
	}
	else
	{
		if (configApp.UseEncryption() == 0)
			socket.reset(new AsyncTCPSocket(configApp.GatewayHost(), configApp.GatewayPort()));
		else
			socket.reset(new AsyncTCPSocket(configApp.GatewayHost(), configApp.GatewayPort(), configApp.SSLClientCertifFile(), configApp.SSLClientKeyFile(), configApp.SSLCACertFile()));
		LOG_INFO("Chnnel is open in client mode. host=" << configApp.GatewayHost() << ", port=" << configApp.GatewayPort());
	}

	isOpen = false;
	socket->ConnectionStatus = boost::bind(&GChannel::ChangeState, this, _1);
	socket->ReceiveBytes = boost::bind(&GChannel::ReceiveBytes, this, _1, _2);
	
	//added
	m_countLeft = 0;
}

GChannel::~GChannel()
{
}

void GChannel::Start()
{
	socket->StartIOService();
}

void GChannel::Stop()
{
	socket->StopIOService();
}

void GChannel::Run(int usec)
{
	socket->RunIOService(usec);
}

void GChannel::SendPacket(GeneralPacket& packet)
{
	if (!isOpen)
	{
		THROW_EXCEPTION1(ChannelException, "Channel not open!");
	}

	//no need
	//const util::NetworkOrder& network = util::NetworkOrder::Instance();
	
	//added
	int len = packet.data.size() + 1/*version*/ + GeneralPacket::GeneralPacket_SIZE_VERSION3;
	if (len > (int)m_sendBuff.size())
		m_sendBuff.resize(packet.data.size() + 1/*version*/ + GeneralPacket::GeneralPacket_SIZE_VERSION3);


	//added
	unsigned char *pBuff = (unsigned char*)m_sendBuff.c_str();

	//no need
	//packet.version = configApp.GatewayPacketVersion();
	//std::basic_ostringstream<boost::uint8_t> buffer;

	//no need
	//util::binary_write(buffer, (boost::uint8_t) packet.version, network);
	//added
	*(pBuff++) = (unsigned char)packet.version;

	//no need
	//util::binary_write(buffer, (boost::uint8_t) packet.serviceType, network);
	//added
	*(pBuff++) = (unsigned char)packet.serviceType;

	//added by Cristian.Guef
	if(packet.version == 3)
	{
		//no need
		//util::binary_write(buffer, (boost::uint32_t) packet.sessionID, network);
		//added
		*((unsigned int*)pBuff) = htonl(packet.sessionID);
		pBuff += sizeof(unsigned int);
	}

	//no need
	//util::binary_write(buffer, packet.trackingID, network);
	//added
	*((unsigned int*)pBuff) = htonl(packet.trackingID);
	pBuff += sizeof(unsigned int);

	if (configApp.GatewayPacketVersion() == 1)
	{
		//no need
		//util::binary_write(buffer, (boost::uint16_t) packet.dataSize, network);
	}
	else // version == 2 || version == 3 (==3 added by Cristian.Guef)
	{
		//no need
		//util::binary_write(buffer, (boost::uint32_t) packet.dataSize, network);
		//added
		*((unsigned int*)pBuff) = htonl(packet.dataSize);
		pBuff += sizeof(unsigned int);
	}

	//added by Cristian.Guef
	if(packet.version == 3)
	{
		//aici se calculeaza header-ul
		//no need
		/*std::basic_string<boost::uint8_t> my_string(buffer.str());
		packet.headerCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
		packet.headerCRC = htonl(packet.headerCRC);
		buffer.write((boost::uint8_t*)&packet.headerCRC, sizeof(unsigned int));*/
		*((unsigned int*)pBuff) = htonl(GenerateCRC32C((unsigned char*)m_sendBuff.c_str(), 1/*version*/ + GeneralPacket::GeneralPacket_SIZE_VERSION3 - 4/*crc*/));
		pBuff += sizeof(unsigned int);
	}

	if (packet.dataSize > 0)
	{
		//no need
		//buffer.write(packet.data.c_str(), packet.dataSize);
		//added
		memcpy(pBuff, packet.data.c_str(), packet.dataSize);
	}


	//TODO:[andy] can we not copy the buffer here?
	//no need
	//std::basic_string<boost::uint8_t> string(buffer.str());
	try
	{
		//no need
		//socket->SendBytes((const char*) string.c_str(), string.size());
		//added
		socket->SendBytes((const char*) m_sendBuff.c_str(), len);

		/*
		if (LOG_DEBUG_ENABLED())
		{
			std::ostringstream dataBytes;
			dataBytes << std::hex << std::uppercase << std::setfill('0');
			//no need
			//for (int i = 0; i < (int) string.size(); i++)
			//added
			for (int i = 0; i < len; i++)
			{
				//no need
				//dataBytes << std::setw(2) << (int) string.at(i) << ' ';
				//added
				dataBytes << std::setw(2) << (int) m_sendBuff.at(i) << ' ';
			}
			LOG_DEBUG("SendPacket:packet=" << packet.ToString() << ",  bytes=<" << dataBytes.str() << ">");
		}
		*/
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
		if (isOpen != open)
		{
			isOpen = open;
			if (!isOpen)
			{
				LOG_INFO("ChangeState: Channel close.");

				//also reset
				m_countLeft = 0;

				// notify of disconnected event
				for (std::vector<boost::function0<void> >::iterator it =
						Disconnected.begin(); it != Disconnected.end(); it++)
				{
					try
					{
						(*it)();
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
				currentReadState = ReadingHeaderVersion;
				for (std::vector<boost::function2<void, const std::string&, int> >::iterator it =
						Connected.begin(); it != Connected.end(); it++)
				{
					try
					{
						(*it)(socket->Host(), socket->Port());
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
	/* commented -no debug
	std::ostringstream dataBytes;
	dataBytes << std::hex << std::uppercase << std::setfill('0');
	for (int i = 0; i < (int) count; i++)
	{
		dataBytes << std::setw(2) << (unsigned int) *((const unsigned char *) buffer + i) << ' ';
	}
	
	LOG_DEBUG("Received bytes=<" << dataBytes.str() << ">.");
	*/
	
	//no need
	//remainingBytes.insert(remainingBytes.end(), buffer, buffer + count);
	//const util::NetworkOrder& network = util::NetworkOrder::Instance();

	//added
	int offset = 0;
	memcpy(m_readBuffer + m_countLeft, buffer, count);
	m_countLeft += count;
	
	while (true) // will break if not enough bytes available
	{
		if (currentReadState == ReadingHeaderVersion)
		{
			//no need
			//if (remainingBytes.size() > 0)
			//added
			if (m_countLeft - offset > 0)
			{
				//no need
				//packetInProgress.version = util::binary_read_seq<boost::uint8_t>(remainingBytes, network);
				//added
				packetInProgress.version = m_readBuffer[offset++];

				currentReadState = ReadingHeader;
			}
			else
			{
				break; /* not enough bytes*/
			}
		}

		if (currentReadState == ReadingHeader)
		{
			int expectedPacketSize = GeneralPacket::GeneralPacket_SIZE_VERSION1;
			if (packetInProgress.version == 2)
			expectedPacketSize = GeneralPacket::GeneralPacket_SIZE_VERSION2;

			//added by Cristian.Guef
			if (packetInProgress.version == 3)
				expectedPacketSize = GeneralPacket::GeneralPacket_SIZE_VERSION3;

			//no need
			//if ((int) remainingBytes.size() >= expectedPacketSize)
			//added
			if (m_countLeft - offset >= expectedPacketSize)
			{
				//no need
				//packetInProgress.serviceType = (GeneralPacket::GServiceTypes) util::binary_read_seq<boost::uint8_t>(
				//		remainingBytes, network);
				//added
				packetInProgress.serviceType = (GeneralPacket::GServiceTypes)m_readBuffer[offset++];

				//added
				if (packetInProgress.version == 3)
				{
					//no need
					//packetInProgress.sessionID = util::binary_read_seq<boost::uint32_t>(remainingBytes, network);
					//added
					packetInProgress.sessionID = htonl(*((unsigned int*)(m_readBuffer + offset)));
					offset += sizeof(unsigned int);
				}

				//no need
				//packetInProgress.trackingID = util::binary_read_seq<boost::int32_t>(remainingBytes, network);
				//added
				packetInProgress.trackingID = htonl(*((unsigned int*)(m_readBuffer + offset)));
				offset += sizeof(unsigned int);

				if (packetInProgress.version == 1)
				{
					//no need
					//packetInProgress.dataSize = util::binary_read_seq<boost::uint16_t>(remainingBytes, network);
				}
				else if (packetInProgress.version == 2 || packetInProgress.version == 3 /* == 3 added by Cristian.Guef*/)
				{
					//no need
					//packetInProgress.dataSize = util::binary_read_seq<boost::uint32_t>(remainingBytes, network);
					//added
					packetInProgress.dataSize = htonl(*((unsigned int*)(m_readBuffer + offset)));
					offset += sizeof(unsigned int);
				}


				//added by Cristian.Guef
				if (packetInProgress.version == 3)
				{
					//no need
					//packetInProgress.headerCRC = util::binary_read_seq<boost::uint32_t>(remainingBytes, network);
					//added
					packetInProgress.headerCRC = htonl(*((unsigned int*)(m_readBuffer + offset)));
					offset += sizeof(unsigned int);
				}

				currentReadState = ReadingData;
			}
			else
			{
				break; /* not enough bytes*/
			}
		}

		if (currentReadState == ReadingData)
		{
			//no need
			//if (remainingBytes.size() >= packetInProgress.dataSize)
			//added
			if (m_countLeft - offset >= (int)packetInProgress.dataSize)
			{
				//no need
				//packetInProgress.data.insert(packetInProgress.data.end(), remainingBytes.begin(), remainingBytes.begin()
				//		+ packetInProgress.dataSize);
				//remainingBytes.erase(remainingBytes.begin(), remainingBytes.begin() + packetInProgress.dataSize);
				//added
				packetInProgress.data.resize(packetInProgress.dataSize);
				memcpy((void*)packetInProgress.data.c_str(), m_readBuffer + offset, packetInProgress.dataSize);
				offset += packetInProgress.dataSize;
				

				currentReadState = DataReadCompleted;
			}
			else
			{
				break;/* not enough bytes*/
			}
		}

		if (currentReadState == DataReadCompleted)
		{

			try
			{
				//LOG_DEBUG("ReceivePacket: " << packetInProgress.ToString());
				ReceivePacket(packetInProgress);
			}
			catch (std::exception& ex)
			{
				LOG_ERROR("ReceiveBytes: unhandled error notifying ReceivePacket. error=" << ex.what());
			}

			packetInProgress = GeneralPacket();
			currentReadState = ReadingHeaderVersion;
		}

	} /*while(true)*/

	//added
	memmove(m_readBuffer, m_readBuffer + offset, (m_countLeft -= offset));
	
}

} //namespace gateway
          } //namespace nisa100
