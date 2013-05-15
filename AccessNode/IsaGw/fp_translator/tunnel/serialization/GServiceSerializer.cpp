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

#include "GServiceSerializer.h"

#include "../util/serialization/Serialization.h"
#include "../util/serialization/NetworkOrder.h"

#include <sstream>

#include <string>
#include <arpa/inet.h>


#include "../gateway/Crc32c.h"

#include "../log/Log.h"


namespace tunnel {
namespace comm {


void GServiceSerializer::Serialize(AbstractGService& request_, gateway::GeneralPacket& packet_)
{
	packet = &packet_;

	//serialize data & complete serviceType
	request_.Accept(*this);
}

void GServiceSerializer::Visit(GSession& session)
{
	//header
	packet->serviceType = gateway::GeneralPacket::GSessionRequest;
	packet->sessionID = session.m_sessionID;  //it must be '0'
	
	//data
	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, (boost::uint32_t)session.m_sessionPeriod, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint16_t)session.m_networkID, util::NetworkOrder::Instance());
	
	//crc
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	//
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}


void GServiceSerializer::Visit(GLease& lease)
{
	//----header
	packet->serviceType = gateway::GeneralPacket::GLeaseRequest;
	packet->sessionID = lease.m_sessionID;

	//
	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, lease.m_leaseID, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_leasePeriod, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint8_t)lease.m_leaseType, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_protocolType, util::NetworkOrder::Instance());
	
	//endpoint
	util::binary_write(data, lease.m_endPointsNo, util::NetworkOrder::Instance());
	data.write(lease.m_endPoint.ipAddress, 16);
	util::binary_write(data, lease.m_endPoint.remotePort, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_endPoint.remoteObjID, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_endPoint.localPort, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_endPoint.localObjID, util::NetworkOrder::Instance());

	//param
	util::binary_write(data, lease.m_parameters.transferMode, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_parameters.updatePolicy, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_parameters.subscriptionPeriod, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_parameters.phase, util::NetworkOrder::Instance());
	util::binary_write(data, lease.m_parameters.staleLimit, util::NetworkOrder::Instance());
	
	//conn
	util::binary_write(data, (boost::uint16_t)lease.m_connInfo.size(), util::NetworkOrder::Instance());
	data.write(lease.m_connInfo.c_str(), lease.m_connInfo.size());

	//crc
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	//
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}



void  GServiceSerializer::Visit(GClientServer_C& client)
{
	//header
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;
	packet->sessionID = client.m_sessionID;  //it must be '0'

	//data
	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, client.m_leaseID, util::NetworkOrder::Instance());
	util::binary_write(data, client.m_buffer, util::NetworkOrder::Instance());
	util::binary_write(data, client.m_transferMode, util::NetworkOrder::Instance());

	//reqdata
	util::binary_write(data, (boost::uint16_t)client.m_reqData.size(), util::NetworkOrder::Instance());
	data.write(client.m_reqData.c_str(), client.m_reqData.size());

	//transac
	util::binary_write(data, (boost::uint16_t)client.m_transacInfo.size(), util::NetworkOrder::Instance());
	data.write(client.m_transacInfo.c_str(), client.m_transacInfo.size()); 

	//crc
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));

	//
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}
	
void  GServiceSerializer::Visit(GClientServer_S& server)
{
	
}


} //namespace comm
} //namespace tunnel
