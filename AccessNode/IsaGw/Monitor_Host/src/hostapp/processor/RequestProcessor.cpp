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

#include "RequestProcessor.h"
#include "../CommandsProcessor.h"

//added by Cristian.Guef
#include "../commandmodel/Nisa100ObjectIDs.h"

#include <fstream>
#include <nlib/exception.h>

namespace nisa100 {
namespace hostapp {


//added by Cristian.Guef
extern unsigned int MonitH_GW_SessionID;  //for BulkTransfer

//added by Cristian.Guef
static unsigned int TopoDiagID = 0;

extern void AddBulkInProgress(int deviceID, unsigned short port, unsigned short objID, int cmdID);


void RequestProcessor::Process(AbstractGServicePtr service_, Command& command_, CommandsProcessor& processor_)
{
	command = &command_;
	processor = &processor_;
	service = service_;

	service->Accept(*this);
}

void RequestProcessor::Visit(GTopologyReport& topologyReport)
{
	//added by Cristian.Guef
	TopoDiagID++;
	topologyReport.m_DiagID = TopoDiagID;
	
	processor->SendRequest(*command, service);

	//added by Cristian.Guef
	AbstractGServicePtr ptrDevListReq(new GDeviceListReport(TopoDiagID));
	ptrDevListReq->CommandID = command->commandID;
	processor->SendRequest(*command, ptrDevListReq);
}

//added by Cristian.Guef
void RequestProcessor::Visit(GDeviceListReport& deviceListReport)
{
	processor->SendRequest(*command, service);
}
void RequestProcessor::Visit(GNetworkHealthReport& networkListReport)
{
	processor->SendRequest(*command, service);
}

void RequestProcessor::Visit(GScheduleReport& scheduleReport)
{

	DevicePtr Device = processor->devices.FindDevice(scheduleReport.m_devID);
	if (!Device)
	{
		THROW_EXCEPTION1(DeviceNotFoundException, scheduleReport.m_devID);
	}
	scheduleReport.m_forDeviceIP = Device->IP();

	processor->SendRequest(*command, service);
}
void RequestProcessor::Visit(GNeighbourHealthReport& neighbourHealthReport)
{
	DevicePtr Device = processor->devices.FindDevice(neighbourHealthReport.m_devID);
	if (!Device)
	{
		THROW_EXCEPTION1(DeviceNotFoundException, neighbourHealthReport.m_devID);
	}
	neighbourHealthReport.m_forDeviceIP = Device->IP();

	processor->SendRequest(*command, service);
}
void RequestProcessor::Visit(GDeviceHealthReport& devHealthReport)
{
	for (unsigned int i = 0; i < devHealthReport.m_devIDs.size(); i++)
	{
		DevicePtr Device = processor->devices.FindDevice(devHealthReport.m_devIDs[i]);
		if (!Device)
		{
			THROW_EXCEPTION1(DeviceNotFoundException, devHealthReport.m_devIDs[i]);
		}
		devHealthReport.m_forDeviceIPs.push_back(Device->IP());
	}

	//de test
	//if (devHealthReport.m_devIDs.size() != 61)
		//printf("error");
	
	processor->SendRequest(*command, service);
}
void RequestProcessor::Visit(GNetworkResourceReport& netResourceReport)
{
	processor->SendRequest(*command, service);
}

void RequestProcessor::Visit(GAlert_Subscription& alertSubscription)
{
	processor->SendRequest(*command, service);
}
void RequestProcessor::Visit(GAlert_Indication& alertIndication)
{
	//no neeed 
}

//added by Cristian.Guef
void RequestProcessor::Visit(GDelContract& contract)
{

	DoDeleteLeases::InfoForDelContract info = processor->GetInfoForDelLease(contract.ContractID);
	
	if (info.ResourceID == 0)
	{
		/* it was before
		LOG_ERROR(" request to delete lease for lease_id =" 
					<< (boost::uint32_t)contract.ContractID  
					<< "-> has been made with resourceID = 0");
		THROW_EXCEPTION1(nlib::Exception, "lease not found!");
		*/
		/*the case for bulk and C/S*/
		processor->SendRequest(*command, service);
		return;
	}

	//data to delete
	contract.IPAddress = info.IPAddress;
	contract.ContractType = info.LeaseType;
	contract.m_unResourceID = info.ResourceID;

	LOG_INFO("req to delete lease for device with ip = " 
			<< contract.IPAddress.ToString()
			<< "and dev_id = " << (boost::int32_t)info.DevID
			<< "lease_id = "
			<< (boost::uint32_t)contract.ContractID
			<< "obj_id = "
			<< (boost::uint16_t)(contract.m_unResourceID >> 16)
			<< "tlsap_id = "
			<< (boost::uint16_t)(contract.m_unResourceID & 0xFFFF)
			<< "lease_type = "
			<< (boost::uint16_t)contract.ContractType);

	processor->SendRequest(*command, service);

}

void RequestProcessor::Visit(GContract& contract)
{
	DevicePtr device;
	if (contract.ContractType == GContract::Alert_Subscription)
	{
		device = processor->devices.FindDevice(command->deviceID);
		if (!device)
		{
			THROW_EXCEPTION1(DeviceNotFoundException, command->deviceID);
		}
	}
	else
	{
		device = GetRegisteredDevice(command->deviceID);
	}


	contract.ContractID = 0; //new contract
	
	contract.ContractPeriod = 0;

	contract.IPAddress = device->IP();

	/* commented by Cristian.Guef -> it is already set
	contract.ContractType = GContract::Client;
	*/
	
	//added by Cristian.Guef
	contract.m_ucTransferMode = 0;   //it's not yet specified
	contract.m_ucEndPointsNo = 1;
	contract.m_ucProtocolType = 0;

	//added by Cristian.Guef
	if (contract.ContractType == GContract::Subscriber)
	{
		//added by Cristian.Guef
		/* must be set when configuring publish-subscribe enviroment */
		contract.m_ucUpdatePolicy = 1;
		contract.m_ucStaleLimit = 3;

		//added by Cristian.Guef
		/*
		m_ucPhase - it comes already set when configuring publish-subscribe enviroment
		m_uwSubscriptionPeriod = it comes already set when configuring publish-subscribe enviroment
		*/
	}

	processor->SendRequest(*command, service);

	/* commented by Cristian.Guef
	device->WaitForContract(true);
	*/
	//add by Cristian.Guef
	device->WaitForContract(contract.m_unResourceID, contract.ContractType,  true);
}

//added by Cristian.guef
void RequestProcessor::Visit(GSession& session)
{
	
	session.m_nSessionPeriod = -1;

	processor->SendRequest(service);
}

void RequestProcessor::Visit(GBulk& bulk)
{
	//FIXME [nicu.dascalu] - use path combine ...
	std::basic_ostringstream<char> fullPath;
	fullPath << processor->configApp.PathToFirmwareFiles() << bulk.FileName;

	//TODO [nicu.dascalu] - add specialized exptionsss
	std::ifstream file(fullPath.str().c_str(), std::fstream::binary);
	if (!file)
	{
		LOG_ERROR("File not found=" << fullPath);
		THROW_EXCEPTION1(nlib::Exception, "file not found!");
	}

	std::basic_ostringstream<boost::uint8_t> fileData;
	try
	{
		char buffer;
		file.read(&buffer, 1);
		while (file)
		{
			fileData.write((const boost::uint8_t*)&buffer, 1);
			file.read(&buffer, 1);
		}

		if (!file.eof())
		{
			LOG_WARN("Could not read file from disk. EOF not reached...");
			THROW_EXCEPTION1(nlib::Exception, "failed to read from file! EOF not reached...");
		}
	}
	catch (std::exception& ex)
	{
		LOG_WARN("Could not read file from disk. Reason=" << ex.what());
		THROW_EXCEPTION1(nlib::Exception, "failed to read from file!");
	}

	LOG_INFO("bulk -> Read " << fileData.str().size() << " bytes from the file...");

	
	/* commented by Cristian.Guef
	DevicePtr device = GetContractDevice(command->deviceID);
	bulk.ContractID = device->ContractID();
	*/
	
	if (bulk.type == GBulk::BULK_WITH_SM)
	{
		unsigned short TLDE_SAPID = apSMAP;
		unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
		unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

		//added by Cristian.Guef
		DevicePtr device = GetContractDevice(resourceID, 
						(unsigned char) GContract::BulkTransforClient, command->deviceID);
		bulk.ContractID = device->ContractID(resourceID, (unsigned char) GContract::BulkTransforClient);

	}
	else
	{
		unsigned short TLDE_SAPID = bulk.port;
		unsigned short ObjID = bulk.objID;
		unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

		//added by Cristian.Guef
		DevicePtr device = GetContractDevice(resourceID, 
						(unsigned char) GContract::BulkTransforClient, bulk.devID);
		bulk.ContractID = device->ContractID(resourceID, (unsigned char) GContract::BulkTransforClient);

	}

	bulk.Data = fileData.str();
	bulk.TLDE_SAPID = apSMAP;

	//added by Cristian.Guef
	bulk.m_currentBulkState = GBulk::BulkOpen;
	bulk.bulkDialogID = MonitH_GW_SessionID;  //we don't have parallel transfers
	
	/* commented by Cristian.Guef -- it is operation -bulk_open
	//[andrei.petrut] leave paranthesis around define to avoid dataSize / 10 * 1024 cases...
	int timeoutMinutes = 1 + (bulk.Data.size() / (processor->configApp.BulkDataTransferRate()));
	nlib::TimeSpan timeout = nlib::util::minutes(timeoutMinutes);
	LOG_DEBUG("Setting timeout for GBulk command at " << nlib::ToString(timeout));
	
	processor->SendRequest(*command, service, timeout);
	*/
	
	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(bulk.ContractID);

	//added
	if (bulk.type == GBulk::BULK_WITH_DEV)
	{
		AddBulkInProgress(bulk.devID, bulk.port, bulk.objID, command->commandID);
	}
}

void RequestProcessor::Visit(GClientServer<WriteObjectAttribute>& writeAttribute)
{

	//added by Cristian.Guef
	unsigned short TLDE_SAPID = writeAttribute.Client.channel.mappedTSAPID;
	unsigned short ObjID = writeAttribute.Client.channel.mappedObjectID;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	/* commented by Cristian.Guef
	DevicePtr device = GetContractDevice(command->deviceID);
	writeAttribute.ContractID = device->ContractID();
	*/
	//added by Cristian.Guef
	DevicePtr device = GetContractDevice(resourceID,
		(unsigned char) GContract::Client, command->deviceID);
	writeAttribute.ContractID = device->ContractID(resourceID, (unsigned char) GContract::Client);

	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(writeAttribute.ContractID);
}

void RequestProcessor::Visit(GClientServer<ReadMultipleObjectAttributes>& multipleReadObjectAttributes)
{

	/* commented by Cristian.Guef - it is already done in CommandsFactory::create(...)
	DevicePtr device = GetContractDevice(command->deviceID);
	multipleReadObjectAttributes.ContractID = device->ContractID();

	for (ReadMultipleObjectAttributes::AttributesList::iterator it =
	    multipleReadObjectAttributes.Client.attributes.begin(); it
	    != multipleReadObjectAttributes.Client.attributes.end(); it++)
	{
		DeviceChannel channel;
		if (!processor->devices.FindDeviceChannel(command->deviceID, it->channel.channelNumber, channel))
		{
			//TODO: add specilized exception
			THROW_EXCEPTION1(nlib::Exception, boost::str(boost::format("Channel:'%1%' not found for device: '%2%'")
			    % it->channel.channelNumber % command->deviceID));
		}
		//CHECKME:[Ovidiu.Rauca] TSAPID should be the same in all channels
		if (it == multipleReadObjectAttributes.Client.attributes.begin())
			multipleReadObjectAttributes.Client.TSAPID = channel.mappedTSAPID;

		it->channel = channel;
		it->confirmValue.dataType = channel.channelDataType;
	}
	*/

	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(multipleReadObjectAttributes.ContractID);
}


void RequestProcessor::Visit(PublishSubscribe& ps)
{

	//added by Cristian.Guef
	//unsigned short TLDE_SAPID = ps.PublisherChannel.mappedTSAPID;
	//unsigned short ObjID = ps.PublisherChannel.mappedObjectID;
	//unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	//added by Cristian.Guef
	//unsigned int PubResourceID = resourceID;

	DevicesPtrList devicesWihoutContract;
	DevicePtr publisherDevice = GetRegisteredDevice(ps.PublisherDeviceID);

	/* commented by Cristian.Guef
	if (!publisherDevice->HasContract())
	*/
	/*added by Cristian.Guef
	if (!publisherDevice->HasContract(resourceID, (unsigned char) GContract::Client))
	{
		LOG_DEBUG("Publisher device:" << publisherDevice->Mac().ToString() << " -req- -> has  no c/s lease");
		
		//added by Cristian.Guef
		publisherDevice->m_unThrownResourceID = resourceID;
		publisherDevice->m_unThrownLeaseType = GContract::Client;
		
		devicesWihoutContract.push_back(publisherDevice);
	}
	*/

	/*commented by Cristian.Guef
	DevicePtr subscriberDevice = GetRegisteredDevice(ps.SubscriberDeviceID);
	*/

	/* commented by Cristian.Guef
	if (!subscriberDevice->HasContract())
	{
		LOG_DEBUG("Subscriber device:" << subscriberDevice->Mac().ToString() << " doesn't have valid contract");

		devicesWihoutContract.push_back(subscriberDevice);
	}
	*/

	if (!devicesWihoutContract.empty())
	{
		THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
	}

	/* commented by Cristian.Guef
	DeviceChannel publisherChannel;
	if (!processor->devices.FindDeviceChannel(ps.PublisherDeviceID, ps.PublisherChannel.channelNumber, publisherChannel))
	{
		//TODO: add specilized exception
		THROW_EXCEPTION1(nlib::Exception, boost::str(boost::format("Publish channel:'%1%' not found for device: '%2%'")
		    % ps.PublisherChannel.channelNumber % ps.PublisherDeviceID));
	}
	*/

	ps.PublisherMAC = publisherDevice->Mac();
	ps.PublisherStatus = Command::rsNoStatus;
	
	/* commented by Cristian.Guef
	ps.PublisherChannel = publisherChannel;
	*/

	/* commented by Cristian.Guef
	ps.SubscriberMAC = subscriberDevice->Mac();
	*/

	ps.SubscriberStatus = Command::rsNoStatus;
	
	//boost::shared_ptr<GClientServer<Publish> > publish(new GClientServer<Publish>());
	//publish->Client.ps = boost::static_pointer_cast<PublishSubscribe>(service);
	//publish->ContractID = publisherDevice->ContractID();
	//publish->CommandID = ps.CommandID;

	boost::shared_ptr<GClientServer<Subscribe> > subscribe(new GClientServer<Subscribe>());
	subscribe->Client.ps = boost::static_pointer_cast<PublishSubscribe>(service);


	/* commented by Cristian.Guef
	subscribe->ContractID = subscriberDevice->ContractID();
	*/
	
	subscribe->CommandID = ps.CommandID;
//	processor->SendRequests(*command, publish, subscribe);
	
	//added by Cristian.Guef
	subscribe->Client.m_currentSubscribeState = Subscribe::SubscribeDoLeaseSubscriber;

	//added by Cristian.Guef
	subscribe->Client.m_handedPublisherIP = publisherDevice->IP();

	//added by Cristian.Guef
	//subscribe->Client.m_handedPublisherContractID_CS = publisherDevice->ContractID(PubResourceID, GContract::Client);
	
	//send first the commad to the subscriber 
	processor->SendRequest(*command, subscribe);
}

void RequestProcessor::Visit(GClientServer<Publish>& publish)
{
}

void RequestProcessor::Visit(GClientServer<Subscribe>& subscribe)
{
}

//added by Cristian.Guef
void RequestProcessor::Visit(GClientServer<GetSizeForPubIndication>& getSizeForPubIndication)
{

}

void RequestProcessor::Visit(PublishIndication& publishIndication)
{
}

void RequestProcessor::Visit(GClientServer<GetFirmwareVersion>& getFirmwareVersion)
{
	//added by Cristian.Guef
	unsigned short TLDE_SAPID = apSMAP;
	unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	/* commented by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(command->deviceID);
	getFirmwareVersion.ContractID = smDevice->ContractID();
	*/
	//added by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(resourceID, 
		(unsigned char) GContract::Client, command->deviceID);
	getFirmwareVersion.ContractID = smDevice->ContractID(resourceID, (unsigned char) GContract::Client);

	//added by Cristian Guef
	LOG_INFO("req -> get firm version lease_id = " << getFirmwareVersion.ContractID);

	DevicePtr destinationDevice = GetRegisteredDevice(getFirmwareVersion.Client.DeviceID);
	getFirmwareVersion.Client.DeviceAddress = destinationDevice->Mac();

	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(getFirmwareVersion.ContractID);
}


void RequestProcessor::Visit(GClientServer<FirmwareUpdate>& startFirmwareUpdate)
{

	//added by Cristian.Guef
	unsigned short TLDE_SAPID = apSMAP;
	unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	/* commented by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(command->deviceID);
	startFirmwareUpdate.ContractID = smDevice->ContractID();
	*/
	//add by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(resourceID,
		(unsigned char) GContract::Client, command->deviceID);
	startFirmwareUpdate.ContractID = smDevice->ContractID(resourceID, (unsigned char) GContract::Client);

	DevicePtr destinationDevice = GetRegisteredDevice(startFirmwareUpdate.Client.DeviceID);
	startFirmwareUpdate.Client.DeviceAddress = destinationDevice->Mac();

	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(startFirmwareUpdate.ContractID);
}

void RequestProcessor::Visit(GClientServer<GetFirmwareUpdateStatus>& getFirmwareUpdateStatus)
{
	//added by Cristian.Guef
	unsigned short TLDE_SAPID = apSMAP;
	unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	/* commented by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(command->deviceID);
	getFirmwareUpdateStatus.ContractID = smDevice->ContractID();
	*/
	//add by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(resourceID, 
		(unsigned char) GContract::Client, command->deviceID);
	getFirmwareUpdateStatus.ContractID = smDevice->ContractID(resourceID, (unsigned char) GContract::Client);

	DevicePtr destinationDevice = GetRegisteredDevice(getFirmwareUpdateStatus.Client.DeviceID);
	getFirmwareUpdateStatus.Client.DeviceAddress = destinationDevice->Mac();

	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(getFirmwareUpdateStatus.ContractID);
}

void RequestProcessor::Visit(GClientServer<CancelFirmwareUpdate>& cancelFirmwareUpdate)
{
	//added by Cristian.Guef
	unsigned short TLDE_SAPID = apSMAP;
	unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	/* commented by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(command->deviceID);
	cancelFirmwareUpdate.ContractID = smDevice->ContractID();
	*/
	//added by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(resourceID,
		(unsigned char) GContract::Client, command->deviceID);
	cancelFirmwareUpdate.ContractID = smDevice->ContractID(resourceID, (unsigned char) GContract::Client);


	DevicePtr destinationDevice = GetRegisteredDevice(cancelFirmwareUpdate.Client.DeviceID);
	cancelFirmwareUpdate.Client.DeviceAddress = destinationDevice->Mac();

	processor->SendRequest(*command, service);

	/// cancel in DB without waiting the respomse to the command. Even if it fails there is nothing else to do
	processor->devices.SetFirmDlStatus( cancelFirmwareUpdate.Client.DeviceID, 10 /*FWTYPE(device->Type())*/, DevicesManager::FirmDlStatus_Cancelled);

	//lease mng
	processor->leaseTrackMng.RemoveLease(cancelFirmwareUpdate.ContractID);
}

//added by Cristian.Guef
void RequestProcessor::Visit(GClientServer<GetContractsAndRoutes>& getContractsAndRoutes)
{
	unsigned short TLDE_SAPID = apUAP1;
	unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;


	DevicePtr smDevice = GetContractDevice(resourceID,
		(unsigned char) GContract::Client, command->deviceID);
	getContractsAndRoutes.ContractID = smDevice->ContractID(resourceID, (unsigned char) GContract::Client);

	if (getContractsAndRoutes.Client.isForFirmDl)
		getContractsAndRoutes.Client.smIP = smDevice->IP();

	processor->SendRequest(*command, service);

	//lease mng
	////no need to remove it for mnd cause there is an automatic task using it
	//processor->leaseTrackMng.RemoveLease(getContractsAndRoutes.ContractID);
}

//added by Cristian.Guef
void RequestProcessor::Visit(GClientServer<GISACSRequest>& CSRequest)
{
	unsigned short TLDE_SAPID = CSRequest.Client.Info.m_tsapID;
	unsigned short ObjID = CSRequest.Client.Info.m_objID;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;


	DevicePtr device = GetContractDevice(resourceID,
		(unsigned char) GContract::Client, command->deviceID);
	CSRequest.ContractID = device->ContractID(resourceID, (unsigned char) GContract::Client);

	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(CSRequest.ContractID);
}

void RequestProcessor::Visit(GClientServer<ResetDevice>& resetDevice)
{
	//added by Cristian.Guef
	unsigned short TLDE_SAPID = apUAP1;
	unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	/* commented by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(command->deviceID);
	resetDevice.ContractID = smDevice->ContractID();
	*/
	//added by Cristian.Guef
	DevicePtr smDevice = GetContractDevice(resourceID,
		(unsigned char) GContract::Client, command->deviceID);
	resetDevice.ContractID = smDevice->ContractID(resourceID, (unsigned char) GContract::Client);
	
	DevicePtr destinationDevice = GetRegisteredDevice(resetDevice.Client.DeviceID);
	resetDevice.Client.DeviceIP = destinationDevice->IP();

	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(resetDevice.ContractID);
}

void RequestProcessor::Visit(GSensorFrmUpdateCancel &sensorFrmUpdateCancel)
{
	unsigned short TLDE_SAPID = sensorFrmUpdateCancel.port;
	unsigned short ObjID = sensorFrmUpdateCancel.objID;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	//added by Cristian.Guef
	DevicePtr device = GetContractDevice(resourceID, 
		(unsigned char) GContract::BulkTransforClient,sensorFrmUpdateCancel.devID);
	sensorFrmUpdateCancel.ContractID = device->ContractID(resourceID, (unsigned char) GContract::BulkTransforClient);

	sensorFrmUpdateCancel.bulkDialogID = MonitH_GW_SessionID;  //we don't have parallel transfers for the same device
	
	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(sensorFrmUpdateCancel.ContractID);
}
void RequestProcessor::Visit(GClientServer<GetChannelsStatistics>& getChannels)
{
	unsigned short TLDE_SAPID = apUAP1;
	unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;

	DevicePtr smDevice = GetContractDevice(resourceID,
		(unsigned char) GContract::Client, command->deviceID);
	getChannels.ContractID = smDevice->ContractID(resourceID, (unsigned char) GContract::Client);
	
	DevicePtr destinationDevice = GetRegisteredDevice(getChannels.Client.DeviceID);
	getChannels.Client.DeviceIP = destinationDevice->IP();

	processor->SendRequest(*command, service);

	//lease mng
	processor->leaseTrackMng.RemoveLease(getChannels.ContractID);
}

DevicePtr RequestProcessor::GetDevice(int deviceID)
{
	DevicePtr device = processor->devices.FindDevice(deviceID);
	if (!device)
	{
		THROW_EXCEPTION1(DeviceNotFoundException, deviceID);
	}
	return device;
}

DevicePtr RequestProcessor::GetRegisteredDevice(int deviceID)
{
	DevicePtr device = GetDevice(deviceID);
	if (device->Status() != Device::dsRegistered)
	{
		LOG_DEBUG("Device:" << device->Mac().ToString() << " not registered");
		THROW_EXCEPTION1(DeviceNodeRegisteredException, device);
	}
	return device;
}

/* commented by Cristian.Guef
DevicePtr RequestProcessor::GetContractDevice(int deviceID)
{
	DevicePtr device = GetRegisteredDevice(deviceID);
	if (!device->HasContract())
	{
		LOG_DEBUG("Device:" << device->Mac().ToString() << " doesn't have valid contract");
		DevicesPtrList devicesWihoutContract;
		devicesWihoutContract.push_back(device);
		THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
	}

	return device;
}
*/
//added by Cristian.Guef
DevicePtr RequestProcessor::GetContractDevice(unsigned int resourceID, unsigned char leaseType,
											  int deviceID)
{
	DevicePtr device = GetRegisteredDevice(deviceID);
	if (!device->HasContract(resourceID, leaseType))
	{
		LOG_DEBUG("Device:" << device->Mac().ToString() << " doesn't have valid lease");
		DevicesPtrList devicesWihoutContract;

		//added by Cristian.Guef
		device->m_unThrownResourceID = resourceID;
		device->m_unThrownLeaseType = leaseType;

		devicesWihoutContract.push_back(device);
		THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
	}

	return device;
}
} // namespace hostapp
    } // namespace nisa100
