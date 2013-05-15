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

#include "../../util/serialization/Serialization.h"
#include "../../util/serialization/NetworkOrder.h"
#include "../commandmodel/Nisa100ObjectIDs.h"
#include "../model/Device.h"

#include <sstream>

//added by Cristian.Guef
#include <string>
#include <arpa/inet.h>

//added by Cristian.Guef
#include "../../gateway/crc32c.h"


namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
extern unsigned int MonitH_GW_SessionID; //only for DeviceListReport and NetworkListReport


void GServiceSerializer::Serialize(AbstractGService& request_, gateway::GeneralPacket& packet_)
{
	packet = &packet_;

	//serialize data & complete serviceType
	request_.Accept(*this);
}

void GServiceSerializer::Visit(GTopologyReport& topologyReport)
{
	packet->serviceType = gateway::GeneralPacket::GTopologyReportRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//no data to send.
	packet->dataSize = 0;
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GDeviceListReport& deviceListReport)
{
	packet->serviceType = gateway::GeneralPacket::GDeviceListReportRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//no data to send.
	packet->dataSize = 0;
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GNetworkHealthReport& networkHealthReport)
{
	packet->serviceType = gateway::GeneralPacket::GNetworkHealthReportRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//no data to send.
	packet->dataSize = 0;
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GScheduleReport& scheduleReport)
{
	
	packet->serviceType = gateway::GeneralPacket::GScheduleReportRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//
	std::basic_ostringstream<boost::uint8_t> data;
	data.write((boost::uint8_t*)scheduleReport.m_forDeviceIP.Address(), IPv6::SIZE);

	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GNeighbourHealthReport& neighbourHealthReport)
{
	packet->serviceType = gateway::GeneralPacket::GNeighbourHealthReportRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//
	std::basic_ostringstream<boost::uint8_t> data;
	data.write((boost::uint8_t*)neighbourHealthReport.m_forDeviceIP.Address(), IPv6::SIZE);

	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GDeviceHealthReport& devHealthReport)
{
	packet->serviceType = gateway::GeneralPacket::GDeviceHealthReportRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//
	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, (boost::uint16_t)devHealthReport.m_forDeviceIPs.size(), util::NetworkOrder::Instance());
	for (unsigned int i = 0; i < devHealthReport.m_forDeviceIPs.size(); i++)
	{
		data.write((boost::uint8_t*)devHealthReport.m_forDeviceIPs[i].Address(), IPv6::SIZE);
	}
	

	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GNetworkResourceReport& netResourceReport)
{
	packet->serviceType = gateway::GeneralPacket::GNetworkResourceRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//no data to send.
	packet->dataSize = 0;
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GAlert_Subscription& alertSubscription)
{
	packet->serviceType = gateway::GeneralPacket::GAlertSubscriptionRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//
	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, alertSubscription.LeaseID, util::NetworkOrder::Instance());
		
	for (unsigned int i = 0; i < alertSubscription.CategoryTypeList.size(); i++)
	{
		util::binary_write(data, (boost::uint8_t)1/*category*/, util::NetworkOrder::Instance());
		util::binary_write(data, alertSubscription.CategoryTypeList[i].Subscribe, util::NetworkOrder::Instance());
		util::binary_write(data, alertSubscription.CategoryTypeList[i].Enable, util::NetworkOrder::Instance());
		util::binary_write(data, alertSubscription.CategoryTypeList[i].Category, util::NetworkOrder::Instance());
	}
	
	for (unsigned int i = 0; i < alertSubscription.NetAddrTypeList.size(); i++)
	{
		util::binary_write(data, (boost::uint8_t)2/*network address*/, util::NetworkOrder::Instance());
		util::binary_write(data, alertSubscription.NetAddrTypeList[i].Subscribe, util::NetworkOrder::Instance());
		util::binary_write(data, alertSubscription.NetAddrTypeList[i].Enable, util::NetworkOrder::Instance());
		data.write((boost::uint8_t*)alertSubscription.NetAddrTypeList[i].DevAddr.Address(), IPv6::SIZE);
		util::binary_write(data, alertSubscription.NetAddrTypeList[i].EndPointPort, util::NetworkOrder::Instance());
		util::binary_write(data, alertSubscription.NetAddrTypeList[i].EndObjID, util::NetworkOrder::Instance());
		util::binary_write(data, alertSubscription.NetAddrTypeList[i].AlertType, util::NetworkOrder::Instance());
	}

	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GAlert_Indication& alertIndication)
{
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GDelContract& contract)
{

	//----header
	packet->serviceType = gateway::GeneralPacket::GContractRequest;
	
	packet->sessionID = MonitH_GW_SessionID;

	//----data
	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, contract.ContractID, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint32_t)0 /*delete contract*/, util::NetworkOrder::Instance());

	util::binary_write(data, (boost::uint8_t)contract.ContractType, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint8_t)contract.m_ucProtocolType, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint8_t)contract.m_ucEndPointsNo, util::NetworkOrder::Instance());

	//pt test -hardcoded
	//boost::uint8_t address[IPv6::SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	//data.write((boost::uint8_t*)IPv6(address).Address(), IPv6::SIZE);
	
	data.write((boost::uint8_t*)contract.IPAddress.Address(), IPv6::SIZE);

	util::binary_write(data, (boost::uint16_t)(contract.m_unResourceID & 0xFFFF), util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint16_t)(contract.m_unResourceID >> 16), util::NetworkOrder::Instance());

	//localPort and local obj
	util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
	
	//added by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());


	//szconn - added by Cristian.Guef
	util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
	
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

void GServiceSerializer::Visit(GContract& contract)
{
	//----header
	packet->serviceType = gateway::GeneralPacket::GContractRequest;
	
	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	//----data
	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, contract.ContractID, util::NetworkOrder::Instance());
	util::binary_write(data, contract.ContractPeriod, util::NetworkOrder::Instance());

	//added by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)contract.ContractType, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint8_t)contract.m_ucProtocolType, util::NetworkOrder::Instance());
	
	
	if (contract.ContractType == GContract::Alert_Subscription)
	{
		util::binary_write(data, (boost::uint8_t)1, util::NetworkOrder::Instance());

		boost::uint8_t address[IPv6::SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		data.write((boost::uint8_t*)IPv6(address).Address(), IPv6::SIZE);
	
		//added by Cristian.Guef
		util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
	}
	else
	{
		util::binary_write(data, (boost::uint8_t)contract.m_ucEndPointsNo, util::NetworkOrder::Instance());

		//pt test -hardcoded
		//boost::uint8_t address[IPv6::SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		//data.write((boost::uint8_t*)IPv6(address).Address(), IPv6::SIZE);
		
		data.write((boost::uint8_t*)contract.IPAddress.Address(), IPv6::SIZE); //FIXME [nicu.dascalu] - ???
	
		//added by Cristian.Guef
		util::binary_write(data, (boost::uint16_t)(contract.m_unResourceID & 0xFFFF), util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint16_t)(contract.m_unResourceID >> 16), util::NetworkOrder::Instance());
	}
	
	//localPort and local obj
	util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());

	//added by Cristian.Guef
	if (contract.ContractType == GContract::Client || contract.ContractType == GContract::Subscriber
						|| contract.ContractType == GContract::Alert_Subscription)
		util::binary_write(data, (boost::uint8_t)contract.m_ucTransferMode, util::NetworkOrder::Instance());
	else
		util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());

	//added by Cristian.Guef
	if(contract.ContractType == GContract::Subscriber)
	{
		util::binary_write(data, (boost::uint8_t)contract.m_ucUpdatePolicy, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint16_t)contract.m_wSubscriptionPeriod, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint8_t)contract.m_ucPhase, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint8_t)contract.m_ucStaleLimit, util::NetworkOrder::Instance());
	}
	else
	{
		util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());
		if (contract.ContractType == GContract::Client || contract.ContractType == GContract::BulkTransforClient)
			util::binary_write(data, (boost::uint16_t)contract.m_committedBurst, util::NetworkOrder::Instance());
		else
			util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
		
		util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());
	}

	//szconn - added by Cristian.Guef
	util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());	

	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

//added by Cristian.guef
void GServiceSerializer::Visit(GSession& session)
{
	//header
	packet->serviceType = gateway::GeneralPacket::GSessionRequest;
	packet->sessionID = 0; //create a new one
	

	//data
	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, (boost::uint32_t)session.m_nSessionPeriod, util::NetworkOrder::Instance());
	util::binary_write(data, (boost::uint16_t)session.m_uwNetworkID, util::NetworkOrder::Instance());
	
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	
	packet->data = data.str();
	packet->dataSize = packet->data.size();

}

void GServiceSerializer::Visit(GBulk& bulk)
{
	/* commented by Cristian.Guef
	packet->serviceType = gateway::GeneralPacket::GBulkRequest;
	*/
	
	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, bulk.ContractID, util::NetworkOrder::Instance());
	
	//added by Cristian.Guef
	util::binary_write(data, (boost::uint32_t)bulk.bulkDialogID, util::NetworkOrder::Instance());
	
	
	//added by Cristian.Guef
	switch (bulk.m_currentBulkState)
	{
	case GBulk::BulkOpen:
		packet->serviceType = gateway::GeneralPacket::GBulkOpenRequest;

		util::binary_write(data, (boost::uint8_t)0/*hardcoded - download type*/, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint16_t)0/*hardcoded - block size (must be returned)*/, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint32_t)bulk.Data.size(), util::NetworkOrder::Instance());
		
		if (bulk.type == 0/*with sm*/)
		{
			util::binary_write(data, (boost::uint8_t)1/* hardcoded - "file" resource type*/, util::NetworkOrder::Instance());
			util::binary_write(data, (boost::uint8_t)bulk.FileName.size(), util::NetworkOrder::Instance());
			data.write((boost::uint8_t*)bulk.FileName.c_str(), bulk.FileName.size());
		}
		else
		{
			util::binary_write(data, (boost::uint8_t)2/* hardcoded - "raw" resource type*/, util::NetworkOrder::Instance());
		}
		
		break;

	case GBulk::BulkTransfer:
		packet->serviceType = gateway::GeneralPacket::GBulkTransferRequest;

		util::binary_write(data, (boost::uint16_t)(bulk.currentBlockCount + 1), util::NetworkOrder::Instance());
		data.write((boost::uint8_t*)bulk.Data.c_str(), bulk.currentBlockSize);

		//remove data sent
		bulk.Data.erase(0,bulk.currentBlockSize);
		
		LOG_DEBUG("Bulk_transfer --maxBlockSize = " << 
				(boost::uint32_t)bulk.maxBlockSize << 
				" and currentBlockSize = " << 
				(boost::uint32_t)bulk.currentBlockSize <<
				" and maxBlockCount = " << 
				(boost::uint32_t)bulk.maxBlockCount <<
				" currentBlockCount = " <<
				(boost::uint32_t)bulk.currentBlockCount );
		
		break;
	case GBulk::BulkEnd:
		packet->serviceType = gateway::GeneralPacket::GBulkCloseRequest;
		break;
	}
	


	/* commented by Cristian.Guef
	util::binary_write(data, bulk.TLDE_SAPID, util::NetworkOrder::Instance());
	
	util::binary_write(data, (boost::uint8_t)bulk.FileName.size(), util::NetworkOrder::Instance());
	data.write((boost::uint8_t*)bulk.FileName.c_str(), bulk.FileName.size());
	
	data.write((boost::uint8_t*)bulk.Data.c_str(), bulk.Data.size());
	util::binary_write(data, (boost::uint8_t)0, util::NetworkOrder::Instance());
	*/


	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();	
}

void GServiceSerializer::Visit(GClientServer<WriteObjectAttribute>& writeAttribute)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, writeAttribute.ContractID, network);

	//added by Cristian.Guef
	boost::uint8_t buffer = 0;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);


	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)writeAttribute.Client.channel.mappedTSAPID, network);
	*/

	util::binary_write(data, (boost::uint8_t)oatWriteRequest, network);

	util::binary_write(data, (boost::uint16_t)writeAttribute.Client.channel.mappedObjectID, network);
	
	util::binary_write(data, (boost::uint16_t)writeAttribute.Client.channel.mappedAttributeID, network);
	
	//added by Cristian.Guef
	boost::uint16_t attributeIndex1 = 0;
	util::binary_write(data, attributeIndex1, network);
	boost::uint16_t attributeIndex2 = 0;
	util::binary_write(data, attributeIndex2, network);
	
	//added by Cristian.Guef
	boost::uint16_t requestLen = 0;

	switch (writeAttribute.Client.writeValue.dataType)
	{
	case ChannelValue::cdtUInt8:
		//added by Cristian.Guef
		requestLen = 1;
		util::binary_write(data, requestLen, network);

		util::binary_write(data, writeAttribute.Client.writeValue.value.uint8, network);
		break;
	case ChannelValue::cdtInt8:
		//added by Cristian.Guef
		requestLen = 1;
		util::binary_write(data, requestLen, network);

		util::binary_write(data, writeAttribute.Client.writeValue.value.int8, network);
		break;
	case ChannelValue::cdtUInt16:
		//added by Cristian.Guef
		requestLen = 2;
		util::binary_write(data, requestLen, network);

		util::binary_write(data, writeAttribute.Client.writeValue.value.uint16, network);
		break;
	case ChannelValue::cdtInt16:
			//added by Cristian.Guef
			requestLen = 2;
			util::binary_write(data, requestLen, network);

			util::binary_write(data, writeAttribute.Client.writeValue.value.int16, network);
			break;
	case ChannelValue::cdtUInt32:
			//added by Cristian.Guef
			requestLen = 4;
			util::binary_write(data, requestLen, network);

			util::binary_write(data, writeAttribute.Client.writeValue.value.uint32, network);
			break;
	case ChannelValue::cdtInt32:
			//added by Cristian.Guef
			requestLen = 4;
			util::binary_write(data, requestLen, network);

			util::binary_write(data, writeAttribute.Client.writeValue.value.int32, network);
			break;
	//TODO:-check this
	//case ChannelValue::cdtFloat32:
	//			util::binary_write(data, writeAttribute.Client.writeValue.value.float32, network);
	//			break;
	default:
		assert("invalid ConfirmValueSize!" && false);
	}

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/


	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

void GServiceSerializer::Visit(GClientServer<ReadMultipleObjectAttributes>& multipleReadObjectAttributes)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, multipleReadObjectAttributes.ContractID, network);

	//added by Cristian.Guef
	boost::uint8_t buffer = 0;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)multipleReadObjectAttributes.Client.TSAPID, network);
	*/
	
	for (ReadMultipleObjectAttributes::AttributesList::const_iterator it =
			multipleReadObjectAttributes.Client.attributes.begin(); it
			!= multipleReadObjectAttributes.Client.attributes.end(); it++)
	{
		util::binary_write(data, (boost::uint8_t)oatReadRequest, network);		
		util::binary_write(data, (boost::uint16_t)it->channel.mappedObjectID, network);
		util::binary_write(data, (boost::uint16_t)it->channel.mappedAttributeID, network);

		//added by Cristian.Guef
		boost::uint16_t attributeIndex1 = 0;
		util::binary_write(data, attributeIndex1, network);
		boost::uint16_t attributeIndex2 = 0;
		util::binary_write(data, attributeIndex2, network);
	}

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/

	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

void GServiceSerializer::Visit(PublishSubscribe& publishSubscribe)
{
	THROW_EXCEPTION1(SerializationException, "PublishSubscribe is not seriaalizable!");
}

void GServiceSerializer::Visit(GClientServer<Publish>& publish)
{
	
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, publish.ContractID, network);

	
	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)publish.Client.ps->PublisherChannel.mappedTSAPID, network);


	 commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);

	util::binary_write(data, (boost::uint16_t)publish.Client.ps->PublisherChannel.mappedObjectID, network);
	
	util::binary_write(data, (boost::uint16_t)PUBLISHER_PUBLISH_METHOD, network);
	


	 Commented by Cristian.Guef
	 
	 - UInt8 publisherCannelID
	 - UInt8 subscriberChannelID
	 - 64-bit EUI64SubscriberAddress
	 - UInt16 period
	 - UInt16 phase
	
        util::binary_write(data, (boost::uint8_t)publish.Client.ps->PublisherChannel.mappedAttributeID, network);
	util::binary_write(data, (boost::uint8_t)publish.Client.ps->SubscriberChannel.mappedAttributeID, network);
	data.write((boost::uint8_t*)publish.Client.ps->SubscriberMAC.Address(), MAC::SIZE);
	util::binary_write(data, (boost::uint16_t)publish.Client.ps->Period, network);
	util::binary_write(data, (boost::uint16_t)publish.Client.ps->Phase, network);
	  end comment 


	commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}



void GServiceSerializer::Visit(GClientServer<Subscribe>& subscribe)
{
	
	//added by Cristian.Guef
	if (subscribe.Client.m_currentSubscribeState == Subscribe::SubscribeDoLeaseSubscriber)
	{
		//----header
		packet->serviceType = gateway::GeneralPacket::GContractRequest;
		
		packet->sessionID = MonitH_GW_SessionID;

		//----data
		std::basic_ostringstream<boost::uint8_t> data;
		util::binary_write(data, (boost::uint32_t)0 /*new lease id*/, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint32_t)0 /*infinite*/, util::NetworkOrder::Instance());

		util::binary_write(data, (boost::uint8_t)GContract::Subscriber, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint8_t)0 /*protocol type*/, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint8_t)1 /*no. of endpoints*/, util::NetworkOrder::Instance());


		data.write((boost::uint8_t*)subscribe.Client.m_handedPublisherIP.Address(), IPv6::SIZE); //FIXME [nicu.dascalu] - ???

		util::binary_write(data, (boost::uint16_t)subscribe.Client.ps->PublisherChannel.mappedTSAPID, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint16_t)subscribe.Client.ps->PublisherChannel.mappedObjectID, util::NetworkOrder::Instance());

		//localPort and local obj
		util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());

		util::binary_write(data, (boost::uint8_t)1 /* transfer mode - hardcoded as create_contrat*/, util::NetworkOrder::Instance());

		util::binary_write(data, (boost::uint8_t)1 /* update policy - hardcoded as create_contrat*/, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint16_t)subscribe.Client.ps->Period, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint8_t)subscribe.Client.ps->Phase, util::NetworkOrder::Instance());
		util::binary_write(data, (boost::uint8_t)subscribe.Client.ps->StaleLimit, util::NetworkOrder::Instance());
		

		//szconn - added by Cristian.Guef
		util::binary_write(data, (boost::uint16_t)0, util::NetworkOrder::Instance());	

		std::basic_string<boost::uint8_t> my_string(data.str());
		unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
		dataCRC = htonl(dataCRC);
		data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

		packet->data = data.str();
		packet->dataSize = packet->data.size();
		return;
	}

	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

//	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	/*commented by Cristian.Guef
	util::binary_write(data, subscribe.ContractID, network);
	*/ 

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)apUAP_EvalKits, network);
	*/

	
	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);

	util::binary_write(data, (boost::uint16_t)SUBSCRIBER_OBJECT, network);
	
	util::binary_write(data, (boost::uint16_t)SUBSCRIBER_SUBSCRIBE_METHOD, network);
	*/

	//commneted by Cristian.Guef
	/*
	 -	UInt8 publisherChannelID - added since 1.0.2
	 - UInt8 subscriberChannelID - added since 1.0.2

	 - 64-bit EUI64PublisherAddress
	 - UInt16 period
	 - UInt16 phase
	 - UInt32 lowThreshold
	 - UInt32 highThreshold
	 */
/*	util::binary_write(data, (boost::uint8_t)subscribe.Client.ps->PublisherChannel.mappedAttributeID, network);
	util::binary_write(data, (boost::uint8_t)subscribe.Client.ps->SubscriberChannel.mappedAttributeID, network);
	data.write((boost::uint8_t*)subscribe.Client.ps->PublisherMAC.Address(), MAC::SIZE);
	util::binary_write(data, (boost::uint16_t)subscribe.Client.ps->Period, network);
	util::binary_write(data, (boost::uint16_t)subscribe.Client.ps->Phase, network);
	util::binary_write(data, (boost::uint32_t)subscribe.Client.ps->SubscriberLowThreshold, network);
	util::binary_write(data, (boost::uint32_t)subscribe.Client.ps->SubscriberHighThreshold, network);
	---end commented */

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}


//added by Cristian.Guef
void GServiceSerializer::Visit(GClientServer<GetSizeForPubIndication>& getSizeForPubIndication)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, getSizeForPubIndication.ContractID, network);

	boost::uint8_t buffer = 1;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	util::binary_write(data, (boost::uint8_t)oatReadRequest, network);		
	util::binary_write(data, (boost::uint16_t)getSizeForPubIndication.Client.m_Concentrator_id, network);
	util::binary_write(data, (boost::uint16_t)6/*array of ObjID,AttrID,Index,Size*/, network);

	boost::uint16_t attributeIndex1 = 0/*entire array*/;
	util::binary_write(data, attributeIndex1, network);
	boost::uint16_t attributeIndex2 = 0;
	util::binary_write(data, attributeIndex2, network);
	

	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

void GServiceSerializer::Visit(PublishIndication& publishIndication)
{
	THROW_EXCEPTION1(SerializationException, "PublishIndication is not seriaalizable!");
}

void GServiceSerializer::Visit(GClientServer<GetFirmwareVersion>& getFirmwareVersion)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, getFirmwareVersion.ContractID, network);

	//added by Cristian.Guef
	boost::uint8_t buffer = 1;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)apDMAP, network);
	*/

	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);

	util::binary_write(data, (boost::uint16_t)UPLOAD_DOWNLOAD_OBJECT, network);
	
	util::binary_write(data, (boost::uint16_t)GET_FIRMWAREVERSION_METHOD, network);

	//added by Cristian.Guef
	boost::uint16_t requestLen = MAC::SIZE;
	util::binary_write(data, requestLen, network);

	//- 64-bit EUI64 Device Address
	data.write((boost::uint8_t*)getFirmwareVersion.Client.DeviceAddress.Address(), MAC::SIZE);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/

	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();

}

void GServiceSerializer::Visit(GClientServer<FirmwareUpdate>& startFirmwareUpdate)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, startFirmwareUpdate.ContractID, network);

	//added by Cristian.Guef
	boost::uint8_t buffer = 1;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)apDMAP, network);
	*/

	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);

	util::binary_write(data, (boost::uint16_t)UPLOAD_DOWNLOAD_OBJECT, network);
	
	util::binary_write(data, (boost::uint16_t)START_FIRMWAREUPDATE_METHOD, network);

	//added by Cristian.Guef
	boost::uint16_t requestLen = MAC::SIZE + 16 + 
		(boost::uint16_t)startFirmwareUpdate.Client.FirmwareFileName.length();
	util::binary_write(data, requestLen, network);

	//- 64-bit EUI64 Device Address
	data.write((boost::uint8_t*)startFirmwareUpdate.Client.DeviceAddress.Address(), MAC::SIZE);
	boost::uint16_t fileNameLength = (boost::uint16_t)startFirmwareUpdate.Client.FirmwareFileName.length();
	util::binary_write(data, fileNameLength, network);
	data.write((boost::uint8_t*)startFirmwareUpdate.Client.FirmwareFileName.c_str(), fileNameLength);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/

	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	packet->data = data.str();
	packet->dataSize = packet->data.size();

}

void GServiceSerializer::Visit(GClientServer<GetFirmwareUpdateStatus>& getFirmwareUpdateStatus)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, getFirmwareUpdateStatus.ContractID, network);
	
	//added by Cristian.Guef
	boost::uint8_t buffer = 1;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)apDMAP, network);
	*/

	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);

	util::binary_write(data, (boost::uint16_t)UPLOAD_DOWNLOAD_OBJECT, network);
	
	util::binary_write(data, (boost::uint16_t)GET_FIRMWAREUPDATESTATUS_METHOD, network);

	//added by Cristian.Guef
	boost::uint16_t requestLen = MAC::SIZE;
	util::binary_write(data, requestLen, network);

	//- 64-bit EUI64 Device Address
	data.write((boost::uint8_t*)getFirmwareUpdateStatus.Client.DeviceAddress.Address(), MAC::SIZE);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/

	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();

}

void GServiceSerializer::Visit(GClientServer<CancelFirmwareUpdate>& cancelFirmwareUpdate)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, cancelFirmwareUpdate.ContractID, network);

	//added by Cristian.Guef
	boost::uint8_t buffer = 1;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	/*commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)apDMAP, network);
	*/

	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);

	util::binary_write(data, (boost::uint16_t)UPLOAD_DOWNLOAD_OBJECT, network);
	
	util::binary_write(data, (boost::uint16_t)CANCEL_FIRMWAREUPDATE_METHOD, network);

	//added by Cristian.Guef
	boost::uint16_t requestLen = MAC::SIZE;
	util::binary_write(data, requestLen, network);

	//- 64-bit EUI64 Device Address
	data.write((boost::uint8_t*)cancelFirmwareUpdate.Client.DeviceAddress.Address(), MAC::SIZE);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/

	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();

}


//added by Cristian.Guef
void GServiceSerializer::Visit(GClientServer<GetContractsAndRoutes>& getContractsAndRoutes)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, getContractsAndRoutes.ContractID, network);

	boost::uint8_t buffer = 1;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);
	util::binary_write(data, (boost::uint16_t)SYSTEM_MONITORING_OBJECT, network);
	util::binary_write(data, (boost::uint16_t)GET_CONTRACTSANDROUTES_METHOD, network);

	boost::uint16_t requestLen = 0;
	util::binary_write(data, requestLen, network);

	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

//added by Cristian.Guef
void GServiceSerializer::Visit(GClientServer<GISACSRequest>& CSRequest)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, CSRequest.ContractID, network);

	boost::uint8_t buffer = 0;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	util::binary_write(data, (boost::uint8_t)CSRequest.Client.Info.m_reqType, network);
	util::binary_write(data, (boost::uint16_t)CSRequest.Client.Info.m_objID, network);
	util::binary_write(data, (boost::uint16_t)CSRequest.Client.Info.m_objResID, network);

	if (CSRequest.Client.Info.m_reqType != oatExecuteRequest)
	{
		util::binary_write(data, (boost::uint16_t)CSRequest.Client.Info.m_attrIndex1, network);
		util::binary_write(data, (boost::uint16_t)CSRequest.Client.Info.m_attrIndex2, network);
	}

	if (CSRequest.Client.Info.m_reqType != oatReadRequest)
	{
		boost::uint16_t requestLen = CSRequest.Client.Info.m_strReqDataBuff.size()/2;
		util::binary_write(data, requestLen, network);

		//- write buffer
		for (unsigned int i = 0; i < CSRequest.Client.Info.m_strReqDataBuff.size(); i+=2)
		{
			std::string tmp;
			tmp.push_back(CSRequest.Client.Info.m_strReqDataBuff[i]);
			tmp.push_back(CSRequest.Client.Info.m_strReqDataBuff[i+1]);
			
			boost::uint16_t val;
			std::istringstream hexstring(tmp);
			hexstring >> std::hex >> val;

			util::binary_write(data, (boost::uint8_t)val, network);
		}
	}

	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

void GServiceSerializer::Visit(GClientServer<ResetDevice>& resetDevice)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, resetDevice.ContractID, network);

	//added by Cristian.Guef
	boost::uint8_t buffer = 0;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)apUAP1, network);
	*/

	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);

	util::binary_write(data, (boost::uint16_t)SYSTEM_MONITORING_OBJECT, network);
	
	util::binary_write(data, (boost::uint16_t)RESET_DEVICE_METHOD, network);

	//added by Cristian.Guef
	boost::uint16_t requestLen = IPv6::SIZE + sizeof(char);
	util::binary_write(data, requestLen, network);

	//- 128-bit Device Address
	data.write((boost::uint8_t*)resetDevice.Client.DeviceIP.Address(), IPv6::SIZE);

	//added by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)resetDevice.Client.restart_type, network);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)0, network); //Cache = 0
	*/

	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

void GServiceSerializer::Visit(GSensorFrmUpdateCancel &sensorFrmUpdateCancel)
{
	packet->sessionID = MonitH_GW_SessionID;

	std::basic_ostringstream<boost::uint8_t> data;
	util::binary_write(data, sensorFrmUpdateCancel.ContractID, util::NetworkOrder::Instance());
	
	//added by Cristian.Guef
	util::binary_write(data, (boost::uint32_t)sensorFrmUpdateCancel.bulkDialogID, util::NetworkOrder::Instance());
	
	packet->serviceType = gateway::GeneralPacket::GBulkCloseRequest;
	
	//added by Cristian.Guef
	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	

	packet->data = data.str();
	packet->dataSize = packet->data.size();	
}

void GServiceSerializer::Visit(GClientServer<GetChannelsStatistics>& getChannels)
{
	packet->serviceType = gateway::GeneralPacket::GClientServerRequest;

	//added by Cristian.Guef
	packet->sessionID = MonitH_GW_SessionID;

	const util::NetworkOrder& network = util::NetworkOrder::Instance();
	std::basic_ostringstream<boost::uint8_t> data;

	util::binary_write(data, getChannels.ContractID, network);

	//added by Cristian.Guef
	boost::uint8_t buffer = 0;
	util::binary_write(data, buffer, network);
	boost::uint8_t TransferMode = 0;
	util::binary_write(data, TransferMode, network);

	/* commented by Cristian.Guef
	util::binary_write(data, (boost::uint8_t)apUAP1, network);
	*/

	util::binary_write(data, (boost::uint8_t)oatExecuteRequest, network);

	util::binary_write(data, (boost::uint16_t)SYSTEM_MONITORING_OBJECT, network);
	
	util::binary_write(data, (boost::uint16_t)GET_CHANNELS_STATISTICS, network);

	//added by Cristian.Guef
	boost::uint16_t requestLen = IPv6::SIZE;
	util::binary_write(data, requestLen, network);

	//- 128-bit Device Address
	data.write((boost::uint8_t*)getChannels.Client.DeviceIP.Address(), IPv6::SIZE);

	std::basic_string<boost::uint8_t> my_string(data.str());
	unsigned int dataCRC = GenerateCRC32C(my_string.c_str(), my_string.size());
	dataCRC = htonl(dataCRC);
	data.write((boost::uint8_t*)&dataCRC, sizeof(unsigned int));
	
	packet->data = data.str();
	packet->dataSize = packet->data.size();
}

}//namespace hostapp
} //namespace nisa100
