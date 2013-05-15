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

#include "GServiceUnserializer.h"

#include "../util/serialization/Serialization.h"
#include "../util/serialization/NetworkOrder.h"

#include <cassert>
#include <sstream>

#include "../gateway/Crc32c.h"
#include <arpa/inet.h>

#include "../log/Log.h"


namespace tunnel {
namespace comm {

static bool VerifyCRCPacketHeader_v3(const gateway::GeneralPacket *packet)
{
	assert(packet->version == 3);

	const util::NetworkOrder& network = util::NetworkOrder::Instance();

	std::basic_ostringstream<boost::uint8_t> buffer;

	util::binary_write(buffer, (boost::uint8_t) packet->version, network);
	util::binary_write(buffer, (boost::uint8_t) packet->serviceType, network);

	util::binary_write(buffer, (boost::uint32_t) packet->sessionID, network);
	
	util::binary_write(buffer, packet->trackingID, network);
	util::binary_write(buffer, (boost::uint32_t) packet->dataSize, network);
	
	std::basic_string<boost::uint8_t> my_string(buffer.str());
	boost::uint32_t headerCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	//headerCRC = htonl(headerCRC);

	return packet->headerCRC == headerCRC ? true : false;
}

static bool VerifyCRCPacketData_v3(const gateway::GeneralPacket *packet)
{

	assert(packet->version == 3);

	if (packet->dataSize == 0)
	{
		return true;
	}

	std::basic_ostringstream<boost::uint8_t> buffer;

	buffer.write(packet->data.c_str(), packet->dataSize - sizeof(unsigned int));
	std::basic_string<boost::uint8_t> my_string(buffer.str());
	unsigned int packDataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	//packDataCRC = htonl(packDataCRC);

	std::basic_ostringstream<boost::uint8_t> buffer_crc;
	buffer_crc.write(packet->data.c_str() + packet->dataSize - sizeof(unsigned int), sizeof(unsigned int));
	std::basic_string<boost::uint8_t> my_dataCRC(buffer_crc.str());
	unsigned int dataCRC = (my_dataCRC[0] << 24) | (my_dataCRC[1] << 16) | (my_dataCRC[2] << 8) | my_dataCRC[3];

	return packDataCRC == dataCRC ? true : false;
}


// returned values: 0 -success
//					-28
//					-43
//					-44
static ResponseStatus VerifyReceivedPacket_v3(const gateway::GeneralPacket *packet, unsigned int sessionID)
{
	if (packet->sessionID != sessionID)
		return rsFailure_InvalidReturnedSessionID;

	if (VerifyCRCPacketHeader_v3(packet) == false)
		return rsFailure_InvalidHeaderCRC;

	if (VerifyCRCPacketData_v3(packet) == false)
		return rsFailure_InvalidDataCRC;

	return rsSuccess;
}

static ResponseStatus VerifyReceivedPacket_v3(const gateway::GeneralPacket *packet)
{
	if (VerifyCRCPacketHeader_v3(packet) == false)
		return rsFailure_InvalidHeaderCRC;

	if (VerifyCRCPacketData_v3(packet) == false)
		return rsFailure_InvalidDataCRC;

	return rsSuccess;
}

ResponseStatus ParseClientServerCommunicationStatus(std::basic_istringstream<boost::uint8_t>& data,
  const util::NetworkOrder& network)
{
	boost::uint8_t commStatus = util::binary_read<boost::uint8_t>(data, network);
	switch (commStatus)
	{
		case 0:
		return rsSuccess;
		case 1:
		return rsFailure_NoUnBufferedReq;
		case 2:
		return rsFailure_InvalidBufferedReq;
		case 3:
		return rsFailure_GatewayLeaseExpired;
		case 4:
		return rsFailure_GatewayUnknown;

	}
	return rsFailure_GatewayInvalidFailureCode;
}



void GServiceUnserializer::Unserialize(AbstractGService& response_, const gateway::GeneralPacket& packet_)
{
	packet = &packet_;

	try
	{
		//LOG_DEBUG("Unserialize: try to unserialize Packet=" << packet_.ToString() << " with Response="
				//<< response_.ToString());

		//unserialize
		  response_.Accept(*this);
	  }
	  catch (SerializationException&)
	  {
		  LOG_ERROR("Unserialize: catch SerializationException");
		  throw;
	  }
	  catch (util::NotEnoughBytesException&)
	  {
		  LOG_ERROR("Unserialize: catch NotEnoughBytesException");
		  THROW_EXCEPTION1(SerializationException, "Not enough bytes!");
	  }
	  catch(std::exception& ex)
	  {
		  LOG_ERROR("Unserialize: catch std::exception=" << ex.what());
		  THROW_EXCEPTION1(SerializationException, ex.what());
	  }
	  catch(...)
	  {
		  LOG_ERROR("Unserialize: catch unknown exception");
		  THROW_EXCEPTION1(SerializationException, "unknown exception");
	  }
  }

  void GServiceUnserializer::Visit(GSession& session)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GSessionConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=LeaseConfirm!");
	  }


	  ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != rsSuccess)
	  {
		  session.m_status = st;
		  return;
	  }
	  
	  //data
	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();


	  session.m_sessionID = packet->sessionID;

	  boost::uint8_t sessionStatus = util::binary_read<boost::uint8_t>(data, network);
	  switch (sessionStatus)
	  {
		  case 0: session.m_status = rsSuccess; break;

		  case 1: session.m_status = rsSuccess_SessionLowerPeriod/* it wouldn't be the case for us*/; return;

		  case 2: session.m_status = rsFailure_ThereIsNoSession; return;

		  case 3: session.m_status = rsFailure_SessionNotCreated; return;

		  case 4:
		   session.m_status = rsFailure_GatewayUnknown;
		  return;
		  default:
		  LOG_WARN("Unserialize session=" << session.ToString() << " unknown status code=" << (boost::int32_t)sessionStatus );
		  session.m_status = rsFailure_GatewayInvalidFailureCode; return;
	  }

	  session.m_sessionPeriod = util::binary_read<boost::int32_t>(data, network);
  }

  void GServiceUnserializer::Visit(GLease& lease)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GLeaseConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=LeaseConfirm!");
	  }

	  //added by Cristian.Guef
	  ResponseStatus st = VerifyReceivedPacket_v3(packet, lease.m_sessionID);
	  if (st != rsSuccess)
	  {
		  lease.m_status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  boost::uint8_t leaseStatus = util::binary_read<boost::uint8_t>(data, network);
	  switch (leaseStatus)
	  {
		  case 0: lease.m_status = rsSuccess; break;

		  case 1: lease.m_status = rsSuccess_LeaseLowerPeriod /* it wouldn't be the case for us*/; return;

		  case 2: lease.m_status = rsFailure_ThereisNoLease; return;

		  case 3: lease.m_status = rsFailure_GatewayNoContractsAvailable; return;

		  case 4: lease.m_status = rsFailure_InvalidDevice; return;
		  case 5: lease.m_status = rsFailure_InvalidLeaseType; return;
		  case 6: lease.m_status = rsFailure_InvalidLeaseTypeInform; return;

		  case 7:
		  lease.m_status = rsFailure_GatewayUnknown;
		  return;
		  default:
		  LOG_WARN("Unserialize lease=" << lease.ToString() << " unknown status code=" << (boost::int32_t)leaseStatus );
		  lease.m_status = rsFailure_GatewayInvalidFailureCode; return;
	  }

	  lease.m_leaseID = util::binary_read<boost::uint32_t>(data, network);

		
	  lease.m_leasePeriod = util::binary_read<boost::uint32_t>(data, network);
		
  }

  
  void GServiceUnserializer::Visit(GClientServer_C& client)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  ResponseStatus st = VerifyReceivedPacket_v3(packet, client.m_sessionID);
	  if (st != rsSuccess)
	  {
		  client.m_status = st;
		  return;
	  }
	  
	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  client.m_status = ParseClientServerCommunicationStatus(data, network);
	  if (client.m_status != rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }

	  //data
	  client.m_respData = packet->data;

	  //strip off crc
	  client.m_respData.erase(client.m_respData.size() - 4, 4);
  }

  void GServiceUnserializer::Visit(GClientServer_S& server)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerRequest)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=GClientServerRequest!");
	  }

	  //added by Cristian.Guef
	  ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != rsSuccess)
	  {
		  server.m_status = st;
		  return;
	  }
	  server.m_status = rsSuccess;

	  //data
	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  //leaseID
	  util::binary_read<boost::uint32_t>(data, network);
	  int req_data_size = util::binary_read<boost::uint16_t>(data, network);

	  //data
	  for (int i = 0; i < req_data_size; i++)
		server.m_reqData.push_back(util::binary_read<boost::uint8_t>(data, network));
	  
	  //tunnel_info
	  util::binary_read_bytes(data, server.m_tunnelInfo.foreignDestAddress, sizeof(server.m_tunnelInfo.foreignDestAddress));
	  util::binary_read_bytes(data, server.m_tunnelInfo.foreignSrcAddress, sizeof(server.m_tunnelInfo.foreignSrcAddress));
	  
	  int conn_info_size = util::binary_read<boost::uint16_t>(data, network);
	  for (int i = 0; i < conn_info_size; i++)
		server.m_tunnelInfo.connInfo.push_back(util::binary_read<boost::uint8_t>(data, network));
  }

} /*namspace hostapp*/
} /*namspace nisa100*/
