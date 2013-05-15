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

#include "CommandsFactory.h"
#include "model/Device.h"
#include "commandmodel/IGServiceVisitor.h"
#include "commandmodel/Nisa100ObjectIDs.h"

#include <boost/lexical_cast.hpp> //for converting numeric <-> to string

//added by Cristian.Guef
#include "DevicesManager.h"
#include "./processor/ProcessorExceptions.h"
#include <cstdio>

namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
DevicePtr GetDevice(DevicesManager *devices, int deviceID)
{
	DevicePtr device = devices->FindDevice(deviceID);
	if (!device)
	{
		THROW_EXCEPTION1(DeviceNotFoundException, deviceID);
	}
	return device;
}

//added by Cristian.Guef
DevicePtr GetRegisteredDevice(DevicesManager *devices, int deviceID)
{
	DevicePtr device = GetDevice(devices, deviceID);
	if (device->Status() != Device::dsRegistered)
	{
		//LOG_DEBUG("Device:" << device->Mac().ToString() << " not registered");

		THROW_EXCEPTION1(DeviceNodeRegisteredException, device);
	}
	return device;
}

//added by Cristian.Guef
DevicePtr GetContractDevice(DevicesManager *devices, unsigned int resourceID,
							unsigned char leaseType, int deviceID)
{
	DevicePtr device = GetRegisteredDevice(devices, deviceID);
	
	if (!device->HasContract(resourceID, leaseType))
	{
		//LOG_DEBUG("Device:" << device->Mac().ToString() << " doesn't have valid contract");
		DevicesPtrList devicesWihoutContract;

		device->m_unThrownResourceID = resourceID;
		device->m_unThrownLeaseType = leaseType;

		devicesWihoutContract.push_back(device);
		THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
	}

	return device;
}

const DeviceChannel& GetPSSubscriberChannel()
{
	static DeviceChannel channel;
	channel.channelNumber = 1;
	channel.channelName = "PS channel subscriber";
	channel.channelDataType = ChannelValue::cdtUInt32;
	channel.mappedTSAPID = apUAP_EvalKits;
	channel.mappedObjectID = SUBSCRIBER_OBJECT;
	//HARDCODED [nicu.dascalu] - attribute id for gw subscriber
	channel.mappedAttributeID = 1;

	return channel;
}

const DeviceChannel& GetLLSubscriberChannel()
{
	static DeviceChannel channel;
	channel.channelNumber = 1;
	channel.channelName = "LL channel subscriber";
	channel.channelDataType = ChannelValue::cdtUInt32;
	channel.mappedTSAPID = apUAP_EvalKits;
	channel.mappedObjectID = SUBSCRIBER_OBJECT;
	//HARDCODED [nicu.dascalu] - attribute id for LL subscriber
	channel.mappedAttributeID = 1;

	return channel;
}

const DeviceChannel& GetDigitalOutputChannel()
{
	static DeviceChannel channel;
	channel.channelNumber = 1;
	channel.channelName = "Digital Output Writter";
	channel.channelDataType = ChannelValue::cdtUInt16;
	channel.mappedTSAPID = apUAP_EvalKits;
	channel.mappedObjectID = SUBSCRIBER_OBJECT;
	//HARDCODED [nicu.dascalu] - attribute id for gw subscriber
	channel.mappedAttributeID = 1;

	return channel;
}



int GetParameterValue(CommandParameter::ParameterCode parameterCode, const Command::ParametersList& list,
  const std::string& parameterName)
{
	for (Command::ParametersList::const_iterator it = list.begin(); it != list.end(); it++)
	{
		if (it->parameterCode == parameterCode)
		{
			try
			{
				return boost::lexical_cast<int>(it->parameterValue);
			}
			catch(boost::bad_lexical_cast &)
			{
				THROW_EXCEPTION1(InvalidCommandException, parameterName + "Parameter value invalid!");
			}
		}
	}
	THROW_EXCEPTION1(InvalidCommandException, parameterName + " Parameter not found!");
}

bool GetLeaseCommittedBurst(const Command::ParametersList& list, int &committedBurst)
{
	for (Command::ParametersList::const_iterator it = list.begin(); it != list.end(); it++)
	{
		if (it->parameterCode == CommandParameter::LeaseCommittedBurst)
		{
			try
			{
				committedBurst = boost::lexical_cast<int>(it->parameterValue);
				return true;
			}
			catch(boost::bad_lexical_cast &)
			{
				THROW_EXCEPTION1(InvalidCommandException, " 'LeaseCommittedBurst' Parameter value invalid!");
			}
		}
	}
	LOG_WARN(" 'LeaseCommittedBurst' Parameter not found");
	return false;
}

std::string GetParameterValueAsString(CommandParameter::ParameterCode parameterCode,
  const Command::ParametersList& list, const std::string& parameterName)
{
	for (Command::ParametersList::const_iterator it = list.begin(); it != list.end(); it++)
	{
		if (it->parameterCode == parameterCode)
		{
			return it->parameterValue;
		}
	}
	THROW_EXCEPTION1(InvalidCommandException, parameterName + " Parameter not found!");
}

//added by Cristian.Guef
std::string GetParameterValueAsStringWithoutException(CommandParameter::ParameterCode parameterCode,
  const Command::ParametersList& list, const std::string& parameterName)
{
	for (Command::ParametersList::const_iterator it = list.begin(); it != list.end(); it++)
	{
		if (it->parameterCode == parameterCode)
		{
			return it->parameterValue;
		}
	}
	std::string str; //empty
	return str;
}


void GetParametersValues(CommandParameter::ParameterCode parameterCode,
  const Command::ParametersList& allParameterslist, Command::ParametersList& commandParameterslist)
{
	for (Command::ParametersList::const_iterator it = allParameterslist.begin(); it != allParameterslist.end(); it++)
	{
		if (it->parameterCode == parameterCode)
		{
			commandParameterslist.push_back(*it);
		}
	}
}

//added by Cristian.Guef
void GetSubscriptionList(Command& command, GAlert_Subscription::CategoryTypeListT &categoryList, 
											GAlert_Subscription::NetAddrTypeListT &netAddrList)
{
	//category_list
	{
		//subscribe
		{
			Command::ParametersList list;
			GetParametersValues(CommandParameter::Alert_Category_Subscribe, command.parameters, list);
			for (Command::ParametersList::const_iterator it = list.begin(); it != list.end(); it++)
			{
				try
				{
					categoryList.push_back(GAlert_Subscription::CategoryType());
					GAlert_Subscription::CategoryType &catType = *categoryList.rbegin();
					catType.Subscribe = (boost::uint8_t)boost::lexical_cast<int>(it->parameterValue);
				}
				catch(boost::bad_lexical_cast &)
				{
					THROW_EXCEPTION1(InvalidCommandException, "Alert_Category_Subscribe Parameter value invalid!");
				}
			}
		}
		//enable
		{
			Command::ParametersList list;
			GetParametersValues(CommandParameter::Alert_Category_Enable, command.parameters, list);
			int i = 0;
			for (Command::ParametersList::const_iterator it = list.begin(); 
							it != list.end(); it++, i++)
			{
				try
				{
					categoryList[i].Enable = (boost::uint8_t)boost::lexical_cast<int>(it->parameterValue);
				}
				catch(boost::bad_lexical_cast &)
				{
					THROW_EXCEPTION1(InvalidCommandException, "Alert_Category_Enable Parameter value invalid!");
				}
			}
		}
		//category
		{
			Command::ParametersList list;
			GetParametersValues(CommandParameter::Alert_Category_Category, command.parameters, list);
			int i = 0;
			for (Command::ParametersList::const_iterator it = list.begin(); 
							it != list.end(); it++, i++)
			{
				try
				{
					categoryList[i].Category = (boost::uint8_t)boost::lexical_cast<int>(it->parameterValue);
				}
				catch(boost::bad_lexical_cast &)
				{
					THROW_EXCEPTION1(InvalidCommandException, "Alert_Category_Category Parameter value invalid!");
				}
			}
		}
		
		//netaddr_list
		{
			//subscribe
			{
				Command::ParametersList list;
				GetParametersValues(CommandParameter::Alert_NetAddr_Subscribe, command.parameters, list);
				for (Command::ParametersList::const_iterator it = list.begin(); it != list.end(); it++)
				{
					try
					{
						netAddrList.push_back(GAlert_Subscription::NetAddrType());
						GAlert_Subscription::NetAddrType &netAddr = *netAddrList.rbegin();
						netAddr.Subscribe = (boost::uint8_t)boost::lexical_cast<int>(it->parameterValue);
					}
					catch(boost::bad_lexical_cast &)
					{
						THROW_EXCEPTION1(InvalidCommandException, "Alert_NetAddr_Subscribe Parameter value invalid!");
					}
				}
			}
			//enable
			{
				Command::ParametersList list;
				GetParametersValues(CommandParameter::Alert_NetAddr_Enable, command.parameters, list);
				int i = 0;
				for (Command::ParametersList::const_iterator it = list.begin(); 
								it != list.end(); it++, i++)
				{
					try
					{
						netAddrList[i].Enable = (boost::uint8_t)boost::lexical_cast<int>(it->parameterValue);
					}
					catch(boost::bad_lexical_cast &)
					{
						THROW_EXCEPTION1(InvalidCommandException, "Alert_NetAddr_Enable Parameter value invalid!");
					}
				}
			}
			//dev_id
			{
				Command::ParametersList list;
				GetParametersValues(CommandParameter::Alert_NetAddr_DevID, command.parameters, list);
				int i = 0;
				for (Command::ParametersList::const_iterator it = list.begin(); 
								it != list.end(); it++, i++)
				{
					try
					{
						int dev_id = boost::lexical_cast<int>(it->parameterValue);
						DevicePtr Device = command.devicesManager->FindDevice(dev_id);
						if (!Device)
						{
							THROW_EXCEPTION1(DeviceNotFoundException, dev_id);
						}
						netAddrList[i].DevAddr = Device->IP();
					}
					catch(boost::bad_lexical_cast &)
					{
						THROW_EXCEPTION1(InvalidCommandException, "Alert_NetAddr_DevID Parameter value invalid!");
					}
				}
			}
			//port_id
			{
				Command::ParametersList list;
				GetParametersValues(CommandParameter::Alert_NetAddr_TLSAPID, command.parameters, list);
				int i = 0;
				for (Command::ParametersList::const_iterator it = list.begin(); 
								it != list.end(); it++, i++)
				{
					try
					{
						netAddrList[i].EndPointPort = (boost::uint16_t)boost::lexical_cast<int>(it->parameterValue);
					}
					catch(boost::bad_lexical_cast &)
					{
						THROW_EXCEPTION1(InvalidCommandException, "Alert_NetAddr_TLSAPID Parameter value invalid!");
					}
				}
			}
			//obj_id
			{
				Command::ParametersList list;
				GetParametersValues(CommandParameter::Alert_NetAddr_ObjID, command.parameters, list);
				int i = 0;
				for (Command::ParametersList::const_iterator it = list.begin(); 
								it != list.end(); it++, i++)
				{
					try
					{
						netAddrList[i].EndObjID = (boost::uint16_t)boost::lexical_cast<int>(it->parameterValue);
					}
					catch(boost::bad_lexical_cast &)
					{
						THROW_EXCEPTION1(InvalidCommandException, "Alert_NetAddr_ObjID Parameter value invalid!");
					}
				}
			}
			//alert_type
			{
				Command::ParametersList list;
				GetParametersValues(CommandParameter::Alert_NetAddr_AlertType, command.parameters, list);
				int i = 0;
				for (Command::ParametersList::const_iterator it = list.begin(); 
								it != list.end(); it++, i++)
				{
					try
					{
						netAddrList[i].AlertType = (boost::uint8_t)boost::lexical_cast<int>(it->parameterValue);
					}
					catch(boost::bad_lexical_cast &)
					{
						THROW_EXCEPTION1(InvalidCommandException, "Alert_NetAddr_AlertType Parameter value invalid!");
					}
				}
			}
		}
	}
	
	//logs
	LOG_INFO("\talert_sub_categ_list_cmd -> begin ----------");
	for (unsigned int i = 0; i < categoryList.size(); i++)
	{
		LOG_INFO("\talert_sub_categ_list_cmd -> entry = " << (boost::uint16_t) i 
				<< " subscribe = " << (boost::uint16_t)categoryList[i].Subscribe
				<< " enable = " << (boost::uint16_t)categoryList[i].Enable
				<< " category = " << (boost::uint16_t)categoryList[i].Category);
	}
	LOG_INFO("\talert_sub_categ_list_cmd -> end ----------");

	LOG_INFO("\talert_sub_netAddr_list_cmd -> begin ----------");
	for (unsigned int i = 0; i < netAddrList.size(); i++)
	{
		LOG_INFO("\talert_sub_netAddr_list_cmd -> entry = " << (boost::uint16_t) i 
				<< " subscribe = " << (boost::uint16_t)netAddrList[i].Subscribe
				<< " enable = " << (boost::uint16_t)netAddrList[i].Enable
				<< " ip = " << netAddrList[i].DevAddr.ToString()
				<< " port = " << netAddrList[i].EndPointPort
				<< " obj_id = " << netAddrList[i].EndObjID
				<< " alert_type = " << (boost::uint16_t)netAddrList[i].AlertType);
	}
	LOG_INFO("\talert_sub_netAddr_list_cmd -> end ----------");

}

//added by Cristian.Guef
void CreateContractsForRead(Command& command) //force creating contracts before fragmenting command
{
	Command::ParametersList commandParametersList;
	GetParametersValues(CommandParameter::ReadValue_Channel, command.parameters, commandParametersList);
	LOG_DEBUG(boost::str(boost::format("Read command have:'%1%' channels") % commandParametersList.size()));

	if (commandParametersList.empty())
		THROW_EXCEPTION1(InvalidCommandException, "Channels list is empty!");

	unsigned short TLDE_SAPID = 0;
	unsigned short ObjID = 0;
	unsigned int resourceID = 0;
	DevicesPtrList devicesWihoutContract;
	
	for (Command::ParametersList::const_iterator it = commandParametersList.begin(); it != commandParametersList.end(); it++)
	{
		DeviceChannel channel;
		//if (!command.devicesManager->FindDeviceChannel(command.deviceID, boost::lexical_cast<int>(it->parameterValue), channel, 1 /*simple_interface*/))
		//{
		//	//TODO: add specilized exception
		//	THROW_EXCEPTION1(nlib::Exception, boost::str(boost::format("Channel:'%1%' not found for device: '%2%'")
		//		% boost::lexical_cast<int>(it->parameterValue) % command.deviceID));
		//}
		if (!command.devicesManager->FindDeviceChannel(boost::lexical_cast<int>(it->parameterValue), channel))
		{
			//TODO: add specilized exception
			THROW_EXCEPTION1(nlib::Exception, boost::str(boost::format("Channel:'%1%' not found for device: '%2%'")
				% boost::lexical_cast<int>(it->parameterValue) % command.deviceID));
		}

		TLDE_SAPID = channel.mappedTSAPID;
		ObjID = channel.mappedObjectID;
		resourceID = (ObjID << 16) | TLDE_SAPID;

		DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
		
		if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
		{
			LOG_DEBUG("read_channel_cmd -> device:" << device->Mac().ToString() << " has  no c/s lease");
			
			//added by Cristian.Guef
			device->m_unThrownResourceID = resourceID;
			device->m_unThrownLeaseType = GContract::Client;
			
			devicesWihoutContract.push_back(device);
		}

		//renew lease with new committed burst parameter
		int committedBurst = 0;
		if (GetLeaseCommittedBurst(command.parameters, committedBurst))
		{
			if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
			{
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}
		}

	}
		
	if (!devicesWihoutContract.empty())
	{
		THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
	}

}

extern bool IsBulkInProgreass(int deviceID);
extern void GetBulkInProgressVal(int deviceID, unsigned short &port, unsigned short &objID, int &cmdID);

/* commented by Cristian.Guef
AbstractGServicePtr CommandsFactory::Create(const Command& command)
*/
//added by Cristian.Guef
AbstractGServicePtr CommandsFactory::Create(Command& command)
{
	LOG_DEBUG("Create: GService for command:" << command.ToString());

	if(command.commandCode == Command::ccSession) //added by Cristian.Guef
	{
		return AbstractGServicePtr(new GSession(command.m_uwNetworkIDForSessionGSAP));
	}
	else if (command.commandCode == Command::ccGetTopology)
	{
		return AbstractGServicePtr(new GTopologyReport());
	}
	else if (command.commandCode == Command::ccGetDeviceList)
	{
		return AbstractGServicePtr(new GDeviceListReport(-1));
	}
	else if (command.commandCode == Command::ccCreateClientServerContract)
	{
		/* commented by Cristian.Guef
		return AbstractGServicePtr(new GContract());
		*/
		//added by Cristian.Guef
		return AbstractGServicePtr(new GContract(command.m_unResourceIDForContractCmd, 
			command.m_ucLeaseTypeForContractCmd, command.m_committedBurst, command.m_dbCmdIDForContract));
	}
	else if (command.commandCode == Command::ccDelContract) //added by Cristian.Guef
	{
		boost::int32_t contract_id = (boost::int32_t)GetParameterValue(CommandParameter::DelContract_ContractID,
		    command.parameters, "lease_id parameter for deleting lease");

		boost::shared_ptr<GDelContract> delContract(new GDelContract(contract_id));

		delContract->ContractType = command.ContractType;
		delContract->IPAddress = command.IPAddress;
		delContract->m_unResourceID = command.ResourceID;
		return delContract;
	}
	else if (command.commandCode == Command::ccReadValue)
	{
		LOG_DEBUG("Create: GService for command:" << command.ToString());
		
		//added by Cristian.Guef
		if(command.m_MultipleReadDBCmd.IndexforFragmenting == 0)
			CreateContractsForRead(command);

		Command::ParametersList commandParametersList;
		GetParametersValues(CommandParameter::ReadValue_Channel, command.parameters, commandParametersList);
		LOG_DEBUG(boost::str(boost::format("Read command have:'%1%' channels") % commandParametersList.size()));

		if (commandParametersList.empty())
			THROW_EXCEPTION1(InvalidCommandException, "Channels list is empty!");

		boost::shared_ptr<GClientServer<ReadMultipleObjectAttributes> >
		    multipleRead(new GClientServer<ReadMultipleObjectAttributes>);

		/* commented by Cristian.Guef
		multipleRead->Client.attributes.reserve(commandParametersList.size());
		*/

		//added by Cristian.Guef - now begins fragmenting
		DeviceChannel channel;
		//if (!command.devicesManager->FindDeviceChannel(command.deviceID, 
		//		boost::lexical_cast<int>(commandParametersList[
		//		command.m_MultipleReadDBCmd.IndexforFragmenting].parameterValue), 
		//		channel, 1 /*simple_interface*/))
		//{
		//	//TODO: add specilized exception
		//	THROW_EXCEPTION1(nlib::Exception, boost::str(boost::format("Channel:'%1%' not found for device: '%2%'")
		//		% boost::lexical_cast<int>(commandParametersList[command.m_MultipleReadDBCmd.IndexforFragmenting].parameterValue) % command.deviceID));
		//}
		if (!command.devicesManager->FindDeviceChannel(boost::lexical_cast<int>(commandParametersList[
				command.m_MultipleReadDBCmd.IndexforFragmenting].parameterValue), channel))
		{
			//TODO: add specilized exception
			THROW_EXCEPTION1(nlib::Exception, boost::str(boost::format("Channel:'%1%' not found for device: '%2%'")
				% boost::lexical_cast<int>(commandParametersList[command.m_MultipleReadDBCmd.IndexforFragmenting].parameterValue) % command.deviceID));
		}
		ReadMultipleObjectAttributes::ObjectAttribute attribute;
		attribute.channel = channel;
		attribute.confirmValue.dataType = channel.channelDataType; 
		multipleRead->Client.attributes.push_back(attribute);
		multipleRead->Client.m_unDialogueID = command.m_MultipleReadDBCmd.DiagID;
		multipleRead->Client.m_unSequenceNo = (unsigned int)command.m_MultipleReadDBCmd.IndexforFragmenting;
		multipleRead->Client.m_unTotalAttributesNo = commandParametersList.size();
		command.m_MultipleReadDBCmd.IndexforFragmenting++;
		if(command.m_MultipleReadDBCmd.IndexforFragmenting >= (int)commandParametersList.size()){
			command.m_MultipleReadDBCmd.IndexforFragmenting = -1;
		}
		
		//added by Cristian.Guef
		unsigned short TLDE_SAPID = channel.mappedTSAPID;
		unsigned short ObjID = channel.mappedObjectID;
		unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
		DevicePtr device = GetContractDevice(command.devicesManager, 
												resourceID,	GContract::Client, command.deviceID);
		multipleRead->ContractID = device->ContractID(resourceID, GContract::Client);


		/* commented by Cristian.Guef
		for (Command::ParametersList::const_iterator it = commandParametersList.begin(); it != commandParametersList.end(); it++)
		{
			try
			{
				ReadMultipleObjectAttributes::ObjectAttribute attribute;
				attribute.channel.channelNumber = boost::lexical_cast<int>(it->parameterValue); //in fact is the channel No
				multipleRead->Client.attributes.push_back(attribute);
			}
			catch(boost::bad_lexical_cast &)
			{
				THROW_EXCEPTION1(InvalidCommandException,
						boost::str(boost::format("ChannelNo:'%1%' is invalid!") % it->parameterValue));
			}
		}
		*/

		return multipleRead;
	}
	else if (command.commandCode == Command::ccPublishSubscribe)
	{
		/* commented by Cristian.Guef
		int publisherChannel = GetParameterValue(CommandParameter::PublishSubscribe_PublisherChannelNo, command.parameters,
		    "Publish_ChannelNo");
			*/

		//added by Cristian.Guef
		boost::uint16_t concentrator_id = (boost::uint16_t)GetParameterValue(CommandParameter::PublishSubscribe_Concentrator_id,
		    command.parameters, "Concentrator_id");
		boost::uint16_t concentrator_tlsap_id = (boost::uint16_t)GetParameterValue(
		    CommandParameter::PublishSubscribe_Concentrator_tlsap_id, command.parameters, "Concentrator_tlsap_id");

		boost::uint8_t stale_limit = (boost::uint8_t)GetParameterValue(
		    CommandParameter::PublishSubscribe_StaleLimit, command.parameters, "Publish_stale_limit");

		boost::uint16_t publishFrequency = (boost::uint16_t)GetParameterValue(CommandParameter::PublishSubscribe_Frequency,
		    command.parameters, "Publish_Frequency");
		boost::uint16_t publishFrequencyPhase = (boost::uint16_t)GetParameterValue(
		    CommandParameter::PublishSubscribe_FrequencyFaze, command.parameters, "Publish_FrequencyFaze");


		/* commented by Cristian.Guef
		int subscriberDeviceID = GetParameterValue(CommandParameter::PublishSubscribe_SubscriberDeviceID,
		    command.parameters, "Subscribe_DeviceID");
			*/



		boost::shared_ptr<PublishSubscribe> ps(new PublishSubscribe());
		ps->isLocalLoop = false;
		ps->Period = publishFrequency;
		ps->Phase = publishFrequencyPhase;

		ps->PublisherDeviceID = command.deviceID;
		ps->PublisherStatus = Command::rsNoStatus;
		
		/*commented by Cristian.Guef
		ps->PublisherChannel.channelNumber = publisherChannel;
		*/

		//added by Cristian.Guef
		ps->PublisherChannel.mappedTSAPID = concentrator_tlsap_id;
		ps->PublisherChannel.mappedObjectID = concentrator_id;
		ps->PublisherChannel.mappedAttributeID = concentrator_id; //for inheriting situation
		ps->StaleLimit = stale_limit;

		/* commented by Cristian.Guef
		ps->SubscriberDeviceID = subscriberDeviceID;
		ps->SubscriberStatus = Command::rsNoStatus;
		*/
		
		ps->SubscriberLowThreshold = 0;
		ps->SubscriberHighThreshold = 1;
		
		/* commented by Cristian.Guef
		ps->SubscriberChannel = GetPSSubscriberChannel();
		*/

		//added by Cristian.Guef
		ps->pcoChannelList = command.pcoChannelList;
		ps->pcoChannelIndex = command.pcoChannelIndex;
		ps->dataContentVer = command.dataContentVer;
		ps->interfaceType = command.interfaceType;


		LOG_INFO("---------------> send publish_subscribe" << 
		 		"\n\t\t\t command with the following parameters:" <<
		 		"\n\t\t\t publisher_device id = " << (boost::uint32_t)ps->PublisherDeviceID <<
				"\n\t\t\t  publisher_obj_id = " << (boost::uint32_t)ps->PublisherChannel.mappedObjectID <<
				"\n\t\t\t  publisher_tlsap_id = " << (boost::uint32_t)ps->PublisherChannel.mappedTSAPID <<
				"\n\t\t\t  publish_period = " << (boost::uint32_t)ps->Period <<
				"\n\t\t\t  publish_phase = " << (boost::int32_t)ps->Phase <<
				"\n\t\t\t  publish_stale_limit = " << (boost::int32_t)ps->StaleLimit);

		return ps;
	}
	else if (command.commandCode == Command::ccLocalLoop)
	{
		int publisherChannel = GetParameterValue(CommandParameter::LocalLoop_PublisherChannelNo, command.parameters,
		    "LocalLoop_ChannelNo");
		boost::uint16_t publishFrequency = (boost::uint16_t)GetParameterValue(CommandParameter::LocalLoop_Frequency,
		    command.parameters, "LocalLoop_Frequency");
		boost::uint16_t publishFrequencyPhase = (boost::uint16_t)GetParameterValue(
		    CommandParameter::LocalLoop_FrequencyFaze, command.parameters, "LocalLoop_FrequencyFaze");

		int subscriberDeviceID = GetParameterValue(CommandParameter::LocalLoop_SubscriberDeviceID, command.parameters,
		    "LocalLoop_Subscribe_DeviceID");

		int lowThreshold = GetParameterValue(CommandParameter::LocalLoop_SubscriberLowThreshold, command.parameters,
		    "LocalLoop_LowThreshold");
		int highThreshold = GetParameterValue(CommandParameter::LocalLoop_SubscriberHighThreshold, command.parameters,
		    "LocalLoop_HighThreshold");

		boost::shared_ptr<PublishSubscribe> ps(new PublishSubscribe());
		ps->isLocalLoop = true;
		ps->Period = publishFrequency;
		ps->Phase = publishFrequencyPhase;
		ps->PublisherDeviceID = command.deviceID;
		ps->PublisherStatus = Command::rsNoStatus;
		ps->PublisherChannel.channelNumber = publisherChannel;

		ps->SubscriberDeviceID = subscriberDeviceID;
		ps->SubscriberStatus = Command::rsNoStatus;
		ps->SubscriberLowThreshold = lowThreshold;
		ps->SubscriberHighThreshold = highThreshold;
		ps->SubscriberChannel = GetLLSubscriberChannel();

		return ps;
	}
	else if (command.commandCode == Command::ccDigitalOutputOn)
	{
		boost::shared_ptr<GClientServer<WriteObjectAttribute> > digitalOn(new GClientServer<WriteObjectAttribute>);
		digitalOn->Client.channel = GetDigitalOutputChannel();

		digitalOn->Client.writeValue.value.uint16 = 1;
		//HARDCODED: dataType = uint16
		digitalOn->Client.writeValue.dataType =  ChannelValue::cdtUInt16;

		//added by Cristian.Guef
		{
			unsigned short TLDE_SAPID = digitalOn->Client.channel.mappedTSAPID;
			unsigned short ObjID = digitalOn->Client.channel.mappedObjectID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}
			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}


		return digitalOn;
	}
	else if (command.commandCode == Command::ccDigitalOutputOff)
	{
		boost::shared_ptr<GClientServer<WriteObjectAttribute> > digitalOff(new GClientServer<WriteObjectAttribute>);
		digitalOff->Client.channel = GetDigitalOutputChannel();

		digitalOff->Client.writeValue.value.uint16 = 0;
		//HARDCODED: dataType = uint16
		digitalOff->Client.writeValue.dataType =  ChannelValue::cdtUInt16;

		//added by Cristian.Guef
		{
			unsigned short TLDE_SAPID = digitalOff->Client.channel.mappedTSAPID;
			unsigned short ObjID = digitalOff->Client.channel.mappedObjectID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}
			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}


		return digitalOff;
	}
	else if (command.commandCode == Command::ccGetFirmwareVersion)
	{
		int destinationDeviceID = GetParameterValue(CommandParameter::GetFirmware_DeviceAddress, command.parameters,
		    "GetFirmware_DeviceAddress");
		boost::shared_ptr<GClientServer<GetFirmwareVersion> > getFirmware(new GClientServer<GetFirmwareVersion>);
		getFirmware->Client.DeviceID = destinationDeviceID;

		//added by Cristian.Guef
		{
			unsigned short TLDE_SAPID = apSMAP;
			unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::Client;
					
					devicesWihoutContract.push_back(device);
				}
			}

			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}

		return getFirmware;
	}
	else if (command.commandCode == Command::ccFirmwareUpdate)
	{
		int destinationDeviceID = GetParameterValue(CommandParameter::FirmwareUpdate_DeviceAddress, command.parameters,
		    "FirmwareUpdate_DeviceAddress");
		std::string firmwareFile = GetParameterValueAsString(CommandParameter::FirmwareUpdate_FileName, command.parameters,
		    "FirmwareUpdate_FileName");

		boost::shared_ptr<GClientServer<FirmwareUpdate> > firmwareUpdate(new GClientServer<FirmwareUpdate>);
		firmwareUpdate->Client.DeviceID = destinationDeviceID;
		firmwareUpdate->Client.FirmwareFileName = firmwareFile;


		//added by Cristian.Guef
		{
			unsigned short TLDE_SAPID = apSMAP;
			unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::Client;
					
					devicesWihoutContract.push_back(device);
				}
			}


			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}

		return firmwareUpdate;
		
	}
	else if (command.commandCode == Command::ccGetFirmwareUpdateStatus)
	{
		int destinationDeviceID = GetParameterValue(CommandParameter::GetFirmwareUpdateStatus_DeviceAddress,
		    command.parameters, "GetFirmwareUpdateStatus_DeviceAddress");
		boost::shared_ptr<GClientServer<GetFirmwareUpdateStatus> >
		    firmwareUpdateStatus(new GClientServer<GetFirmwareUpdateStatus>);
		firmwareUpdateStatus->Client.DeviceID = destinationDeviceID;

		//added by Cristian.Guef
		{
			unsigned short TLDE_SAPID = apSMAP;
			unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::Client;
					
					devicesWihoutContract.push_back(device);
				}
			}


			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}

		return firmwareUpdateStatus;
	}
	else if (command.commandCode == Command::ccCancelFirmwareUpdate)
	{
		int destinationDeviceID = GetParameterValue(CommandParameter::CancelFirmware_DeviceAddress, command.parameters,
		    "CancelFirmware_DeviceAddress");
		boost::shared_ptr<GClientServer<CancelFirmwareUpdate> > cancelFirmware(new GClientServer<CancelFirmwareUpdate>);
		cancelFirmware->Client.DeviceID = destinationDeviceID;

		//added by Cristian.Guef
		{
			unsigned short TLDE_SAPID = apSMAP;
			unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::Client;
					
					devicesWihoutContract.push_back(device);
				}
			}


			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}
		
		return cancelFirmware;
	}
	else if (command.commandCode == Command::ccGetContractsAndRoutes) //added by Cristian.Guef
	{
		boost::shared_ptr<GClientServer<GetContractsAndRoutes> > getCntractAndRouteInfos(new GClientServer<GetContractsAndRoutes>);
		if (command.isFirmwareDownload)
			getCntractAndRouteInfos->Client.isForFirmDl = true;
		return getCntractAndRouteInfos;
	}
	else if (command.commandCode == Command::ccISAClientServerRequest) //added by Cristian.Guef
	{
		boost::shared_ptr<GClientServer<GISACSRequest> > getISACSRequest(new GClientServer<GISACSRequest>);

		getISACSRequest->Client.Info.m_tsapID = GetParameterValue(CommandParameter::ISACSRequest_TSAPID, command.parameters, "ISACSRequest_TSAPID");;
		getISACSRequest->Client.Info.m_reqType = GetParameterValue(CommandParameter::ISACSRequest_ReqType, command.parameters, "ISACSRequest_ReqType");;
		getISACSRequest->Client.Info.m_objID = GetParameterValue(CommandParameter::ISACSRequest_ObjID, command.parameters, "ISACSRequest_ObjID");;
		getISACSRequest->Client.Info.m_objResID = GetParameterValue(CommandParameter::ISACSRequest_ObjResourceID, command.parameters, "ISACSRequest_ObjResourceID");;
		
		if (getISACSRequest->Client.Info.m_tsapID < 0 || getISACSRequest->Client.Info.m_tsapID > 15)
			THROW_EXCEPTION1(InvalidCommandException, "ISACSRequest_DataBuffer -> not a valid tsapid");
		
		{
			unsigned short TLDE_SAPID = getISACSRequest->Client.Info.m_tsapID;
			unsigned short ObjID = getISACSRequest->Client.Info.m_objID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::Client;
					
					devicesWihoutContract.push_back(device);
				}
			}

			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}

		switch (getISACSRequest->Client.Info.m_reqType)
		{
		case 5/*0x5 - execute*/:
			getISACSRequest->Client.Info.m_attrIndex1 = -1;
			getISACSRequest->Client.Info.m_attrIndex2 = -1;
			
			getISACSRequest->Client.Info.m_strReqDataBuff = GetParameterValueAsStringWithoutException(CommandParameter::ISACSRequest_DataBuffer, command.parameters, "ISACSRequest_DataBuffer");
			//check for hexstring
			if (getISACSRequest->Client.Info.m_strReqDataBuff.size() % 2)
				THROW_EXCEPTION1(InvalidCommandException, "ISACSRequest_DataBuffer -> not a hexstring");
			for (unsigned int i = 0; i < getISACSRequest->Client.Info.m_strReqDataBuff.size(); i++)
			{
				if ((getISACSRequest->Client.Info.m_strReqDataBuff[i] >= '0' && getISACSRequest->Client.Info.m_strReqDataBuff[i] <= '9') ||
					(getISACSRequest->Client.Info.m_strReqDataBuff[i] >= 'A' && getISACSRequest->Client.Info.m_strReqDataBuff[i] <= 'F') ||
					(getISACSRequest->Client.Info.m_strReqDataBuff[i] >= 'a' && getISACSRequest->Client.Info.m_strReqDataBuff[i] <= 'f'))
				{
					//it's a hexvalue
				}
				else
				{
					THROW_EXCEPTION1(InvalidCommandException, "ISACSRequest_DataBuffer -> not a hexstring");
				}
			}
			break;
		case 4/*0x4 - write*/:
			getISACSRequest->Client.Info.m_attrIndex1 = GetParameterValue(CommandParameter::ISACSRequest_AttrIndex1, command.parameters, "ISACSRequest_AttrIndex1");
			getISACSRequest->Client.Info.m_attrIndex2 = GetParameterValue(CommandParameter::ISACSRequest_AttrIndex2, command.parameters, "ISACSRequest_AttrIndex2");;
			
			getISACSRequest->Client.Info.m_strReqDataBuff = GetParameterValueAsString(CommandParameter::ISACSRequest_DataBuffer, command.parameters, "ISACSRequest_DataBuffer");
			//check for hexstring
			if (getISACSRequest->Client.Info.m_strReqDataBuff.size() % 2)
				THROW_EXCEPTION1(InvalidCommandException, "ISACSRequest_DataBuffer -> not a hexstring");
			for (unsigned int i = 0; i < getISACSRequest->Client.Info.m_strReqDataBuff.size(); i++)
			{
				if ((getISACSRequest->Client.Info.m_strReqDataBuff[i] >= '0' && getISACSRequest->Client.Info.m_strReqDataBuff[i] <= '9') ||
					(getISACSRequest->Client.Info.m_strReqDataBuff[i] >= 'A' && getISACSRequest->Client.Info.m_strReqDataBuff[i] <= 'F') ||
					(getISACSRequest->Client.Info.m_strReqDataBuff[i] >= 'a' && getISACSRequest->Client.Info.m_strReqDataBuff[i] <= 'f'))
				{
					//it's a hexvalue
				}
				else
				{
					THROW_EXCEPTION1(InvalidCommandException, "ISACSRequest_DataBuffer -> not a hexstring");
				}
			}
			break;
		case 3/*0x3 - read*/:
			getISACSRequest->Client.Info.m_attrIndex1 = GetParameterValue(CommandParameter::ISACSRequest_AttrIndex1, command.parameters, "ISACSRequest_AttrIndex1");
			getISACSRequest->Client.Info.m_attrIndex2 = GetParameterValue(CommandParameter::ISACSRequest_AttrIndex2, command.parameters, "ISACSRequest_AttrIndex2");;

			getISACSRequest->Client.Info.m_ReadAsPublish = GetParameterValue(CommandParameter::ISACSRequest_ReadAsPublish, command.parameters, "ISACSRequest_ReadAsPublish");
			if (getISACSRequest->Client.Info.m_ReadAsPublish == 1)
			{
				//check for channel in db
				PublisherConf::COChannel csChannel;
				csChannel.tsapID = getISACSRequest->Client.Info.m_tsapID;
				csChannel.objID = getISACSRequest->Client.Info.m_objID;
				csChannel.attrID = getISACSRequest->Client.Info.m_objResID;
				csChannel.index1 = getISACSRequest->Client.Info.m_attrIndex1;
				csChannel.index2 = getISACSRequest->Client.Info.m_attrIndex2;
				if (command.devicesManager->IsDeviceChannel(command.deviceID, csChannel) == false)
				{
					PublisherConf::COChannelListT list;
					csChannel.format = -1;/*no format*/
					csChannel.name = "N/A";
					csChannel.unitMeasure = "N/A";
					list.push_back(csChannel);
					command.devicesManager->SavePublishChannels(command.deviceID, list);
					command.devicesManager->DeleteReadings(command.deviceID);
					command.devicesManager->CreateEmptyReadings(command.deviceID);
				}

			}
			break;
		default:
			THROW_EXCEPTION1(InvalidCommandException, "ISACSRequest_DataBuffer -> invalid request type");
		}

		return getISACSRequest;
	}
	else if (command.commandCode == Command::ccResetDevice)
	{
		int deviceID =
		    GetParameterValue(CommandParameter::ResetDevice_DeviceID, command.parameters, "ResetDevice_DeviceID");

		//added by Cristian.Guef
		int restartType =
		    GetParameterValue(CommandParameter::ResetDevice_RestartType, command.parameters, "ResetDevice_RestartType");

		//printf("restart_tyrp este = %d", restartType);
		//fflush(stdout);

		boost::shared_ptr<GClientServer<ResetDevice> > resetDevice(new GClientServer<ResetDevice>);
		resetDevice->Client.DeviceID = deviceID;

		//added by Cristian.Guef
		resetDevice->Client.restart_type = restartType;

		//added by Cristian.Guef
		{
			unsigned short TLDE_SAPID = apUAP1;
			unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			//printf ("commnad deviceid = %d", command.deviceID);
			//fflush(stdout);
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::Client;
					
					devicesWihoutContract.push_back(device);
				}
			}


			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}

		return resetDevice;
	}
	else if (command.commandCode == Command::ccFirmwareUpload)
	{
		boost::shared_ptr<GBulk> bulk;
		std::string fileName = GetParameterValueAsString(CommandParameter::FirmwareUpload_FileName, command.parameters,
			    "FirmwareUpload_FileName");

		//check if sm device
		if (command.devicesManager->IsSMDevice(command.deviceID))
		{
			bulk.reset(new GBulk(0/*with sm*/));
			bulk->FileName = fileName;
			bulk->port = apSMAP;
			bulk->objID = UPLOAD_DOWNLOAD_OBJECT;
			bulk->devID = command.deviceID;
			return bulk;
		}
		
		THROW_EXCEPTION1(InvalidCommandException, " device should be sm!");
	}
	else if (command.commandCode == Command::ccSensorBoardFirmwareUpdate)
	{
		boost::shared_ptr<GBulk> bulk;
		std::string fileName = GetParameterValueAsString(CommandParameter::SensorFirmwareUpdate_Filename, command.parameters,
			    "FirmwareUpload_FileName");

		int port = GetParameterValue(CommandParameter::SensorFirmwareUpdate_Port, command.parameters, "Port");
		int objID = GetParameterValue(CommandParameter::SensorFirmwareUpdate_ObjID, command.parameters, "ObjID");
		int deviceID = GetParameterValue(CommandParameter::SensorFirmwareUpdate_DeviceID, command.parameters, "deviceID");

		if (port < 0 || port > 15)
			THROW_EXCEPTION1(InvalidCommandException, "SensorBoardFirmwareUpdate -> not a valid tsapid");

		char message[250];
		sprintf(message, "Already exists an active firmware update with this deviceID=%d !", deviceID);
		if (IsBulkInProgreass(deviceID))
			THROW_EXCEPTION1(InvalidCommandException, message);

		int committedBurst = 0;
		{
			unsigned short TLDE_SAPID = port;
			unsigned short ObjID = objID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			LOG_INFO ("bulk commnad deviceid = " << deviceID);
			DevicePtr device = GetRegisteredDevice(command.devicesManager, deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::BulkTransforClient))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::BulkTransforClient;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			//int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::BulkTransforClient, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::BulkTransforClient;
					
					devicesWihoutContract.push_back(device);
				}
			}


			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}

		bulk.reset(new GBulk(1/*with device*/));
		bulk->FileName = fileName;
		bulk->port = port;
		bulk->objID = objID;
		bulk->devID = deviceID;
		bulk->committedBurst = committedBurst;
		return bulk;
	}
	else if (command.commandCode == Command::ccCancelBoardFirmwareUpdate)
	{
		int deviceID = GetParameterValue(CommandParameter::CancelSensorFirmwareUpdate_DeviceID, command.parameters, "deviceID");

		char message[250];
		sprintf(message, "No active firmware update in progress with this deviceID=%d !", deviceID);
		if (!IsBulkInProgreass(deviceID))
			THROW_EXCEPTION1(InvalidCommandException, message);

		boost::shared_ptr<GSensorFrmUpdateCancel> cancel;
		cancel.reset(new GSensorFrmUpdateCancel());
		int cmdID = 0;
		GetBulkInProgressVal(deviceID, cancel->port, cancel->objID, cmdID);

		cancel->devID = deviceID;

		{
			unsigned short TLDE_SAPID = cancel->port;
			unsigned short ObjID = cancel->objID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			LOG_INFO ("cancel bulk commnad deviceid = " << deviceID);
			DevicePtr device = GetRegisteredDevice(command.devicesManager, deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::Client;
					
					devicesWihoutContract.push_back(device);
				}
			}


			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}

		return cancel;
	}
	else if (command.commandCode == Command::ccGetChannelsStatistics)
	{
		int deviceID = GetParameterValue(CommandParameter::GetChannelsStatistics_DeviceID, command.parameters, "deviceID");
		boost::shared_ptr<GClientServer<GetChannelsStatistics> > getChannels(new GClientServer<GetChannelsStatistics>);
		
		getChannels->Client.DeviceID = deviceID;

		//added by Cristian.Guef
		{
			unsigned short TLDE_SAPID = apUAP1;
			unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			
			//printf ("commnad deviceid = %d", command.deviceID);
			//fflush(stdout);
			DevicePtr device = GetRegisteredDevice(command.devicesManager, command.deviceID);
			DevicesPtrList devicesWihoutContract;
			if (!device->HasContract(resourceID, (unsigned char) GContract::Client))
			{
				LOG_DEBUG("device:" << device->Mac().ToString() << " has  no c/s lease");
				
				device->m_unThrownResourceID = resourceID;
				device->m_unThrownLeaseType = GContract::Client;
				
				devicesWihoutContract.push_back(device);
			}

			//renew lease with new committed burst parameter
			int committedBurst = 0;
			if (GetLeaseCommittedBurst(command.parameters, committedBurst))
			{
				if (device->IsCommittedBurstGreater(resourceID, (unsigned char) GContract::Client, committedBurst))
				{
					device->m_unThrownResourceID = resourceID;
					device->m_unThrownLeaseType = GContract::Client;
					
					devicesWihoutContract.push_back(device);
				}
			}


			if (!devicesWihoutContract.empty())
			{
				THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
			}
		}

		return getChannels;
	}
	else if	(command.commandCode == Command::ccNetworkHealthReport)	//added by Cristian.Guef
	{
		boost::shared_ptr<GNetworkHealthReport> netHealthReport(new GNetworkHealthReport());
		return netHealthReport;
	}
	else if (command.commandCode == Command::ccScheduleReport) //added by Cristian.Guef
	{

		if (command.parameters.size() != 1)
		{
			THROW_EXCEPTION1(InvalidCommandException, "more than one parameter");
		}

		int deviceID =
		    GetParameterValue(CommandParameter::ScheduleReport_DevID, command.parameters, "ScheduleReport_DeviceID");

		LOG_INFO("sched_rep_cmd -> dev_id = " << deviceID);
		boost::shared_ptr<GScheduleReport> schedule(new GScheduleReport(deviceID));
		return schedule;
	}
	else if (command.commandCode == Command::ccNeighbourHealthReport) //added by Cristian.Guef
	{
		if (command.parameters.size() > 1)
		{
			THROW_EXCEPTION1(InvalidCommandException, "more than one parameter");
		}

		int deviceID =
		    GetParameterValue(CommandParameter::NeighbourHealthReport_DevID, command.parameters, "NeighbourHealthReport_DeviceID");
		LOG_INFO("neighbour_health_rep_cmd -> dev_id = " << deviceID);
		boost::shared_ptr<GNeighbourHealthReport> neighbour(new GNeighbourHealthReport(deviceID));
		return neighbour;
	}
	else if (command.commandCode == Command::ccDevHealthReport) //added by Cristian.Guef
	{
		Command::ParametersList list;
		GetParametersValues(CommandParameter::DevHealthReport_DevIDs, command.parameters, list);
		std::vector<int> dev_ids;

		LOG_INFO("dev_health_rep_cmd -> dev_no = " << list.size());
		for (Command::ParametersList::const_iterator it = list.begin(); it != list.end(); it++)
		{
			try
			{
				dev_ids.push_back(boost::lexical_cast<int>(it->parameterValue));
				
				LOG_INFO("\tdev_health_rep_cmd -> dev_id = " << boost::lexical_cast<int>(it->parameterValue));
			}
			catch(boost::bad_lexical_cast &)
			{
				THROW_EXCEPTION1(InvalidCommandException, "DevHealthReport_DevIDs Parameter value invalid!");
			}
		}
			
		boost::shared_ptr<GDeviceHealthReport> dev_health(new GDeviceHealthReport(dev_ids));
		return dev_health;
	}
	else if (command.commandCode == Command::ccNetResourceReport) //added by Cristian.Guef
	{
		boost::shared_ptr<GNetworkResourceReport> netResorceReport(new GNetworkResourceReport());
		return netResorceReport;
	}
	else if (command.commandCode == Command::ccAlertSubscription) //added by Cristian.Guef
	{

	
		DevicePtr device = command.devicesManager->FindDevice(command.deviceID);
		if (!device)
		{
			THROW_EXCEPTION1(DeviceNotFoundException, command.deviceID);
		}
		DevicesPtrList devicesWihoutContract;
		if (!device->HasContract(0/*no resource*/, (unsigned char) GContract::Alert_Subscription))
		{
			LOG_DEBUG("device:" << device->Mac().ToString() << " has no alert lease");
			
			device->m_unThrownResourceID = 0/*no resource*/;
			device->m_unThrownLeaseType = GContract::Alert_Subscription;
			
			devicesWihoutContract.push_back(device);
		}
		if (!devicesWihoutContract.empty())
		{
			THROW_EXCEPTION1(DeviceHasNoContractException, devicesWihoutContract);
		}
		


		boost::shared_ptr<GAlert_Subscription> alertSubscription(new GAlert_Subscription());

		GetSubscriptionList(command, alertSubscription->CategoryTypeList, alertSubscription->NetAddrTypeList);
		

		alertSubscription->LeaseID = device->ContractID(0/*no resource*/, (unsigned char) GContract::Alert_Subscription);
		return alertSubscription;
	}

	LOG_ERROR("Create: Unknon CommandCode=" << command.commandCode);
	THROW_EXCEPTION1(InvalidCommandException, "unknown command");
}

} // namespace hostapp
} // namespace nisa100
