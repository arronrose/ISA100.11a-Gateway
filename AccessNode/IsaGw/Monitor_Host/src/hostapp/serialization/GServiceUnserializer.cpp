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

#include <../AccessNode/Shared/DurationWatcher.h>

#include "../../util/serialization/Serialization.h"
#include "../../util/serialization/NetworkOrder.h"

#include <cassert>
#include <sstream>

//added by Cristian.Guef
#include "../../gateway/crc32c.h"
#include <arpa/inet.h>
#include <sstream>
#include <iomanip>
#include <string.h>

#include <tai.h>

namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <nlib/datetime.h>


//added by Cristian.Guef
struct MyDateTime
{
	nlib::DateTime	ReadingTime;
	short 			milisec;
};

//added by Cristian.Guef
 static void gettime(MyDateTime& myDT)
{
  struct timeval tv;
  time_t curtime;
  gettimeofday(&tv, NULL); 
  curtime=tv.tv_sec;
  struct tm * timeinfo;

  timeinfo = gmtime(&curtime);
  myDT.ReadingTime = nlib::CreateTime(timeinfo->tm_year+1900, timeinfo->tm_mon + 1,
  									timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, 
  									timeinfo->tm_sec);
  myDT.milisec = tv.tv_usec/1000;
  
  //printf ("\n Current local time and date: %s.%d", nlib::ToString(myDT.ReadingTime).c_str(), myDT.milisec);
  //fflush(stdout);
  
}

 static nlib::DateTime gettime(int time)
{
  time_t curtime;
  curtime=time;
  struct tm * timeinfo;

  timeinfo = gmtime(&curtime);
  return nlib::CreateTime(timeinfo->tm_year+1900, timeinfo->tm_mon + 1,
  									timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, 
  									timeinfo->tm_sec);
}


static nlib::DateTime NetworkHealthTime;



//added by Cristian.Guef
extern unsigned int MonitH_GW_SessionID; //only for DeviceListReport and NetworkListReport

//added by Cristian.Guef
static bool VerifyCRCPacketHeader_v3(const gateway::GeneralPacket *packet)
{
	assert(packet->version == 3);

	/* old_way
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
	*/
	//added new way
	struct Header
	{
		unsigned char version;
		unsigned char serviceType;
		unsigned int  sessionID;
		unsigned int  trackingID;
		unsigned int  dataSize;
	}__attribute__((packed)) msgHeader = {packet->version, packet->serviceType, 
					htonl(packet->sessionID), htonl(packet->trackingID), htonl(packet->dataSize)};
	boost::uint32_t headerCRC = GenerateCRC32C((unsigned char*)&msgHeader, sizeof(msgHeader));
	
	return packet->headerCRC == headerCRC;
}

//added by Cristian.Guef
static bool VerifyCRCPacketData_v3(const gateway::GeneralPacket *packet)
{

	assert(packet->version == 3);

	if (packet->dataSize == 0)
	{
		return true;
	}

	/* old_way
	std::basic_ostringstream<boost::uint8_t> buffer;

	buffer.write(packet->data.c_str(), packet->dataSize - sizeof(unsigned int));
	std::basic_string<boost::uint8_t> my_string(buffer.str());
	unsigned int packDataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	//packDataCRC = htonl(packDataCRC);

	std::basic_ostringstream<boost::uint8_t> buffer_crc;
	buffer_crc.write(packet->data.c_str() + packet->dataSize - sizeof(unsigned int), sizeof(unsigned int));
	std::basic_string<boost::uint8_t> my_dataCRC(buffer_crc.str());
	unsigned int dataCRC = (my_dataCRC[0] << 24) | (my_dataCRC[1] << 16) | (my_dataCRC[2] << 8) | my_dataCRC[3];
	*/
	//added new way
	int dataLen = packet->dataSize - sizeof(unsigned int);
	unsigned int packDataCRC = GenerateCRC32C(packet->data.c_str(), dataLen);
	unsigned int dataCRC = htonl(*((unsigned int*)(packet->data.c_str() + dataLen)));
	return packDataCRC == dataCRC;
}


//added by Cristian.Guef
// returned values: 0 -success
//					-28
//					-43
//					-44
static Command::ResponseStatus VerifyReceivedPacket_v3(const gateway::GeneralPacket *packet)
{
	if (packet->sessionID != MonitH_GW_SessionID)
		return Command::rsFailure_InvalidReturnedSessionID;

	if (VerifyCRCPacketHeader_v3(packet) == false)
		return Command::rsFailure_InvalidHeaderCRC;

	if (VerifyCRCPacketData_v3(packet) == false)
		return Command::rsFailure_InvalidDataCRC;

	return Command::rsSuccess;
}

static Command::ResponseStatus VerifyReceivedPacket_v3_whithout_session(const gateway::GeneralPacket *packet)
{
	if (VerifyCRCPacketHeader_v3(packet) == false)
		return Command::rsFailure_InvalidHeaderCRC;

	if (VerifyCRCPacketData_v3(packet) == false)
		return Command::rsFailure_InvalidDataCRC;

	return Command::rsSuccess;
}

Command::ResponseStatus ParseClientServerCommunicationStatus(std::basic_istringstream<boost::uint8_t>& data,
  const util::NetworkOrder& network)
{
	boost::uint8_t commStatus = util::binary_read<boost::uint8_t>(data, network);
	switch (commStatus)
	{
		case 0:
		return Command::rsSuccess;
		/* commented by Cristian.Guef
		case 1:
		return Command::rsFailure_GatewayTimeout;
		case 2:
		return Command::rsFailure_GatewayInvalidContract;
		*/

		//added by Cristian.Guef
		case 1:
		return Command::rsFailure_NoUnBufferedReq;
		case 2:
		return Command::rsFailure_InvalidBufferedReq;
		case 3:
		return Command::rsFailure_GatewayContractExpired;
		case 4:
		return Command::rsFailure_GatewayUnknown;

	}
	return Command::rsFailure_GatewayInvalidFailureCode;
}

/**
 * Parse comm status & sfc code if exists.
 */
Command::ResponseStatus ParseClientServerStatus(std::basic_istringstream<boost::uint8_t>& data,
		const util::NetworkOrder& network)
{
	boost::uint8_t commStatus = util::binary_read<boost::uint8_t>(data, network);
	switch (commStatus)
	{
		case 0:
		{
			//added by Cristian.Guef
			
			util::binary_read<boost::uint32_t>(data, network); // skip txSeconds
			util::binary_read<boost::uint32_t>(data, network); // skip txUSeconds
			//NO SM IMPLEMENTATION YET */
			util::binary_read<boost::uint8_t>(data, network); //reqType
			util::binary_read<boost::uint16_t>(data, network); //ObjID

			boost::uint16_t sfcCode = util::binary_read<boost::uint8_t>(data, network);
			if(sfcCode == 0xFF)
			{
				boost::uint8_t sfcCodeExtended = util::binary_read<boost::uint8_t>(data, network);
				return (Command::ResponseStatus)(Command::rsFailure_ExtendedDeviceError - sfcCodeExtended);
			}
			else
			return (sfcCode == 0) ? Command::rsSuccess : (Command::ResponseStatus) (Command::rsFailure_DeviceError - sfcCode);
		}
		/* commented by Cristian.Guef
		case 1:
		return Command::rsFailure_GatewayTimeout;

		case 2:
		return Command::rsFailure_GatewayInvalidContract;
		*/

		//added by Cristian.Guef
		case 1:
		return Command::rsFailure_NoUnBufferedReq;
		case 2:
		return Command::rsFailure_InvalidBufferedReq;
		case 3:
		return Command::rsFailure_GatewayContractExpired;

	}
	return Command::rsFailure_GatewayUnknown;
}

Command::ResponseStatus ParseClientServerStatus(unsigned char *pData, int &offset)
{
	//no need
	//boost::uint8_t commStatus = util::binary_read<boost::uint8_t>(data, network);
	//added
	boost::uint8_t commStatus = pData[offset++];

	switch (commStatus)
	{
		case 0:
		{
			//added by Cristian.Guef
			//no need
			//util::binary_read<boost::uint32_t>(data, network); // skip txSeconds
			//util::binary_read<boost::uint32_t>(data, network); // skip txUSeconds
			//util::binary_read<boost::uint8_t>(data, network); //reqType
			//util::binary_read<boost::uint16_t>(data, network); //ObjID
			offset += (sizeof(unsigned long) + sizeof(unsigned long) + sizeof(unsigned char) + sizeof(unsigned short)); 

			//no need
			//boost::uint16_t sfcCode = util::binary_read<boost::uint8_t>(data, network);
			//added
			boost::uint16_t sfcCode = pData[offset++];

			//no need
			//if(sfcCode == 0xFF)
			//{
			//	boost::uint8_t sfcCodeExtended = util::binary_read<boost::uint8_t>(data, network);
			//	return (Command::ResponseStatus)(Command::rsFailure_ExtendedDeviceError - sfcCodeExtended);
			//}
			//else

			return (sfcCode == 0) ? Command::rsSuccess : (Command::ResponseStatus) (Command::rsFailure_DeviceError - sfcCode);
		}
		/* commented by Cristian.Guef
		case 1:
		return Command::rsFailure_GatewayTimeout;

		case 2:
		return Command::rsFailure_GatewayInvalidContract;
		*/

		//added by Cristian.Guef
		case 1:
		return Command::rsFailure_NoUnBufferedReq;
		case 2:
		return Command::rsFailure_InvalidBufferedReq;
		case 3:
		return Command::rsFailure_GatewayContractExpired;

	}
	return Command::rsFailure_GatewayUnknown;
}

void GServiceUnserializer::Unserialize(AbstractGService& response_, const gateway::GeneralPacket& packet_)
{
	packet = &packet_;

	try
	{
		LOG_DEBUG("Unserialize: try to unserialize Packet=" << packet_.ToString() << " with Response="
				<< response_.ToString());

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

#define TOPO_CHECK_POINT_1	 (sizeof(unsigned short)/*noOfDevices*/ + sizeof(unsigned short)/*noOfBackbones*/ + sizeof(unsigned long)/*CRC*/)
#define TOPO_CHECK_POINT_2	 (IPv6::SIZE + sizeof(unsigned short)/*noOfNeighbours*/ + sizeof(unsigned short)/*noOfGraphs*/)
#define TOPO_CHECK_POINT_3	 (sizeof(unsigned short)/*graphID*/ + sizeof(unsigned short)/*noOfGraphMembers*/)
#define BACKBONE_UNIT_SIZE	 (IPv6::SIZE + sizeof(unsigned short)/*subnetID*/)
#define TOPO_NEIGHBOUR_UNIT_SIZE	 (IPv6::SIZE + sizeof(unsigned char)/*clockSource*/)

  void GServiceUnserializer::Visit(GTopologyReport& topologyReport)
  {
	  /*validate with	C# code that works
	   */
	  if (packet->serviceType != gateway::GeneralPacket::GTopologyReportConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=TopologyReportConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  topologyReport.Status = st;
		  return;
	  }

	  //no need
	  //std::basic_istringstream<boost::uint8_t> data(packet->data);
	  //const util::NetworkOrder& network = util::NetworkOrder::Instance();
	  //added
	  unsigned char *pData = (unsigned char*)packet->data.c_str();
	  int offset = 0;

	  //no need
	  //boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);
	  //added
	  boost::uint8_t status = pData[offset++]; 

	  switch (status)
	  {
		  case 0: topologyReport.Status = Command::rsSuccess; break;

		  case 1:
				topologyReport.Status = Command::rsFailure_GatewayCommunication; break;
			  
		  default:
		  topologyReport.Status = Command::rsFailure_GatewayInvalidFailureCode; break;
	  }

	  if (topologyReport.Status != Command::rsSuccess)
	  {
		  return;
	  }

	  //added
	  if (packet->data.size() - offset < (int)TOPO_CHECK_POINT_1)
		  THROW_EXCEPTION0(util::NotEnoughBytesException);
		  
	  //no need
	  //int totalDevices = util::binary_read<boost::int16_t>(data, network);
	  //added
	  unsigned short totalDevices = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);

	  //no need
	  //int totalBBRs = util::binary_read<boost::int16_t>(data, network);
	  //added
	  unsigned short totalBBRs = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);
	  

	  LOG_INFO("topo_rep_unser -> total devices = " <<  (boost::uint32_t)totalDevices
						<< "and total BBRs = " <<  (boost::uint32_t)totalBBRs);
	  LOG_DEBUG("topo_rep_unser--> begin --------");

	  //added
	  if ((int)packet->data.size() - offset < (int)TOPO_CHECK_POINT_2)
		THROW_EXCEPTION0(util::NotEnoughBytesException);

	  topologyReport.DevicesList.reserve(totalDevices);
	  for (int indexDevice = 0; indexDevice < totalDevices; indexDevice++)
	  {
		  topologyReport.DevicesList.push_back(GTopologyReport::Device());
		  GTopologyReport::Device& deviceAdded = *topologyReport.DevicesList.rbegin();
		  {
			  //no need
			  //boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
			  //deviceAdded.DeviceIP = IPv6(address);
			  //added
			  deviceAdded.DeviceIP = IPv6(pData + offset);
			  offset += IPv6::SIZE;
		  }
	  	
		  /*Neighbours*/
		  //no need
		  //int totalNeighbours = util::binary_read<boost::uint16_t>(data, network);
		  //added
		  unsigned short totalNeighbours = htons(*((unsigned short*)(pData + offset)));
		  offset += sizeof(unsigned short);

		  //added by Cristian.Guef -"Graphs"
		  //no need
		  //int totalGraphs = util::binary_read<boost::uint16_t>(data, network);
		  //added
		  unsigned short totalGraphs = htons(*((unsigned short*)(pData + offset)));
		  offset += sizeof(unsigned short);
		  

		  LOG_DEBUG("topo_rep_unser - dev = " << (boost::uint32_t)indexDevice << 
		  	 					" with ip = " << deviceAdded.DeviceIP.ToString() <<
		  						" and totalNeighbours = " << (boost::uint32_t)totalNeighbours);

		  //added
		  if ((int)packet->data.size() - offset < (int)(totalNeighbours*TOPO_NEIGHBOUR_UNIT_SIZE))
			THROW_EXCEPTION0(util::NotEnoughBytesException);

		  //deviceAdded.NeighborsList.reserve(totalNeighbours);
		  for (int indexNeighbour = 0; indexNeighbour < totalNeighbours; indexNeighbour++)
		  {
			  //deviceAdded.NeighborsList.push_back(GTopologyReport::Device::Neighbour());
			  //GTopologyReport::Device::Neighbour& neighbourAdded = *deviceAdded.NeighborsList.rbegin();
			  GTopologyReport::Device::Neighbour neighbourAdded;
			  {
				  //no need
				  //boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
				  //neighbourAdded.DeviceIP = IPv6(address);
				  //added
				  neighbourAdded.DeviceIP = IPv6(pData + offset);
				  offset += IPv6::SIZE;
			  }
			  	
			  //no need
			  //neighbourAdded.ClockSource = util::binary_read<boost::uint8_t>(data, network);
			  //added
			  neighbourAdded.ClockSource = pData[offset++];
			   	
			   LOG_DEBUG("topo_rep_unser \t neigbour dev = " << 
			   					(boost::uint32_t)indexNeighbour <<
			   					" with ip = " <<  neighbourAdded.DeviceIP.ToString() <<
								" with clockSource = " <<  (boost::uint32_t)neighbourAdded.ClockSource);

			   if (deviceAdded.NeighborsList.insert(neighbourAdded).second == false)
			   {
				   LOG_WARN("duplicated neighbourIP found = " << neighbourAdded.DeviceIP.ToString());
			   }

		  } /* for int indexNeighbour*/

		  LOG_DEBUG("topo_rep_unser - dev = " << (boost::uint32_t)indexDevice <<
		  				" with totalgraph = "<<  (boost::uint32_t)totalGraphs);
		 

		  deviceAdded.GraphsList.reserve(totalGraphs);
		  for (int indexGraph = 0; indexGraph < totalGraphs; indexGraph++)
		  {

			  //added
			  if ((int)packet->data.size() - offset < (int)TOPO_CHECK_POINT_3)
				THROW_EXCEPTION0(util::NotEnoughBytesException);

			  deviceAdded.GraphsList.push_back(GTopologyReport::Device::Graph());
			  GTopologyReport::Device::Graph& graphAdded = *deviceAdded.GraphsList.rbegin();
			  
			  //no need
			  //graphAdded.GraphID = util::binary_read<boost::uint16_t>(data, network);
			  //added
			  graphAdded.GraphID = htons(*((unsigned short*)(pData + offset)));
			  offset += sizeof(unsigned short);

			  //no need
		  	  //int graphMembers = util::binary_read<boost::uint16_t>(data, network);
			  //added
			  unsigned short graphMembers = htons(*((unsigned short*)(pData + offset)));
			  offset += sizeof(unsigned short);

			  if ((int)packet->data.size() - offset < (int)graphMembers*IPv6::SIZE)
				THROW_EXCEPTION0(util::NotEnoughBytesException);

			  graphAdded.infoList.reserve(graphMembers);
		  	  for (int indexMember = 0; indexMember < graphMembers; indexMember++)
		  	  {
				   //no need
				  //boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
				  //graphAdded.infoList.push_back(GTopologyReport::Device::Graph::Info (IPv6(address)));
				  //added
				  graphAdded.infoList.push_back(GTopologyReport::Device::Graph::Info (IPv6(pData + offset)));
				  offset += IPv6::SIZE;
				 
				  LOG_DEBUG("topo_rep_unser \t graph_id = " << (boost::uint32_t)graphAdded.GraphID  <<
		  	   	   		" , indexGraph = " << (boost::uint32_t)indexGraph <<
		  	   	   		", indexMember = " << (boost::uint32_t)indexMember <<
						" with ip-ul = " << graphAdded.infoList[indexMember].neighbour.ToString());
		  	  }
		  }
	  }

	  //added
	  if ((int)packet->data.size() - offset < (int)(totalBBRs*BACKBONE_UNIT_SIZE))
		THROW_EXCEPTION0(util::NotEnoughBytesException);

	  //added by Cristian.Guef
	  topologyReport.m_BBRsList.resize(totalBBRs);
	  for(int i = 0; i < totalBBRs; i++)
	  {
		  //no need
		  //boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
		  //topologyReport.m_BBRsList[i].address = IPv6(address);
		  //added
		  topologyReport.m_BBRsList[i].address = IPv6(pData + offset);
		  offset += IPv6::SIZE;

		  //no need
		  //topologyReport.m_BBRsList[i].subnetID = util::binary_read<boost::uint16_t>(data, network);
		  //added
		  topologyReport.m_BBRsList[i].subnetID = htons(*((unsigned short*)(pData + offset)));
		  offset += sizeof(unsigned short);
	  }
	

	  LOG_DEBUG("topo_rep_unser--> end --------");

	  //sort (topology might come in unsortedly)
	  //SortTopologyListByIP(topologyReport);

  }

  //added by Cristian.Guef
  bool MySortPredicate(const GTopologyReport::Device& d1, const GTopologyReport::Device& d2)
  {
	  return d1.DeviceIP < d2.DeviceIP;
  }
  void GServiceUnserializer::SortTopologyListByIP(GTopologyReport& topologyReport)
  {
	  // Sort the vector using predicate and std::sort
	  std::sort(topologyReport.DevicesList.begin(), topologyReport.DevicesList.end(), MySortPredicate);
  }

//added
#define DEVLIST_CHECK_POINT_1	(sizeof(unsigned short)/*noOfDevices*/ + sizeof(unsigned long)/*CRC*/)
#define DEVLIST_CHECK_POINT_2	(IPv6::SIZE + sizeof(unsigned short)/*deviceType*/ + MAC::SIZE + sizeof(unsigned char)/*powerStatus*/ + 5*sizeof(unsigned char)/*visible_strings_size*/)

  //added by Cristian.Guef
  void GServiceUnserializer::Visit(GDeviceListReport& deviceListReport)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GDeviceListReportConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=TopologyReportConfirm!");
	  }

	   Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  deviceListReport.Status = st;
		  return;
	  }

	  //no need
	  //std::basic_istringstream<boost::uint8_t> data(packet->data);
	  //const util::NetworkOrder& network = util::NetworkOrder::Instance();
	  //added
	  unsigned char *pData = (unsigned char*)packet->data.c_str();
	  int offset = 0;


	  //no need
	  //boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);
	  //added
	  boost::uint8_t status = pData[offset++];

	  switch (status)
	  {
		  case 0: deviceListReport.Status = Command::rsSuccess; break;

		  case 1:
		 	  deviceListReport.Status = Command::rsFailure_GatewayCommunication; break;
		  default:
		  deviceListReport.Status = Command::rsFailure_GatewayInvalidFailureCode; break;
	  }

	  if (deviceListReport.Status != Command::rsSuccess)
	  {
		  return;
	  }

	  //no need
	  //int totalDevices = util::binary_read<boost::int16_t>(data, network);
	  //added
	  if ((int)packet->data.size() - offset < (int)DEVLIST_CHECK_POINT_1)
		THROW_EXCEPTION0(util::NotEnoughBytesException);
	  unsigned short totalDevices = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);

	  LOG_INFO("dev_list_rep_unser -> total devices =  " <<  (boost::uint32_t)totalDevices);
	  
	  LOG_DEBUG("dev_list_rep_unser--> begin --------");
	  for (int indexDevice = 0; indexDevice < totalDevices; indexDevice++)
	  {

		  //added
		  if ((int)packet->data.size() - offset < (int)DEVLIST_CHECK_POINT_2)
			THROW_EXCEPTION0(util::NotEnoughBytesException);

		  boost::uint8_t address[IPv6::SIZE]; 
		  //no need
		  //util::binary_read_bytes(data, address, sizeof(address));
		  //added
		  memcpy(address, pData + offset, sizeof(address));
		  offset += IPv6::SIZE;

		  GDeviceListReport::Device dev;

		  //no need
		  //dev.DeviceType = util::binary_read<boost::uint16_t>(data, network);
		  //added
		  dev.DeviceType = htons(*((unsigned short*)(pData + offset)));
		  offset += sizeof(unsigned short);

		  {
			  //no need
			  //boost::uint8_t address2[MAC::SIZE]; util::binary_read_bytes(data, address2, sizeof(address2));
			  //dev.DeviceMAC = MAC(address2);
			  //added
			  dev.DeviceMAC = MAC(pData + offset);
			  offset += MAC::SIZE;
		  }
		 
		  //power supply
		  //no need
		  //dev.PowerSupplyStatus = util::binary_read<boost::uint8_t>(data, network);
		  //added
		  dev.PowerSupplyStatus = pData[offset++];
		  
		  //joinStatus
		  dev.DeviceStatus = pData[offset++];
		  
			LOG_DEBUG("dev_list_rep_unser - dev = " << (boost::uint32_t)indexDevice 
				<< " with ip = " << (IPv6(address)).ToString() 
				<< " and mac = " << dev.DeviceMAC.ToString()
				<< " and type = " << (boost::uint32_t)dev.DeviceType
				<< " and status = " << (boost::uint32_t)dev.DeviceStatus
				<< " and powerStatus = " << (boost::uint32_t)dev.PowerSupplyStatus);

			//added
			int len;
			char temp[20];
		  //manufacture
			{
			  /* no need
			  std::ostringstream dataBytes; 
			  dataBytes << std::hex << std::uppercase << std::setfill('0');

			  int strLength = util::binary_read<boost::uint8_t>(data, network);
			  for (int str_i = 0; str_i < strLength; str_i++)
			  {
				  dataBytes << std::setw(2) << (int)(util::binary_read<boost::uint8_t>(data, network));
				  
			  }
			  dev.DeviceManufacture = dataBytes.str();
			  */
			  //added
			  len = pData[offset++];
			  if ((int)packet->data.size() - offset < len)
				THROW_EXCEPTION0(util::NotEnoughBytesException);
			  dev.DeviceManufacture.resize(2*len);
			  int i = 0;
			  while(len--)
			  {
			  	sprintf(temp, "%02X", pData[offset++]);
			  	dev.DeviceManufacture[i++] = temp[0];
			  	dev.DeviceManufacture[i++] = temp[1];
			  }
			}

		  //model
		  {
			  /* no need
			  std::ostringstream dataBytes; 
			  dataBytes << std::hex << std::uppercase << std::setfill('0');

			  int strLength = util::binary_read<boost::uint8_t>(data, network);
			  for (int str_i = 0; str_i < strLength; str_i++)
			  {
				  dataBytes << std::setw(2) << (int)(util::binary_read<boost::uint8_t>(data, network));
				  
			  }
			  dev.DeviceModel = dataBytes.str();
			  */
			  //added
			  len = pData[offset++];
			  if ((int)packet->data.size() - offset < len)
				THROW_EXCEPTION0(util::NotEnoughBytesException);
			  dev.DeviceModel.resize(2*len);
			  int i = 0;
			  while(len--)
			  {
			  	sprintf(temp, "%02X", pData[offset++]);
			  	dev.DeviceModel[i++] = temp[0];
			  	dev.DeviceModel[i++] = temp[1];
			  }
			}


		  //revision
		  {
			  /* no need
			  std::ostringstream dataBytes; 
			  dataBytes << std::hex << std::uppercase << std::setfill('0');

			  int strLength = util::binary_read<boost::uint8_t>(data, network);
			  for (int str_i = 0; str_i < strLength; str_i++)
			  {
				  dataBytes << std::setw(2) << (int)(util::binary_read<boost::uint8_t>(data, network));
				  
			  }
			  dev.DeviceRevision = dataBytes.str();
			  */
			  //added
			  len = pData[offset++];
			  if ((int)packet->data.size() - offset < len)
				THROW_EXCEPTION0(util::NotEnoughBytesException);
			  dev.DeviceRevision.resize(2*len);
			  int i = 0;
			  while(len--)
			  {
			  	sprintf(temp, "%02X", pData[offset++]);
			  	dev.DeviceRevision[i++] = temp[0];
			  	dev.DeviceRevision[i++] = temp[1];
			  }
			}
		
		  //tag
		  {
			  /* no need
			  std::ostringstream dataBytes; 
			  dataBytes << std::hex << std::uppercase << std::setfill('0');

			  int strLength = util::binary_read<boost::uint8_t>(data, network);
			  for (int str_i = 0; str_i < strLength; str_i++)
			  {
				  dataBytes << std::setw(2) << (int)(util::binary_read<boost::uint8_t>(data, network));
				  
			  }
			  dev.DeviceTag = dataBytes.str();
			  */
			  //added
			  len = pData[offset++];
			  if ((int)packet->data.size() - offset < len)
				THROW_EXCEPTION0(util::NotEnoughBytesException);
			  dev.DeviceTag.resize(2*len);
			  int i = 0;
			  while(len--)
			  {
			  	sprintf(temp, "%02X", pData[offset++]);
			  	dev.DeviceTag[i++] = temp[0];
			  	dev.DeviceTag[i++] = temp[1];
			  }
			}
		  
		  //serialNo
		  {
			  len = pData[offset++];
			  if ((int)packet->data.size() - offset < len)
				THROW_EXCEPTION0(util::NotEnoughBytesException);
			  dev.DeviceSerialNo.resize(2*len);
			  int i = 0;
			  while(len--)
			  {
			  	sprintf(temp, "%02X", pData[offset++]);
			  	dev.DeviceSerialNo[i++] = temp[0];
			  	dev.DeviceSerialNo[i++] = temp[1];
			  }
		  }
			
		  LOG_DEBUG("dev_list_rep_unser - dev = " << (boost::uint32_t)indexDevice <<
		  		" with manufacture = " << dev.DeviceManufacture <<
		  		" and model = " << dev.DeviceModel << 
		  		" and revision = " << dev.DeviceRevision <<
				" and tag = " << dev.DeviceTag); 
		  
		  deviceListReport.DevicesMap.insert(GDeviceListReport::DevicesMapT::value_type(IPv6(address), dev));
	  }
	  
	  LOG_DEBUG("dev_list_rep_unser--> end --------");
  }


#define TAI_SIZE				(sizeof(unsigned long)/*sec*/ + sizeof(unsigned short)/*usec*/)
#define NETWORKHEALTH_REP_SIZE	(sizeof(unsigned long)/*netID*/ + sizeof(unsigned char)/*netType*/ + \
								   sizeof(unsigned short)/*deviceCount*/ + TAI_SIZE/*startDate*/ + TAI_SIZE/*currentDate*/ + \
								   sizeof(unsigned long)/*DPDUsent*/ + sizeof(unsigned long)/*DPDULost*/ + \
								   sizeof(unsigned char)/*latency*/ + sizeof(unsigned char)/*path_reliability*/ + \
								   sizeof(unsigned char)/*data_reliability*/ + sizeof(unsigned long)/*join_count*/)
#define NET_REP_CHECK_POINT_1	(NETWORKHEALTH_REP_SIZE + sizeof(unsigned long)/*crc*/)
#define NET_DEV_REP_SIZE		(IPv6::SIZE + TAI_SIZE/*startDate*/ + TAI_SIZE/*currentDate*/ + \
									sizeof(unsigned long)/*DPDUsent*/ + sizeof(unsigned long)/*DPDULost*/ + \
									sizeof(unsigned char)/*latency*/ + sizeof(unsigned char)/*path_reliability*/ + \
								   sizeof(unsigned char)/*data_reliability*/ + sizeof(unsigned long)/*join_count*/)


  //added by Cristian.Guef
  void GServiceUnserializer::Visit(GNetworkHealthReport& networkHealthReport)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GNetworkHealthReportConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=GNetworkHealthReportConfirm!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  networkHealthReport.Status = st;
		  return;
	  }

	 //no need
	  //std::basic_istringstream<boost::uint8_t> data(packet->data);
	  //const util::NetworkOrder& network = util::NetworkOrder::Instance();
	  //added
	  unsigned char *pData = (unsigned char*)packet->data.c_str();
	  int offset = 0;

	  //no need
	  //boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);
	  //added
	  boost::uint8_t status = pData[offset++];

	  switch (status)
	  {
	   case 0: 
		   networkHealthReport.Status = Command::rsSuccess; 
		   break;
		case 1:
		  	networkHealthReport.Status = Command::rsFailure_GatewayCommunication; 
			break;
		default:
		    networkHealthReport.Status = Command::rsFailure_GatewayInvalidFailureCode; 
			break;
	  }

	  LOG_INFO("net_rep_unser -> status = " << (boost::int32_t)networkHealthReport.Status);

	  if (networkHealthReport.Status != Command::rsSuccess)
		  return;


	  if ((int)packet->data.size() - offset < (int)NET_REP_CHECK_POINT_1)
		THROW_EXCEPTION0(util::NotEnoughBytesException);

	  //no need
	  //boost::uint16_t deviceCount = util::binary_read<boost::uint16_t>(data, network);
	  //added
	  boost::uint16_t deviceCount = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);

	  LOG_INFO("net_rep_unser -> total devices =  " <<  (boost::uint32_t)deviceCount);

	  //network_health
	  //no need
	  //networkHealthReport.m_NetworkHealth.networkID = util::binary_read<boost::uint32_t>(data, network);
	  //added
      networkHealthReport.m_NetworkHealth.networkID = htonl(*((unsigned long*)(pData + offset)));
	  offset += sizeof(unsigned long);

	  //no need
	  //networkHealthReport.m_NetworkHealth.networkType = util::binary_read<boost::uint8_t>(data, network);
	  //added
	  networkHealthReport.m_NetworkHealth.networkType = pData[offset++];

	  //no need
	  //networkHealthReport.m_NetworkHealth.deviceCount = util::binary_read<boost::uint16_t>(data, network);
	  //added
	  networkHealthReport.m_NetworkHealth.deviceCount = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);

	  //no need
	  //boost::uint32_t netStartSeconds = util::binary_read<boost::uint32_t>(data, network);
	  //added
	  boost::uint32_t netStartSeconds = htonl(*((unsigned long*)(pData + offset)));
	  offset += sizeof(unsigned long);

	  //no need
	  //boost::uint16_t netStartFractionOfSeconds = util::binary_read<boost::uint16_t>(data, network);
	  //added
	  boost::uint16_t netStartFractionOfSeconds = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);

	  networkHealthReport.m_NetworkHealth.startDate = gettime(netStartSeconds);
	  
	  //no need
	  //boost::uint32_t netCurrSeconds = util::binary_read<boost::uint32_t>(data, network);
	  //added
	  boost::uint32_t netCurrSeconds = htonl(*((unsigned long*)(pData + offset)));
	  offset += sizeof(unsigned long);

	  //no need
	  //boost::uint16_t netCurrFractionOfSeconds = util::binary_read<boost::uint16_t>(data, network);
	  //added
	  boost::uint16_t netCurrFractionOfSeconds = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);
	  
	  networkHealthReport.m_NetworkHealth.currentDate = gettime(netCurrSeconds);
		
	  NetworkHealthTime = networkHealthReport.m_NetworkHealth.currentDate;

	  //no need
	  //networkHealthReport.m_NetworkHealth.DPDUsSent = util::binary_read<boost::uint32_t>(data, network);
	  //added
	  networkHealthReport.m_NetworkHealth.DPDUsSent = htonl(*((unsigned long*)(pData + offset)));
	  offset += sizeof(unsigned long);

	  //no need
	  //networkHealthReport.m_NetworkHealth.DPDUsLost = util::binary_read<boost::uint32_t>(data, network);
	  //data
	  networkHealthReport.m_NetworkHealth.DPDUsLost = htonl(*((unsigned long*)(pData + offset)));
	  offset += sizeof(unsigned long);

	  //no need
	  //networkHealthReport.m_NetworkHealth.GPDULatency = util::binary_read<boost::uint8_t>(data, network);
	  //networkHealthReport.m_NetworkHealth.GPDUPathReliability = util::binary_read<boost::uint8_t>(data, network);
	  // networkHealthReport.m_NetworkHealth.GPDUDataReliability = util::binary_read<boost::uint8_t>(data, network);
	  //added
	  networkHealthReport.m_NetworkHealth.GPDULatency = pData[offset++];
	  networkHealthReport.m_NetworkHealth.GPDUPathReliability = pData[offset++];
	  networkHealthReport.m_NetworkHealth.GPDUDataReliability = pData[offset++];
	  
	  //no need
	  //networkHealthReport.m_NetworkHealth.joinCount = util::binary_read<boost::uint32_t>(data, network);
	  //added
	  networkHealthReport.m_NetworkHealth.joinCount = htonl(*((unsigned long*)(pData + offset)));
	  offset += sizeof(unsigned long);

	  LOG_INFO("net_rep_unser - with inf:" 
					<< " networkID = " << networkHealthReport.m_NetworkHealth.networkID
					<< " networkType = " << (boost::uint32_t)networkHealthReport.m_NetworkHealth.networkType
					<< " deviceCount = " << networkHealthReport.m_NetworkHealth.deviceCount
					<< " startDate.Seconds = " << netStartSeconds
					<< " startDate.FractionOfSeconds = " << netStartFractionOfSeconds
					<< " currentDate.Seconds = " << netCurrSeconds
					<< " currentDate.FractionOfSeconds = " << netCurrFractionOfSeconds
					<< " DPDUsSent = " << networkHealthReport.m_NetworkHealth.DPDUsSent
					<< " DPDUsLost = " << networkHealthReport.m_NetworkHealth.DPDUsLost
					<< " GPDULatency = " << (boost::uint32_t)networkHealthReport.m_NetworkHealth.GPDULatency
					<< " GPDUPathReliability = " << (boost::uint32_t)networkHealthReport.m_NetworkHealth.GPDUPathReliability
					<< " GPDUDataReliability = " << (boost::uint32_t)networkHealthReport.m_NetworkHealth.GPDUDataReliability
					<< " joinCount = " << networkHealthReport.m_NetworkHealth.joinCount);
	 	  
	 LOG_INFO("net_rep_unser --> begin --------");
	 

	 if ((int)packet->data.size() - offset < (int)(networkHealthReport.m_NetworkHealth.deviceCount*NET_DEV_REP_SIZE))
				THROW_EXCEPTION0(util::NotEnoughBytesException);

	  for (int indexDevice = 0; indexDevice < deviceCount; indexDevice++)
	  {

		  boost::uint8_t address[IPv6::SIZE]; 
		  
		  //no need
		  //util::binary_read_bytes(data, address, sizeof(address));
		  //added
		  memcpy(address, pData + offset, sizeof(address));
		  offset += sizeof(address);

		  		 
		  GNetworkHealthReport::NetDeviceHealth netDevHealth;
		  
		  //no need
		  //boost::uint32_t devStartSeconds = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  boost::uint32_t devStartSeconds = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  //no need
		  //boost::uint16_t devStartFractionOfSeconds = util::binary_read<boost::uint16_t>(data, network);
		  //added
		  boost::uint16_t devStartFractionOfSeconds = htons(*((unsigned short*)(pData + offset)));
		  offset += sizeof(unsigned short);
		  
		  netDevHealth.startDate = gettime(devStartSeconds);
		  
		  //no need
		  //boost::uint32_t devCurrSeconds = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  boost::uint32_t devCurrSeconds = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  //no need
		  //boost::uint16_t devCurrFractionOfSeconds = util::binary_read<boost::uint16_t>(data, network);
		  //added
		  boost::uint16_t devCurrFractionOfSeconds = htons(*((unsigned short*)(pData + offset)));
		  offset += sizeof(unsigned short);
		  
		  netDevHealth.currentDate = gettime(devCurrSeconds);

		  //no need
		  //netDevHealth.DPDUsSent = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  netDevHealth.DPDUsSent = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  //no need
		  //netDevHealth.DPDUsLost = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  netDevHealth.DPDUsLost = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  //no need
		  //netDevHealth.GPDULatency = util::binary_read<boost::uint8_t>(data, network);
		  //netDevHealth.GPDUPathReliability = util::binary_read<boost::uint8_t>(data, network);
		  //netDevHealth.GPDUDataReliability = util::binary_read<boost::uint8_t>(data, network);
		  //added
		  netDevHealth.GPDULatency = pData[offset++];
		  netDevHealth.GPDUPathReliability = pData[offset++];
		  netDevHealth.GPDUDataReliability = pData[offset++];
		  
		  //no need
		  //netDevHealth.joinCount = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  netDevHealth.joinCount = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);
		  
		  networkHealthReport.DevicesMap.insert(GNetworkHealthReport::DevicesMapT::value_type(IPv6(address), netDevHealth));

		  LOG_INFO("net_rep_unser - dev = " << indexDevice << "with inf:" 
					<< " startDate.Seconds = " << devStartSeconds
					<< " startDate.FractionOfSeconds = " << devStartFractionOfSeconds
					<< " currentDate.Seconds = " << devCurrSeconds
					<< " currentDate.FractionOfSeconds = " << devCurrFractionOfSeconds
					<< " DPDUsSent = " << netDevHealth.DPDUsSent
					<< " DPDUsLost = " << netDevHealth.DPDUsLost
					<< " GPDULatency = " << (boost::uint32_t)netDevHealth.GPDULatency
					<< " GPDUPathReliability = " << (boost::uint32_t)netDevHealth.GPDUPathReliability
					<< " GPDUDataReliability = " << (boost::uint32_t)netDevHealth.GPDUDataReliability
					<< " joinCount = " << netDevHealth.joinCount);
	  }

	  LOG_INFO("net_rep_unser --> end --------");
  }


#define CHANNEL_SIZE			(sizeof(unsigned char)/*channelNo*/ + sizeof(unsigned char)/*channelStatus*/)
#define SCHED_CHECK_POINT_1		(sizeof(unsigned char)/*channelsNo*/ + sizeof(unsigned short)/*devicesNo*/ + sizeof(unsigned int)/*CRC*/)
#define SCHED_CHECK_POINT_2		(IPv6::SIZE + sizeof(unsigned short)/*superframesNo*/)
#define SCHED_CHECK_POINT_3		(sizeof(unsigned short)/*superFrameID*/ + sizeof(unsigned short)/*timeSlotsNo*/ + \
									sizeof(unsigned int)/*startTime*/ + sizeof(unsigned short)/*linksNo*/)
#define LINK_UNIT_SIZE			(IPv6::SIZE + sizeof(unsigned short)/*slotIndex*/ + sizeof(unsigned short)/*linkPeriod*/ + sizeof(unsigned short)/*slotLength*/ + \
								    sizeof(unsigned char)/*channelNo*/ + sizeof(unsigned char)/*direction*/ + sizeof(unsigned char)/*linktype*/)

  //added by Cristian.Guef
void GServiceUnserializer::Visit(GScheduleReport& scheduleReport)
{
	if (packet->serviceType != gateway::GeneralPacket::GScheduleReportConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ScheduleReportConfirm!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  scheduleReport.Status = st;
		  return;
	  }

	  //no need
	  //std::basic_istringstream<boost::uint8_t> data(packet->data);
	  //const util::NetworkOrder& network = util::NetworkOrder::Instance();
	  //added
	  unsigned char *pData = (unsigned char*)packet->data.c_str();
	  int offset = 0;

	  //no need
	  //boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);
	  //added
	  boost::uint8_t status = pData[offset++];

	  switch (status)
	  {
		  case 0: 
			  scheduleReport.Status = Command::rsSuccess; 
			  break;
		  case 1:
		  	  scheduleReport.Status = Command::rsFailure_GatewayCommunication; 
			  break;
		  default:
			  scheduleReport.Status = Command::rsFailure_GatewayInvalidFailureCode; 
			  break;
	  }

	  LOG_INFO("sched_rep_unser -> status = " << (boost::int32_t)scheduleReport.Status);

	  if (scheduleReport.Status != Command::rsSuccess)
		  return;
	  
	  if ((int)packet->data.size() - offset < (int)SCHED_CHECK_POINT_1)
				THROW_EXCEPTION0(util::NotEnoughBytesException);

	  //read data
	  //no need
	  //boost::uint8_t channelCount = util::binary_read<boost::uint8_t>(data, network);
	  //added
	  boost::uint8_t channelCount = pData[offset++];

	  //no need
	  //boost::uint16_t deviceCount = util::binary_read<boost::uint16_t>(data, network);
	  //added
	  boost::uint16_t deviceCount = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);

	//channel list
	 LOG_INFO("sched_rep_unser - channelCount = " << (boost::uint32_t)channelCount);
	 LOG_INFO("sched_rep_unser -- channel_list --> begin --------");

	 if ((int)packet->data.size() - offset < (int)(channelCount*CHANNEL_SIZE))
				THROW_EXCEPTION0(util::NotEnoughBytesException);

	 for (int j = 0; j < channelCount; j ++)
	  {
			    scheduleReport.channelList.push_back(GScheduleReport::Channel());
				GScheduleReport::Channel &channel = *scheduleReport.channelList.rbegin();
				
				//no need
				//channel.channelNumber = util::binary_read<boost::uint8_t>(data, network);
				//channel.channelStatus = util::binary_read<boost::uint8_t>(data, network);
				//added
				channel.channelNumber = pData[offset++];
				channel.channelStatus = pData[offset++];

				LOG_INFO("sched_rep_unser - index = " << j <<
						 " channelNumber = " << (boost::uint32_t)channel.channelNumber <<
						 " channelStatus = " << (boost::uint32_t)channel.channelStatus);
		}
	   LOG_INFO("sched_rep_unser -- channel_list --> end --------");
	   

	  //list
	  LOG_INFO("sched_rep_unser -> total devices =  " <<  (boost::uint16_t)deviceCount); 
	  LOG_INFO("sched_rep_unser --> begin --------");
	   for (int i = 0; i < deviceCount; i++)
	   {
			
		   if ((int)packet->data.size() - offset < (int)SCHED_CHECK_POINT_2)
				THROW_EXCEPTION0(util::NotEnoughBytesException);

		   scheduleReport.DeviceScheduleList.push_back(GScheduleReport::DeviceSchedule());
		   GScheduleReport::DeviceSchedule& deviceScheduleAdded = *scheduleReport.DeviceScheduleList.rbegin();
		  {
			  //no need
			  //boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
			  //deviceScheduleAdded.networkAddress = IPv6(address);
			  //added
			  deviceScheduleAdded.networkAddress = IPv6(pData + offset);
			  offset += IPv6::SIZE;
		  }

		   
		  LOG_INFO("sched_rep_unser - dev = " << (boost::uint32_t)i 
				<< " with ip = " << (deviceScheduleAdded.networkAddress).ToString());


		  //super frame list
		  //no need
		  //boost::uint16_t frameCount = util::binary_read<boost::uint16_t>(data, network);
		  //added
		  boost::uint16_t frameCount = htons(*((unsigned short*)(pData + offset)));
		  offset += sizeof(unsigned short);
		  
		  
		  LOG_INFO("sched_rep_unser - frameCount = " << (boost::uint16_t)frameCount);
		  LOG_INFO("sched_rep_unser -- frame_list --> begin --------");
		  for (int j = 0; j < frameCount; j++)
		  {
			   if ((int)packet->data.size() - offset < (int)SCHED_CHECK_POINT_3)
					THROW_EXCEPTION0(util::NotEnoughBytesException);

				deviceScheduleAdded.superframeList.push_back(GScheduleReport::Superframe());
				GScheduleReport::Superframe& frame = *deviceScheduleAdded.superframeList.rbegin();

				//no need
				//frame.superframeID = util::binary_read<boost::uint16_t>(data, network);
				//added
				frame.superframeID = htons(*((unsigned short*)(pData + offset)));
				offset += sizeof(unsigned short);

				//no need
				//frame.timeSlotsCount = util::binary_read<boost::uint16_t>(data, network);
				//added
				frame.timeSlotsCount = htons(*((unsigned short*)(pData + offset)));
				offset += sizeof(unsigned short);

				//no need
				//boost::uint32_t startTime = util::binary_read<boost::uint32_t>(data, network);
				//added
				boost::uint32_t startTime = htonl(*((unsigned long*)(pData + offset)));
				offset += sizeof(unsigned long);

				if (NetworkHealthTime == (nlib::DateTime()))
					frame.startTime = gettime(startTime);
				else
					frame.startTime = NetworkHealthTime + nlib::util::seconds(startTime);


				//no need
				//boost::uint16_t linksCount = util::binary_read<boost::uint16_t>(data, network);
				//added
				boost::uint16_t linksCount = htons(*((unsigned short*)(pData + offset)));
				offset += sizeof(unsigned short);
				
				LOG_INFO("sched_rep_unser - index = " << j <<
						 " superframeID = " << (boost::uint16_t)frame.superframeID <<
						 " timeSlotsCount = " << (boost::uint16_t)frame.timeSlotsCount <<
						" startTime = " << (boost::int32_t)startTime <<
						" linksCount = " << (boost::uint32_t)linksCount);
				
				LOG_INFO("sched_rep_unser -- link_list --> begin --------");

				if ((int)packet->data.size() - offset < (int)(linksCount*LINK_UNIT_SIZE))
					THROW_EXCEPTION0(util::NotEnoughBytesException);

				for (int k = 0; k < linksCount; k++)
				{
					frame.linkList.push_back(GScheduleReport::Link());
					GScheduleReport::Link & link = *frame.linkList.rbegin();
					
					{
					  //no need
					  //boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
					  //link.networkAddress = IPv6(address);
					  //added
					  link.networkAddress = IPv6(pData + offset);
					  offset += IPv6::SIZE;
					}

					//no need
					//link.slotIndex = util::binary_read<boost::uint16_t>(data, network);
					//added
					link.slotIndex = htons(*((unsigned short*)(pData + offset)));
					offset += sizeof(unsigned short);

					link.linkPeriod = htons(*((unsigned short*)(pData + offset)));
					offset += sizeof(unsigned short);

					//no need
					//link.slotLength = util::binary_read<boost::uint16_t>(data, network);
					//added
					link.slotLength = htons(*((unsigned short*)(pData + offset)));
					offset += sizeof(unsigned short);

					//no need
					//link.channelNumber = util::binary_read<boost::uint8_t>(data, network);
					//link.direction = util::binary_read<boost::uint8_t>(data, network);
					//link.linkType = util::binary_read<boost::uint8_t>(data, network);
					//added
					link.channelNumber = pData[offset++];
					link.direction = pData[offset++];
					link.linkType = pData[offset++];

					LOG_INFO("sched_rep_unser - index = " << (boost::uint32_t)k 
							<< " with ip = " << (link.networkAddress).ToString()
							<< " slotIndex = " << link.slotIndex
							<< " linkPeriod = " << link.linkPeriod
							<< " slotLength = " << link.slotLength
							<< " channel_no = " << (boost::uint32_t)link.channelNumber
							<< " direction = " << (boost::uint32_t)link.direction
							<< " link.linkType = " << (boost::uint32_t)link.linkType);
					
				}
				LOG_INFO("sched_rep_unser -- link_list --> end --------");
		  }
		  LOG_INFO("sched_rep_unser -- frame_list --> end --------");
	   }
	   LOG_INFO("sched_rep_unser --> end --------");
}

#define NEIGHBOUR_UNIT_SIZE		(IPv6::SIZE + sizeof(unsigned char)/*link_status*/ + sizeof(unsigned long)/*DPDUTrasmitted*/ + \
									sizeof(unsigned long)/*DPDUReceived*/ + sizeof(unsigned long)/*DPDUFailedTrasmitted*/ + \
									sizeof(unsigned long)/*DPDUFailedReceived*/ + sizeof(unsigned char)/*signalStrength*/ + \
									sizeof(unsigned char)/*signalQuality*/)
#define NEIGHBOUR_REP_CHECK_POINT	(sizeof(unsigned short)/*neighboursNo*/ + sizeof(unsigned int)/*CRC*/)

//added by Cristian.Guef
void GServiceUnserializer::Visit(GNeighbourHealthReport& neighbourHealthReport)
{
	if (packet->serviceType != gateway::GeneralPacket::GNeighbourHealthReportConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=NeighbourHealthReportConfirm!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  neighbourHealthReport.Status = st;
		  return;
	  }

	  unsigned char *pData = (unsigned char*)packet->data.c_str();
	  int offset = 0;

	  boost::uint8_t status = pData[offset++];

	  switch (status)
	  {
		  case 0: 
			  neighbourHealthReport.Status = Command::rsSuccess; 
			  break;
		  case 1:
		  	  neighbourHealthReport.Status = Command::rsFailure_GatewayCommunication; 
			  break;
		  default:
			  neighbourHealthReport.Status = Command::rsFailure_GatewayInvalidFailureCode; 
			  break;
	  }

	  if (neighbourHealthReport.Status != Command::rsSuccess)
	  {
		LOG_INFO("neighbour_health_rep_unser for dev_ip = " << neighbourHealthReport.m_forDeviceIP.ToString() <<
			"-> status = " << (boost::int32_t)neighbourHealthReport.Status);
	  	  return;
	  }

	  if ((int)packet->data.size() - offset < (int)NEIGHBOUR_REP_CHECK_POINT)
		 THROW_EXCEPTION0(util::NotEnoughBytesException);

		 MyDateTime myDT;
	  gettime(myDT);
	  neighbourHealthReport.timestamp = myDT.ReadingTime;

	  boost::uint16_t neighbourCount = htons(*((unsigned short*)(pData + offset)));
	  offset += sizeof(unsigned short);

	LOG_INFO("neighbour_health IP " << neighbourHealthReport.m_forDeviceIP.ToString()  << " neighbors = " <<  (boost::uint16_t)neighbourCount);
	
	if ((int)packet->data.size() - offset < (int)(neighbourCount*NEIGHBOUR_UNIT_SIZE))
		 THROW_EXCEPTION0(util::NotEnoughBytesException);

	   for(int i = 0; i < neighbourCount; i++)
	   {

		   boost::uint8_t address[IPv6::SIZE]; 
		   
		   memcpy(address, pData + offset, sizeof(address));
		   offset += IPv6::SIZE;
		  
		  GNeighbourHealthReport::NeighbourHealth neighbour;

		  neighbour.linkStatus = pData[offset++];

		  neighbour.DPDUsTransmitted = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  neighbour.DPDUsReceived = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  neighbour.DPDUsFailedTransmission = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  neighbour.DPDUsFailedReception = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);
		  neighbour.signalStrength = htons(*((unsigned short*)(pData + offset)));
	 	  offset += sizeof(unsigned short);
		  neighbour.signalQuality = pData[offset++];

		 LOG_DEBUG("neighbour_health dev " << IPv6(address).ToString()
				<< " linkStatus = " << (boost::uint32_t)neighbour.linkStatus
				<< " DPDUs(TX/RX) " << neighbour.DPDUsTransmitted << "/" << neighbour.DPDUsReceived
				<< " DPDUsFail(TX/RX) " << neighbour.DPDUsFailedTransmission << "/" << neighbour.DPDUsFailedReception
				<< " signalStrength = " << (boost::int32_t)neighbour.signalStrength
				<< " signalQuality = " << (boost::uint32_t)neighbour.signalQuality);
		  
		  neighbourHealthReport.NeighboursMap.insert(GNeighbourHealthReport::NeighboursMapT::value_type(IPv6(address), neighbour));
	   }
}

#define DEV_HEALTH_CHECK_POINT_1	(sizeof(unsigned short)/*devicesNo*/ + sizeof(unsigned long)/*crc*/)
#define DEV_HEALTH_UNIT_SIZE		(IPv6::SIZE + 4*sizeof(unsigned long))

void GServiceUnserializer::Visit(GDeviceHealthReport& devHealthReport)
{
	if (packet->serviceType != gateway::GeneralPacket::GDeviceHealthReportConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=DeviceHealthReportConfirm!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  devHealthReport.Status = st;
		  return;
	  }

	  //no need
	  //std::basic_istringstream<boost::uint8_t> data(packet->data);
	  //const util::NetworkOrder& network = util::NetworkOrder::Instance();
	  //added
	  unsigned char *pData = (unsigned char*)packet->data.c_str();
	  int offset = 0;

	  //no need
	  //boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);
	  //added
	  boost::uint8_t status = pData[offset++];

	  switch (status)
	  {
		  case 0: 
			  devHealthReport.Status = Command::rsSuccess; 
			  break;
		  case 1:
		  	  devHealthReport.Status = Command::rsFailure_GatewayCommunication; 
			  break;
		  default:
			  devHealthReport.Status = Command::rsFailure_GatewayInvalidFailureCode; 
			  break;
	  }

	  LOG_INFO("dev_health_rep_unser -> status = " << (boost::int32_t)devHealthReport.Status);
	  if (devHealthReport.Status != Command::rsSuccess)
		  return;

	  //added
	  if ((int)packet->data.size() - offset < (int)DEV_HEALTH_CHECK_POINT_1)
		 THROW_EXCEPTION0(util::NotEnoughBytesException);

		MyDateTime myDT;
	  gettime(myDT);
	  devHealthReport.timestamp =  myDT.ReadingTime;
	  
	  //read data
	  //no need
	  // boost::uint32_t deviceCount = util::binary_read<boost::uint16_t>(data, network);
	  //added
	   boost::uint32_t deviceCount = htons(*((unsigned short*)(pData + offset)));
		  offset += sizeof(unsigned short);

	   LOG_INFO("dev_health_rep_unser -> total devices =  " <<  (boost::uint32_t)deviceCount);
	   LOG_INFO("dev_health_rep_unser --> begin --------");

	   //added
	  if ((int)packet->data.size() - offset < (int)(deviceCount *DEV_HEALTH_UNIT_SIZE))
		 THROW_EXCEPTION0(util::NotEnoughBytesException);

	   for (unsigned int i = 0; i < deviceCount; i++)
	   {
		   devHealthReport.DeviceHealthList.push_back(GDeviceHealthReport::DeviceHealth());
		   GDeviceHealthReport::DeviceHealth & Device = *devHealthReport.DeviceHealthList.rbegin();

		  {
			  //no need
			  //boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
			  //Device.networkAddress = IPv6(address);
			  //added
			  Device.networkAddress = IPv6(pData + offset);
			  offset += IPv6::SIZE;
		  }

		  //no need
		  //Device.DPDUsTransmitted = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  Device.DPDUsTransmitted = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  //no need
		  //Device.DPDUsReceived = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  Device.DPDUsReceived = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  //no need
		  //Device.DPDUsFailedTransmission = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  Device.DPDUsFailedTransmission = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  //no need
		  //Device.DPDUsFailedReception = util::binary_read<boost::uint32_t>(data, network);
		  //added
		  Device.DPDUsFailedReception = htonl(*((unsigned long*)(pData + offset)));
		  offset += sizeof(unsigned long);

		  LOG_INFO("dev_health_rep_unser - dev = " << (boost::uint32_t)i 
				<< " with ip = " << (Device.networkAddress).ToString()
				<< " DPDUsTransmitted = " << Device.DPDUsTransmitted
				<< " DPDUsReceived = " << Device.DPDUsReceived
				<< " DPDUsFailedTransmission = " << Device.DPDUsFailedTransmission
				<< " DPDUsFailedReception = " << Device.DPDUsFailedReception);

	   }
	   LOG_INFO("dev_health_rep_unser --> end --------");
}

void GServiceUnserializer::Visit(GNetworkResourceReport& netResourceReport)
{
	if (packet->serviceType != gateway::GeneralPacket::GNetworkResourceConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=NetworkResourceReportConfirm!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  netResourceReport.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);
	  switch (status)
	  {
		  case 0: 
			  netResourceReport.Status = Command::rsSuccess; 
			  break;
		  case 1:
		  	  netResourceReport.Status = Command::rsFailure_GatewayNotJoined; 
			  break;
		  case 2:
			  netResourceReport.Status = Command::rsFailure_GatewayUnknown; 
			  break;
		  default:
			  netResourceReport.Status = Command::rsFailure_GatewayInvalidFailureCode; 
			  break;
	  }

	  LOG_INFO("net_resource_rep_unser -> status = " << (boost::int32_t)netResourceReport.Status);
	  if (netResourceReport.Status != Command::rsSuccess)
		  return;


	  //read data
	   boost::uint32_t subnetCount = util::binary_read<boost::uint8_t>(data, network);
	   LOG_INFO("net_resource_rep_unser -> total subnets =  " <<  (boost::uint32_t)subnetCount);
	   LOG_INFO("net_resource_rep_unser --> begin --------");
	   for (unsigned int i = 0; i < subnetCount; i++)
	   {
		   netResourceReport.NetReportList.push_back(GNetworkResourceReport::NetReport());
		   GNetworkResourceReport::NetReport & subnet = *netResourceReport.NetReportList.rbegin();

		  {
			  boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
			  subnet.BBRAddress = IPv6(address);
		  }
		  subnet.SubnetID = util::binary_read<boost::uint16_t>(data, network);
		  subnet.SlotLength = util::binary_read<boost::uint32_t>(data, network);
		  subnet.SlotsOccupied = util::binary_read<boost::uint32_t>(data, network);
		  subnet.AperiodicData_X = util::binary_read<boost::uint32_t>(data, network);
		  subnet.AperiodicData_Y = util::binary_read<boost::uint32_t>(data, network);
		  subnet.AperiodicMgmt_X = util::binary_read<boost::uint32_t>(data, network);
	      subnet.AperiodicMgmt_Y = util::binary_read<boost::uint32_t>(data, network);
		  subnet.PeriodicData_X = util::binary_read<boost::uint32_t>(data, network);
		  subnet.PeriodicData_Y = util::binary_read<boost::uint32_t>(data, network);
		  subnet.PeriodicMgmt_X = util::binary_read<boost::uint32_t>(data, network);
		  subnet.PeriodicMgmt_Y = util::binary_read<boost::uint32_t>(data, network);

		  LOG_INFO("net_resource_rep_unser - subnet = " << (boost::uint32_t)i 
				<< " with bbr_ip = " << (subnet.BBRAddress).ToString()
				<< " SubnetID = " << subnet.SubnetID
				<< " SlotLength = " << subnet.SlotLength
				<< " SlotsOccupied = " << subnet.SlotsOccupied
				<< " AperiodicData_X = " << subnet.AperiodicData_X
				<< " AperiodicData_Y = " << subnet.AperiodicData_Y
				<< " AperiodicMgmt_X = " << subnet.AperiodicMgmt_X
				<< " AperiodicMgmt_Y = " << subnet.AperiodicMgmt_Y
				<< " PeriodicData_X = " << subnet.PeriodicData_X
				<< " PeriodicData_Y = " << subnet.PeriodicData_Y
				<< " PeriodicMgmt_X = " << subnet.PeriodicMgmt_X
				<< " PeriodicMgmt_Y = " << subnet.PeriodicMgmt_Y);
	   }
	   LOG_INFO("net_resource_rep_unser --> end --------");
}

//added by Cristian.Guef
void GServiceUnserializer::Visit(GAlert_Subscription& alertSubscription)
{
	if (packet->serviceType != gateway::GeneralPacket::GAlertSubscriptionConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=GAlertSubscriptionConfirm!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  alertSubscription.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);
	  switch (status)
	  {
		  case 0: 
			  alertSubscription.Status = Command::rsSuccess; 
			  break;
		  case 1:
		  	  alertSubscription.Status = Command::rsFailure_InvalidCategory; 
			  break;
		  case 2:
			  alertSubscription.Status = Command::rsFailure_InvalidIndividualAlert; 
			  break;
		  case 3:
			  alertSubscription.Status = Command::rsFailure_GatewayUnknown; 
			  break;
		  default:
		      alertSubscription.Status = Command::rsFailure_GatewayInvalidFailureCode; 
			  break;
	  }

	  LOG_INFO("alert_sub_unser -> status = " << (boost::int32_t)alertSubscription.Status);
	  if (alertSubscription.Status == Command::rsSuccess || 
	  			alertSubscription.Status == Command::rsFailure_GatewayUnknown ||
	  			alertSubscription.Status == Command::rsFailure_GatewayInvalidFailureCode)
		  return;


	  //read data
	   boost::uint32_t subType = util::binary_read<boost::uint8_t>(data, network);
	   LOG_INFO("alert_sub_unser -> subType =  " <<  (boost::uint32_t)subType);
	   if (subType == 1)
	   {
			LOG_INFO("alert_sub_unser -> subscribe =  " <<  (boost::uint32_t)util::binary_read<boost::uint8_t>(data, network));
			LOG_INFO("alert_sub_unser -> enable =  " <<  (boost::uint32_t)util::binary_read<boost::uint8_t>(data, network));
			LOG_INFO("alert_sub_unser -> category =  " <<  (boost::uint32_t)util::binary_read<boost::uint8_t>(data, network));
	   }
	   else
	   {
		   LOG_INFO("alert_sub_unser -> subscribe =  " <<  (boost::uint32_t)util::binary_read<boost::uint8_t>(data, network));
		   LOG_INFO("alert_sub_unser -> enable =  " <<  (boost::uint32_t)util::binary_read<boost::uint8_t>(data, network));
			{
			  boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
			  LOG_INFO("alert_sub_unser -> ip =  " <<  IPv6(address).ToString());
			}
			LOG_INFO("alert_sub_unser -> port =  " <<  (boost::uint32_t)util::binary_read<boost::uint16_t>(data, network));
			LOG_INFO("alert_sub_unser -> obj_id =  " <<  (boost::uint32_t)util::binary_read<boost::uint16_t>(data, network));
			LOG_INFO("alert_sub_unser -> alert_type =  " <<  (boost::uint32_t)util::binary_read<boost::uint8_t>(data, network));
		  
	   }

}

//added by Cristian.Guef
void GServiceUnserializer::Visit(GAlert_Indication& alertIndication)
{
	if (packet->serviceType != gateway::GeneralPacket::GAlertSubscriptionIndication)
	{
		THROW_EXCEPTION1(SerializationException, "Expected ServiceType=GAlertSubscriptionIndication!");
	}
	Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);

	alertIndication.Status = st;
	
	if (st != Command::rsSuccess)
	{
		LOG_INFO("WARNING alert_ind_unser -> status =  " <<  (boost::uint16_t)st);
		return;
	}

	std::basic_istringstream<boost::uint8_t> data(packet->data);
	const util::NetworkOrder& network = util::NetworkOrder::Instance();

	boost::uint8_t address[IPv6::SIZE]; util::binary_read_bytes(data, address, sizeof(address));
	alertIndication.NetworkAddress	= IPv6(address);
	alertIndication.EndpointPort	= util::binary_read<boost::uint16_t>(data, network);
	alertIndication.ObjectID		= util::binary_read<boost::uint16_t>(data, network);
	alertIndication.Time.Seconds	= util::binary_read<boost::uint32_t>(data, network);
	alertIndication.Time.FractionOfSeconds = util::binary_read<boost::uint16_t>(data, network);
	alertIndication.Class			= util::binary_read<boost::uint8_t>(data, network);
	alertIndication.Direction		= util::binary_read<boost::uint8_t>(data, network);
	alertIndication.Category		= util::binary_read<boost::uint8_t>(data, network);
	alertIndication.Type			= util::binary_read<boost::uint8_t>(data, network);
	alertIndication.Priority		= util::binary_read<boost::uint8_t>(data, network);
	int data_len					= util::binary_read<boost::uint16_t>(data, network);

	LOG_INFO("alert_ind_unser " <<  alertIndication.NetworkAddress.ToString() << "/" << alertIndication.EndpointPort << "/" << alertIndication.ObjectID
		<< " UTC " << alertIndication.Time.Seconds - (TAI_OFFSET + CurrentUTCAdjustment)
		<< "; UTC(10^-6) = "
		<< (((uint32_t)alertIndication.Time.FractionOfSeconds * 1000000) >> 16)
		<< " Class " << (boost::uint16_t)alertIndication.Class);
	LOG_INFO("alert_ind_unser "
		<< " Direction " << (boost::uint16_t)alertIndication.Direction
		<< " Category " << (boost::uint16_t)alertIndication.Category
		<< " Type " << (boost::uint16_t)alertIndication.Type
		<< " Priority " << (boost::uint16_t)alertIndication.Priority
		<< " data_len " << (boost::uint32_t)data_len);

	  alertIndication.Data.resize(data_len);
	  util::binary_read_bytes(data, alertIndication.Data.c_str(), data_len);

}


 //added by Cristian.Guef
 void GServiceUnserializer::Visit(GDelContract& contract)
 {
	if (packet->serviceType != gateway::GeneralPacket::GContractConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=LeaseConfirm!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  contract.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	   boost::uint8_t contractStatus = util::binary_read<boost::uint8_t>(data, network);
	  switch (contractStatus)
	  {
		  case 0: contract.Status = Command::rsSuccess; break;

		  case 1: contract.Status = Command::rsSuccess_ContractLowerPeriod /* it wouldn't be the case for us*/; return;

		  case 2: contract.Status = Command::rsFailure_ThereisNoLease; return;

		  case 3: contract.Status = Command::rsFailure_GatewayNoContractsAvailable; return;

		  case 4: contract.Status = Command::rsFailure_InvalidDevice; return;
		  case 5: contract.Status = Command::rsFailure_InvalidLeaseType; return;
		  case 6: contract.Status = Command::rsFailure_InvalidLeaseTypeInform; return;

		  case 7:
		  contract.Status = Command::rsFailure_GatewayUnknown; return;
		  default:
		  LOG_WARN("Unserialize del_lease=" << contract.ToString() << " unknown status code=" << (boost::int32_t)contractStatus );
		  contract.Status = Command::rsFailure_GatewayInvalidFailureCode; return;
	  }

     unsigned int contractID = util::binary_read<boost::uint32_t>(data, network);
	 if (contract.ContractID != contractID)
	 {
		 contract.Status = Command::rsFailure_InvalidReturnedLeaseID;
		 return;
	 }

		
	 unsigned int contractPeriod = util::binary_read<boost::uint32_t>(data, network);
	 if (contractPeriod != 0)
	 {
		contract.Status = Command::rsFailure_InvalidReturnedLeasePeriod;
		return;
	 }
	
 }

  void GServiceUnserializer::Visit(GContract& contract)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GContractConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=LeaseConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  contract.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  // commented by Cristian.Guef
	  /*status
	  if (0 != util::binary_read<boost::uint8_t>(data, network))
	  {
		  contract.Status = Command::rsFailure_GatewayCommunication;
		  //nothing to parse
		  return;
	  }
	  */

	  //added by Cristian.Guef
	  boost::uint8_t contractStatus = util::binary_read<boost::uint8_t>(data, network);
	  switch (contractStatus)
	  {
		  case 0: contract.Status = Command::rsSuccess; break;

		  case 1: contract.Status = Command::rsSuccess_ContractLowerPeriod /* it wouldn't be the case for us*/; return;

		  case 2: contract.Status = Command::rsFailure_ThereisNoLease; return;

		  case 3: contract.Status = Command::rsFailure_GatewayNoContractsAvailable; return;

		  case 4: contract.Status = Command::rsFailure_InvalidDevice; return;
		  case 5: contract.Status = Command::rsFailure_InvalidLeaseType; return;
		  case 6: contract.Status = Command::rsFailure_InvalidLeaseTypeInform; return;

		  case 7:
		  contract.Status = Command::rsFailure_GatewayUnknown;
		  return;
		  default:
		  LOG_WARN("Unserialize lease=" << contract.ToString() << " unknown status code=" << (boost::int32_t)contractStatus );
		  contract.Status = Command::rsFailure_GatewayInvalidFailureCode; return;
	  }

	  /* commented by Cristian.Guef
	  contract.ContractID = util::binary_read<boost::uint16_t>(data, network);
	  */
	  //added by Cristian.Guef
	  contract.ContractID = util::binary_read<boost::uint32_t>(data, network);

		
	  /* commented by Cristian.Guef
	  contract.ContractPeriod = util::binary_read<boost::uint32_t>(data, network);
	  */


	//added by Cristian.Guef
	  contract.ContractPeriod = util::binary_read<boost::uint32_t>(data, network);
	 

	  /* commented by Cristian.Guef
	  boost::uint8_t contractStatus = util::binary_read<boost::uint8_t>(data, network);
	  switch (contractStatus)
	  {
		  case 0: contract.Status = Command::rsSuccess; break;

		  case 1: contract.Status = Command::rsSuccess_LowerPeriod; break;

		  case 2: contract.Status = Command::rsFailure_ThereisNoLease; break;

		  case 3: contract.Status = Command::rsFailure_GatewayNoContractsAvailable; break;

		  case 4: contract.Status = Command::rsFailure_InvalidDevice; break;
		  case 5: contract.Status = Command::rsFailure_InvalidLeaseType; break;
		  case 6: contract.Status = Command::rsFailure_InvalidLeaseTypeInform; break;
		  case 7: contract.Status = Command::rsFailure_LeaseFail; break;

		  default:
		  LOG_WARN("Unserialize contract=" << contract.ToString() << " unknown status code=" << contractStatus );
		  contract.Status = Command::rsFailure_GatewayUnknown;
		  break;
	  }
	  */
		
  }

  //added by Cristian.Guef
  void GServiceUnserializer::Visit(GSession& session)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GSessionConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=LeaseConfirm!");
	  }


	  Command::ResponseStatus st = VerifyReceivedPacket_v3_whithout_session(packet);
	  if (st != Command::rsSuccess)
	  {
		  session.Status = st;
		  return;
	  }
	  
	  //data
	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();


	  session.m_unSessionID = packet->sessionID;

	  boost::uint8_t sessionStatus = util::binary_read<boost::uint8_t>(data, network);
	  switch (sessionStatus)
	  {
		  case 0: session.Status = Command::rsSuccess; break;

		  case 1: session.Status = Command::rsSuccess_SessionLowerPeriod/* it wouldn't be the case for us*/; return;

		  case 2: session.Status = Command::rsFailure_ThereIsNoSession; return;

		  case 3: session.Status = Command::rsFailure_SessionNotCreated; return;

		  case 4:
		   session.Status = Command::rsFailure_GatewayUnknown;
		  return;
		  default:
		  LOG_WARN("Unserialize session=" << session.ToString() << " unknown status code=" << (boost::int32_t)sessionStatus );
		  session.Status = Command::rsFailure_GatewayInvalidFailureCode; return;
	  }

	  session.m_nSessionPeriod = util::binary_read<boost::int32_t>(data, network);
  }

  void GServiceUnserializer::Visit(GBulk& bulk)
  {
	  /* comment by Cristian.Guef
	  if (packet->serviceType != gateway::GeneralPacket::GBulkConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=Bulk!");
	  }
	  */


	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();


	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  bulk.Status = st;
		  return;
	  }

	  /* commented by Cristian.Guef
	  switch (util::binary_read<boost::uint8_t>(data, network))
	  {
		  case 0:
		  bulk.Status = Command::rsSuccess;
		  break;

		  case 1:
		  bulk.Status = Command::rsFailure_GatewayTimeout;
		  break;

		  case 2:
		  bulk.Status = Command::rsFailure_GatewayInvalidContract;
		  break;
	
		  default:
		  bulk.Status = Command::rsFailure_GatewayUnknown;
		  break;
	  }
	  */

	  //added by Cristian.Guef
	  boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);

	  //added by Cristian.Guef
	  switch (bulk.m_currentBulkState)
	  {
	  case GBulk::BulkOpen:

		  if (packet->serviceType != gateway::GeneralPacket::GBulkOpenConfirm)
		  {
			  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=Bulk!");
		  }

		  switch (status)
		  {
			  case 0:
			  bulk.Status = Command::rsSuccess;
			  break;

			  case 1:
			  bulk.Status = Command::rsFailure_ItemExceedsLimits;
			  return;

			  case 2:
			  bulk.Status = Command::rsFailure_UnknownResource;
			  return;

			  case 3:
			  bulk.Status = Command::rsFailure_InvalidMode;
			  return;

			  case 4:
			  bulk.Status = Command::rsFailure_GatewayUnknown;
			  return;
			  default:
				bulk.Status = Command::rsFailure_GatewayInvalidFailureCode; return;
		  }
		  break;

	  case GBulk::BulkTransfer:
		  if (packet->serviceType != gateway::GeneralPacket::GBulkTransferConfirm)
		  {
			  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=Bulk!");
		  }

		  switch (status)
		  {
			  case 0:
			  bulk.Status = Command::rsSuccess;
			  break;

			  case 1:
			  bulk.Status = Command::rsFailure_CommunicationFailed;
			  return;

			  case 2:
			  bulk.Status = Command::rsFailure_TransferAborted;
			  return;

			  case 3:
			  bulk.Status = Command::rsFailure_GatewayUnknown;
			  return;
			  default:
				bulk.Status = Command::rsFailure_GatewayInvalidFailureCode; return;
		  }
		  break;

	  case GBulk::BulkEnd:
		  if (packet->serviceType != gateway::GeneralPacket::GBulkCloseConfirm)
		  {
			  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=Bulk!");
		  }
		  if (status != 0)
		  {
			  bulk.Status = Command::rsFailure_GatewayInvalidFailureCode;
			  return;
		  }

		  bulk.Status = Command::rsSuccess;
		  break;
	  }

	  
	  //added by Cristian.Guef
	  boost::uint32_t ItemSize;
	  
	  //added by Cristian.Guef
	  switch (bulk.m_currentBulkState)
	  {
	  case GBulk::BulkOpen:
		  bulk.maxBlockSize = util::binary_read<boost::uint16_t>(data, network);
		  ItemSize = util::binary_read<boost::uint32_t>(data, network);
		
		  //printf("\n din unserialize------ Bulk_open --maxBlockSize = %d si ItemSize = %d ", 
		  //				bulk.maxBlockSize, ItemSize);
		  //fflush(stdout);
		  LOG_DEBUG(" unserialize------> Bulk_open --maxBlockSize = " <<
		  			(boost::uint32_t)bulk.maxBlockSize <<  
		  			" and ItemSize =  " << (boost::uint32_t)ItemSize);
		
		  //added by Cristian.Guef nu-i nevoie
		  //if (bulk.maxBlockSize > bulk.Data.size())
		  //{
		  //bulk.Status = Command::rsFailure_InvalidBlockSize;
	      //	return;
		  //}
		  break;

	  case GBulk::BulkTransfer:
		  //nothing for download
		  break;

	  case GBulk::BulkEnd:
		  break;
	  }


  }

  void GServiceUnserializer::Visit(GClientServer<WriteObjectAttribute>& writeAttribute)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  writeAttribute.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  writeAttribute.Status = ParseClientServerStatus(data, network);
  }

  void GServiceUnserializer::Visit(GClientServer<ReadMultipleObjectAttributes>& multipleReadObjectAttributes)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  multipleReadObjectAttributes.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  multipleReadObjectAttributes.Status = ParseClientServerCommunicationStatus(data, network);
	  if (multipleReadObjectAttributes.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }
	
	  //added by Cristian.Guef
	  //there will be just one iteration because of fragmenting

	  for (ReadMultipleObjectAttributes::AttributesList::iterator it =
			  multipleReadObjectAttributes.Client.attributes.begin(); it
			  != multipleReadObjectAttributes.Client.attributes.end(); it++)
	  {

		  //added by Cristian.Guef
		  //struct timeval tv;
		  it->tv.tv_sec = util::binary_read<boost::uint32_t>(data, network);
		  it->tv.tv_usec = util::binary_read<boost::uint32_t>(data, network); 
		  //NO SM IMPLEMENTATION YET*/
		  util::binary_read<boost::uint8_t>(data, network); //reqType
		  util::binary_read<boost::uint16_t>(data, network); //objID


		  //TODO: add management for extended sfcCode
		  boost::uint8_t sfCode = util::binary_read<boost::uint8_t>(data, network);
		  it->confirmStatus = (sfCode == 0)
		  ? Command::rsSuccess
		  : (Command::ResponseStatus) (Command::rsFailure_DeviceError - sfCode);

		  if (it->confirmStatus != Command::rsSuccess)
		  {
			  continue; //goto next channel
		  }

		  //added by Cristian.Guef
		  //MyDateTime myDT;
		  //gettime(myDT, tv);
		  //NO SM IMPLEMENTATION YET*/
		  //gettime(myDT);
		  //it->ReadingTime = myDT.ReadingTime;
		  //it->milisec = myDT.milisec;

		  LOG_DEBUG("Read ObjID=" << (int)it->channel.mappedObjectID << "Read AttributeID=" << it->channel.mappedAttributeID << ", with DataType=" << it->confirmValue.dataType);

		 //added by Cristian.Guef
		 boost::uint16_t respLength = util::binary_read<boost::uint16_t>(data, network);
		
		 //added by Cristian.Guef
		 if (it->channel.withStatus != 0)
		 {
			 it->IsISA = true;
			 it->ValueStatus = util::binary_read<boost::uint8_t>(data, network); //ISA standard
			 			 
			 switch(respLength)
			{
		  	case 2 /*1 value_status + one byte*/:
				if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 2 /*1 value_status + one byte*/)
				{
					multipleReadObjectAttributes.Status = Command::rsFailure_ReadValueUnexpectedSize;
					return;
				}
						
				if (it->confirmValue.dataType == ChannelValue::cdtUInt8)
		  			it->confirmValue.value.uint8 = util::binary_read<boost::uint8_t>(data, network);
		  		else
		  			it->confirmValue.value.int8 = util::binary_read<boost::int8_t>(data, network);
		  		break;
		  	case 3 /*1 value_status + two bytes*/:
				if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 3 /*1 value_status + two bytes*/)
				{
					multipleReadObjectAttributes.Status = Command::rsFailure_ReadValueUnexpectedSize;
					return;
				}
	
		  		if (it->confirmValue.dataType == ChannelValue::cdtUInt16)
		  			it->confirmValue.value.uint16 = util::binary_read<boost::uint16_t>(data, network);
		  		else
		  			it->confirmValue.value.int16 = util::binary_read<boost::int16_t>(data, network);
		  		break;
		  	case 5 /*1 value_status + four bytes*/:
				if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 5 /*1 value_status + four bytes*/ )
				{
					multipleReadObjectAttributes.Status = Command::rsFailure_ReadValueUnexpectedSize;
					return;
				}
	
		  		if (it->confirmValue.dataType == ChannelValue::cdtUInt32)
		  			it->confirmValue.value.uint32 = util::binary_read<boost::uint32_t>(data, network);
		  		else if (it->confirmValue.dataType == ChannelValue::cdtInt32)
		  			it->confirmValue.value.int32 = util::binary_read<boost::int32_t>(data, network);
		  		else if (it->confirmValue.dataType == ChannelValue::cdtFloat32)
				{	uint32_t * p = (uint32_t*)&(it->confirmValue.value.float32);
					*p = util::binary_read<boost::uint32_t>(data, network);
				}
	
		  		break;
		  	default:
		  		LOG_ERROR("ReadMultipleObject - unexpected data size");
		  		multipleReadObjectAttributes.Status = Command::rsFailure_ReadValueUnexpectedSize;
		  		return;
			}
			 
		 }
		 else
		 {
			 it->IsISA = false;
			 
			 switch(respLength)
			{
		  	case 1 /*one byte*/:
				if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 1 /*one byte*/)
				{
					multipleReadObjectAttributes.Status = Command::rsFailure_ReadValueUnexpectedSize;
					return;
				}
	
		  		if (it->confirmValue.dataType == ChannelValue::cdtUInt8)
		  			it->confirmValue.value.uint8 = util::binary_read<boost::uint8_t>(data, network);
		  		else
		  			it->confirmValue.value.int8 = util::binary_read<boost::int8_t>(data, network);
		  		break;
		  	case 2 /*two bytes*/:
				if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 2 /*two bytes*/)
				{
					multipleReadObjectAttributes.Status = Command::rsFailure_ReadValueUnexpectedSize;
					return;
				}
	
		  		if (it->confirmValue.dataType == ChannelValue::cdtUInt16)
		  			it->confirmValue.value.uint16 = util::binary_read<boost::uint16_t>(data, network);
		  		else
		  			it->confirmValue.value.int16 = util::binary_read<boost::int16_t>(data, network);
		  		break;
		  	case 4 /*four bytes*/:
				if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 4 /*four bytes*/ )
				{
					multipleReadObjectAttributes.Status = Command::rsFailure_ReadValueUnexpectedSize;
					return;
				}
	
		  		if (it->confirmValue.dataType == ChannelValue::cdtUInt32)
		  			it->confirmValue.value.uint32 = util::binary_read<boost::uint32_t>(data, network);
		  		else if (it->confirmValue.dataType == ChannelValue::cdtInt32)
		  			it->confirmValue.value.int32 = util::binary_read<boost::int32_t>(data, network);
		  		else if (it->confirmValue.dataType == ChannelValue::cdtFloat32)
				{	uint32_t * p = (uint32_t*)&(it->confirmValue.value.float32);
					*p = util::binary_read<boost::uint32_t>(data, network);
				}
	
		  		break;
		  	default:
		  		LOG_ERROR("ReadMultipleObject - unexpected data size");
		  		multipleReadObjectAttributes.Status = Command::rsFailure_ReadValueUnexpectedSize;
		  		return;
			}
			 
		 }

		
		LOG_DEBUG("Set value on ReadMultipleObject is:" << it->confirmValue.ToString());
	  }
  }

  void GServiceUnserializer::Visit(PublishSubscribe& publishSubscribe)
  {
	  THROW_EXCEPTION1(SerializationException, "PublishSubscribe is not seriaalizable!");
  }

  void GServiceUnserializer::Visit(GClientServer<Publish>& publish)
  {


	//added by Cristian.Guef
	  if (publish.Client.m_currentPublishState == Publish::PublishDoLeaseSubscriber)
	  {

		  if (packet->serviceType != gateway::GeneralPacket::GContractRequest)
		  {
			  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
		  }

		  	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
			  if (st != Command::rsSuccess)
			  {
				  publish.Status = st;
				  return;
			  }


		  std::basic_istringstream<boost::uint8_t> data(packet->data);
		  const util::NetworkOrder& network = util::NetworkOrder::Instance();

		   boost::uint8_t contractStatus = util::binary_read<boost::uint8_t>(data, network);
		  switch (contractStatus)
		  {
			  case 0: publish.Status = Command::rsSuccess; break;

			  case 1: publish.Status = Command::rsSuccess_ContractLowerPeriod /* it wouldn't be the case for us*/; return;

			  case 2: publish.Status = Command::rsFailure_ThereisNoLease; return;

			  case 3: publish.Status = Command::rsFailure_GatewayNoContractsAvailable; return;

			  case 4: publish.Status = Command::rsFailure_InvalidDevice; return;
			  case 5: publish.Status = Command::rsFailure_InvalidLeaseType; return;
			  case 6: publish.Status = Command::rsFailure_InvalidLeaseTypeInform; return;
			  
			  case 7:
			  publish.Status = Command::rsFailure_GatewayUnknown;
			  return;
			  default:
			  LOG_WARN("Unserialize for publish -> lease"  << " unknown status code=" << (boost::int32_t)contractStatus );
			  publish.Status = Command::rsFailure_GatewayInvalidFailureCode; return;
		  }

		  publish.Client.m_ObtainedLeaseID_S = util::binary_read<boost::uint32_t>(data, network);
		  publish.Client.m_ObtainedLeasePeriod_S = util::binary_read<boost::uint32_t>(data, network);
		  return;
	  }


	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  publish.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  publish.Status = ParseClientServerStatus(data, network);

  }

  void GServiceUnserializer::Visit(GClientServer<Subscribe>& subscribe)
  {

	 //added by Cristian.Guef
	  if (subscribe.Client.m_currentSubscribeState == Subscribe::SubscribeDoLeaseSubscriber)
	  {

		  if (packet->serviceType != gateway::GeneralPacket::GContractConfirm)
		  {
			  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=GContractConfirm!");
		  }
			
		  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
		  if (st != Command::rsSuccess)
		  {
			  subscribe.Status = st;
			  return;
		  }

		  std::basic_istringstream<boost::uint8_t> data(packet->data);
		  const util::NetworkOrder& network = util::NetworkOrder::Instance();

		   boost::uint8_t contractStatus = util::binary_read<boost::uint8_t>(data, network);
		  switch (contractStatus)
		  {
			  case 0: subscribe.Status = Command::rsSuccess; break;

			  case 1: subscribe.Status = Command::rsSuccess_ContractLowerPeriod /* it wouldn't be the case for us*/; return;

			  case 2: subscribe.Status = Command::rsFailure_ThereisNoLease; return;

			  case 3: subscribe.Status = Command::rsFailure_GatewayNoContractsAvailable; return;

			  case 4: subscribe.Status = Command::rsFailure_InvalidDevice; return;
			  case 5: subscribe.Status = Command::rsFailure_InvalidLeaseType; return;
			  case 6: subscribe.Status = Command::rsFailure_InvalidLeaseTypeInform; return;
			  
			  case 7:
			  subscribe.Status = Command::rsFailure_GatewayUnknown;
			  return;
			  default:
			  LOG_WARN("Unserialize for subscribe -> lease"  << " unknown status code=" << (boost::int32_t)contractStatus );
			  subscribe.Status = Command::rsFailure_GatewayInvalidFailureCode; return;
		  }

		  subscribe.Client.m_ObtainedLeaseID_S = util::binary_read<boost::uint32_t>(data, network);
		  subscribe.Client.m_ObtainedLeasePeriod_S = util::binary_read<boost::uint32_t>(data, network);
		  return;
	  }


	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	
	  /*status*/
	  subscribe.Status = ParseClientServerStatus(data, network);

	  if (subscribe.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }

 }

  //added by Cristian.Guef
  void GServiceUnserializer::Visit(GClientServer<GetSizeForPubIndication>& getSizeForPubIndication)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  getSizeForPubIndication.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  getSizeForPubIndication.Status = ParseClientServerStatus(data, network);
	  if (getSizeForPubIndication.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }
	
	  boost::uint16_t respLength = util::binary_read<boost::uint16_t>(data, network);
	  
	   if (respLength%sizeof(Subscribe::ObjAttrSize) != 0)
		{
			LOG_ERROR(" unserialize ------> get_size_for_indication with error 'invalid publihed data received' with length = " << 
				(boost::int32_t)respLength << "which is not multiple with 8 bytes (array elem size)");
			getSizeForPubIndication.Status = Command::rsFailure_InvalidPublisherDataReceived;
			return;
		}
		
		getSizeForPubIndication.Client.m_vecReadObjAttrSize.resize(respLength/sizeof(Subscribe::ObjAttrSize));

		LOG_INFO(" unserialize ---------> get_size_for_indication  with content_version = " << 
				(boost::uint32_t)getSizeForPubIndication.Client.m_ContentVersion << 
				"\n\t read_obj_attr_index_size array_length = " << 
				(boost::int32_t)respLength);
	  LOG_INFO("------------get_size_for_indication - array begin-----------------------------");
	  	
		for(unsigned int i = 0; i < getSizeForPubIndication.Client.m_vecReadObjAttrSize.size(); i++)
		{
			//objID
			getSizeForPubIndication.Client.m_vecReadObjAttrSize[i].ObjID = util::binary_read<boost::uint16_t>(data, network);
			//attrid
			getSizeForPubIndication.Client.m_vecReadObjAttrSize[i].AttrID = util::binary_read<boost::uint16_t>(data, network);
			//attr_index
			getSizeForPubIndication.Client.m_vecReadObjAttrSize[i].AttrIndex = util::binary_read<boost::uint16_t>(data, network);
			//size
			getSizeForPubIndication.Client.m_vecReadObjAttrSize[i].Size = util::binary_read<boost::uint16_t>(data, network);

			LOG_INFO("\n\t\t si vec[" << (boost::int32_t)i << "].obj_id = " << 
						(boost::int32_t)getSizeForPubIndication.Client.m_vecReadObjAttrSize[i].ObjID <<
						"\n\t\t si vec[" << (boost::int32_t)i << "].attr_id = " << 
						(boost::int32_t)getSizeForPubIndication.Client.m_vecReadObjAttrSize[i].AttrID <<
						"\n\t\t si vec[" << (boost::int32_t)i << "].attr_index = " << 
						(boost::int32_t)getSizeForPubIndication.Client.m_vecReadObjAttrSize[i].AttrIndex <<  
						"\n\t\t si vec[" << (boost::int32_t)i << "].size = " << 
						(boost::int32_t)getSizeForPubIndication.Client.m_vecReadObjAttrSize[i].Size <<
						"\n");

		}
	  LOG_INFO("------------ get_size_for_indication - array end-----------------------------");
  }


#define PUBLISH_MIN_LEN  (sizeof(unsigned long)/*leaseID*/ + sizeof(unsigned long)/*CRC*/ + sizeof(unsigned char)/*content_version*/ + sizeof(unsigned char)/*freshSeq*/ +sizeof(unsigned long)/*sec*/ +sizeof(unsigned long)/*usec*/)
  
  extern bool IsFullAPI(int pubHandle);

  void GServiceUnserializer::Visit(PublishIndication& publishIndication)
  {
  	  switch (packet->serviceType)
	  {
		case gateway::GeneralPacket::GPublishIndication:
			publishIndication.m_IndicationType = PublishIndication::Publish_Indication;
			break;
		case gateway::GeneralPacket::GSubscribeTimer:
			publishIndication.m_IndicationType = PublishIndication::Subscriber_Timer;
			break;
		case gateway::GeneralPacket::GWatchdogTimer:
			publishIndication.m_IndicationType = PublishIndication::Watchdog_Timer;
			break;
		default:
			THROW_EXCEPTION1(SerializationException, "Expected ServiceType=GPublishIndication or GSubscriberTimer or GWatchTimer!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  publishIndication.Status = st;
		  return;
	  }

	  unsigned char *pData = (unsigned char*)packet->data.c_str();
	  if ( (unsigned int)packet->data.size() < PUBLISH_MIN_LEN)
		THROW_EXCEPTION0(util::NotEnoughBytesException);

	  publishIndication.Status = Command::rsSuccess;

	  //added by Cristian.Guef
	  /*
	  Sequence of
	  Content Version
	  Freshness Seq. No.
	  Pdata Len
	  Pdata
	  */	
	  
	 
	  // added by Cristian.Guef
	  //util::binary_read<boost::uint32_t>(data, network); //skip leaseID
	  //added
	  pData += 4;

	  //struct timeval tv;
	  //tv.tv_sec = util::binary_read<boost::uint32_t>(data, network);
	  //tv.tv_usec = util::binary_read<boost::uint32_t>(data, network);
	  //gettime(publishIndication.ReadingTime, publishIndication.milisec, tv);
	  //NO SM IMPLEMENTATION YET*/
	  //gettime(myDT);
	  
	  //publishIndication.tv.tv_sec = util::binary_read<boost::uint32_t>(data, network);
	  publishIndication.tv.tv_sec = htonl(*((unsigned int*)pData));
	  pData+=4;

	  //publishIndication.tv.tv_usec = util::binary_read<boost::uint32_t>(data, network);
	  publishIndication.tv.tv_usec = htonl(*((unsigned int*)pData));
	  pData+=4;

	  //publishIndication.m_ContentVersion = util::binary_read<boost::uint8_t>(data, network);
	  publishIndication.m_ContentVersion = *(pData++);

	  //publishIndication.m_FreshSeqNo = util::binary_read<boost::uint8_t>(data, network);
	  publishIndication.m_FreshSeqNo = *(pData++);


	  if (IsFullAPI(publishIndication.m_publishHandle))
	  {
		publishIndication.m_PDataLen = (int)packet->data.size() - PUBLISH_MIN_LEN;
		LOG_DEBUG("Publish interface_type=full_api with data size=" << publishIndication.m_PDataLen);
	  }
	  else
	  {
		publishIndication.m_PDataLen = *(pData++);
		if (publishIndication.m_PDataLen != ((int)packet->data.size() - PUBLISH_MIN_LEN - 1/*octet(data_size)*/))
		{
			publishIndication.m_DataBuff.resize(0);
			LOG_ERROR("publish -> invalid data size = " << publishIndication.m_PDataLen << "for simple interface!");
			return;
		}
		LOG_DEBUG("Publish interface_type=simple_api with data size=" << publishIndication.m_PDataLen);
	  }
	
	  publishIndication.m_DataBuff.resize(publishIndication.m_PDataLen);
	  //util::binary_read_bytes(data, publishIndication.m_DataBuff.c_str(), publishIndication.m_PDataLen);
	  memcpy((void*)publishIndication.m_DataBuff.c_str(), pData, publishIndication.m_PDataLen);
	  
}

 
  void GServiceUnserializer::Visit(GClientServer<GetFirmwareVersion>& getFirmwareVersion)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  getFirmwareVersion.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  getFirmwareVersion.Status = ParseClientServerStatus(data, network);
	  if (getFirmwareVersion.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }

	  //added by Cristian.Guef
	  util::binary_read<boost::uint16_t>(data, network);


	  /*op status*/
	  getFirmwareVersion.Client.OperationStatus = util::binary_read<boost::uint8_t>(data, network);
	  boost::uint8_t fw[VersionFW::SIZE];
	  util::binary_read_bytes(data, fw, VersionFW::SIZE);
	  getFirmwareVersion.Client.FirmwareVersion = VersionFW(fw);
  }

  void GServiceUnserializer::Visit(GClientServer<FirmwareUpdate>& startFirmwareUpdate)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  startFirmwareUpdate.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  startFirmwareUpdate.Status = ParseClientServerStatus(data, network);
	  if (startFirmwareUpdate.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }

	  //added by Cristian.Guef
	  util::binary_read<boost::uint16_t>(data, network);

	  /*op status*/
	  startFirmwareUpdate.Client.OperationStatus = util::binary_read<boost::uint8_t>(data, network);
  }

  void GServiceUnserializer::Visit(GClientServer<GetFirmwareUpdateStatus>& getFirmwareUpdateStatus)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  getFirmwareUpdateStatus.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  getFirmwareUpdateStatus.Status = ParseClientServerStatus(data, network);
	  if (getFirmwareUpdateStatus.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }

	  //added by Cristian.Guef
	  util::binary_read<boost::uint16_t>(data, network);

	  /*op status*/
	  getFirmwareUpdateStatus.Client.OperationStatus = util::binary_read<boost::uint8_t>(data, network);
	  getFirmwareUpdateStatus.Client.CurrentPhase = util::binary_read<boost::uint8_t>(data, network);
	  getFirmwareUpdateStatus.Client.PercentDone = util::binary_read<boost::uint8_t>(data, network);
  }

  void GServiceUnserializer::Visit(GClientServer<CancelFirmwareUpdate>& cancelFirmwareUpdate)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  cancelFirmwareUpdate.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  cancelFirmwareUpdate.Status = ParseClientServerStatus(data, network);
	  if (cancelFirmwareUpdate.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }

	  //added by Cristian.Guef
	  util::binary_read<boost::uint16_t>(data, network);
	 
	  /*op status*/
	  cancelFirmwareUpdate.Client.OperationStatus = util::binary_read<boost::uint8_t>(data, network);
  }

  void GServiceUnserializer::Visit(GClientServer<ResetDevice>& resetDevice)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfitm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  resetDevice.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  resetDevice.Status = ParseClientServerStatus(data, network);
  }

  
  void GServiceUnserializer::Visit(GSensorFrmUpdateCancel &sensorFrmUpdateCancel)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GBulkCloseConfirm)
	  {
			  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=Bulk!");
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  sensorFrmUpdateCancel.Status = st;
		  return;
	  }

	  boost::uint8_t status = util::binary_read<boost::uint8_t>(data, network);
	  
	  if (status != 0)
	  {
		  sensorFrmUpdateCancel.Status = Command::rsFailure_GatewayInvalidFailureCode;
		  return;
	  }

	  sensorFrmUpdateCancel.Status = Command::rsSuccess;
  }

  void GServiceUnserializer::Visit(GClientServer<GetChannelsStatistics>& getChannels)
  {
	  if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  //added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  getChannels.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  getChannels.Status = ParseClientServerStatus(data, network);
	  if (getChannels.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }

	  //added by Cristian.Guef
	  util::binary_read<boost::uint16_t>(data, network);

	  for (unsigned i = 0; i < sizeof(getChannels.Client.CCABackoffList); i++)
	  {
		unsigned char byte = util::binary_read<boost::uint8_t>(data, network);
		char szByte[20];
		sprintf(szByte, "%02X", byte);
		getChannels.Client.strCCABackoffList += szByte;
	  }

  }

#define CS_CONTRACTS_SIZE	(sizeof(unsigned short)/*contractID*/ + sizeof(unsigned char)/*serviceType*/ + sizeof(unsigned long)/*activationTime*/ + \
							sizeof(unsigned short)/*sourceSAP*/ + IPv6::SIZE + sizeof(unsigned short)/*destinatioSAP*/ + \
							sizeof(unsigned long)/*expirationTime*/ + sizeof(unsigned char)/*priority*/ + sizeof(unsigned short)/*NSDUSize*/ + \
							sizeof(unsigned char)/*reliability*/ + sizeof(unsigned short)/*period*/ + sizeof(unsigned char)/*phase*/ + \
							sizeof(unsigned short)/*deadline*/ + sizeof(short)/*comittedBurst*/ + sizeof(short)/*excessBurst*/ + \
							sizeof(unsigned char)/*maxSendWindow*/)

#define	CS_CHECK_POINT_0	(sizeof(unsigned char)/*devicesNo*/ + sizeof(unsigned long)/*crc*/) 
#define CS_CHECK_POINT_1	(sizeof(unsigned char)/*contractsNo*/ + sizeof(unsigned char)/*routesNo*/ + IPv6::SIZE)
#define CS_CHECK_PONIT_2	(sizeof(unsigned short)/*index*/ + sizeof(unsigned char)/*size*/ + sizeof(unsigned char)/*alternative*/ + \
							sizeof(unsigned char)/*fowardlimit*/)

extern void SetGreaterCommittedBurst(IPv6 &ip, int committed);
void GServiceUnserializer::Visit(GClientServer<GetContractsAndRoutes>& getContractsAndRoutes)
{
	//test time
	WATCH_DURATION_INIT_DEF(oDurationWatcher);
	

	if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	{
		THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	}

	//added by Cristian.Guef
	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  getContractsAndRoutes.Status = st;
		  return;
	  }

	//no need
	//std::basic_istringstream<boost::uint8_t> data(packet->data);
	//const util::NetworkOrder& network = util::NetworkOrder::Instance();
    //added
	unsigned char *pData = (unsigned char*)packet->data.c_str();
	int offset = 0;

	/*status*/
	//no need
	//getContractsAndRoutes.Status = ParseClientServerStatus(data, network);
	getContractsAndRoutes.Status = ParseClientServerStatus(pData, offset);
	if (getContractsAndRoutes.Status != Command::rsSuccess)
	{
		/*nothing to parse*/
		return;
	}

	//no need
	//util::binary_read<boost::uint16_t>(data, network); //skip ResponseLen
	//added
	offset += sizeof(unsigned short);

	//added
	if ((int)packet->data.size() - offset < (int)CS_CHECK_POINT_0)
		THROW_EXCEPTION0(util::NotEnoughBytesException);


	//added
	unsigned char devicesNo = pData[offset++];

	LOG_INFO("contracts_and_routes -> devicesNo =  " <<  (boost::uint32_t)devicesNo);
	
	while(devicesNo--)
	{

		//added
		if ((int)packet->data.size() - offset < (int)CS_CHECK_POINT_1)
			THROW_EXCEPTION0(util::NotEnoughBytesException);

		//added
		IPv6 sourceAddress = IPv6(pData + offset);
		offset += IPv6::SIZE;

		//firmwaredownload
		bool fromSM = false;
		if (getContractsAndRoutes.Client.isForFirmDl && sourceAddress == getContractsAndRoutes.Client.smIP)
			fromSM = true;

		//count
		//no need
		//unsigned char contracsNo = util::binary_read<boost::uint8_t>(data, network);
		//added
		unsigned char contracsNo = pData[offset++];

		//no need
		//unsigned char routesNo = util::binary_read<boost::uint8_t>(data, network);
		//added
		unsigned char routesNo = pData[offset++];

		LOG_INFO("contracts_and_routes -> contracsNo =  " <<  (boost::uint32_t)contracsNo
								<< " and routesNo =  " <<  (boost::uint32_t)routesNo
								<< "for dev_ip = " << sourceAddress.ToString());

		//contracts
		 LOG_DEBUG("contracts_and_routes_unser--> begin 'contracts_list' --------");

		//added
		if ((int)packet->data.size() - offset < (int)(contracsNo*CS_CONTRACTS_SIZE))
			THROW_EXCEPTION0(util::NotEnoughBytesException);

		getContractsAndRoutes.Client.DevicesInfo.push_back(ContractsAndRoutes());
		ContractsAndRoutes &Info = *getContractsAndRoutes.Client.DevicesInfo.rbegin();

		for (int i = 0; i < contracsNo; i++)
		{
			Info.ContractsList.push_back(ContractsAndRoutes::Contract());
			ContractsAndRoutes::Contract &contract = *Info.ContractsList.rbegin();
			
			contract.sourceAddress = sourceAddress;
			
			contract.contractID = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);
			
			// Communication_Service_Type  0:periodic, 1: aperiodic
			contract.serviceType = pData[offset++];

			contract.activationTime = htonl(*((unsigned long*)(pData + offset)));
			offset += sizeof(unsigned long);

			contract.sourceSAP = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);

			contract.destinationAddress = IPv6(pData + offset);
			offset += IPv6::SIZE;

			contract.destinationSAP = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);

			contract.expirationTime = htonl(*((unsigned long*)(pData + offset)));
			offset += sizeof(unsigned long);
			
			contract.priority = pData[offset++];

			contract.NSDUSize = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);

			// Assigned_Reliability_And_PublishAutoRetransmit
			contract.reliability = pData[offset++];

			contract.period = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);

			contract.phase = pData[offset++];

			contract.deadline = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);

			contract.comittedBurst = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);

			contract.excessBurst = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);

			contract.maxSendWindow = pData[offset++];

			LOG_DEBUG("contracts_and_routes_unser - " 
					<< " contractID = " << (boost::uint32_t)contract.contractID 
					<< " serviceType = " << (boost::uint32_t)contract.serviceType 
					<< " activationTime = " << contract.activationTime
					<< " sourceAddress = " << contract.sourceAddress.ToString()
					<< " sourceSAP = " << (boost::uint32_t)contract.sourceSAP
					<< " destinationAddress = " << contract.destinationAddress.ToString()
					<< " destinationSAP = " << (boost::uint32_t)contract.destinationSAP
					<< " expirationTime = " << contract.expirationTime
					<< " priority = " << (boost::uint32_t)contract.priority
					<< " NSDUSize = " << (boost::uint32_t)contract.NSDUSize
					<< " reliability = " << (boost::uint32_t)contract.reliability
					<< " period = " << (boost::uint32_t)contract.period
					<< " phase = " << (boost::uint32_t)contract.phase
					<< " deadline = " << (boost::uint32_t)contract.deadline
					<< " comittedBurst = " << (boost::uint32_t) contract.comittedBurst
					<< " excessBurst = " << (boost::uint32_t)contract.excessBurst
					<< " maxSendWindow = " << (boost::uint32_t)contract.maxSendWindow);

			if (Info.ContractIDsList.insert(
				ContractsAndRoutes::ContractIDsListT::value_type(contract.contractID, contract.destinationAddress)).second == false)
			{
				LOG_WARN("duplicated contracted_id and destAddress");
			}

			//firmwaredownload
			if (getContractsAndRoutes.Client.isForFirmDl)
			{
				if (fromSM)
				{
					if (contract.sourceSAP == 1 && contract.destinationSAP == 0 && contract.serviceType == 1)
						SetGreaterCommittedBurst(contract.destinationAddress, contract.comittedBurst);
				}
				else
				{
					if (contract.sourceSAP == 0 && contract.destinationSAP == 1 && contract.serviceType == 1 
										&& contract.destinationAddress == getContractsAndRoutes.Client.smIP)
						SetGreaterCommittedBurst(contract.destinationAddress, contract.comittedBurst);
				}
			}



		}
		LOG_DEBUG("contracts_and_routes_unser--> end 'contracts_list' --------");

		//firmwaredownload
		if (getContractsAndRoutes.Client.isForFirmDl)
			return; //no need to read the other info

		//contracts
		LOG_DEBUG("contracts_and_routes_unser--> begin 'routes_list with routesNo='" << (boost::uint32_t)routesNo << " --------");
		for (int i = 0; i < routesNo; i++)
		{
			//added
			if ((int)packet->data.size() - offset < (int)CS_CHECK_PONIT_2)
				THROW_EXCEPTION0(util::NotEnoughBytesException);

			Info.RoutesList.push_back(ContractsAndRoutes::Route());
			ContractsAndRoutes::Route &route = *Info.RoutesList.rbegin();

			//no need
			//route.index = util::binary_read<boost::uint16_t>(data, network);
			//added
			route.index = htons(*((unsigned short*)(pData + offset)));
			offset += sizeof(unsigned short);

			//no need
			//unsigned char size = util::binary_read<boost::uint8_t>(data, network);
			//added
			unsigned char size = pData[offset++];

			//no need
			//route.alternative = util::binary_read<boost::uint8_t>(data, network);
			//route.forwardLimit = util::binary_read<boost::uint8_t>(data, network);
			//addd
			route.alternative = pData[offset++];
			route.forwardLimit = pData[offset++];

			route.routes.resize(size);
			LOG_DEBUG("contracts_and_routes_unser--> begin 'routeNo=" << (boost::uint32_t)i << " with route_elems_list with elemsNo='" << (boost::uint32_t)size << " --------");
			for(int j = 0; j < size; j++)
			{
				if ((int)packet->data.size() - offset < (int)sizeof(unsigned char))
					THROW_EXCEPTION0(util::NotEnoughBytesException);

				//no need
				//route.routes[j].isGraph = util::binary_read<boost::uint8_t>(data, network);
				//added
				route.routes[j].isGraph = pData[offset++];

				if (route.routes[j].isGraph == 0 /*is node*/)
				{
					if ((int)packet->data.size() - offset < (int)IPv6::SIZE)
						THROW_EXCEPTION0(util::NotEnoughBytesException);

					//no need
					//util::binary_read_bytes(data, route.routes[j].elem.nodeAddress, IPv6::SIZE);
					//added
					memcpy(route.routes[j].elem.nodeAddress, pData + offset, IPv6::SIZE);
					offset += IPv6::SIZE;
				}
				else /*is graph*/
				{
					if ((int)packet->data.size() - offset < (int)sizeof(unsigned short))
						THROW_EXCEPTION0(util::NotEnoughBytesException);

					//no need
					//route.routes[j].elem.graphID = util::binary_read<boost::uint16_t>(data, network) & 0x0FFF;
					//added
					route.routes[j].elem.graphID = htons(*((unsigned short*)(pData + offset)));
					offset += sizeof(unsigned short);
				}
				
				if (route.routes[j].isGraph == 0 /*is node*/)
				{
					LOG_DEBUG("contracts_and_routes_unser - " 
							<< " routeElemNo = " << (boost::uint32_t)j
							<< " is Graph? = " << (boost::uint32_t)route.routes[j].isGraph
							<< " nodeAddress = " << IPv6(route.routes[j].elem.nodeAddress).ToString());
				}
				else
				{
					route.routes[j].elem.graphID = route.routes[j].elem.graphID & 0x0FFF;
					LOG_DEBUG("contracts_and_routes_unser - "
							<< " routeElemNo = " << (boost::uint32_t)j
							<< " is Graph? = " << (boost::uint32_t)route.routes[j].isGraph
							<< " graphID = " << (boost::uint32_t)route.routes[j].elem.graphID);
				}
			}
			LOG_DEBUG("contracts_and_routes_unser--> end 'route_elems_list' --------");
			
			switch(route.alternative)
			{
			case 0:
				{
					if ((int)packet->data.size() - offset < (int)(sizeof(unsigned short) + IPv6::SIZE))
						THROW_EXCEPTION0(util::NotEnoughBytesException);

					//no need
					//route.selectorn.contractID = util::binary_read<boost::uint16_t>(data, network);
					route.selectorn.contractID = htons(*((unsigned short*)(pData + offset)));
					offset += sizeof(unsigned short);

					//no need
					//unsigned char address[IPv6::SIZE];
					//util::binary_read_bytes(data, address, sizeof(address));
					//route.srcAddress = IPv6(address);
					//added
					route.srcAddress = IPv6(pData + offset);
					offset += IPv6::SIZE;
				}
				
				LOG_DEBUG("contracts_and_routes_unser - routeNo = " << (boost::uint32_t)i
							<< " route_id = " << (boost::uint32_t)route.index
							<< " alternative = " << (boost::uint32_t)route.alternative
							<< " forwardLimit = " << (boost::uint32_t)route.forwardLimit
							<< " contractID = " << (boost::uint32_t)route.selectorn.contractID
							<< " srcAddress = " << route.srcAddress.ToString());
				
				break;
			case 1:

				if ((int)packet->data.size() - offset < (int)sizeof(unsigned short))
						THROW_EXCEPTION0(util::NotEnoughBytesException);

				//no need
				//route.selectorn.contractID = util::binary_read<boost::uint16_t>(data, network);
				//added
				route.selectorn.contractID = htons(*((unsigned short*)(pData + offset)));
				offset += sizeof(unsigned short);
				
				LOG_DEBUG("contracts_and_routes_unser - routeNo = " << (boost::uint32_t)i
							<< " route_id = " << (boost::uint32_t)route.index
							<< " alternative = " << (boost::uint32_t)route.alternative
							<< " forwardLimit = " << (boost::uint32_t)route.forwardLimit
							<< " contractID = " << (boost::uint32_t)route.selectorn.contractID);
				break;
			case 2:
				
				if ((int)packet->data.size() - offset < (int)IPv6::SIZE)
					THROW_EXCEPTION0(util::NotEnoughBytesException);

				//no need
				//util::binary_read_bytes(data, route.selectorn.nodeAddress, sizeof(route.selectorn.nodeAddress));
				//added
				memcpy(route.selectorn.nodeAddress, pData + offset, sizeof(route.selectorn.nodeAddress));
				offset += IPv6::SIZE;

				if (Info.Altern2DestsList.insert(
							ContractsAndRoutes::Altern2DestsListT::value_type(
							IPv6(route.selectorn.nodeAddress), i)).second == false)
				{
					LOG_WARN("duplicated destAddress for alternative 2");
				}

				LOG_DEBUG("contracts_and_routes_unser - routeNo = " << (boost::uint32_t)i
							<< " route_id = " << (boost::uint32_t)route.index
							<< " alternative = " << (boost::uint32_t)route.alternative
							<< " forwardLimit = " << (boost::uint32_t)route.forwardLimit
							<< " nodeAddress = " << IPv6(route.selectorn.nodeAddress).ToString());
				break;
			case 3:
				LOG_DEBUG("contracts_and_routes_unser - routeNo = " << (boost::uint32_t)i
							<< " route_id = " << (boost::uint32_t)route.index
							<< " alternative = " << (boost::uint32_t)route.alternative
							<< " forwardLimit = " << (boost::uint32_t)route.forwardLimit);
				break;
			default:
				LOG_ERROR("invalid alternative received");
				continue;
			}
		}
		LOG_DEBUG("contracts_and_routes_unser--> end 'routes_list' --------");

	}

	//test
	WATCH_DURATION_DEF(oDurationWatcher);
}

//added by Cristian.Guef
static void FillPublishData(int respLength, ISACSInfo &csInfo, std::basic_istringstream<boost::uint8_t> &data, const util::NetworkOrder& network)
{
	 if (csInfo.m_objID != 129/*NIVIS_hardcoded*/)
	 {
		 csInfo.m_isISA = true;
		 csInfo.m_valueStatus = util::binary_read<boost::uint8_t>(data, network); //ISA standard
		 			 
		switch(respLength)
		{
	  	case 2 /*1 value_status + one byte*/:
			if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 2 /*1 value_status + one byte*/)
			{
				LOG_ERROR("ISACSRequest - unexpected data size");
				return;
			}		
			csInfo.m_rawData = util::binary_read<boost::uint8_t>(data, network);
	  		break;
	  	case 3 /*1 value_status + two bytes*/:
			if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 3 /*1 value_status + two bytes*/)
			{
				LOG_ERROR("ISACSRequest - unexpected data size");
				return;
			}
	  		csInfo.m_rawData = util::binary_read<boost::int16_t>(data, network);
	  		break;
	  	case 5 /*1 value_status + four bytes*/:
			if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 5 /*1 value_status + four bytes*/ )
			{
				LOG_ERROR("ISACSRequest - unexpected data size");
				return;
			}
	  		csInfo.m_rawData = util::binary_read<boost::uint32_t>(data, network);
	  		break;
	  	default:
	  		LOG_ERROR("ISACSRequest - unexpected data size");
	  		return;
		}	 
	 }
	 else
	 {
		csInfo.m_isISA = false;
		 
		switch(respLength)
		{
	  	case 1 /*one byte*/:
			if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 1 /*one byte*/)
			{
				LOG_ERROR("ISACSRequest - unexpected data size");
				return;
			}
	  		csInfo.m_rawData = util::binary_read<boost::int8_t>(data, network);
	  		break;
	  	case 2 /*two bytes*/:
			if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 2 /*two bytes*/)
			{
				LOG_ERROR("ISACSRequest - unexpected data size");
				return;
			}
	  		csInfo.m_rawData = util::binary_read<boost::int16_t>(data, network);
	  		break;
	  	case 4 /*four bytes*/:
			if ((data.str().size() - 1 /*status*/ - 8/*time*/ - 1 /*req_type*/ - 2/*objid*/ - 1 /*sfc*/ - 2 /*data_len*/ - 4 /*dataCRC*/) != 4 /*four bytes*/ )
			{
				LOG_ERROR("ISACSRequest - unexpected data size");
				return;
			}
	  		csInfo.m_rawData = util::binary_read<boost::uint32_t>(data, network);
	  		break;
	  	default:
	  		LOG_DEBUG("ReadMultipleObject - unexpected data size");
	  		return;
		}
		 
	 }
}
void GServiceUnserializer::Visit(GClientServer<GISACSRequest>& CSRequest)
{
	 if (packet->serviceType != gateway::GeneralPacket::GClientServerConfirm)
	  {
		  THROW_EXCEPTION1(SerializationException, "Expected ServiceType=ClientServerConfirm!");
	  }

	  Command::ResponseStatus st = VerifyReceivedPacket_v3(packet);
	  if (st != Command::rsSuccess)
	  {
		  CSRequest.Status = st;
		  return;
	  }

	  std::basic_istringstream<boost::uint8_t> data(packet->data);
	  const util::NetworkOrder& network = util::NetworkOrder::Instance();

	  /*status*/
	  CSRequest.Status = ParseClientServerCommunicationStatus(data, network);
	  if (CSRequest.Status != Command::rsSuccess)
	  {
		  /*nothing to parse*/
		  return;
	  }
	
	  //MyDateTime myDT;
	  //struct timeval tv;
	  CSRequest.Client.Info.tv.tv_sec = util::binary_read<boost::uint32_t>(data, network);
	  CSRequest.Client.Info.tv.tv_usec = util::binary_read<boost::uint32_t>(data, network); 
	  //gettime(myDT, tv);
	  //NO SM IMPLEMENTATION YET*/
	  //gettime(myDT);
	  
	  util::binary_read<boost::uint8_t>(data, network); //reqType
	  util::binary_read<boost::uint16_t>(data, network); //objID


	  //TODO: add management for extended sfcCode
	  boost::uint8_t sfCode = util::binary_read<boost::uint8_t>(data, network);
	  CSRequest.Status = (sfCode == 0)
	  ? Command::rsSuccess
	  : (Command::ResponseStatus) (Command::rsFailure_DeviceError - sfCode);

	  if (CSRequest.Status != Command::rsSuccess)
	  {
		  return;
	  }

	  if (CSRequest.Client.Info.m_reqType == 4 /*write*/)
	  {
		  return;
	  }

	  //CSRequest.Client.Info.m_readTime = myDT.ReadingTime;
	  //CSRequest.Client.Info.m_milisec = myDT.milisec;

    boost::uint16_t respLength = util::binary_read<boost::uint16_t>(data, network);
	
	if (CSRequest.Client.Info.m_ReadAsPublish == 1) //store also to DeviceReadings
		FillPublishData(respLength, CSRequest.Client.Info, data, network);

	for (int i = 0; i < respLength; i++)
	{
		unsigned char byte = util::binary_read<boost::uint8_t>(data, network);
		char szByte[20];
		sprintf(szByte, "%02X", byte);
		CSRequest.Client.Info.m_strRespDataBuff += szByte;
	}
	  
}




} /*namspace hostapp*/
} /*namspace nisa100*/
