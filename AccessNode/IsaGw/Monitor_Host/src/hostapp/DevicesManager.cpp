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

#include "DevicesManager.h"
#include "commandmodel/Nisa100ObjectIDs.h"

#include <cmath>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp> //for converting numeric <-> to string


//added by Cristian.Guef
#include "./processor/ReportsProcessor.h"
#include "./processor/PublishedDataMng.h"
#include <../AccessNode/Shared/DurationWatcher.h>

namespace nisa100 {
namespace hostapp {


DevicesManager::DevicesManager(IFactoryDal& factoryDal_, ConfigApp& p_rConfigApp) :
	factoryDal(factoryDal_), m_rConfigApp(p_rConfigApp)
{
	isGatewayConnected = false;
}

DevicesManager::~DevicesManager()
{
}


void DevicesManager::ResetTopology()
{
	repository.Clear();

	DeviceList devices;
	factoryDal.Devices().ResetDevices(Device::dsUnregistered);
	factoryDal.Devices().GetDevices(devices);

	for (DeviceList::iterator it = devices.begin(); it != devices.end(); it++)
	{
		DevicePtr node(new Device(*it));

		/*commented by CRistian.Guef
		repository.Add(node);
		*/
		//added by Cristian.Guef - delete devices that have the same MAC so that it remains just one of them in db
		if (!repository.Add(node))
		{
			NodesRepository::NodesByMACT::iterator it2 = repository.nodesByMAC.find(node->Mac());
			assert(it2 != repository.nodesByMAC.end());
			if (it2->second->Type() == Device::dtGateway)
			{
				factoryDal.Devices().DeleteDevice(node->id);
			}
			else
			{
				if (node->Type() == Device::dtGateway)
				{
					factoryDal.Devices().DeleteDevice(it2->second->id);
					repository.nodesByMAC.erase(it2);
					assert(repository.Add(node));

					if (node->Type() == 2 /*gw*/)
						m_gwDevPtr = node;
				}
				else
				{
					factoryDal.Devices().DeleteDevice(node->id);
				}
			}
		}
		else
		{
			if (node->Type() == 2 /*gw*/)
				m_gwDevPtr = node;
			if (node->Type() == Device::dtSystemManager)
				m_smDevPtr = node;
		}
	}
	repository.UpdateIPv6Index();
	repository.UpdateIdIndex();

	//[nicu.dascalu] just check that we have configurated a gateway
	GatewayDevice();

	//delete all publish channels
	//DeleteAllPublishChannels();
}

//commented by Cristian.Guef
//void DevicesManager::ProcessTopology(GTopologyReport& topologyReport)
//{
//	LOG_INFO("ProcessTopology: begins. DeviceCount=" << topologyReport.DevicesList.size());
//
//	ResetDevicesChanges();
//
//	//FIXME [nicu.dascalu] - make Process Top transactional, do not alter cash is an eror occured
//
//	// STEP 0: detect if SM is rejoined, and mark all devices as rejoined
//	{
//		LOG_INFO("Checking for SM rejoin...");
//		GTopologyReport::DevicesListT::const_iterator it = topologyReport.DevicesList.begin();
//		for (; it != topologyReport.DevicesList.end(); it++)
//		{
//			if (it->DeviceType == Device::dtSystemManager)
//			{
//				break;
//			}
//		}
//
//		bool isSMRejoined = false;
//		DevicePtr existingSM = SystemManagerDevice();
//		if (existingSM && it != topologyReport.DevicesList.end())
//		{
//			LOG_INFO("OLD SM RejoinCount=" << existingSM->rejoinCount << " and new RejoinCount=" << it->DeviceRejoinCount);
//			if (existingSM->rejoinCount != it->DeviceRejoinCount)
//			{
//				isSMRejoined = true;
//			}
//		}
//		else
//		{
//			LOG_INFO("No SM found...");
//		}
//
//		if (isSMRejoined)
//		{
//			LOG_INFO("System manager has rejoined. Invalidating all nodes...");
//			for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
//			{
//				it->second->Status(Device::dsUnregistered);
//				it->second->RejoinCount(0);
//			}
//		}
//	}
//
//	{
//		//STEP 1: all unfound devices in topo report are marked as unregistered
//
//		typedef std::map<MAC, const GTopologyReport::Device*> TopoDeviceMap;
//		TopoDeviceMap reportedDevices;
//		for (GTopologyReport::DevicesListT::const_iterator it = topologyReport.DevicesList.begin(); it
//		    != topologyReport.DevicesList.end(); it++)
//		{
//			reportedDevices[it->DeviceMAC] = &(*it);
//		}
//
//		//all unmatched devices in reported topo are marked as unregistered in our repository.
//		for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
//		{
//			if (reportedDevices.end() == reportedDevices.find(it->first))
//			{
//				it->second->Status(Device::dsUnregistered);
//			}
//		}
//	}
//
//	LOG_DEBUG("ProcessTopology: Create or Update exiting devices ...");
//	{
//		//STEP 2: create new or update existing from reported topo
//		for (GTopologyReport::DevicesListT::const_iterator it = topologyReport.DevicesList.begin(); it
//		    != topologyReport.DevicesList.end(); it++)
//		{
//			DevicePtr node = repository.Find(it->DeviceMAC);
//			if (!node)
//			{
//				node.reset(new Device());
//				node->Mac(it->DeviceMAC);
//				repository.Add(node);
//			}
//			//DeviceType field:
//			// first 4 (0-3) bits tell the type of device, SM, GW, BBR, Routing, Non-Routing Device
//			node->Type((Device::DeviceType) it->DeviceType);
//			// bit 4 tells has battery power, bit 5 tells has aquisition board
//			node->DeviceCapabilities((0x20 & it->DeviceCapabilities) != 0, (0x10 & it->DeviceCapabilities) != 0);
//			node->DeviceImplType(it->DeviceImplementationType);
//
//			node->RejoinCount(it->DeviceRejoinCount);
//
//			switch (it->DeviceStatus)
//			{
//			case 3: /*JOIN_CONFIRMED*/
//			{
//				node->Status(Device::dsRegistered);
//				node->IP(it->DeviceIP);
//				break;
//			}
//			case 2: /*JOIN_REQUEST_RECEIVED*/
//			{
//				node->Status(Device::dsRegistering);
//				node->IP(it->DeviceIP);
//				break;
//			}
//			default:
//			{
//				node->Status(Device::dsUnregistered);
//				LOG_WARN("ProcessTopology: unexpected DeviceStatus=" << it->DeviceStatus << " for Device=" << node->ToString());
//				break;
//			}
//			}
//
//			if (node->Rejoined() || !node->IsRegistered())
//			{
//				LOG_WARN("ProcessTopology: Reset contract for rejoined/unregistered Device=" << node->ToString());
//				node->ResetContractID();
//			}
//		}
//		repository.UpdateIPv6Index();
//
//		LOG_DEBUG("ProcessTopology: Compute node levels ...");
//		ComputeNodeLevels(topologyReport.DevicesList);
//	}
//
//	{
//		//STEP 3: save changes to db
//		LOG_DEBUG("ProcessTopology: Updating db ...");
//		try
//		{
//			//now update to db
//			factoryDal.BeginTransaction();
//
//			for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
//			{
//				if (it->second->Changed())
//				{
//					if (it->second->id == -1)
//					{
//						AddDevice(*it->second);
//						LOG_INFO("ProcessTopology: Discovered new Device=" << it->second->ToLongString());
//					}
//					else
//					{
//						factoryDal.Devices().UpdateDevice(*it->second);
//						LOG_INFO("ProcessTopology: Updated existing Device=" << it->second->ToLongString());
//					}
//				}
//			}
//			repository.UpdateIdIndex(); //some nodes where newly added, so reindex their ids(from db)
//
//			// update neighbors & graphs
//			factoryDal.Devices().CleanDeviceNeighbours();
//			factoryDal.Devices().CleanDeviceGraphs();
//			for (GTopologyReport::DevicesListT::const_iterator it = topologyReport.DevicesList.begin(); it
//					!= topologyReport.DevicesList.end(); it++)
//			{
//				DevicePtr fromDevice = repository.Find(it->DeviceMAC); //alway exists
//				SaveDeviceNeighbours(fromDevice, it->NeighborsList);
//				SaveDeviceGraphs(fromDevice, it->GraphsList);
//			}
//
//			factoryDal.CommitTransaction();
//		}
//		catch (std::exception& ex)
//		{
//			factoryDal.RollbackTransaction();
//			LOG_ERROR("ProcessTopology: Failed to updated device in db! error=" << ex.what());
//			ResetDevicesChanges();
//			throw;
//		}
//		catch (...)
//		{
//			factoryDal.RollbackTransaction();
//			LOG_ERROR("ProcessTopology: Failed to updated device in db! unknown error!");
//			ResetDevicesChanges();
//			throw;
//		}
//	}
//
//	{
//		//STEP 4: notify observer about topology changed, if has been changed
//		bool topoChanged = false;
//		for (NodesRepository::NodesByIDT::iterator it = repository.nodesById.begin(); it != repository.nodesById.end(); it++)
//		{
//			if (it->second->Changed())
//			{
//				topoChanged = true;
//				break;
//			}
//		}
//
//		if (topoChanged)
//		{
//			FireTopologyChanged();
//		}
//	}
//
//	if (LOG_INFO_ENABLED())
//	{
//		int unregisteredDevices = 0;
//		int registeredDevice = 0;
//		int rejoinedDevices = 0;
//
//		LOG_INFO("ProcessTopology: print. DevicesCount=" << repository.nodesByMAC.size());
//		for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
//		{
//			if (it->second->IsRegistered())
//			{
//				registeredDevice++;
//			}
//			else
//			{
//				unregisteredDevices++;
//			}
//			if (it->second->Rejoined())
//			{
//				rejoinedDevices++;
//			}
//			LOG_INFO("ProcessTopology: print.   Device=" << it->second->ToLongString());
//		}
//		LOG_INFO("ProcessTopology: finished. DevicesCount=" << repository.nodesByMAC.size() << " RegisteredCount="
//		    << registeredDevice << " RejoinedCount=" << rejoinedDevices << " UnregisteredCount=" << unregisteredDevices);
//	}
//
//	LOG_DEBUG("ProcessTopology: Reseting devices changes ...");
//	ResetDevicesChanges();
//}


//added by Cristian.Guef
void DevicesManager::UnregisterDevices(ReportsProcessor *pRepProcessor, GTopologyReport& topologyReport, bool &doUpdateIPv6Index)
{
	LOG_DEBUG("unregister_devices--> begin --------");

	if (pRepProcessor == NULL)
	{
		//LOG_DEBUG("unreg - ip_set_size = " << repository.nodesByIPv6.size());

		for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
		{
			it->second->Status(Device::dsUnregistered);
			doUpdateIPv6Index = true;
		}
	}
	else
	{
		ReportsProcessor::TopoStructIndexT::const_iterator j = (*pRepProcessor).GetTopoStructIndex()->begin();

		for(NodesRepository::NodesByIPv6T::const_iterator i = repository.nodesByIPv6.begin(); i != repository.nodesByIPv6.end(); ++i)
		{ //for every ip check if it is in topology_struct

			if (j == (*pRepProcessor).GetTopoStructIndex()->end())
			{
				LOG_WARN("unregister_devices - dev ip = " <<  i->first.ToString() <<
		 				" was set unregistered");
				i->second->Status(Device::dsUnregistered);
				doUpdateIPv6Index = true;
				continue;
			}
			if (i->first == j->first.deviceIP)
			{
				//found a match...
			}
			else
			{
				if (j->first.deviceIP < i->first)
				{
					bool finished = false;
					do
					{
						++j;
						if (j == (*pRepProcessor).GetTopoStructIndex()->end())
						{
							finished = true;
							break;
						}
					}while (j->first.deviceIP < i->first);
					if (finished == true)
					{
						LOG_WARN("unregister_devices - dev ip = " <<  i->first.ToString() <<
			 					" was set unregistered");
						i->second->Status(Device::dsUnregistered);
						doUpdateIPv6Index = true;
						continue; // continue 'for'
					}
					if (i->first < j->first.deviceIP)
					{
						LOG_WARN("unregister_devices - dev ip = " <<  i->first.ToString() <<
			 					" was set unregistered");
						i->second->Status(Device::dsUnregistered);
						doUpdateIPv6Index = true;
						continue;
					}

					//found a match...
				}
				else
				{
					LOG_WARN("unregister_devices - dev ip = " <<  i->first.ToString() <<
			 				" was set unregistered");
					i->second->Status(Device::dsUnregistered);
					doUpdateIPv6Index = true;
					continue;
				}
			}

			//check if mac is the same...
			if (i->second->mac != topologyReport.DevicesList[j->second].DeviceMAC)
			{
				LOG_WARN("unregister_devices - dev ip = " <<  i->first.ToString() <<
			 				" was set unregistered because of MAC diferrence");
				i->second->Status(Device::dsUnregistered);
				doUpdateIPv6Index = true;
			}

			//increment here
			++j;
		}
	}
	LOG_DEBUG("unregister_devices--> end --------");

}


void DevicesManager::UpdateUnregDevices(ReportsProcessor *pRepProcessor)
{

	if (pRepProcessor == NULL)
		return;

	LOG_DEBUG("update_unregister_devices--> begin --------");

	ReportsProcessor::UnregDevsListT::const_iterator j = (*pRepProcessor).GetUnregisteredDevs()->begin();

	DevicePtr node;

	for(NodesRepository::NodesByMACT::iterator i = repository.nodesByMAC.begin(); i != repository.nodesByMAC.end(); ++i)
	{ //for every ip check if it is in topology_struct

		if (i->second->Status() == Device::dsRegistered)
		{
			LOG_DEBUG("update_unregister_devices: this device is registered: " << i->second->ToString());
			continue;
		}

		if (j == (*pRepProcessor).GetUnregisteredDevs()->end())
		{
			i->second->Status(Device::dsUnregistered);
			continue;
		}

		if (i->first == j->mac)
		{
			//found a match...
		}
		else
		{
			if (j->mac < i->first)
			{
				do
				{
					node.reset(new Device());
					node->Mac(j->mac);
					repository.Add(node);

					node->Status((Device::DeviceStatus)j->DeviceStatus);
					node->IP(j->ip);
					node->Type((Device::DeviceType) j->DeviceType);
					node->SetTAG(j->DeviceTag);
					node->SetModel(j->DeviceModel);
					node->SetRevision(j->DeviceRevision);
					node->SetManufacturer(j->DeviceManufacture);
					node->SetSerialNo(j->DeviceSerialNo);
					node->PowerStatus(j->PowerSupplyStatus);

					LOG_DEBUG("update_unregister_devices: this device is not in map and is marked as unregistered: " << j->mac.ToString());
					++j;
					if (j == (*pRepProcessor).GetUnregisteredDevs()->end())
						break;
				}while (j->mac < i->first);

				if (j == (*pRepProcessor).GetUnregisteredDevs()->end())
				{
					i->second->Status(Device::dsUnregistered);
					LOG_DEBUG("update_unregister_devices: this device is set unregistered: " << i->second->ToString());
					continue;
				}

				if (i->first < j->mac)
				{
					i->second->Status(Device::dsUnregistered);
					LOG_DEBUG("update_unregister_devices: this device is set unregistered: " << i->second->ToString());
					continue;
				}

				//found a match...
			}
			else
			{
				i->second->Status(Device::dsUnregistered);
				continue;
			}
		}

		//LOG_INFO("device_status = " << (int)j->DeviceStatus << " for device_mac = " << j->mac);

		i->second->Status((Device::DeviceStatus)j->DeviceStatus);

		if (i->second->IP() != j->ip)
		{
			LoadSubscribeLeaseForDel(i->second);
		}

		i->second->IP(j->ip);
		i->second->Type((Device::DeviceType) j->DeviceType);
		i->second->SetTAG(j->DeviceTag);
		i->second->SetModel(j->DeviceModel);
		i->second->SetRevision(j->DeviceRevision);
		i->second->SetManufacturer(j->DeviceManufacture);
		i->second->SetSerialNo(j->DeviceSerialNo);
		i->second->PowerStatus(j->PowerSupplyStatus);

		LOG_DEBUG("update_unregister_devices: this device was set: " << i->second->ToString());

		//increment here
		++j;
	}

	while (j != (*pRepProcessor).GetUnregisteredDevs()->end())
	{
		node.reset(new Device());
		node->Mac(j->mac);
		repository.Add(node);

		node->Status((Device::DeviceStatus)j->DeviceStatus);
		node->IP(j->ip);
		node->Type((Device::DeviceType) j->DeviceType);
		node->SetTAG(j->DeviceTag);
		node->SetModel(j->DeviceModel);
		node->SetRevision(j->DeviceRevision);
		node->SetManufacturer(j->DeviceManufacture);
		node->SetSerialNo(j->DeviceSerialNo);
		node->PowerStatus(j->PowerSupplyStatus);


		LOG_DEBUG("update_unregister_devices: this device is not in map and is marked as unregistered: " << j->mac.ToString());

		++j;
	}

	LOG_DEBUG("update_unregister_devices--> end --------");
}

bool DevicesManager::IsDevDeletedFromDB(Device& device)
{
	if (!factoryDal.Devices().IsDeviceInDB(device.id))
	{
		LOG_INFO("device with mac = " << device.ToString() << "and with ip = " << device.IP().ToString()  << "has been deleted from db");

		//push it
		InfoToDelete info;
		info.IPAddress = device.IP();
		info.DevID = device.id;

		unsigned int k = 3/*subscriber lease*/; //we only delete leases that are of type 'subscriber'
												//because the others are deleted automatically by the gw
												// when dev is rejoined or unregistered and we are based on (lease expired)
											// when using it
		for (std::map<unsigned int, boost::uint32_t>::iterator l = device.m_ResourceIDToContractID[k].begin();
				l != device.m_ResourceIDToContractID[k].end(); l++)
		{

			//we have contract
			info.ContractID = l->second;
			info.ResourceID = l->first;
			info.LeaseType = k;
			//now
			queueToDeleteLease.push(info);

			LOG_INFO("dev deleted from db -> found LeaseID = " << (boost::uint32_t)info.ContractID
						<< "and Objid = " << (boost::uint32_t)(info.ResourceID >> 16)
						<< "and TLSAP_id = " << (boost::uint32_t)(info.ResourceID & 0xFFFF)
						<< "and lease_type = " << (boost::uint32_t)info.LeaseType
						<< "for dev with ip = " << info.IPAddress.ToString()
						<< "and with mac = " << device.mac.ToString());
		}

		return true;
	}

	return false;
}

void DevicesManager::BuildNewDevice(DevicePtr devPtr, IPv6 ip, GDeviceListReport::Device &devInfo)
{
	devPtr->Mac(devInfo.DeviceMAC);
	devPtr->IP(ip);
	devPtr->Status((Device::DeviceStatus)devInfo.DeviceStatus);
	devPtr->SetTAG(devInfo.DeviceTag);
	devPtr->SetModel(devInfo.DeviceModel);
	devPtr->SetRevision(devInfo.DeviceRevision);
	devPtr->SetManufacturer(devInfo.DeviceManufacture);
	devPtr->SetSerialNo(devInfo.DeviceSerialNo);
	devPtr->PowerStatus(devInfo.PowerSupplyStatus);

	switch (devInfo.DeviceType)
	{
		case 1:
			devPtr->Type((Device::DeviceType) 11/*nonrouting device*/);
			break;
		case 2:
			devPtr->Type((Device::DeviceType) 10/*routing device*/);
			break;
		case 3:
			devPtr->Type((Device::DeviceType) 12/*routing and IO device*/);
			break;
		case 4:
			devPtr->Type((Device::DeviceType) 3/*backbone router*/);
			m_bbrDevPtr = devPtr;
			break;
		case 8:
			devPtr->Type((Device::DeviceType) 2/*gateway*/);
			m_gwDevPtr = devPtr;
			break;
		case 16:
			devPtr->Type((Device::DeviceType) 1/*system manager*/);
			m_smDevPtr = devPtr;
			break;
		default:
			devPtr->Type((Device::DeviceType) 200/*unknown*/);
			break;
	}
}

void DevicesManager::UpdateDevice(DevicePtr devPtr, IPv6 ip, GDeviceListReport::Device &devInfo, bool &doUpdateIPv6Index)
{

	//ip difference -> recreate subscriber lease
	if (devPtr->IP() != ip)
	{
		LoadSubscribeLeaseForDel(devPtr);
	}

	devPtr->IP(ip);
	devPtr->Status((Device::DeviceStatus)devInfo.DeviceStatus);
	if (devPtr->Changed())
		doUpdateIPv6Index = true;

	devPtr->Status((Device::DeviceStatus)devInfo.DeviceStatus);
	devPtr->SetTAG(devInfo.DeviceTag);
	devPtr->SetModel(devInfo.DeviceModel);
	devPtr->SetRevision(devInfo.DeviceRevision);
	devPtr->SetManufacturer(devInfo.DeviceManufacture);
	devPtr->SetSerialNo(devInfo.DeviceSerialNo);
	devPtr->PowerStatus(devInfo.PowerSupplyStatus);

	switch (devInfo.DeviceType)
	{
		case 1:
			devPtr->Type((Device::DeviceType) 11/*nonrouting device*/);
			break;
		case 2:
			devPtr->Type((Device::DeviceType) 10/*routing device*/);
			break;
		case 3:
			devPtr->Type((Device::DeviceType) 12/*routing and IO device*/);
			break;
		case 4:
			devPtr->Type((Device::DeviceType) 3/*backbone router*/);
			m_bbrDevPtr = devPtr;
			break;
		case 8:
			devPtr->Type((Device::DeviceType) 2/*gateway*/);
			m_gwDevPtr = devPtr;
			break;
		case 16:
			devPtr->Type((Device::DeviceType) 1/*system manager*/);
			m_smDevPtr = devPtr;
			break;
		default:
			devPtr->Type((Device::DeviceType) 200/*unknown*/);
			break;
	}
}

extern void SetFreshStatus(int handle, bool val);
void DevicesManager::LoadSubscribeLeaseForDel(DevicePtr dev)
{
	//push it
	InfoToDelete info;
	info.IPAddress = dev->IP();
	info.DevID = dev->id;

	unsigned int k = 3/*subscriber lease*/; //we only delete leases that are of type 'subscriber'
											//because the others are deleted automatically by the gw
											// when dev is rejoined or unregistered and we are based on (lease expired)
											// when using it
	for (std::map<unsigned int, boost::uint32_t>::iterator l = dev->m_ResourceIDToContractID[k].begin();
			l != dev->m_ResourceIDToContractID[k].end(); l++)
	{

		//we have contract
		info.ContractID = l->second;
		info.ResourceID = l->first;
		info.LeaseType = k;
		//now
		queueToDeleteLease.push(info);

		LOG_INFO("load for deleting LeaseID = " << (boost::uint32_t)info.ContractID
					<< "and Objid = " << (boost::uint32_t)(info.ResourceID >> 16)
					<< "and TLSAP_id = " << (boost::uint32_t)(info.ResourceID & 0xFFFF)
					<< "and lease_type = " << (boost::uint32_t)info.LeaseType
					<< "for dev with ip = " << info.IPAddress.ToString()
					<< "and with mac = " << dev->Mac().ToString());
	}

	UpdatePublishFlag(dev->id, 0/*nod data*/);
	SetFreshStatus(dev->GetPublishHandle(), false);
}

void DevicesManager::UpdateGw(const MAC &mac, const IPv6 &ip, bool &doUpdateIPv6Index, bool &doUpdateIdIndex)
{
	bool GWFound = false;
	NodesRepository::NodesByMACT::iterator it2 = repository.nodesByMAC.find(mac);

	if (it2 != repository.nodesByMAC.end())
	{
		if (it2->second->Type() != Device::dtGateway) //remove device and afterwards replace the gw
		{
			factoryDal.Devices().DeleteDevice(it2->second->id);

			LOG_INFO("device with mac = " << it2->first.ToString()
				<< "and with ip = " << it2->second->IP().ToString()  << "has been deleted from db");

			LoadSubscribeLeaseForDel(it2->second);
			repository.nodesByMAC.erase(it2);
		}
		else
		{
			GWFound = true; //ip will be set downwards
		}
	}

	if (GWFound == false)	//replace the gw
	{
		NodesRepository::NodesByMACT::iterator it2 = repository.nodesByMAC.find(m_gwDevPtr->mac);
		if (it2 != repository.nodesByMAC.end())
		{
			DevicePtr node(new Device());
			node->id = it2->second->id;
			node->Type(Device::dtGateway);
			node->Mac(mac);
			node->IP(ip);
			node->status = it2->second->status;
			//node->lastReading = it2->second->lastReading;
			node->deviceLevel = it2->second->deviceLevel;
			node->rejoinCount = it2->second->rejoinCount;
			//gw- has no subnetID
			/* alert begin*/
			node->m_ResourceIDToContractID = it2->second->m_ResourceIDToContractID;
			node->m_ResourceIDToWaitingContractID = it2->second->m_ResourceIDToWaitingContractID;
			node->m_ResourceIDToIssueContractRequestAt = it2->second->m_ResourceIDToIssueContractRequestAt;
			/* alert end*/
			//other fields
			node->powerStatus = it2->second->powerStatus;
			node->m_deviceTAG = it2->second->m_deviceTAG;
			node->m_deviceModel = it2->second->m_deviceModel;
			node->m_deviceRevision = it2->second->m_deviceRevision;
			node->m_deviceManufacturer = it2->second->m_deviceManufacturer;
			node->m_DPDUsTransmitted = it2->second->m_DPDUsTransmitted;
			node->m_DPDUsReceived = it2->second->m_DPDUsReceived;
			node->m_DPDUsFailedTransmission = it2->second->m_DPDUsFailedTransmission;
			node->m_DPDUsFailedReception = it2->second->m_DPDUsFailedReception;
			//erase
			repository.nodesByMAC.erase(it2);
			//add
			repository.Add(node);
		}
		else
		{
			LOG_ERROR("There shoud be found gw = " << m_gwDevPtr->mac.ToString());
			assert(false);
		}

		//mark for update by ipv6
		doUpdateIPv6Index = true;

		//mark for update by id
		doUpdateIdIndex = true;
	}
}

void DevicesManager::ProcessDeviceList(boost::shared_ptr<GDeviceListReport> DevListRepPtr)
{
	bool doUpdateIPv6Index = false;

	static int noDevsCount = 0;
	if (!DevListRepPtr)
	{
		noDevsCount++;
		noDevsCount = noDevsCount == 0 ? noDevsCount + 1 : noDevsCount;
	}
	else
	{
		noDevsCount = 0;
	}
	if (noDevsCount > 1)
		return;

	if (!DevListRepPtr)
	{
		for(NodesRepository::NodesByMACT::iterator j = repository.nodesByMAC.begin();
			j != repository.nodesByMAC.end(); ++j)
		{
			j->second->ResetChanged();

			if (j->second->Status() == Device::dsRegistered)
				doUpdateIPv6Index = true;

			j->second->Status(Device::dsUnregistered);
			if (j->second->Changed())
			{
				factoryDal.Devices().UpdateDevice(*j->second);
				LOG_INFO("device updated as unregistered in db: " << j->second->mac.ToString());
			}
		}
		if (doUpdateIPv6Index)
			repository.UpdateIPv6Index();
		return;
	}

	DevicePtr node;
	bool doUpdateIdIndex = false;
	for (GDeviceListReport::DevicesMapT::iterator i = DevListRepPtr->DevicesMap.begin();
						i != DevListRepPtr->DevicesMap.end(); ++i)
	{

		if (i->second.DeviceType == 8 /*gw*/)
			UpdateGw(i->second.DeviceMAC, i->first, doUpdateIdIndex, doUpdateIPv6Index);

		NodesRepository::NodesByMACT::iterator j = repository.nodesByMAC.find(i->second.DeviceMAC);

		if (j == repository.nodesByMAC.end())
		{
			node.reset(new Device());
			BuildNewDevice(node, i->first, i->second);
			if (!repository.Add(node))
				continue;

			doUpdateIPv6Index = true;
			doUpdateIdIndex = true;
			AddDevice(*node);
			LOG_INFO("device added as new in db: " << node->mac.ToString());
			continue;
		}

		j->second->ResetChanged();

		if (j->second->Status() == Device::dsUnregistered && IsDevDeletedFromDB(*j->second))
		{
			//build
			node = j->second;
			node->id = -1;
			UpdateDevice(node, i->first, i->second, doUpdateIPv6Index);
			AddDevice(*node);

			//clear
			repository.nodesByMAC.erase(j);
			doUpdateIdIndex = true;
			repository.Add(node);
			LOG_INFO("device added as half_registered in db: " << node->mac.ToString());
			continue;
		}
		//
		UpdateDevice(j->second, i->first, i->second, doUpdateIPv6Index);

		//
		if (j->second->Changed())
		{
			factoryDal.Devices().UpdateDevice(*j->second);

			if(j->second->Status() == Device::dsRegistered)
			{
				LOG_INFO("device updated as registered in db: " << j->second->mac.ToString());
			}
			else
			{
				LOG_INFO("device updated as half_registered in db: " << j->second->mac.ToString());
			}
		}


	}

	if (doUpdateIdIndex)
		repository.UpdateIdIndex();

	//make unregistered if not in dev_list
	for(NodesRepository::NodesByMACT::iterator j = repository.nodesByMAC.begin();
		j != repository.nodesByMAC.end(); ++j)
	{
		GDeviceListReport::DevicesMapT::iterator itDev = DevListRepPtr->DevicesMap.find(j->second->ip);
		if (itDev == DevListRepPtr->DevicesMap.end())
		{
			j->second->ResetChanged();

			if (j->second->Status() == Device::dsRegistered)
				doUpdateIPv6Index = true;
			j->second->Status(Device::dsUnregistered);
			if (j->second->Changed())
			{
				factoryDal.Devices().UpdateDevice(*j->second);
				LOG_INFO("device updated as unregistered in db: " << j->second->mac.ToString());
			}
		}
		else
		{
			//check for mac if the same...
			if (j->second->mac != itDev->second.DeviceMAC)
			{
				j->second->ResetChanged();

				if (j->second->Status() == Device::dsRegistered)
					doUpdateIPv6Index = true;
				j->second->Status(Device::dsUnregistered);
				if (j->second->Changed())
				{
					factoryDal.Devices().UpdateDevice(*j->second);
					LOG_INFO("device updated as unregistered in db: " << j->second->mac.ToString());
				}
			}
		}
	}

	if (doUpdateIPv6Index)
		repository.UpdateIPv6Index();

	//LOG_DEBUG("devices_list_come - ip_set_size = " << repository.nodesByIPv6.size());
}


void DevicesManager::SetFirmDlStatus(int deviceID, int fwType , int status/*3-cancelled, 4-completed, 5-failed*/)
{
	factoryDal.Devices().SetFirmDlStatus(deviceID, fwType, status);
}
void DevicesManager::SetFirmDlPercent(int deviceID, int fwType , int percent)
{
	factoryDal.Devices().SetFirmDlPercent(deviceID, fwType, percent);
}
void DevicesManager::SetFirmDlSize(int deviceID, int fwType , int size)
{
	factoryDal.Devices().SetFirmDlSize(deviceID, fwType, size);
}
void DevicesManager::SetFirmDlSpeed(int deviceID, int fwType , int speed)
{
	factoryDal.Devices().SetFirmDlSpeed(deviceID, fwType, speed);
}
void DevicesManager::SetFirmDlAvgSpeed(int deviceID, int fwType, int speed)
{
	factoryDal.Devices().SetFirmDlAvgSpeed(deviceID, fwType, speed);
}


//added by Cristian.Guef
void DevicesManager::ProcessTopology(ReportsProcessor *pRepProcessor, GTopologyReport& topologyReport)
{
	//added by Cristian.Guef
	//optimization, the case when no topology received
	static int noTopoCount = 0;
	if (topologyReport.DevicesList.size() == 0)
	{
		noTopoCount++;
		noTopoCount = noTopoCount == 0 ? noTopoCount + 1 : noTopoCount;
	}
	else
	{
		noTopoCount = 0;
	}
	if (noTopoCount > 1)
		return;


	//added by Cristian.Guef
	bool doUpdateIdIndex = false;

	//added by Cristian.Guef
	if (noTopoCount == 0) //(processing only when toplogy has devices)devices could be deleted from site only when are in unregistered state so...
	{
		//we only work on MAC map

		LOG_INFO("ProcessTopology: start do syncronize with db");
		for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin();
						it != repository.nodesByMAC.end(); )
		{
			//only for devices that are unregistered
			if (it->second->Status() != Device::dsRegistered)
			{
				if (IsDevDeletedFromDB(*it->second))
				{
					NodesRepository::NodesByMACT::iterator currIt = it++;
					repository.nodesByMAC.erase(currIt);
					doUpdateIdIndex = true;
					continue;
				}
			}
			it++;
		}
		LOG_INFO("ProcessTopology: end do syncronize with db has finished");
	}

	//added by Cristian.Guef
	ResetDevicesChanges();
	bool doUpdateIPv6Index = false;


	//added by Cristian.Guef
	//update gw (do not delete and add a new one because there could be the possibilty that
	//when we are processing a response of topology right now other responses may come after
	//which were issued with a deleted gw that has an id in db so it will be an error in web)
	if (topologyReport.GWIndex != -1)
	{
		bool GWFound = false;
		NodesRepository::NodesByMACT::iterator it2 = repository.nodesByMAC.find(topologyReport.DevicesList[topologyReport.GWIndex].DeviceMAC);

		if (it2 != repository.nodesByMAC.end())
		{
			if (it2->second->Type() != Device::dtGateway) //remove device and afterwards replace the gw
			{
				factoryDal.Devices().DeleteDevice(it2->second->id);

				LOG_INFO("device with mac = " << it2->first.ToString()
					<< "and with ip = " << it2->second->IP().ToString()  << "has been deleted from db");

				LoadSubscribeLeaseForDel(it2->second);
				repository.nodesByMAC.erase(it2);
			}
			else
			{
				GWFound = true; //ip will be set downwards
			}
		}

		if (GWFound == false)	//replace the gw
		{
			NodesRepository::NodesByMACT::iterator it2 = repository.nodesByMAC.find(m_gwDevPtr->mac);
			if (it2 != repository.nodesByMAC.end())
			{
				DevicePtr node(new Device());
				node->id = it2->second->id;
				node->Type(Device::dtGateway);
				node->Mac(topologyReport.DevicesList[topologyReport.GWIndex].DeviceMAC);
				node->IP(topologyReport.DevicesList[topologyReport.GWIndex].DeviceIP);
				node->status = it2->second->status;
				//node->lastReading = it2->second->lastReading;
				node->deviceLevel = it2->second->deviceLevel;
				node->rejoinCount = it2->second->rejoinCount;
				//gw- has no subnetID
				/* alert begin*/
				node->m_ResourceIDToContractID = it2->second->m_ResourceIDToContractID;
				node->m_ResourceIDToWaitingContractID = it2->second->m_ResourceIDToWaitingContractID;
				node->m_ResourceIDToIssueContractRequestAt = it2->second->m_ResourceIDToIssueContractRequestAt;
				/* alert end*/
				//other fields
				node->powerStatus = it2->second->powerStatus;
				node->m_deviceTAG = it2->second->m_deviceTAG;
				node->m_deviceModel = it2->second->m_deviceModel;
				node->m_deviceRevision = it2->second->m_deviceRevision;
				node->m_deviceManufacturer = it2->second->m_deviceManufacturer;
				node->m_DPDUsTransmitted = it2->second->m_DPDUsTransmitted;
				node->m_DPDUsReceived = it2->second->m_DPDUsReceived;
				node->m_DPDUsFailedTransmission = it2->second->m_DPDUsFailedTransmission;
				node->m_DPDUsFailedReception = it2->second->m_DPDUsFailedReception;
				//erase
				repository.nodesByMAC.erase(it2);
				//add
				repository.Add(node);
			}
			else
			{
				LOG_ERROR("There shoud be found gw = " << m_gwDevPtr->mac.ToString());
				assert(false);
			}

			//mark for update by ipv6
			doUpdateIPv6Index = true;

			//mark for update by id
			doUpdateIdIndex = true;
		}
	}

	//added by Cristian.Guef
	if (doUpdateIdIndex)
		repository.UpdateIdIndex(); //it could be optimised more (hint: look from device perspective not from devices set perspective)

	LOG_DEBUG("ProcessTopology: begins. DeviceCount=" << topologyReport.DevicesList.size());

	/* commented by Cristian.Guef -moved a little bit upwards
	ResetDevicesChanges();
	*/

	//FIXME [nicu.dascalu] - make Process Top transactional, do not alter cash is an eror occured

	// STEP 0: detect if SM is rejoined, and mark all devices as rejoined
	/* commneted by Cristian.Guef -ther is no need because when rceiving topology we have no rejoin_count_info
	{
		LOG_DEBUG("Checking for SM rejoin...");
		GTopologyReport::DevicesListT::const_iterator it = topologyReport.DevicesList.begin();
		for (; it != topologyReport.DevicesList.end(); it++)
		{
			if (it->DeviceType == Device::dtSystemManager)
			{
				break;
			}
		}

		bool isSMRejoined = false;
		DevicePtr existingSM = SystemManagerDevice();
		if (existingSM && it != topologyReport.DevicesList.end())
		{
			LOG_DEBUG("OLD SM RejoinCount=" << existingSM->rejoinCount << " and new RejoinCount=" << it->DeviceRejoinCount);
			if (existingSM->rejoinCount != it->DeviceRejoinCount)
			{
				isSMRejoined = true;
			}
		}
		else
		{
			LOG_DEBUG("No SM found...");
		}

		if (isSMRejoined)
		{
			LOG_DEBUG("System manager has rejoined. Invalidating all nodes...");
			for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
			{
				it->second->Status(Device::dsUnregistered);
				it->second->RejoinCount(0);
			}
		}
	}
	*/

	/* commented by Cristian.Guef
	{
		//STEP 1: all unfound devices in topo report are marked as unregistered

		typedef std::map<MAC, const GTopologyReport::Device*> TopoDeviceMap;
		TopoDeviceMap reportedDevices;
		for (GTopologyReport::DevicesListT::const_iterator it = topologyReport.DevicesList.begin(); it
		    != topologyReport.DevicesList.end(); it++)
		{
			reportedDevices[it->DeviceMAC] = &(*it);
		}

		//all unmatched devices in reported topo are marked as unregistered in our repository.
		for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
		{
			if (reportedDevices.end() == reportedDevices.find(it->first))
			{
				it->second->Status(Device::dsUnregistered);

				//added by Cristian.Guef
				if (it->second->Changed())	//if node changed then do updateIPv6Index
					doUpdateIPv6Index = true;
			}
		}
	}
	*/

	//an optimized way -turn registered devices into unregistered ones if not found in current topology
	{
		UnregisterDevices(pRepProcessor, topologyReport, doUpdateIPv6Index);
	}

	//added by Cristian.Guef
	DevicesGraph devGraph;

	m_bbrDevPtr = DevicePtr();

	LOG_DEBUG("ProcessTopology: Create or Update exiting devices ...");
	{
		//added by Cristian.Guef
		int vertexNo = 0;
		int SMVertexNo = -1;
		std::deque<int> BBRVertices;

		//added by Cristian.Guef
		devGraph.SetVerticesNo(topologyReport.DevicesList.size());

		//STEP 2: create new or update existing from reported topo
		for (GTopologyReport::DevicesListT::iterator it = topologyReport.DevicesList.begin(); it
		    != topologyReport.DevicesList.end(); it++)
		{

			//added by Cristian.Guef
			if (it->isMarkedDeleted == true)
			{
				vertexNo++;		// - we must have vertexNo = DeviceListIndex
				continue;
			}

			DevicePtr node = repository.Find(it->DeviceMAC);
			if (!node)
			{
				node.reset(new Device());
				node->Mac(it->DeviceMAC);
				repository.Add(node);
			}

			/*commented by Cristian.Guef
			//DeviceType field:
			// first 4 (0-3) bits tell the type of device, SM, GW, BBR, Routing, Non-Routing Device
			node->Type((Device::DeviceType) it->DeviceType); it was moved downwards to optimize things a little bit
			*/

			/*commented by Cristian.Guef
			// bit 4 tells has battery power, bit 5 tells has aquisition board
			node->DeviceCapabilities((0x20 & it->DeviceCapabilities) != 0, (0x10 & it->DeviceCapabilities) != 0);
			node->DeviceImplType(it->DeviceImplementationType);
			*/
			/* comented by Cristian.Guef
			node->RejoinCount(it->DeviceRejoinCount); it was moved downwards to optimize things a little bit
			*/

			switch (it->DeviceStatus)
			{
			case Device::dsRegistered: /*JOIN_CONFIRMED*/
			{
				node->Status(Device::dsRegistered);

				if (node->IP() != it->DeviceIP)
				{
					LoadSubscribeLeaseForDel(node);
				}

				node->IP(it->DeviceIP);

				//added by Cristian.Guef
				if (node->Changed())	//if node changed then do updateIPv6Index
					doUpdateIPv6Index = true;

				break;
			}
			/*
			case 2: //JOIN_REQUEST_RECEIVED
			{
				node->Status(Device::dsRegistering);
				node->IP(it->DeviceIP);
				break;
			}*/
			default:
				{
				LOG_ERROR("ProcessTopology: unexpected DeviceStatus=" << it->DeviceStatus << " for Device=" << node->ToString());
				node->Status(Device::dsUnregistered);
				doUpdateIPv6Index = true;
				break;
				}
			}

			if (node->Rejoined() || !node->IsRegistered())
			{
				//LOG_WARN("ProcessTopology: Reset contract for rejoined/unregistered Device=" << node->ToString());
				//node->ResetContractID(); -no need because regarding c/s there is LeaseExpired error returned by gw

			}

			//added by Cristian.Guef
			node->SetVertexNo(vertexNo);	//usefull when setting dev_id to the coresponding device in toplogy_struct
			if (node->Type() == 1/*sm*/)
			{
				SMVertexNo = vertexNo;
				LOG_DEBUG("node_level -> sm vertex no = " << SMVertexNo);
				node->Level(0);

				m_smDevPtr = node;		//used in 'SystemManagerDevice' method
			}

			//added by Cristian.Guef
			if (node->Type() == 2/*gw*/)
				m_gwDevPtr = node;		//used in 'GatewayDevice' method

			//added by Cristian.Guef
			if (node->Type() == 3/*bbr*/)
			{
				m_bbrDevPtr = node;
				BBRVertices.push_back(vertexNo);
			}

			//added by Cristian.Guef - build the devices_graph
			devGraph.AddDataToVertex(vertexNo, node);
			for (GTopologyReport::Device::NeighborsListT::const_iterator j = it->NeighborsList.begin();
							j != it->NeighborsList.end(); j++)
			{
				if (j->DevListIndex != -1)
				{
					if (topologyReport.DevicesList[j->DevListIndex].isMarkedDeleted == false)
					{
						devGraph.AddArc(vertexNo, j->DevListIndex,j->SignalQuality, j->ClockSource);
						LOG_DEBUG("arc: from ip=" << node->IP().ToString() << "to ip = " << j->DeviceIP.ToString());
					}
					else
					{
						LOG_DEBUG("no arc: from ip=" << node->IP().ToString() << "to ip = " << j->DeviceIP.ToString());
					}
				}
			}
			vertexNo++;

			//added by Cristian.Guef
			if (node->RejoinCount() != it->DeviceRejoinCount)
			{
				UpdatePublishFlag(node->id, 0/*nod data*/);
				SetFreshStatus(node->GetPublishHandle(), false);
			}
				//rejoinedDevices.push(node->Mac());	//to get contracts and routes for this device

			//added by Cristian.Guef
			node->Type((Device::DeviceType) it->DeviceType);
			node->RejoinCount(it->DeviceRejoinCount);
			node->SetTAG(it->deviceTAG);
			node->SetModel(it->deviceModel);
			node->SetRevision(it->deviceRevision);
			node->SetManufacturer(it->deviceManufacturer);
			node->SetSerialNo(it->deviceSerialNo);
			node->SetDPDUsTransmitted(it->DPDUsTransmitted);
			node->SetDPDUsReceived(it->DPDUsReceived);
			node->SetDPDUsFailedTransmission(it->DPDUsFailedTransmission);
			node->SetDPDUsFailedReception(it->DPDUsFailedReception);
			node->PowerStatus(it->PowerSupplyStatus);

		}
		/* commented by Cristian.Guef
		repository.UpdateIPv6Index();
		*/
		//added by Cristian.Guef
		if (doUpdateIPv6Index)
			repository.UpdateIPv6Index(); //it could be optimised more (hint: look from device perspective not from devices set perspective)

		//set subnetIDs for BBRs
		for (unsigned int i =0; i < topologyReport.m_BBRsList.size(); i++)
		{
			DevicePtr node = repository.Find(topologyReport.m_BBRsList[i].address);
			if (!node)
			{
				LOG_WARN("BBR not found, ip = " << topologyReport.m_BBRsList[i].address.ToString());
				continue;
			}
			node->SetSubnetID(topologyReport.m_BBRsList[i].subnetID);
		}

		LOG_DEBUG("ProcessTopology: Compute node levels ...");

		/* commented by Cristian.Guef
		ComputeNodeLevels(topologyReport.DevicesList);
		*/

		//added by Cristian.Guef
		if (SMVertexNo != -1)
		{
			//prepare for graph traversal
			/*unsigned char *pIsTraversed = new unsigned char[devGraph.GetVerticesNo()]; //0 - vertex not traversed
																					//1 - vertex traversed
			memset(pIsTraversed, 0, devGraph.GetVerticesNo()*sizeof(unsigned char));
			pIsTraversed[SMVertexNo] = 1;
			ComputeNodeLevels(devGraph, SMVertexNo, pIsTraversed, 0+1); //BFS (Breadth first search)
			delete pIsTraversed;
			*/
			StartComputeNodeLevels(devGraph, SMVertexNo);
		}


		//added by Cristian.Guef
		if (SMVertexNo != -1)
			ProcessSubnetIDsFromBBRs(devGraph, BBRVertices); //BFS (Breadth first search) for every bbr being not traversed
	}

	UpdateUnregDevices(pRepProcessor);

	{
		//STEP 3: save changes to db
		LOG_DEBUG("ProcessTopology: Updating db ...");
		try
		{
			//added by Cristian.Guef
			int unregisteredDevices = 0;
			int registeredDevice = 0;

			//now update to db
			factoryDal.BeginTransaction();

			for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
			{
				if (it->second->Changed())
				{
					if (it->second->id == -1)
					{
						AddDevice(*it->second);

						/* commented by Cristian.Guef
						LOG_INFO("ProcessTopology: Discovered new Device=" << it->second->ToLongString());
						*/
						//added by Cristian.Guef
						LOG_INFO("ProcessTopology: Discovered new registered Device=" << it->second->ToString());

						//added by Cristian.Guef
						if (!repository.nodesById.insert(NodesRepository::NodesByIDT::value_type(it->second->id, it->second)).second)
						{
							LOG_WARN("ProcessTopology: Same DeviceID:" << it->second->id << "was detected (will be ignored)!");
						}
					}
					else
					{
						factoryDal.Devices().UpdateDevice(*it->second);
						/* commented by Cristian.Guef
						LOG_INFO("ProcessTopology: Updated existing Device=" << it->second->ToLongString());
						*/
						//added by Cristian.Guef
						if (it->second->IsRegistered())
						{
							LOG_INFO("ProcessTopology: DB updated for registered Device=" << it->second->ToString());
						}
						else
						{
							LOG_INFO("ProcessTopology: DB updated for non-registered Device=" << it->second->ToString());
						}
					}
				}

				//added by Cristian.Guef
				if (it->second->IsRegistered())
				{
					LOG_DEBUG("ProcessTopology_topo_struct: dev_ip =" << it->second->IP().ToString() <<
								"has db_id = " << it->second->id);

					if (it->second->GetVertexNo() < topologyReport.DevicesList.size() && it->second->GetVertexNo() >= 0)
					{
						topologyReport.DevicesList[it->second->GetVertexNo()].device_dbID = it->second->id;
					}
					else
					{
						LOG_WARN("ProcessTopology_topo_struct: dev_ip=" << it->second->IP().ToString() << " is not anymore in topology list after intersection with deviceList");
					}

					registeredDevice++;
					if (!it->second->Changed())
					{
						LOG_INFO("ProcessTopology: DB not updated for registered Device=" << it->second->ToString());
					}
				}
				else
				{
					unregisteredDevices++;
					if (!it->second->Changed())
					{
						LOG_INFO("ProcessTopology: DB not updated for non-registered Device=" << it->second->ToString());
					}
				}
				it->second->ResetChanged();

			}
			/*commented by Cristian.Guef -no need
			repository.UpdateIdIndex(); //some nodes where newly added, so reindex their ids(from db)
			*/

			factoryDal.CommitTransaction();

			//added by Cristian.Guef
			LOG_INFO("ProcessTopology: finished. DevicesCount=" << repository.nodesByMAC.size() << " RegisteredCount="
				<< registeredDevice << " UnregisteredCount=" << unregisteredDevices);

		}
		catch (std::exception& ex)
		{
			factoryDal.RollbackTransaction();
			LOG_ERROR("ProcessTopology: Failed to updated device in db! error=" << ex.what());
			ResetDevicesChanges();
			throw;
		}
		catch (...)
		{
			factoryDal.RollbackTransaction();
			LOG_ERROR("ProcessTopology: Failed to updated device in db! unknown error!");
			ResetDevicesChanges();
			throw;
		}
	}


	// update neighbors & graphs
	//try
	//{

		//factoryDal.BeginTransaction();

		factoryDal.Devices().CleanDeviceNeighbours();
		factoryDal.Devices().CleanDeviceGraphs();

		int vertexNo = 0;
		for (GTopologyReport::DevicesListT::iterator it = topologyReport.DevicesList.begin(); it
			!= topologyReport.DevicesList.end(); it++)
		{
			if (vertexNo >= devGraph.GetVerticesNo())
			{
				LOG_WARN("ProcessTopology: unknown device!");
				LOG_WARN("unknown device=" << it->DeviceIP.ToString());
				break;
			}

			DevicePtr fromDevice;
			if (it->isMarkedDeleted == false)
			{
				fromDevice = devGraph.GetVertexData(vertexNo);
				SaveDeviceNeighbours(devGraph, vertexNo);
			}
			else
			{
				vertexNo++;
				continue;
			}
			vertexNo++;

			SaveDeviceGraphs(fromDevice, it->GraphsList);
		}

		//factoryDal.CommitTransaction();

		//added by Cristian.Guef
		LOG_INFO("ProcessTopology: finished. saving graphs and neighbors");
	//}
	/*catch (std::exception& ex)
	{
		factoryDal.RollbackTransaction();
		LOG_ERROR("ProcessTopology: Failed to save graphs or neighbors in db! error=" << ex.what());
	}
	catch (...)
	{
		factoryDal.RollbackTransaction();
		LOG_ERROR("ProcessTopology: Failed to ave graphs or neighbors in db! unknown error!");
	}*/


	//commented by Cristian.Guef - no need to trigger -topochanged (see the fnction)-
	//{
	//	//STEP 4: notify observer about topology changed, if has been changed
	//	bool topoChanged = false;
	//	for (NodesRepository::NodesByIDT::iterator it = repository.nodesById.begin(); it != repository.nodesById.end(); it++)
	//	{
	//		if (it->second->Changed())
	//		{
	//			topoChanged = true;
	//			break;
	//		}
	//	}

	//	if (topoChanged)
	//	{
	//		/* commented by Cristian.Guef
	//		FireTopologyChanged();
	//		*/
	//		//added by Cristian.Guef
	//		FireTopologyChanged(KeepDevContracts);/
	//	}
	//}


	/* commented by Cristian.Guef - no need because it was moved to step 3
	if (LOG_INFO_ENABLED())
	{
		int unregisteredDevices = 0;
		int registeredDevice = 0;
		int rejoinedDevices = 0;

		LOG_INFO("ProcessTopology: print. DevicesCount=" << repository.nodesByMAC.size());
		for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
		{
			if (it->second->IsRegistered())
			{
				registeredDevice++;
			}
			else
			{
				unregisteredDevices++;
			}
			if (it->second->Rejoined())
			{
				rejoinedDevices++;
			}
			LOG_INFO("ProcessTopology: print.   Device=" << it->second->ToLongString());
		}
		LOG_INFO("ProcessTopology: finished. DevicesCount=" << repository.nodesByMAC.size() << " RegisteredCount="
		    << registeredDevice << " RejoinedCount=" << rejoinedDevices << " UnregisteredCount=" << unregisteredDevices);
	}

	LOG_DEBUG("ProcessTopology: Reseting devices changes ...");
	ResetDevicesChanges();
	*/
}


void DevicesManager::AddDevice(Device& device)
{
	try
	{
		factoryDal.Devices().AddDevice(device);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("AddDevice: failed! error=" << ex.what());
	}
}
void DevicesManager::DeleteContracts()
{
	factoryDal.Devices().DeleteContracts();
}
void DevicesManager::ChangeContracts(const IPv6 &sourceIP, const ContractsAndRoutes::ContractsListT &infoList)
{

	DevicePtr source = repository.Find(sourceIP);
	if (!source)
	{
		LOG_WARN("unknown source_dev = " << sourceIP.ToString()
					<< ", so do not store contracts in db!");
		return;
	}
	//factoryDal.Devices().DeleteContracts(source->id);

	for (int i = 0; i < (int)infoList.size(); ++i)
	{

		DevicePtr destination = repository.Find(infoList[i].destinationAddress);
		if (!destination)
		{
			LOG_WARN("unknown destination_dev = " << infoList[i].destinationAddress.ToString()
					<< ", so do not store contract with this device in db!");
			continue;
		}

		try
		{
			factoryDal.Devices().AddContract(source->id, destination->id, infoList[i]);
		}
		catch(std::exception ex)
		{
			LOG_ERROR("The contract could not be save in db!");
		}
	}
}

void DevicesManager::ChangeRoutes(const IPv6 &sourceIP, ContractsAndRoutes::RoutesListT &infoList)
{
	DevicePtr source = repository.Find(sourceIP);
	if (!source)
	{
		LOG_WARN("unknown source_dev = " << sourceIP.ToString()
					<< ", so do not store routes in db!");
		return;
	}
	factoryDal.Devices().DeleteRoutes(source->id);

	for (int i = 0; i < (int)infoList.size(); ++i)
	{
		try
		{
			switch(infoList[i].alternative)
			{
			case 0:
				{
					infoList[i].selector = infoList[i].selectorn.contractID;
					DevicePtr dev = repository.Find(infoList[i].srcAddress);
					if (!dev)
					{
						LOG_WARN("unknown selctor_dev = " << infoList[i].srcAddress.ToString()
									<< ", so do not store routes in db!");
						continue;
					}
					infoList[i].srcAddr = dev->id;
				}
				break;
			case 1:
				infoList[i].selector = infoList[i].selectorn.contractID;
				break;
			case 2:
				{
					DevicePtr dev = repository.Find(IPv6(infoList[i].selectorn.nodeAddress));
					if (!dev)
					{
						LOG_WARN("unknown selctor_dev = " << IPv6(infoList[i].selectorn.nodeAddress).ToString()
									<< ", so do not store routes in db!");
						continue;
					}
					infoList[i].selector = dev->id;
				}
				break;
			default:
				break;
			}


			factoryDal.Devices().AddRouteInfo(source->id, infoList[i].index, infoList[i].alternative,
									infoList[i].forwardLimit, infoList[i].selector, infoList[i].srcAddr);
		}
		catch(std::exception ex)
		{
			LOG_ERROR("The route id = " << infoList[i].index << " could not be save in db!");
		}

		for (int j = 0; j < (int)infoList[i].routes.size(); ++j)
		{
			try
			{
				if (infoList[i].routes[j].isGraph ==  1/* is graph*/)
				{
					factoryDal.Devices().AddRouteLink(source->id, infoList[i].index, j, infoList[i].routes[j].elem.graphID);
				}
				else
				{
					DevicePtr neighbour = repository.Find(IPv6(infoList[i].routes[j].elem.nodeAddress));
					if (!neighbour)
					{
						LOG_WARN("unknown source_dev = " << IPv6(infoList[i].routes[j].elem.nodeAddress).ToString()	<< ", so do not store route_link in db!");
						continue;
					}
					factoryDal.Devices().AddRouteLink(source->id, infoList[i].index, j, neighbour);
				}
			}
			catch(std::exception ex)
			{
				LOG_ERROR("The route index = " << j << " could not be save in db!");
			}
		}
	}
}
void DevicesManager::InsertContractElements(int contractID, int sourceID, std::deque<int/*deviceID*/> & deviceIDs)
{
	int i = 0;
	while (deviceIDs.size() != 0)
	{
		factoryDal.Devices().AddContractElement(contractID, sourceID, i++, deviceIDs.front());
		deviceIDs.pop_front();
	}
}
void DevicesManager::RemoveContractElements(int sourceID)
{
	factoryDal.Devices().RemoveContractElements(sourceID);
}
void DevicesManager::RemoveContractElements()
{
	factoryDal.Devices().RemoveContractElements();
}

//added by Cristian.Guef
void DevicesManager::SaveISACSConfirm(int deviceID, ISACSInfo &confirm)
{
	factoryDal.Devices().SaveISACSConfirm(deviceID, confirm);
}


const std::string strBroadcast = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff";
//added by Cristian.Guef
void DevicesManager::ChangeScheduleInfo(GScheduleReport &scheduleReport)
{
	if (scheduleReport.channelList.size() != 0)
		factoryDal.Devices().RemoveRFChannels();

	DevicePtr devPtr;

	for (int i = 0; i < (int)scheduleReport.channelList.size(); ++i)
	{
		factoryDal.Devices().AddRFChannel(scheduleReport.channelList[i]);
	}

	LOG_DEBUG("DeviceScheduleList.size = " << scheduleReport.DeviceScheduleList.size());
	for (int i = 0; i < (int)scheduleReport.DeviceScheduleList.size(); ++i)
	{
		DevicePtr devPtr = repository.Find(scheduleReport.DeviceScheduleList[i].networkAddress);
		if (!devPtr)
		{
			LOG_WARN("sched_rep_to_db 'ScheduleSuperframe' -> dev_ip = " <<
							scheduleReport.DeviceScheduleList[i].networkAddress.ToString()
							<< "hasn't been found in topology");
			continue;
		}

		factoryDal.Devices().RemoveScheduleSuperframes(devPtr->id);
		factoryDal.Devices().RemoveScheduleLinks(devPtr->id);

		LOG_DEBUG("superframeList.size() = " << scheduleReport.DeviceScheduleList[i].superframeList.size());
		for (int j = 0; j < (int)scheduleReport.DeviceScheduleList[i].superframeList.size(); ++j)
		{
			if (scheduleReport.DeviceScheduleList[i].superframeList[j].superframeID != 0)
			{
				factoryDal.Devices().AddScheduleSuperframe(devPtr->id,
					scheduleReport.DeviceScheduleList[i].superframeList[j]);
			}
			else
			{
				LOG_WARN("sched_rep_to_db 'ScheduleSuperframe' -> dev_ip = " <<
							scheduleReport.DeviceScheduleList[i].networkAddress.ToString()
							<< "has frameID = 0, so skip it");
				continue;
			}

			LOG_DEBUG("linkList.size() = " << scheduleReport.DeviceScheduleList[i].superframeList[j].linkList.size());
			for (int k = 0; k < (int)scheduleReport.DeviceScheduleList[i].superframeList[j].linkList.size(); ++k)
			{

				DevicePtr neighbPtr = repository.Find(scheduleReport.DeviceScheduleList[i].superframeList[j].linkList[k].networkAddress);
				if (!neighbPtr)
				{
					if (scheduleReport.DeviceScheduleList[i].superframeList[j].linkList[k].networkAddress.ToString() == strBroadcast)
					{
						scheduleReport.DeviceScheduleList[i].superframeList[j].linkList[k].devDB_ID = 0;
						factoryDal.Devices().AddScheduleLink(devPtr->id,
							scheduleReport.DeviceScheduleList[i].superframeList[j].superframeID,
							scheduleReport.DeviceScheduleList[i].superframeList[j].linkList[k]);
					}
					else
					{
						LOG_WARN("sched_rep_to_db 'ScheduleLink' -> for dev_ip = " <<
							scheduleReport.DeviceScheduleList[i].networkAddress.ToString()
							<< " the neighbour_ip = " <<
							scheduleReport.DeviceScheduleList[i].superframeList[j].linkList[k].networkAddress.ToString()
							<< "has not been found in topology");
					}
					continue;
				}

				scheduleReport.DeviceScheduleList[i].superframeList[j].linkList[k].devDB_ID = neighbPtr->id;
				factoryDal.Devices().AddScheduleLink(devPtr->id,
							scheduleReport.DeviceScheduleList[i].superframeList[j].superframeID,
							scheduleReport.DeviceScheduleList[i].superframeList[j].linkList[k]);
			}
		}
	}

}

//added by Cristian.Guef
void DevicesManager::ChangeNetworkHealthInfo(GNetworkHealthReport &networkHealthRep)
{
	factoryDal.Devices().RemoveNetworkHealthInfo();
	factoryDal.Devices().RemoveNetHealthDevInfo();

	factoryDal.Devices().AddNetworkHealthInfo(networkHealthRep.m_NetworkHealth);

	for (GNetworkHealthReport::DevicesMapT::iterator i = networkHealthRep.DevicesMap.begin();
		i != networkHealthRep.DevicesMap.end(); ++i)
	{
		if (i->second.dbDevID != 0)
			factoryDal.Devices().AddNetHealthDevInfo(i->second);
		else
		{
			DevicePtr devPtr = repository.Find(i->first);
			if (!devPtr)
			{
				LOG_WARN("network_health_report -> device not found in db = " << i->first.ToString());
			}
			else
			{
				i->second.dbDevID = devPtr->id;
				factoryDal.Devices().AddNetHealthDevInfo(i->second);
			}
		}

	}
}

//added by Cristian.Guef
void DevicesManager::AddDeviceHealthHistory(int deviceID, const nlib::DateTime &timestamp, const GDeviceHealthReport::DeviceHealth &devHeath)
{
	factoryDal.Devices().AddDeviceHealthHistory(deviceID, timestamp, devHeath);
}
void DevicesManager::UpdateDeviceHealth(int deviceID, const GDeviceHealthReport::DeviceHealth &devHeath)
{
	factoryDal.Devices().UpdateDeviceHealth(deviceID, devHeath);
}
void DevicesManager::AddNeighbourHealthHistory(int deviceID, const nlib::DateTime &timestamp, int neighbID, const GNeighbourHealthReport::NeighbourHealth &neighbHealth)
{
	factoryDal.Devices().AddNeighbourHealthHistory(deviceID, timestamp, neighbID, neighbHealth);
}

void DevicesManager::SavePublishChannels(int deviceID, PublisherConf::COChannelListT &list)
{
	try
	{
		factoryDal.BeginTransaction();

		for (int i = 0; i < (int)list.size(); ++i)
			factoryDal.Devices().CreateChannel(deviceID, list[i]);
		factoryDal.CommitTransaction();
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Create publish channels failed! error=" << ex.what());
		factoryDal.RollbackTransaction();
		return;
	}
}

void DevicesManager::DeletePublishChannels(const std::vector<int/*dbChannelNo*/> &list)
{
	for (int i = 0; i < (int)list.size(); ++i)
	{
		factoryDal.Devices().DeleteChannel(list[i]);
		DeleteReading(list[i]);
	}
}

void DevicesManager::DeleteAllPublishChannels()
{
	factoryDal.Devices().DeleteAllChannels();
}

void DevicesManager::MoveChannelToHistory(int channelNo)
{
	factoryDal.Devices().MoveChannelToHistory(channelNo);
}
void DevicesManager::MoveChannelsToHistory(int deviceID)
{
	factoryDal.Devices().MoveChannelsToHistory(deviceID);
}

int DevicesManager::CreateChannel(int deviceID, PublisherConf::COChannel &channel)
{
	return factoryDal.Devices().CreateChannel(deviceID, channel);
}
void DevicesManager::UpdateChannel(int channelNo, const std::string &channelName, const std::string &unitOfMeasure, int channelFormat, int withStatus)
{
	factoryDal.Devices().UpdateChannel(channelNo, channelName, unitOfMeasure, channelFormat, withStatus);
}
void DevicesManager::UpdateChannelsInfo(const PublisherConf::COChannelListT &list)
{
	for (int i = 0; i < (int)list.size(); ++i)
		factoryDal.Devices().UpdateChannel(list[i].dbChannelNo, list[i].name, list[i].unitMeasure, list[i].format, list[i].withStatus);
}

bool DevicesManager::FindDeviceChannel(int channelNo, DeviceChannel& channel)
{
	return factoryDal.Devices().FindDeviceChannel(channelNo, channel);
}

bool DevicesManager::IsDeviceChannel(int deviceID, const PublisherConf::COChannel &channel)
{
	return factoryDal.Devices().IsDeviceChannel(deviceID, channel);
}
bool DevicesManager::IsDeviceChannelInReading(int deviceID, int channelNo)
{
	return factoryDal.Devices().IsDeviceChannelInReading(deviceID, channelNo);
}

bool DevicesManager::HasDeviceChannels(int deviceID)
{
	return factoryDal.Devices().HasDeviceChannels(deviceID);
}
void DevicesManager::SetPublishErrorFlag(int deviceID, int flag)
{
	factoryDal.Devices().SetPublishErrorFlag(deviceID, flag);
}


static bool isLower(std::vector<PublisherConf::COChannel>::const_iterator j, PublisherConf::ChannelIndexT::const_iterator i)
{
	if ((j->tsapID << 16 | j->objID) < (i->first.tsapID << 16 | i->first.objID))
		return true;
	if ((j->tsapID << 16 | j->objID) == (i->first.tsapID << 16 | i->first.objID))
		return (j->attrID << 16 | j->index1 << 8 | j->index2)
					< (i->first.attrID << 16 | i->first.index1 << 8 | i->first.index2);
	return false;
}

static bool isLower(PublisherConf::ChannelIndexT::const_iterator i, std::vector<PublisherConf::COChannel>::const_iterator j)
{
	if ((i->first.tsapID << 16 | i->first.objID) < (j->tsapID << 16 | j->objID))
		return true;
	if ((i->first.tsapID << 16 | i->first.objID) == (j->tsapID << 16 | j->objID))
		return (i->first.attrID << 16 | i->first.index1 << 8 | i->first.index2)
					< (j->attrID << 16 | j->index1 << 8 | j->index2);
	return false;
}

void DevicesManager::ProcessChannelsDifference(int deviceID,
											 PublisherConf::COChannelListT &storedList,
											 const PublisherConf::ChannelIndexT &storedIndex,
											const std::vector<PublisherConf::COChannel> &channels)
{

	//
	PublisherConf::ChannelIndexT::const_iterator i = storedIndex.begin();
	std::vector<PublisherConf::COChannel>::const_iterator j = channels.begin();

	LOG_DEBUG("channel size = " << channels.size());

	for (; i != storedIndex.end(); ++i)
	{

		if (j == channels.end())
		{
			//create channel
			int channelNo = factoryDal.Devices().CreateChannel(deviceID, storedList[i->second]);
			factoryDal.Devices().AddEmptyReading(deviceID, channelNo);
			continue;
		}

		LOG_DEBUG("i: tsapID= " << i->first.tsapID << " objID = " << i->first.objID << " attrID=" << i->first.attrID <<
					" index1 = " << i->first.index1 << " index2 = " << i->first.index2);
		LOG_DEBUG("j: tsapID= " << j->tsapID << " objID = " << j->objID << " attrID=" << j->attrID <<
					" index1 = " << j->index1 << " index2 = " << j->index2);

		if (i->first.tsapID == j->tsapID && i->first.objID == j->objID && i->first.attrID == j->attrID &&
			i->first.index1 == j->index1 && i->first.index2 == j->index2)
		{
			//found a match...
		}
		else
		{
			if (isLower(j, i))
			{
				do
				{
					//move channel
					factoryDal.Devices().MoveChannelToHistory(j->dbChannelNo);
					factoryDal.Devices().DeleteReading(j->dbChannelNo);
					++j;
					LOG_DEBUG("to delete -> j: tsapID= " << j->tsapID << " objID = " << j->objID << " attrID=" << j->attrID <<
							" index1 = " << j->index1 << " index2 = " << j->index2);
					if (j == channels.end())
						break;
				} while (isLower(j, i));

				if (j == channels.end())
				{
					//create chanel
					int channelNo = factoryDal.Devices().CreateChannel(deviceID, storedList[i->second]);
					factoryDal.Devices().AddEmptyReading(deviceID, channelNo);
					continue;
				}
				if (isLower(i, j))
				{
					//create channel
					int channelNo = factoryDal.Devices().CreateChannel(deviceID, storedList[i->second]);
					factoryDal.Devices().AddEmptyReading(deviceID, channelNo);
					continue;
				}

				//found a match...
			}
			else
			{
				//create channel
				int channelNo = factoryDal.Devices().CreateChannel(deviceID, storedList[i->second]);
				factoryDal.Devices().AddEmptyReading(deviceID, channelNo);
				continue;
			}
		}

		//
		storedList[i->second].dbChannelNo = j->dbChannelNo;

		//check channel info
		LOG_DEBUG("stored name = " << storedList[i->second].name << " format=" << (int)storedList[i->second].format << " neasure=" << storedList[i->second].unitMeasure <<
			"loaded name = " << j->name << " format=" << (int)j->format << " neasure=" << j->unitMeasure );
		if (storedList[i->second].name != j->name ||
			storedList[i->second].format != j->format ||
			storedList[i->second].unitMeasure != j->unitMeasure ||
			storedList[i->second].withStatus != j->withStatus)
		{
			//update channel
			factoryDal.Devices().UpdateChannel(j->dbChannelNo, storedList[i->second].name, storedList[i->second].unitMeasure, storedList[i->second].format, storedList[i->second].withStatus);
		}

		++j;
	}

	while (j != channels.end())
	{
		LOG_DEBUG("to delete -> j: tsapID= " << j->tsapID << " objID = " << j->objID << " attrID=" << j->attrID <<
					" index1 = " << j->index1 << " index2 = " << j->index2);

		factoryDal.Devices().MoveChannelToHistory(j->dbChannelNo);
		factoryDal.Devices().DeleteReading(j->dbChannelNo);
		++j;
	}
}

void DevicesManager::ResetDeviceChannels()
{

	PublisherConf::PublisherInfoMAP_T::iterator i = m_rConfigApp.PublishersMapStored.begin();
	std::vector<PublisherConf::COChannel> channels;

	std::vector<MAC> macs;
	GetOrderPublisherMACs(macs);
	std::vector<MAC>::iterator j = macs.begin();

	//compare lists
	for (; j != macs.end(); ++j)
	{
		DevicePtr dev = repository.Find(*j);
		if (!dev)
		{
			LOG_ERROR("device not found in cache, mac=" << j->ToString());
			continue;
		}

		if (i == m_rConfigApp.PublishersMapStored.end())
		{
			DeletePublishInfo(dev->id);
			continue;
		}

		if (*j == i->first)
		{
			//found a match
		}
		else
		{
			if (i->first < *j)
			{
				do
				{
					++i;
					if (i == m_rConfigApp.PublishersMapStored.end())
						break;
				} while (i->first < *j);
				if (i == m_rConfigApp.PublishersMapStored.end())
				{
					DeletePublishInfo(dev->id);
					continue;
				}
				if (*j < i->first)
				{
					DeletePublishInfo(dev->id);
					continue;
				}

				//found a match...
			}
			else
			{
				DeletePublishInfo(dev->id);
				continue;
			}
		}

		//we have a match... WE CAN DELAY THIS KIND OF PROCESSING
		//factoryDal.Devices().GetOrderedChannels(dev->id, channels);
		//ProcessChannelsDifference(dev->id, i->second.coChannelList, i->second.channelIndex, channels);
		++i;
	}

}

void DevicesManager::ProcessDBChannels(int deviceID, PublisherConf::COChannelListT &storedList,
											 const PublisherConf::ChannelIndexT &storedIndex)
{
	std::vector<PublisherConf::COChannel> channels;
	factoryDal.Devices().GetOrderedChannels(deviceID, channels);
	ProcessChannelsDifference(deviceID, storedList, storedIndex, channels);
}

void DevicesManager::DeletePublishInfo(int deviceID)
{
	DeleteReadings(deviceID);
	MoveChannelsToHistory(deviceID);
}
void DevicesManager::GetOrderPublisherMACs(std::vector<MAC> &macs)
{
	factoryDal.Devices().GetOrderedChannelsDevMACs(macs);
}

bool DevicesManager::GetAlertProvision(int &categoryProcessSub, int &categoryDeviceSub, int &categoryNetworkSub, int &categorySecuritySub)
{
	return factoryDal.Devices().GetAlertProvision(categoryProcessSub, categoryDeviceSub, categoryNetworkSub, categorySecuritySub);
}

void DevicesManager::SaveAlertIndication(int devID, int TSAPID, int objID, const nlib::DateTime &timestamp,  int milisec,
		int classType, int direction, int category, int type, int priority, std::string &data)
{
	factoryDal.Devices().SaveAlertInfo(devID, TSAPID, objID, timestamp, milisec, classType, direction, category, type, priority, data);
}

void DevicesManager::DeleteChannelsStatistics(int deviceID)
{
	factoryDal.Devices().DeleteChannelsStatistics(deviceID);
}
void DevicesManager::SaveChannelsStatistics(int deviceID, const std::string& strChannelsStatistics)
{
	factoryDal.Devices().SaveChannelsStatistics(deviceID, strChannelsStatistics);
}

void DevicesManager::ResetDevicesChanges()
{
	//STEP 5: now reset changes for all devices (used for next call of ProcessTopology)
	for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
	{
		it->second->ResetChanged();
	}
}

//commented by Cristian.Guef
//void DevicesManager::ComputeNodeLevels(GTopologyReport::DevicesListT& devicesList)
//{
//	LOG_DEBUG("ComputeNodeLevels: Processing node levels...");
//
//	for (NodesRepository::NodesByMACT::iterator it = repository.nodesByMAC.begin(); it != repository.nodesByMAC.end(); it++)
//	{
//		/*reset the level, except for the SM*/
//		if (it->second->Type() == Device::dtSystemManager)
//			it->second->Level(0);
//		else
//		{
//			it->second->Level(-1);
//			LOG_DEBUG("Setting level = -1 for device with ip = " << it->second->IP().ToString());
//		}
//	}
//
//	bool modified = true;
//	while (modified)
//	{
//		modified = false;
//		for (GTopologyReport::DevicesListT::iterator it = devicesList.begin(); it != devicesList.end(); it++)
//		{
//			DevicePtr node = repository.Find(it->DeviceMAC);
//
//			//added
//			if(node)
//			{
//				LOG_DEBUG("found device in repository with ip = " << node->IP().ToString() <<
//						  "and device_level = " << (boost::int32_t)node->Level());
//			}
//			else
//			{
//				LOG_DEBUG("not found device in repository with = " << it->DeviceMAC.ToString());
//			}
//
//
//			if (node && node->Level() < 0) // not computed yet
//
//			{
//				LOG_DEBUG("ComputeNodeLevels: Trying to place node with id=" << node->id);
//				LOG_DEBUG("Trying to place node with ip = " << node->IP().ToString());
//				for (GTopologyReport::Device::NeighborsListT::const_iterator itNeighbor = it->NeighborsList.begin(); itNeighbor
//				    != it->NeighborsList.end(); itNeighbor++)
//				{
//					DevicePtr nodeNeighbor = repository.Find(itNeighbor->DeviceIP);
//					if (nodeNeighbor)
//					{
//						LOG_DEBUG("ComputeNodeLevels: Node with id=" << node->id << " has neighbor id=" << nodeNeighbor->id
//						    << " on level " << nodeNeighbor->Level());
//
//						/* commented by Cristian.Guef
//						if (nodeNeighbor->Level() >= 0 && ((node->Level() == -1) || (node->Level()> nodeNeighbor->Level() + 1)))
//						*/
//						//added by Cristian.Guef
//						if (nodeNeighbor->Level() >= 0)
//						{
//							//added by Cristian.Guef
//							if (node->Level() <= nodeNeighbor->Level())
//								node->Level(nodeNeighbor->Level() + 1);
//
//							/* commnetd
//							node->Level(nodeNeighbor->Level() + 1);
//							*/
//
//							modified = true;
//							LOG_DEBUG("ComputeNodeLevels: Node id=" << node->id << "was placed on level " << (nodeNeighbor->Level()
//							    + 1) << " as a neighbor of node id= " << nodeNeighbor->id);
//						}
//					}
//
//				}//for (GTopologyReport::Device::NeighborsListT
//
//			}
//		}//for (GTopologyReport::DevicesListT::iterator it
//	}
//
//	LOG_DEBUG("ComputeNodeLevels: Processing node levels ended...");
//}

static void StartComputeNodeLevelsForTraversed(DevicesGraph &devGraph, int vertex, int NodeLevel);
struct TraverseInfo
{
	bool isTraversed;
	int	 level;
	TraverseInfo():isTraversed(false), level(-1)
	{}
};
static void SetNeighbsLevel(DevicesGraph &devGraph, std::queue<int> &neighbVertices/*[int/out]-(out)just traversed*/, int vertexNo, TraverseInfo *pTraverseInfo, int NodeLevel)
{
	for (std::list<DevicesGraph::NeighbourData>::const_iterator y = devGraph.GetNeighbours(vertexNo).begin();
							y != devGraph.GetNeighbours(vertexNo).end(); ++y)
	{
		LOG_DEBUG("for node=" << devGraph.GetVertexData(vertexNo)->IP().ToString() << " -> neighb node=" << devGraph.GetVertexData(y->vertexNo)->IP().ToString());

		if (pTraverseInfo[y->vertexNo].isTraversed == true)
		{
			//if (devGraph.GetVertexData(y->vertexNo)->Type() != Device::dtBackboneRouter &&
			//		devGraph.GetVertexData(y->vertexNo)->Type() != Device::dtGateway)
			if (devGraph.IsArc(y->vertexNo, vertexNo) == false)
			{
				//set longest path for traversed neighbours
				if (pTraverseInfo[y->vertexNo].level < NodeLevel  &&
						devGraph.GetVertexData(y->vertexNo)->Type() != Device::dtBackboneRouter)
				{
					pTraverseInfo[y->vertexNo].level = NodeLevel; //special case
					LOG_DEBUG("traversed node_level -> for dev_ip = " << devGraph.GetVertexData(y->vertexNo)->IP().ToString() << "set level=" << NodeLevel);

					//special case
					StartComputeNodeLevelsForTraversed(devGraph, y->vertexNo, NodeLevel + 1);
				}
			}
			else
			{
				//set longest path for traversed neighbours on the same level with source
				if ((pTraverseInfo[y->vertexNo].level < NodeLevel - 1) &&
						devGraph.GetVertexData(y->vertexNo)->Type() != Device::dtBackboneRouter)
				{
					pTraverseInfo[y->vertexNo].level = NodeLevel -1 ;
					LOG_DEBUG("traversed node_level -> for dev_ip = " << devGraph.GetVertexData(y->vertexNo)->IP().ToString() << "set level=" << (int)(NodeLevel - 1));
				}
			}
			continue;
		}

		pTraverseInfo[y->vertexNo].isTraversed = true;
		pTraverseInfo[y->vertexNo].level = NodeLevel;
		LOG_DEBUG("node_level -> for dev_ip = " << devGraph.GetVertexData(y->vertexNo)->IP().ToString() << "set level=" << NodeLevel);
		neighbVertices.push(y->vertexNo);
	}
}
//longest path
void DevicesManager::StartComputeNodeLevels(DevicesGraph &devGraph, int vertex)
{
	int verticesNo = devGraph.GetVerticesNo();
	TraverseInfo *pTraversed = new TraverseInfo [verticesNo];
	std::queue<int> neighbVertices;
	int NodeLevel = 1;

	//init
	pTraversed[vertex].isTraversed = true;
	pTraversed[vertex].level = 0;

	SetNeighbsLevel(devGraph, neighbVertices, vertex, pTraversed, NodeLevel++);
	int count = neighbVertices.size();
	while (neighbVertices.size() > 0)
	{
		SetNeighbsLevel(devGraph, neighbVertices, neighbVertices.front(), pTraversed, NodeLevel);
		neighbVertices.pop();
		--count;
		if (!count)
		{
			NodeLevel++;
			count = neighbVertices.size();
		}
	}

	for (int i = 0; i < verticesNo; ++i)
	{
		if (devGraph.GetVertexData(i))
			devGraph.GetVertexData(i)->Level(pTraversed[i].level);
	}

	if (devGraph.GetVerticesNo())
		delete[] pTraversed;
}

///////////////////////////////////
//SPECIAL CASE: when a traversed device is lifted from a lower level 
				// to a higher level then this lifting should be done also for
				// all traversed neighbours (connected with only outbound arc) in a BFS algoritm or 
				// one traversed neighbor if exist one with inbound/outbound arcs in cascade
static void SetTraversedNeighbsLevel(DevicesGraph &devGraph, std::queue<int> &neighbVertices/*[int/out]-(out)just outbound traversed only*/, int vertexNo, TraverseInfo *pTraverseInfo, int NodeLevel)
{
	for (std::list<DevicesGraph::NeighbourData>::const_iterator y = devGraph.GetNeighbours(vertexNo).begin();
							y != devGraph.GetNeighbours(vertexNo).end(); ++y)
	{
		
		LOG_DEBUG("special -> for node=" << devGraph.GetVertexData(vertexNo)->IP().ToString() << " -> neighb node=" << devGraph.GetVertexData(y->vertexNo)->IP().ToString());

		if (pTraverseInfo[y->vertexNo].isTraversed == false)
		{
			LOG_DEBUG("special -> node_level -> for dev_ip = " << devGraph.GetVertexData(y->vertexNo)->IP().ToString() << "not traversed");
			continue;
		}

		if (devGraph.IsArc(y->vertexNo, vertexNo) == true)
		{
			LOG_DEBUG("traversed node_level -> for dev_ip = " << devGraph.GetVertexData(y->vertexNo)->IP().ToString() << " with inbound/outbound links");
			continue;
		}

		//set longest path for traversed outbound only neighbours
		if (pTraverseInfo[y->vertexNo].level < NodeLevel  &&
			devGraph.GetVertexData(y->vertexNo)->Type() != Device::dtBackboneRouter)
		{
			pTraverseInfo[y->vertexNo].level = NodeLevel;
			LOG_DEBUG("traversed node_level -> for dev_ip = " << devGraph.GetVertexData(y->vertexNo)->IP().ToString() << "set level=" << NodeLevel);
			neighbVertices.push(y->vertexNo);	
		}
	}
}
static void StartComputeNodeLevelsForTraversed(DevicesGraph &devGraph, int vertex, int NodeLevel)
{
	int verticesNo = devGraph.GetVerticesNo();
	TraverseInfo *pTraversed = new TraverseInfo [verticesNo];
	std::queue<int> neighbVertices;

	SetTraversedNeighbsLevel(devGraph, neighbVertices, vertex, pTraversed, NodeLevel++);
	int count = neighbVertices.size();
	while (neighbVertices.size() > 0)
	{
		SetTraversedNeighbsLevel(devGraph, neighbVertices, neighbVertices.front(), pTraversed, NodeLevel);
		neighbVertices.pop();
		--count;
		if (!count)
		{
			NodeLevel++;
			count = neighbVertices.size();
		}
	}

	if (devGraph.GetVerticesNo())
		delete[] pTraversed;
}				
///////////////////////////////////


//added by Cristian.Guef
void DevicesManager::ComputeNodeLevels(DevicesGraph &devGraph, int vertex, unsigned char *pTraversed, int NodeLevel)
{
	std::queue<int> devListIndexes;

	for (std::list<DevicesGraph::NeighbourData>::const_iterator y = devGraph.GetNeighbours(vertex).begin();
							y != devGraph.GetNeighbours(vertex).end(); y++)
	{
		if (pTraversed[y->vertexNo] == 1/*traversed*/)
		{
			//set longest path for traversed neighbours
			//if (devGraph.GetVertexData(y->vertexNo)->Level() == NodeLevel - 1 && NodeLevel != 1)
			//	devGraph.GetVertexData(y->vertexNo)->Level(NodeLevel);
			continue;
		}

		devGraph.GetVertexData(y->vertexNo)->Level(NodeLevel);
		LOG_DEBUG("node_level -> for dev_ip = " << devGraph.GetVertexData(y->vertexNo)->IP().ToString() << "set level=" << NodeLevel);
		pTraversed[y->vertexNo] = 1/*traversed*/;
		devListIndexes.push(y->vertexNo);
	}

	while (devListIndexes.size() > 0)
	{
		ComputeNodeLevels(devGraph, devListIndexes.front(), pTraversed, NodeLevel + 1);
		devListIndexes.pop();
	}
}


//added by Cristian.Guef
void DevicesManager::ProcessSubnetIDsFromBBRs(DevicesGraph &devGraph, std::deque<int> &BBRVertices)
{
	//prepare for graph traversal
	unsigned char *pIsTraversed = new unsigned char[devGraph.GetVerticesNo()]; //0 - vertex not traversed
																			  //1 - vertex traversed
	memset(pIsTraversed, 0, devGraph.GetVerticesNo()*sizeof(unsigned char));


	while(BBRVertices.size() != 0)
	{
		DevicePtr bbrNode = devGraph.GetVertexData(BBRVertices.front());
		if (pIsTraversed[BBRVertices.front()] == 1) //only not traversed bbrs represent a new subnetID
		{
			BBRVertices.pop_front();
			continue;
		}
		if (bbrNode->GetSubnetID() == -1 /*no subnet ID*/)
		{
			LOG_WARN("BBR has no subnetID, dev_mac = " << bbrNode->mac.ToString());
		}
		else //an existing bbr in topology
		{
			ComputeSubnetID(devGraph, BBRVertices.front(), pIsTraversed, bbrNode->GetSubnetID()); //BFS (Breadth First Search)
		}
		BBRVertices.pop_front();
	}

	delete pIsTraversed;
}
void DevicesManager::ComputeSubnetID(DevicesGraph &devGraph, int vertex, unsigned char *pTraversed, int subnetID)
{
	std::queue<int> devListIndexes;

	for (std::list<DevicesGraph::NeighbourData>::const_iterator y = devGraph.GetNeighbours(vertex).begin();
							y != devGraph.GetNeighbours(vertex).end(); y++)
	{
		if (pTraversed[y->vertexNo] == 1/*traversed*/)
			continue;
		if (devGraph.GetVertexData(y->vertexNo)->Type() == 1/*sm*/) //do not process
			continue;
		if (devGraph.GetVertexData(y->vertexNo)->Type() == 2/*gw*/) //do not process
			continue;

		devGraph.GetVertexData(y->vertexNo)->SetSubnetID(subnetID);
		LOG_DEBUG("subnetID -> for dev_ip = " << devGraph.GetVertexData(y->vertexNo)->IP().ToString() << "set subnetID= " << subnetID);
		pTraversed[y->vertexNo] = 1/*traversed*/;
		devListIndexes.push(y->vertexNo);
	}

	while (devListIndexes.size() > 0)
	{
		ComputeSubnetID(devGraph, devListIndexes.front(), pTraversed, subnetID);
		devListIndexes.pop();
	}
}

void DevicesManager::SaveDeviceNeighbours(DevicesGraph &devGraph, int vertexNo)
{
	for (std::list<DevicesGraph::NeighbourData>::const_iterator itNeighbour = devGraph.GetNeighbours(vertexNo).begin(); itNeighbour
	    != devGraph.GetNeighbours(vertexNo).end(); ++itNeighbour)
	{
		DevicePtr devFrom = devGraph.GetVertexData(vertexNo);
		if (!devFrom)
		{
			LOG_WARN("ProcessDeviceNeighbors: unknown source device");
			continue;
		}
		DevicePtr devTo = devGraph.GetVertexData(itNeighbour->vertexNo);
		if (!devTo)
		{
			LOG_WARN("ProcessDeviceNeighbors: unknown dezstination device");
			continue;
		}
		try
		{
			factoryDal.Devices().CreateDeviceNeighbour(devFrom->id, devTo->id, itNeighbour->signalQuality, itNeighbour->clockSource);
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("ProcessTopology: Failed to save neighbor in db! error=" << ex.what() << "info: from dev_id=" << devFrom->id << "to devid="<<devTo->id);
		}
		catch (...)
		{
			LOG_ERROR("ProcessTopology: Failed to save neighbor in db! info: from dev_id=" << devFrom->id << "to devid="<<devTo->id);
		}
	}
}

void DevicesManager::SaveDeviceGraphs(const DevicePtr& nodeFrom, GTopologyReport::Device::GraphsListT& graphs)
{
	for (GTopologyReport::Device::GraphsListT::iterator itGraph = graphs.begin(); itGraph != graphs.end(); ++itGraph)
	{
		for (unsigned int i = 0; i < itGraph->infoList.size(); i++)
		{
			DevicePtr nodeTo = repository.Find(itGraph->infoList[i].neighbour);
			if (!nodeTo) //not found
			{
				LOG_WARN("ProcessDeviceGraphs: Unknown device graph reported!");
				LOG_WARN("ProcessDeviceGraphs: reported graph=" << itGraph->infoList[i].neighbour.ToString());
			}
			else
			{
				if (!nodeFrom)
				{
					LOG_WARN("ProcessDeviceGraphs: Unknown source device!");
					continue;
				}
				itGraph->infoList[i].DevListIndex = nodeTo->GetVertexNo();
				

				try
				{
					factoryDal.Devices().CreateDeviceGraph(nodeFrom->id, nodeTo->id, itGraph->GraphID);
				}
				catch (std::exception& ex)
				{
					LOG_ERROR("ProcessTopology: Failed to save neighbor in db! error=" << ex.what() << "info: from dev_id=" << nodeFrom->id << "to devid="<<nodeTo->id);
				}
				catch (...)
				{
					LOG_ERROR("ProcessTopology: Failed to save neighbor in db! info: from dev_id=" << nodeFrom->id << "to devid=" <<nodeTo->id);
				}
			}
		}
	}
}


DevicePtr DevicesManager::FindDevice(boost::int32_t deviceID) const
{
	return repository.Find(deviceID);
}

DevicePtr DevicesManager::FindDevice(const MAC& deviceMAC) const
{
	return repository.Find(deviceMAC);
}

void DevicesManager::UpdatePublishFlag(int deviceID, int flag)
{
	return factoryDal.Devices().UpdatePublishFlag(deviceID, flag);
}

const DevicePtr DevicesManager::GatewayDevice() const
{

	//added by Cristian.Guef
	//it should be only one gw in the system - see "ProcessTopology" function

	/* commented by Cristian.Guef
	for (NodesRepository::NodesByIDT::const_iterator it = repository.nodesById.begin(); it != repository.nodesById.end(); it++)
	{
		if (it->second->Type() == Device::dtGateway)
			return it->second;//found it
	}
	*/

	//added by Cristian.Guef
	if (m_gwDevPtr)
		return m_gwDevPtr;

	THROW_EXCEPTION0(NoGatewayException);
}



void DevicesManager::HandleGWConnect(const std::string& host, int port)
{
	LOG_INFO("Gateway reconnected ...");
	isGatewayConnected = true;

	ChangeGatewayStatus(Device::dsUnregistered);
	DevicePtr gateway = GatewayDevice();
	if (gateway)
	{
		factoryDal.Devices().AddDeviceConnections(gateway->id, host, port);
	}
	else
	{
		LOG_WARN("Gateway device not found...");
	}
}
void DevicesManager::HandleGWDisconnect()
{
	LOG_WARN("Gateway disconnected!");
	isGatewayConnected = false;

	ChangeGatewayStatus(Device::dsNotConnected);
	DevicePtr gateway = GatewayDevice();
	if (gateway)
	{
		factoryDal.Devices().RemoveDeviceConnections(gateway->id);
	}
	else
	{
		LOG_WARN("Gateway device not found...");
	}

}

bool const DevicesManager::GatewayConnected()
{
	return isGatewayConnected;
}

void DevicesManager::ChangeGatewayStatus(Device::DeviceStatus newStatus)
{
	try
	{
		DevicePtr gateway = GatewayDevice();

		gateway->ResetChanged();
		gateway->Status(newStatus);
		if (gateway->Changed())
		{
			factoryDal.Devices().UpdateDevice(*gateway);
			gateway->ResetChanged();
		}
	}
	catch(std::exception& ex)
	{
		LOG_ERROR("ChangeGatewayState: Failed to update gateway status! error=" << ex.what());
	}
	catch(...)
	{
		LOG_ERROR("ChangeGatewayState: Failed to update gateway status! unknown error!");
	}
}

const DevicePtr DevicesManager::SystemManagerDevice() const
{
	/* commented by Cristian.Guef
	for (NodesRepository::NodesByIDT::const_iterator it = repository.nodesById.begin(); it != repository.nodesById.end(); it++)
	{
		if (it->second->Type() == Device::dtSystemManager)
			return it->second;//found it
	}
	*/

	//added by Cristian.Guef
	if (!m_smDevPtr)
		return DevicePtr();

	return m_smDevPtr;
}

bool DevicesManager::IsSMDevice(int DeviceID)
{
	DevicePtr dev = FindDevice(DeviceID);

	if (!dev)
		return false;

	if (dev->Type() == Device::dtSystemManager)
			return true;
	return false;
}

const DevicePtr DevicesManager::BBRDevice() const
{
	if (!m_bbrDevPtr)
		return DevicePtr();
		
	return m_bbrDevPtr;
}

const NodesRepository& DevicesManager::Devices() const
{
	return repository;
}

/* commented by Cristian.Guef
void DevicesManager::SetDeviceContract(const DevicePtr& device, int contractID)
{
	//the contracts should be unique in all system at current time

	//try to detect contract duplicates
	for (NodesRepository::NodesByIDT::const_iterator it = repository.nodesById.begin(); it != repository.nodesById.end(); it++)
	{
		if ((it->second->id != device->id) //is different
		    && it->second->HasContract() && (it->second->ContractID() == contractID))
		{
			LOG_WARN("SetDeviceContract: duplicated contracts detected! ExitingDevice=" << it->second->ToString()
			    << " NewDevice=" << device->ToString() << " with ContractID=" << contractID
			    << " (for existing device the ContractID is reseted)");
			it->second->ResetContractID();
		}
	}

	device->ContractID(contractID);
}
*/
//added by Cristian.Guef
void DevicesManager::SetDeviceContract(const DevicePtr& device, unsigned int contractID,
									   int resourceID, unsigned char leaseType, boost::int16_t committedBurst,
									   unsigned char &leaseTypeReplaced)
{
	//the contracts should be unique in all system at current time

	//try to detect contract duplicates
	for (NodesRepository::NodesByIDT::const_iterator it = repository.nodesById.begin(); it != repository.nodesById.end(); it++)
	{
		//if ((it->second->id != device->id) //is different
		//    && it->second->HasContract(resourceID, leaseType) && (it->second->ContractID(resourceID, leaseType) == contractID))
		//{
		//	LOG_WARN("SetDeviceContract: duplicated contracts detected! ExitingDevice=" << it->second->ToString()
		//	    << " NewDevice=" << device->ToString() << " with ContractID=" << contractID
		//	    << " (for existing device the ContractID is reseted)");
		//	it->second->ResetContractID();
		//}

		//delete contractID if found already in the system
		unsigned int leaseTypes = it->second->m_ResourceIDToContractID.size();
		for(unsigned int k = 0; k < leaseTypes; k++){
			for (std::map<unsigned int, boost::uint32_t>::iterator l = it->second->m_ResourceIDToContractID[k].begin();
				l != it->second->m_ResourceIDToContractID[k].end(); l++)
			{
				if (l->second == contractID)
				{
					/* don't delete it on gw because it is already deleted
					//we found
					InfoToDelete info;
					info.IPAddress = it->second->IP();
					info.ContractID = l->second;
					info.ResourceID = l->first;
					info.LeaseType = k;
					//push it
					queueToDeleteLease.push(info);
					*/
					LOG_WARN("SetDeviceLease: duplicated leases detected! ExitingDevice=" << it->second->ToString()
							<<" -> with leaseID = " << (boost::uint32_t)l->second
							<< "and Objid = " << (boost::uint32_t)(l->first >> 16)
							<< "and TLSAP_id = " << (boost::uint32_t)(l->first & 0xFFFF)
							<< "and lease_type = " << (boost::uint32_t)k

							<< " ----- NewDevice=" << device->ToString() << " -> with leaseID=" << contractID
							<< "and Objid = " << (boost::uint32_t)(resourceID >> 16)
							<< "and TLSAP_id = " << (boost::uint32_t)(resourceID & 0xFFFF)
							<< "and lease_type = " << (boost::uint32_t)leaseType

							<< " (for existing device the LeaseID is deleted)");

					//now reset contract id to force recreate it
					it->second->ResetContractID(l->first, k /*leasetype*/);

					leaseTypeReplaced = k;

					goto finished;
				}
			}
		}

	}

finished:
	device->ContractID(resourceID, leaseType, contractID);
	device->LeaseCommittedBurst(resourceID, leaseType, committedBurst);
}

/*
static void gettime(nlib::DateTime &ReadingTime, short &milisec, const struct timeval &tv)
{
  time_t curtime;
  curtime=tv.tv_sec;
  struct tm * timeinfo;

  timeinfo = gmtime(&curtime);
  ReadingTime = nlib::CreateTime(timeinfo->tm_year+1900, timeinfo->tm_mon + 1,
  									timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min,
  									timeinfo->tm_sec);
  milisec = tv.tv_usec/1000;

  //printf ("\n Current local time and date: %s.%d", nlib::ToString(myDT.ReadingTime).c_str(), myDT.milisec);
  //fflush(stdout);

}
*/

#define THRESHOLD 150
void DevicesManager::SaveReadings(const DeviceReadingsList& readings)
{
	//added
	WATCH_DURATION_INIT_DEF(oDurationWatcher);

	LOG_ERROR("SHOULD MUST NOT be called");
	factoryDal.BeginTransaction();
	try
	{
		LOG_INFO("SaveReadings: Begin transaction. ReadingsCount=" << readings.size());


		//added by Cristian.Guef
		int db_id = 0;

		for (DeviceReadingsList::const_iterator it = readings.begin();
				it != readings.end(); it++)
		{

			SaveReading(*it);

				LOG_DEBUG("SaveReadings: one saved."
					<< " Type=" << (int)it->readingType
					<< " DeviceID=" << it->deviceID
					<< ", ChannelNo=" << it->channelNo
					<< ", Value=" << it->rawValue.ToString());
			//added by Cristian.Guef
			if (db_id != it->deviceID && it->readingType == DeviceReading::PublishSubcribe)
			{
				//db_id == it->deviceID;
				//nlib::DateTime ReadingTime;
				//short milisec;
				//gettime(ReadingTime, milisec, it->tv);
				//UpdateReadingTimeforDevice(it->deviceID, ReadingTime);
				factoryDal.Devices().UpdateReadingTimeforDevice(it->deviceID, it->tv);
			}
		}
		factoryDal.CommitTransaction();

		WATCH_DURATION_DEF(oDurationWatcher);
		LOG_INFO("SaveReadings: Finished transaction. ReadingsCount=" << readings.size());
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("SaveReadings: Failed. error=" << ex.what());
		factoryDal.RollbackTransaction();
	}
	catch (...)
	{
		LOG_ERROR("SaveReadings: Failed. unknown error!");
		factoryDal.RollbackTransaction();
	}
}

void DevicesManager::SaveReading(const DeviceReading& reading)
{
	/* commented by Cristian.Guef
	double computedValue;
	ComputeReadingValue(reading, computedValue);

	LOG_DEBUG("SaveReading: Type=" << (int) reading.readingType << " DeviceID=" << reading.deviceID << ", ChannelNo="
	    << reading.channel.channelNumber << ", RawValue=" << reading.rawValue.ToString() << ", Value=" << reading.value);
	*/

	//added by Cristian.Guef
	//reading.value = reading.rawValue.ToString();

	factoryDal.Devices().UpdateReading(reading);
}



void DevicesManager::SaveReadings(PublishedDataMng *pPublishedDataMng)
{
	std::list<int> *pReadingHandles = pPublishedDataMng->GetReadingHandles();
	std::list<int>::iterator i = pReadingHandles->begin();
	int handle = 0;
	WATCH_DURATION_INIT_DEF(oDurationWatcher); // try to not include logs
	//

	std::queue<int> handles;

	std::map<int,DeviceReading*> oChannelLastValue;

	try
	{

		WATCH_DURATION_DEF(oDurationWatcher);
		factoryDal.BeginTransaction();

		while (i != pReadingHandles->end())
		{
			handle = *i++;

			//for setting fresh data status
			handles.push(handle);

			//
			DeviceReadingsList *pReadings = pPublishedDataMng->GetReadings(handle);

			if (!pReadings || pReadings->empty())
			{
				continue;
			}

			//old_way
			//nlib::DateTime ReadingTime;
			//short milisec;
			//gettime(ReadingTime, milisec, pReadings->begin()->tv);
			//UpdateReadingTimeforDevice((dev_ID = pReadings->begin()->deviceID), ReadingTime);
			//new_way
			int dev_ID = 0;
			//factoryDal.Devices().UpdateReadingTimeforDevice((dev_ID = pReadings->begin()->deviceID), pReadings->begin()->tv);

			dev_ID = pReadings->begin()->deviceID;
			//update publish flag
			if (pPublishedDataMng->IsFreshData(handle) == false)
			{
				UpdatePublishFlag(dev_ID, 1/*fresh_data*/);
				pPublishedDataMng->SetFreshDataStatus(handle, true);
			}

			//
			for (DeviceReadingsList::iterator it = pReadings->begin(); it != pReadings->end(); it++)
			{
				DeviceReading* pReading = &(*it);

				std::map<int,DeviceReading*>::iterator itChannel = oChannelLastValue.find(pReading->channelNo);

				if (itChannel == oChannelLastValue.end())
				{
					oChannelLastValue [pReading->channelNo] = pReading;
				}
				else
				{
					DeviceReading* pMapReading = itChannel->second;
					if (pReading->tv.tv_sec > pMapReading->tv.tv_sec)
					{
						oChannelLastValue [pReading->channelNo] = pReading;
					}
				}


				if (m_rConfigApp.IsDeviceReadingsHistoryEnable())
				{
						factoryDal.Devices().AddReading(*it,true);

							LOG_DEBUG("SaveReadings: one saved."
								<< " Type=" << (int)it->readingType
								<< " DeviceID=" << it->deviceID
								<< ", ChannelNo=" << it->channelNo
								<< ", Value=" << it->rawValue.ToString());
				}
			}
			//
			//devices->SaveReadings(*pPublishedDataMng->GetReadings(handle));

			LOG_DEBUG("SaveReadings: ReadingsCount=" << pReadings->size() << " for devID=" << dev_ID);

		}



		std::map<int,DeviceReading*>::iterator itChannels = oChannelLastValue.begin();

		for (;itChannels != oChannelLastValue.end(); itChannels++)
		{
			DeviceReading* pReading = itChannels->second;

			factoryDal.Devices().UpdateReading(*pReading);

				LOG_DEBUG("SaveReadings: one saved."
					<< " Type=" << (int)pReading->readingType
					<< " DeviceID=" << pReading->deviceID
					<< ", ChannelNo=" << pReading->channelNo
					<< ", Value=" << pReading->rawValue.ToString());
		}

		factoryDal.CommitTransaction();
		WATCH_DURATION_DEF(oDurationWatcher);
		//LOG_DEBUG("SaveReadings: Finished transaction.");


		LOG_INFO("SaveReadings: STAT ReadingsCount=" << pPublishedDataMng->GetAllReadingsNo()
			<< " UpdateReadingsCount=" << oChannelLastValue.size());

	}
	catch (std::exception& ex)
	{
		LOG_ERROR("SaveReadings: Failed. error=" << ex.what());

		while(!handles.empty())
		{
			pPublishedDataMng->ClearReadings(handles.front());
			handles.pop();
		}
		factoryDal.RollbackTransaction();
		return;
	}
	catch (...)
	{
		LOG_ERROR("SaveReadings: Failed. unknown error!");
		//if transaction error
		while(!handles.empty())
		{
			pPublishedDataMng->ClearReadings(handles.front());
			handles.pop();
		}
		factoryDal.RollbackTransaction();
		return;
	}

	while(!handles.empty())
	{
		pPublishedDataMng->ClearReadings(handles.front());
		handles.pop();
	}
}

void DevicesManager::CreateReading(const DeviceReading& reading)
{
	UpdatePublishFlag(reading.deviceID, 1/*fresh_data*/);
	factoryDal.Devices().AddReading(reading);
}
void DevicesManager::CreateEmptyReadings(int deviceID)
{
	factoryDal.Devices().AddEmptyReadings(deviceID);
}
void DevicesManager::CreateEmptyReading(int deviceID, int channelNo)
{
	factoryDal.Devices().AddEmptyReading(deviceID, channelNo);
}
void DevicesManager::DeleteReading(int channelNo)
{
	factoryDal.Devices().DeleteReading(channelNo);
}
void DevicesManager::DeleteReadings(int deviceID)
{
	factoryDal.Devices().DeleteReadings(deviceID);
}
void FormatOutputValue(int channelType, double computedValue, std::string& stringComputedValue)
{
	if (channelType == DeviceChannel::ctDigital)
	{
		stringComputedValue = boost::str(boost::format("%1%") % computedValue);
	}
	else
	{
		//HACK:[Ovidiu.Rauca] - to avoid values like -0.0000 (logged issue about this)
		if (computedValue > -1 && computedValue < 0)
		{
			double intComputedValue = (int)(computedValue*10000);
			computedValue = intComputedValue/10000;
		}
		stringComputedValue = boost::str(boost::format("%.4f") % computedValue);
	}
}



bool DevicesManager::ComputeReadingValue(const DeviceReading& reading, double& computedValue)
{
	/*
	const DeviceChannel& channel = reading.channel;
	if (channel.maxRawValue != channel.minRawValue)
	{

		LOG_DEBUG("Computing reading value... dataType:=" << reading.rawValue.dataType << " rawValue:="
		    << reading.rawValue.GetDoubleValue());
		computedValue = channel.minValue + double((reading.rawValue.GetDoubleValue() - channel.minRawValue) * (channel.maxValue
						- channel.minValue)) / (channel.maxRawValue - channel.minRawValue);
		LOG_DEBUG("Computed reading value:=" << computedValue);

		FormatOutputValue(channel.channelType, computedValue, reading.value);
		return true;
	}
	else
	{
		reading.value = "N/A"; //INFO: NaN
		return false;
	}
	*/
	return false;
}

bool DevicesManager::GetNextFirmwareUpload(int notFailedForMinutes, FirmwareUpload& result)
{
	return factoryDal.Devices().GetNextFirmwareUpload(notFailedForMinutes, result);
}

bool DevicesManager::GetFirmwareUpload(boost::uint32_t id, FirmwareUpload& result)
{
	return factoryDal.Devices().GetFirmwareUpload(id, result);
}

void DevicesManager::UpdateFirmware(const FirmwareUpload& upload)
{
	factoryDal.Devices().UpdateFirmwareUpload(upload);
}

void DevicesManager::ResetPendingFirmwareUploads()
{
	factoryDal.Devices().ResetPendingFirmwareUploads();
}

}//hostapp
}//nisa100
