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


#include "DoPreconfigureSubscriberLease.h"


#include <list>

namespace nisa100 {
namespace hostapp {


void DoPreconfigureSubscriberLease_::IssuePublishSubscribeCmd(const MAC& targetDevice, int devID, const PublisherConf::COData &coData, PublisherConf::COChannelListT &list)
{
	Command DoPublishSubscribe;
	DoPublishSubscribe.commandCode = Command::ccPublishSubscribe;
	DoPublishSubscribe.deviceID = devID;
	DoPublishSubscribe.pcoChannelList = &list;
	DoPublishSubscribe.dataContentVer = coData.dataContentVer;

	//parameters
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_Frequency,
		boost::lexical_cast<std::string>(coData.dataPeriod)));
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_FrequencyFaze,
		boost::lexical_cast<std::string>(coData.dataPhase)));
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_Concentrator_id,
		boost::lexical_cast<std::string>(coData.objID)));
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_Concentrator_tlsap_id,
		boost::lexical_cast<std::string>(coData.tsapID)));
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_StaleLimit,
		boost::lexical_cast<std::string>(coData.dataStaleLimit)));

	DoPublishSubscribe.generatedType = Command::cgtAutomatic;
	commands.CreateCommand(DoPublishSubscribe, boost::str(boost::format("system: do subscribe for device:=%1% information")
		% targetDevice.ToString()));

	LOG_INFO("Automatic Do SubscriberLease request was made!"
		<< boost::str(boost::format(" DeviceID=%1%, DeviceMAC=%2%") % DoPublishSubscribe.deviceID % targetDevice.ToString()));

}

void DoPreconfigureSubscriberLease_::IssuePublishSubscribeCmd(const MAC& targetDevice, int devID, const PublisherConf::COData &coData, 
		PublisherConf::COChannelListT &list, PublisherConf::ChannelIndexT &index)
{
	Command DoPublishSubscribe;
	DoPublishSubscribe.commandCode = Command::ccPublishSubscribe;
	DoPublishSubscribe.deviceID = devID;
	DoPublishSubscribe.pcoChannelList = &list;
	DoPublishSubscribe.pcoChannelIndex = &index;
	DoPublishSubscribe.dataContentVer = coData.dataContentVer;
	DoPublishSubscribe.interfaceType = coData.interfaceType;

	//parameters
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_Frequency,
		boost::lexical_cast<std::string>(coData.dataPeriod)));
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_FrequencyFaze,
		boost::lexical_cast<std::string>(coData.dataPhase)));
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_Concentrator_id,
		boost::lexical_cast<std::string>(coData.objID)));
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_Concentrator_tlsap_id,
		boost::lexical_cast<std::string>(coData.tsapID)));
	DoPublishSubscribe.parameters.push_back(CommandParameter(CommandParameter::PublishSubscribe_StaleLimit,
		boost::lexical_cast<std::string>(coData.dataStaleLimit)));

	DoPublishSubscribe.generatedType = Command::cgtAutomatic;
	commands.CreateCommand(DoPublishSubscribe, boost::str(boost::format("system: do subscribe for device:=%1% information")
		% targetDevice.ToString()));

	LOG_INFO("Automatic Do SubscriberLease request was made!"
		<< boost::str(boost::format(" DeviceID=%1%, DeviceMAC=%2%") % DoPublishSubscribe.deviceID % targetDevice.ToString()));

}

void DoPreconfigureSubscriberLease_::Check()
{
	if (!devices.GatewayConnected())
		return; // no reason to send commands to a disconnected GW

	if (IsGatewayReconnected == true)
		return; // no reason to send commands without a session created

	if (DoDeleteSubscriberLease == true)
	{

		LOG_DEBUG("gateway reconnected, so deletion of subscriber leases has begun.");
		DeleteLeases();
		LOG_DEBUG("gateway reconnected, so deletion of subscriber leases has ended.");
		DoDeleteSubscriberLease = false;
	}


	if (configApp.signal2Issued == true)
		ReloadPublishersWithSignal();

	try
	{
		int pubHasSubLease = 0;
		ConfigurePublishers(pubHasSubLease);

		LOG_INFO("configured publishers = " << (boost::uint32_t)configApp.PublishersMapStored.size()
					<< "and only " << (boost::uint32_t)pubHasSubLease << "of them have subscriber lease");
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Check: Do Lease Subscription failed! error=" << ex.what());
	}

}



void DoPreconfigureSubscriberLease_::DeleteLeases()
{
	PublisherConf::PublisherInfoMAP_T::iterator i = configApp.PublishersMapStored.begin();
	NodesRepository::NodesByMACT::const_iterator j = devices.repository.nodesByMAC.begin();
	for (; i != configApp.PublishersMapStored.end(); i++)
	{

		if (j == devices.repository.nodesByMAC.end())
		{
			LOG_DEBUG("delete_leases->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
			continue;
		}
		if (i->first == j->first)
		{
			//found a match...
		}
		else
		{
			if (j->first < i->first)
			{
				bool finished = false;
				do
				{
					j++;
					if (j == devices.repository.nodesByMAC.end())
					{
						finished = true;
						break;
					}
				}while (j->first < i->first);
				if (finished == true)
				{
					LOG_DEBUG("delete_leases->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
					continue; // continue 'for'
				}
				if (i->first < j->first)
				{
					LOG_DEBUG("delete_leases->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
					continue;
				}

				//found a match...
			}
			else
			{
				LOG_DEBUG("delete_leases->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
				continue;
			}
		}


		LOG_DEBUG("delete_leases->pub with mac = " << i->first.ToString() << "found in cache... so check for subscriber lease...");


		//if device has contract
		unsigned short TLDE_SAPID = i->second.coData.tsapID;
		unsigned short ObjID = i->second.coData.objID;
		unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
		if (j->second->HasContract(resourceID, 3/*subscriberleasetype*/))
		{
			LOG_DEBUG("delete_leases->pub with mac = " << i->first.ToString() << "has subscriber lease, so push it on deletion queue...");

			//queue lease
			DoDeleteLeases::InfoForDelContract info;
			info.IPAddress = j->second->IP();
			info.DevID = j->second->id;
			info.LeaseType = 3/*subscriberleasetype*/;
			info.ResourceID = resourceID;

			doDeleteLeases.AddNewInfoForDelLease(
						j->second->ContractID(resourceID, 3/*subscriberleasetype*/),
						info);
			//now reset contract id to force recreate it -not now but just when received confirm
			//device->ResetContractID(resourceID, 3/*subscriberleasetype*/);
		}
		else
		{
			LOG_DEBUG("delete_leases->pub with mac = " << i->first.ToString() << "has no subscriber lease");
		}

		j++;
	}
}

// true there is at least one lease sent (subscriber)
//false - no lease sent and devPtr is filled up with the proper one found in repository for further processing
bool DoPreconfigureSubscriberLease_::IsAnyLeaseSent()
{
	PublisherConf::PublisherInfoMAP_T::iterator i = configApp.PublishersMapStored.begin();
	NodesRepository::NodesByMACT::const_iterator j = devices.repository.nodesByMAC.begin();
	for (; i != configApp.PublishersMapStored.end(); i++)
	{

		if (j == devices.repository.nodesByMAC.end())
		{
			LOG_DEBUG("is_any_lease_sent->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
			i->second.coData.devPtr.reset();
			continue;
		}
		if (i->first == j->first)
		{
			//found a match...
		}
		else
		{
			if (j->first < i->first)
			{
				bool finished = false;
				do
				{
					j++;
					if (j == devices.repository.nodesByMAC.end())
					{
						finished = true;
						break;
					}
				}while (j->first < i->first);
				if (finished == true)
				{
					LOG_DEBUG("is_any_lease_sent->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
					i->second.coData.devPtr.reset();
					continue; // continue 'for'
				}
				if (i->first < j->first)
				{
					LOG_DEBUG("is_any_lease_sent->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
					i->second.coData.devPtr.reset();
					continue;
				}

				//found a match...
			}
			else
			{
				LOG_DEBUG("is_any_lease_sent->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
				i->second.coData.devPtr.reset();
				continue;
			}
		}

		i->second.coData.devPtr = j->second;	//for later processing
		if (j->second->issuePublishSubscribeCmd == false)
		{
			LOG_DEBUG("is_any_lease_sent->pub with mac = " << i->first.ToString() << "has its lease sent...");
			return true;
		}
		LOG_DEBUG("are_leases_sent->pub with mac = " << i->first.ToString() << "has not its lease sent...");
		j++;
	}

	return false;
}

//0 lease deleted
//-1 no device
//-2 no lease
int DoPreconfigureSubscriberLease_::DoDeleteLease(const DevicePtr devPtr, unsigned short tsapID, unsigned short objID)
{
	if (!devPtr)
		return -1;

	//if device has contract
	unsigned short TLDE_SAPID = tsapID;
	unsigned short ObjID = objID;
	unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
	if (devPtr->HasContract(resourceID, 3/*subscriberleasetype*/))
	{
		//queue lease
		DoDeleteLeases::InfoForDelContract info;
		info.IPAddress = devPtr->IP();
		info.DevID = devPtr->id;
		info.LeaseType = 3/*subscriberleasetype*/;
		info.ResourceID = resourceID;

		doDeleteLeases.AddNewInfoForDelLease(
					devPtr->ContractID(resourceID, 3/*subscriberleasetype*/),
					info);
		//now reset contract id to force recreate it -not now but just when received confirm
		//device->ResetContractID(resourceID, 3/*subscriberleasetype*/);
		return 0;
	}

	return -2;
}


// rez
// 0 - do not update channels list in cache
// 1 - do update channels list in cache
int DoPreconfigureSubscriberLease_::ProcessChannelsDifference(int loadedDeviceID,
			const PublisherConf::COChannelListT &storedList, const PublisherConf::ChannelIndexT &storedIndex,
			PublisherConf::COChannelListT &loadedList, const PublisherConf::ChannelIndexT &loadedIndex)
{

	int rez = 0;
	//
	PublisherConf::ChannelIndexT::const_iterator j = storedIndex.begin();
	PublisherConf::ChannelIndexT::const_iterator i = loadedIndex.begin();

	for (; i != loadedIndex.end(); ++i)
	{
		if (j == storedIndex.end())
		{
			rez = 1;
			//create channel
			int channelNo = devices.CreateChannel(loadedDeviceID, loadedList[i->second]);
			devices.CreateEmptyReading(loadedDeviceID, channelNo);
			continue;
		}
		if (i->first == j->first)
		{
			//found a match...
		}
		else
		{
			if (j->first < i->first)
			{
				do
				{
					rez = 1;
					//move channel
					devices.MoveChannelToHistory(storedList[j->second].dbChannelNo);
					devices.DeleteReading(storedList[j->second].dbChannelNo);
					++j;
					if (j == storedIndex.end())
						break;
				} while (j->first < i->first);

				if (j == storedIndex.end())
				{
					rez = 1;
					//create chanel
					int channelNo = devices.CreateChannel(loadedDeviceID, loadedList[i->second]);
					devices.CreateEmptyReading(loadedDeviceID, channelNo);
					continue;
				}
				if (i->first < j->first)
				{
					rez = 1;
					//create channel
					int channelNo = devices.CreateChannel(loadedDeviceID, loadedList[i->second]);
					devices.CreateEmptyReading(loadedDeviceID, channelNo);
					continue;
				}

				//found a match...
			}
			else
			{
				rez = 1;
				//create channel
				int channelNo = devices.CreateChannel(loadedDeviceID, loadedList[i->second]);
				devices.CreateEmptyReading(loadedDeviceID, channelNo);
				continue;
			}
		}

		//
		loadedList[i->second].dbChannelNo = storedList[j->second].dbChannelNo;

		//check channel info
		LOG_DEBUG("stored name = " << storedList[j->second].name << " format=" << (int)storedList[j->second].format << " measure=" << storedList[j->second].unitMeasure << " withStatus=" << storedList[j->second].withStatus <<
			"loaded name = " << loadedList[i->second].name << " format=" << (int)loadedList[i->second].format << " neasure=" << loadedList[i->second].unitMeasure << " withStatus=" << loadedList[i->second].withStatus);
		if (storedList[j->second].name != loadedList[i->second].name ||
			storedList[j->second].format != loadedList[i->second].format ||
			storedList[j->second].unitMeasure != loadedList[i->second].unitMeasure || 
			storedList[j->second].withStatus != loadedList[i->second].withStatus)
		{
			//update channel
			devices.UpdateChannel(storedList[j->second].dbChannelNo, loadedList[i->second].name, loadedList[i->second].unitMeasure, loadedList[i->second].format, loadedList[i->second].withStatus);
			rez = 1;
		}

		++j;
	}
	
	while (j != storedIndex.end())
	{
		rez = 1;
		//move channel
		devices.MoveChannelToHistory(storedList[j->second].dbChannelNo);
		devices.DeleteReading(storedList[j->second].dbChannelNo);
		++j;
	}

	return rez;
}

// 0  - the same
// -1 - publish channels are not the same
// -2 - channel info are not the same
int DoPreconfigureSubscriberLease_::GetChannelsDifference(const PublisherConf::COChannelListT &storedList, const PublisherConf::ChannelIndexT &storedIndex,
							PublisherConf::COChannelListT &loadedList, const PublisherConf::ChannelIndexT &loadedIndex)
{

	//
	if (storedList.size() != loadedList.size())
		return -1;

	PublisherConf::ChannelIndexT::const_iterator i = storedIndex.begin();
	PublisherConf::ChannelIndexT::const_iterator j = loadedIndex.begin();

	int result = 0;
	for (; i != storedIndex.end(); i++)
	{

		if (j == loadedIndex.end())
		{
			//LOG
			return -1;
		}
		if (i->first == j->first)
		{
			//found a match...
		}
		else
		{
			//LOG
			return -1;
		}

		//
		loadedList[j->second].dbChannelNo = storedList[i->second].dbChannelNo;

		//check channel info
		LOG_DEBUG("stored name = " << storedList[i->second].name << " format=" << (int)storedList[i->second].format << " neasure=" << storedList[i->second].unitMeasure <<
			"loaded name = " << loadedList[j->second].name << " format=" << (int)loadedList[j->second].format << " neasure=" << loadedList[j->second].unitMeasure );
		if (storedList[i->second].name != loadedList[j->second].name ||
			storedList[i->second].format != loadedList[j->second].format ||
			storedList[i->second].unitMeasure != loadedList[j->second].unitMeasure)
		{
			result = -2;
		}

		j++;
	}
	return result;
}

/*
 * priority checking:													decoupled (but not independent) perspectives:
 *	1. Concetrator (tsapID, objID, dataPeriod, dataPhase, staleLimit)	<- item from Lease Create perspective
 *	2. PublishChannel (tsapID, objID, attrID, index1, index2)			<- item from DB Delete perspective
 *	3. ChannelInfo (format, name, unitOfMesurement)						<- item from DB Update perspective
 *	4. DataVersion (publish_version)									<- item from Published Data Update perspective
 */
/*
 * items comparing cases:
 * a) when 1 fails -> new lease, new db channles, new publishing_info for the coresponding CO, skip checking 2..4
 * b) when 2 fails -> delete (all)old channels and add(all) new channels, skip checking 3
 * c) when 3 fails -> update (all)old channels
 * d) when 4 fails -> update it
 * NOTE!
 */
extern void ChangeDataVersionNo(int handle, unsigned char version);
extern void ChangeInterfaceType(int handle, unsigned char interfaceType);
extern void NewPublishChannles(int handle, const PublisherConf::COChannelListT &list);
void DoPreconfigureSubscriberLease_::ReloadPublishersWithSignal()
{
	//disable signal2 processing
	configApp.processSignal2 = false;

	if (IsAnyLeaseSent() ==  true)
		return;		//waiting for leases request to get response


	//compare publishers
	///////////////////////////////////////////////////
	PublisherConf::PublisherInfoMAP_T::const_iterator i = configApp.PublishersMapStored.begin();
	PublisherConf::PublisherInfoMAP_T::iterator j = configApp.PublishersMapLoaded.begin();
	for (; i != configApp.PublishersMapStored.end(); i++)
	{

		if (j == configApp.PublishersMapLoaded.end())
		{
			LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "not found in loaded_map... so delete lease");

			int result = DoDeleteLease(i->second.coData.devPtr, i->second.coData.tsapID, i->second.coData.objID);
			switch(result)
			{
			case 0:
				LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "has subscriber lease, so push it on deletion queue...");
				break;
			case -1:
				LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
				break;
			case -2:
				LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "has no subscriber lease");
				break;
			default:
				assert(false);
			}
			if (i->second.coData.devPtr)
				devices.DeletePublishInfo(i->second.coData.devPtr->id);
			continue;
		}
		if (i->first == j->first)
		{
			//found a match...
		}
		else
		{
			if (j->first < i->first)
			{
				bool finished = false;
				do
				{
					//we have new publishers...
					++j;
					if (j == configApp.PublishersMapLoaded.end())
					{
						finished = true;
						break;
					}
				}while (j->first < i->first);
				if (finished == true)
				{
					LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "not found in loaded_map... so delete lease");

					int result = DoDeleteLease(i->second.coData.devPtr, i->second.coData.tsapID, i->second.coData.objID);
					switch(result)
					{
					case 0:
						LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "has subscriber lease, so push it on deletion queue...");
						break;
					case -1:
						LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
						break;
					case -2:
						LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "has no subscriber lease");
						break;
					default:
						assert(false);
					}
					if (i->second.coData.devPtr)
						devices.DeletePublishInfo(i->second.coData.devPtr->id);
					continue; // continue 'for'
				}
				if (i->first < j->first)
				{
					LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "not found in loaded_map... so delete lease");

					int result = DoDeleteLease(i->second.coData.devPtr, i->second.coData.tsapID, i->second.coData.objID);
					switch(result)
					{
					case 0:
						LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "has subscriber lease, so push it on deletion queue...");
						break;
					case -1:
						LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
						break;
					case -2:
						LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "has no subscriber lease");
						break;
					default:
						assert(false);
					}
					if (i->second.coData.devPtr)
						devices.DeletePublishInfo(i->second.coData.devPtr->id);
					continue;
				}

				//found a match...
			}
			else
			{
				LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "not found in loaded_map... so delete lease");

				int result = DoDeleteLease(i->second.coData.devPtr, i->second.coData.tsapID, i->second.coData.objID);
				switch(result)
				{
				case 0:
					LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "has subscriber lease, so push it on deletion queue...");
					break;
				case -1:
					LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
					break;
				case -2:
					LOG_DEBUG("compare_publishers->pub with mac = " << i->first.ToString() << "has no subscriber lease");
					break;
				default:
					assert(false);
				}
				if (i->second.coData.devPtr)
					devices.DeletePublishInfo(i->second.coData.devPtr->id);
				continue;
			}
		}

		LOG_DEBUG("match->pub with mac = " << i->first.ToString() << "and mac" << j->first.ToString());

		//the concentrator MAC is the same
		//see lease differences
		j->second.coData.HasLease = i->second.coData.HasLease;
		if (i->second.coData.tsapID != j->second.coData.tsapID || i->second.coData.objID != j->second.coData.objID ||
			i->second.coData.dataPeriod != j->second.coData.dataPeriod || i->second.coData.dataStaleLimit != j->second.coData.dataStaleLimit ||
			i->second.coData.dataPhase != j->second.coData.dataPhase)
		{
			if (i->second.coData.devPtr)
			{
				DoDeleteLease(i->second.coData.devPtr, i->second.coData.tsapID, i->second.coData.objID);
				
				ProcessChannelsDifference(i->second.coData.devPtr->id, i->second.coChannelList, i->second.channelIndex,
								j->second.coChannelList, j->second.channelIndex);
			}

			++j;
			continue;
		}

		if (i->second.coData.devPtr)
			if (ProcessChannelsDifference(i->second.coData.devPtr->id, i->second.coChannelList, i->second.channelIndex,
							j->second.coChannelList, j->second.channelIndex) == 1/*do update*/)
			{
				if (i->second.coData.HasLease == true)
					NewPublishChannles(i->second.coData.devPtr->GetPublishHandle(), j->second.coChannelList);
			}

		/*
		//see the differences
		int res;
		if ((res = GetChannelsDifference(i->second.coChannelList, i->second.channelIndex,
			j->second.coChannelList, j->second.channelIndex)) < 0)
		{
			if (res == -1 && i->second.coData.HasLease == true)
			{
				std::vector<int> list;
				list.resize(i->second.coChannelList.size());
				for (int k = 0; k < i->second.coChannelList.size(); k++)
					list[k] = i->second.coChannelList[k].dbChannelNo;
				devices.DeletePublishChannels(list);
				devices.SavePublishChannels(i->second.coData.devPtr->id, j->second.coChannelList);
				NewPublishChannles(i->second.coData.devPtr->GetPublishHandle(), j->second.coChannelList);
			}
			if (res == -2 && i->second.coData.HasLease == true)
				devices.UpdateChannelsInfo(j->second.coChannelList);
		}
		*/
		if (i->second.coData.dataContentVer != j->second.coData.dataContentVer && i->second.coData.HasLease == true)
			ChangeDataVersionNo(i->second.coData.devPtr->GetPublishHandle(), j->second.coData.dataContentVer);

		if (i->second.coData.interfaceType != j->second.coData.interfaceType && i->second.coData.HasLease == true)
			ChangeInterfaceType(i->second.coData.devPtr->GetPublishHandle(), j->second.coData.interfaceType);
		
		++j;
	}

	//finished processing
	configApp.PublishersMapStored.clear();
	configApp.PublishersMapStored = configApp.PublishersMapLoaded;
	configApp.PublishersMapLoaded.clear();

	//enable signal2 processing
	configApp.signal2Issued = false;
	configApp.processSignal2 = true;
}

void DoPreconfigureSubscriberLease_::ConfigurePublishers(int &pubHasSubLease)
{
	PublisherConf::PublisherInfoMAP_T::iterator i = configApp.PublishersMapStored.begin();
	NodesRepository::NodesByMACT::const_iterator j = devices.repository.nodesByMAC.begin();
	for (; i != configApp.PublishersMapStored.end(); i++)
	{
		if (i->second.coData.HasLease == true)
		{
			pubHasSubLease++;
			continue;
		}

		if (j == devices.repository.nodesByMAC.end())
		{
			LOG_DEBUG("configure_publishers->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
			continue;
		}
		if (i->first == j->first)
		{
			//found a match...
		}
		else
		{
			if (j->first < i->first)
			{
				bool finished = false;
				do
				{
					j++;
					if (j == devices.repository.nodesByMAC.end())
					{
						finished = true;
						break;
					}
				}while (j->first < i->first);
				if (finished == true)
				{
					LOG_DEBUG("configure_publishers->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
					continue; // continue 'for'
				}
				if (i->first < j->first)
				{
					LOG_DEBUG("configure_publishers->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
					continue;
				}

				//found a match...
			}
			else
			{
				LOG_DEBUG("configure_publishers->pub with mac = " << i->first.ToString() << "not found in cache... Skipping check...");
				continue;
			}
		}

		if (j->second->IsRegistered() == false)
		{
			LOG_DEBUG("configure_publishers->pub with mac = " << i->first.ToString() << "not registered ... Skipping check...");

			unsigned short TLDE_SAPID = i->second.coData.tsapID;
			unsigned short ObjID = i->second.coData.objID;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			if (j->second->HasContract(resourceID, 3/*subscriberleasetype*/))
			{
				pubHasSubLease++;
				i->second.coData.HasLease = true; //no need to check it again
			}

			j++;
			continue;
		}

		//if device has contract
		unsigned short TLDE_SAPID = i->second.coData.tsapID;
		unsigned short ObjID = i->second.coData.objID;
		unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
		if (j->second->HasContract(resourceID, 3/*subscriberleasetype*/))
		{
			LOG_DEBUG("pub with mac = " << i->first.ToString() << "has already subscriber lease");
			pubHasSubLease++;
			i->second.coData.HasLease = true; //no need to check it again
			j++;
			continue;
		}

		if(j->second->issuePublishSubscribeCmd == true){
			LOG_DEBUG("pub with mac = " << i->first.ToString() << " has no subscriber lease, so send subscriber lease request ...");

			IssuePublishSubscribeCmd(i->first, j->second->id, i->second.coData, i->second.coChannelList, i->second.channelIndex);
			j->second->issuePublishSubscribeCmd = false;
			j++;
			continue;
		}


		LOG_DEBUG("pub with mac = " << i->first.ToString() << "has no subscriber lease and already sent subscriber lease request");
		j++;
	}
}


}//namespace hostapp
}//namespace nisa100
