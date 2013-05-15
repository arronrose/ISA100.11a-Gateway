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


#include "ReportsProcessor.h"

#include <list>
#include <queue>

namespace nisa100 {
namespace hostapp {


/*TOPOLOGY_REPORT*/

void ReportsProcessor::ClearTopoStructIndexForNeighbours(boost::shared_ptr<GNeighbourHealthReport> neighboursPtr/*[in/out]*/)
{
	assert(neighboursPtr);

	for (GNeighbourHealthReport::NeighboursMapT::iterator l = neighboursPtr->NeighboursMap.begin();
				l != neighboursPtr->NeighboursMap.end(); l++)
	{
		l->second.topoStructIndex = -1;
	}
}

void ReportsProcessor::SetSignalQualityForNeighbours(int devListIndex,
													 GTopologyReport::Device::NeighborsListT::iterator i,
										GTopologyReport::Device::NeighborsListT::iterator iEnd,
										const boost::shared_ptr<GNeighbourHealthReport> neighboursPtr)
{

	assert(i != iEnd && neighboursPtr->NeighboursMap.size() > 0);

	GNeighbourHealthReport::NeighboursMapT::iterator l = neighboursPtr->NeighboursMap.begin();
	
	do	//perform for neighbours
	{
		if (l == neighboursPtr->NeighboursMap.end())
		{
			LOG_WARN("topo_struct_fill - neighbour dev ip = " <<  i->DeviceIP.ToString() <<
					" NOT FOUND in neighbour_health_rep so its signal quality is set to '0'");
			boost::uint8_t *pval = (boost::uint8_t*)&i->SignalQuality; //
			*pval = 0;
			++i;
			continue;
		}
		if (i->DeviceIP == l->first)
		{
			//found a match...
		}
		else 
		{
			if (l->first < i->DeviceIP)
			{
				bool finished = false;
				do
				{
					//clear also
					l->second.topoStructIndex = -1;
	
					++l;
					if (l == neighboursPtr->NeighboursMap.end())
					{
						finished = true;
						break;
					}
				}while (l->first < i->DeviceIP);
				if (finished == true)
				{
					LOG_WARN("topo_struct_fill - neighbour dev ip = " <<  i->DeviceIP.ToString() <<
							" NOT FOUND in neighbour_health_rep so its signal quality is set to '0'");
					boost::uint8_t *pval = (boost::uint8_t*)&i->SignalQuality; //
					*pval = 0;
					++i;
					continue;
				}
				if (i->DeviceIP < l->first)
				{
					LOG_WARN("topo_struct_fill - neighbour dev ip = " <<  i->DeviceIP.ToString() <<
							" NOT FOUND in neighbour_health_rep so its signal quality is set to '0'");
					boost::uint8_t *pval = (boost::uint8_t*)&i->SignalQuality; //
					*pval = 0;
					++i;
					continue;
				}
				
				//found a match...
			}
			else
			{
				LOG_WARN("topo_struct_fill - neighbour dev ip = " <<  i->DeviceIP.ToString() <<
					" NOT FOUND in neighbour_health_rep so its signal quality is set to '0'");
				boost::uint8_t *pval = (boost::uint8_t*)&i->SignalQuality; //
				*pval = 0;
				++i;
				continue;
			}
		}
		
		
		//for history processing
		l->second.topoStructIndex = i->DevListIndex;

		boost::uint8_t *pval = (boost::uint8_t*)&i->SignalQuality; //
		*pval = l->second.signalQuality;

		LOG_DEBUG("topo_struct_fill - dev_index = " << devListIndex <<
				"and neighbour ip = " << l->first.ToString() <<
				" has signal quality =  " <<  (boost::uint32_t)l->second.signalQuality);
		//increment here
		++i;
		++l;
	}while(i != iEnd);

	//clear
	while (l != neighboursPtr->NeighboursMap.end())
	{
		l->second.topoStructIndex = -1;
		++l;
	}
}


void ReportsProcessor::CreateIndexForTopoStruct(const GTopologyReport &TopoRep)
{
	LOG_INFO("sorted_topo_struct_fill -> found topo_rep with size = " << (boost::uint32_t)TopoRep.DevicesList.size());
	
	LOG_DEBUG("sorted_topo_struct_fill--> begin --------");
	
	int indexDevice = 0;
	m_TopoStructIndex.clear();
	for(GTopologyReport::DevicesListT::const_iterator i = TopoRep.DevicesList.begin(); 
					i != TopoRep.DevicesList.end(); ++i)
	{ 

		TopologyIndexKey key;
		key.deviceIP = i->DeviceIP;
		
		if (m_TopoStructIndex.insert(TopoStructIndexT::value_type(key, indexDevice)).second == false)
		{
			LOG_WARN("duplicated device for topology -> dev_ip = " << i->DeviceIP.ToString());
		}
		indexDevice++;
	}
	LOG_DEBUG("sorted_topo_struct_fill--> end --------");
}

struct TopoNeighbour
{
	IPv6									deviceIP;
	std::list<int* /*indexInNeighbour*/>	IndexesPtrList;
};

static void BuildNeighboursList(GTopologyReport::Device::NeighborsListT::iterator i,
								GTopologyReport::Device::NeighborsListT::iterator iEnd,
								std::list<TopoNeighbour> &neighboursList/*[in/out]*/)
{
	assert(i != iEnd);

	std::list<TopoNeighbour>::iterator j = neighboursList.begin();
	
	do	//perform for neighbours
	{
		if (j == neighboursList.end())
		{
			LOG_DEBUG("build_topo_neighbours_list - neighbour dev ip = " <<  i->DeviceIP.ToString() <<
					" NOT FOUND in list, so add it to the list.");
			
			neighboursList.push_back(TopoNeighbour());
			TopoNeighbour &neighbour = *neighboursList.rbegin();
			neighbour.deviceIP = i->DeviceIP.ToString();
			neighbour.IndexesPtrList.push_back((int*)&i->DevListIndex);
			++i;
			continue;
		}
		if (i->DeviceIP == j->deviceIP)
		{
			//found a match...
		}
		else 
		{
			if (j->deviceIP < i->DeviceIP)
			{
				bool finished = false;
				do
				{
					++j;
					if (j == neighboursList.end())
					{
						finished = true;
						break;
					}
				}while (j->deviceIP < i->DeviceIP);
				if (finished == true)
				{
					LOG_DEBUG("build_topo_neighbours_list - neighbour dev ip = " <<  i->DeviceIP.ToString() <<
						" NOT FOUND in list, so add it to the list.");
					neighboursList.push_back(TopoNeighbour());
					TopoNeighbour &neighbour = *neighboursList.rbegin();
					neighbour.deviceIP = i->DeviceIP.ToString();
					neighbour.IndexesPtrList.push_back((int*)&i->DevListIndex);
					++i;
					continue;
				}
				if (i->DeviceIP < j->deviceIP)
				{
					LOG_DEBUG("build_topo_neighbours_list - neighbour dev ip = " <<  i->DeviceIP.ToString() <<
						" NOT FOUND in list, so add it to the list.");
					TopoNeighbour neighbour;
					neighbour.deviceIP = i->DeviceIP;
					neighbour.IndexesPtrList.push_back((int*)&i->DevListIndex);
					neighboursList.insert(j, neighbour);
					++i;
					continue;
				}
				
				//found a match...
			}
			else 
			{
				LOG_DEBUG("build_topo_neighbours_list - neighbour dev ip = " <<  i->DeviceIP.ToString() <<
						" NOT FOUND in list, so add it to the list.");
				TopoNeighbour neighbour;
				neighbour.deviceIP = i->DeviceIP;
				neighbour.IndexesPtrList.push_back((int*)&i->DevListIndex);
				neighboursList.insert(j, neighbour);
				++i;
				continue;
			}
		}
		
		j->IndexesPtrList.push_back((int*)&i->DevListIndex);

		LOG_DEBUG("build_topo_neighbours_list - neighbour ip = " << i->DeviceIP.ToString() << " added to the list ");
		//increment here
		++i;
		++j;
	}while (i != iEnd);
	
}

static void SetDevListIndexForNeighbours(int topoDevIndex, std::list<int* /*indexInNeighbour*/>	&IndexesPtrList)
{
	assert (IndexesPtrList.size() > 0);

	while (IndexesPtrList.size())
	{
		*(IndexesPtrList.front()) = topoDevIndex;
		IndexesPtrList.pop_front();
	}
}

void ReportsProcessor::FillDevListIndexForNeighbours(GTopologyReport &TopoRep /*[in/out]*/)
{
	std::list<TopoNeighbour> neighboursList;
	
	LOG_DEBUG("build_topo_neighbours_list--> begin --------");
	for(TopoStructIndexT::const_iterator i = m_TopoStructIndex.begin(); i != m_TopoStructIndex.end(); )
	{ //for every neighbour add devListIndex
		
		LOG_DEBUG("build_topo_neighbours_list--> for dev_ip = " << i->first.deviceIP.ToString());
		if (TopoRep.DevicesList[i->second].NeighborsList.size() != 0)
		{
			BuildNeighboursList(TopoRep.DevicesList[i->second].NeighborsList.begin(),
				TopoRep.DevicesList[i->second].NeighborsList.end(), neighboursList);
		}
		++i;
	}
	LOG_DEBUG("build_topo_neighbours_list--> end --------");


	LOG_DEBUG("topo_struct_neighbour_fill--> begin --------");
	std::list<TopoNeighbour>::iterator j = neighboursList.begin();

	for(TopoStructIndexT::const_iterator i = m_TopoStructIndex.begin(); i != m_TopoStructIndex.end(); ++i)
	{ 
		
		if (j == neighboursList.end())
		{
			LOG_DEBUG("topo_struct_neighbour_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
	 				" NOT FOUND in neighbours_list so skip it");
			continue;
		}
		if (i->first.deviceIP == j->deviceIP)
		{
			//found a match...
		}
		else 
		{
			if (j->deviceIP < i->first.deviceIP)
			{
				bool finished = false;
				do
				{
					++j;
					if (j == neighboursList.end())
					{
						finished = true;
						break;
					}
				}while (j->deviceIP < i->first.deviceIP);
				if (finished == true)
				{
					LOG_DEBUG("topo_struct_neighbour_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in neighbours_list so skip it");
					continue; // continue 'for'
				}
				if (i->first.deviceIP < j->deviceIP)
				{
					LOG_DEBUG("topo_struct_neighbour_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in neighbours_list so skip it");
					continue;
				}
				
				//found a match...
			}
			else 
			{
				LOG_DEBUG("topo_struct_neighbour_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 				" NOT FOUND in neighbours_list so skip it");
				continue;
			}
		}

		//set dev_list_index for every neighbour
		LOG_DEBUG("topo_struct_neighbour_fill dev ip = " <<  i->first.deviceIP.ToString() 
					<< "and neighBourip = " << j->deviceIP.ToString() << " and index = " << i->second);
		SetDevListIndexForNeighbours(i->second, j->IndexesPtrList);

		//increment here
		++j;
	}
	LOG_DEBUG("topo_struct_neighbour_fill--> end --------");
}

void ReportsProcessor::FillTopoStructWithSignalQuality(GTopologyReport &TopoRep /*[in/out]*/, 
							std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::iterator Ibegin,
							std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::iterator Iend,
							int mapSize/* for info only*/)
{
	LOG_INFO("topo_struct_fill -> found neighbour_health_rep with size = " << (boost::uint32_t)mapSize <<
					" and topo_rep with size = " << (boost::uint32_t)TopoRep.DevicesList.size());

	LOG_DEBUG("topo_struct_fill--> begin --------");
	
	std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::iterator j = Ibegin;

	for(TopoStructIndexT::const_iterator i = m_TopoStructIndex.begin(); i != m_TopoStructIndex.end(); ++i)
	{ //for every neighbour add signalQuality
		
		if (j == Iend)
		{
			LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
	 				" NOT FOUND in neighbour_health_rep so its signal quality is set to '0'");
			//default values for signal quality are already set
			continue;
		}
		if (i->first.deviceIP == j->first)
		{
			//found a match...
		}
		else 
		{
			if (j->first < i->first.deviceIP)
			{
				bool finished = false;
				do
				{
					//now clear
					ClearTopoStructIndexForNeighbours(j->second);
	
					++j;
					if (j == Iend)
					{
						finished = true;
						break;
					}
				}while (j->first < i->first.deviceIP);
				if (finished == true)
				{
					LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in neighbour_health_rep so its signal quality is set to '0'");
					//default values for signal quality are already set
					continue; // continue 'for'
				}
				if (i->first.deviceIP < j->first)
				{
					LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in neighbour_health_rep so its signal quality is set to '0'");
					//default values for signal quality are already set
					continue;
				}
				
				//found a match...
			}
			else 
			{
				LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 				" NOT FOUND in neighbour_health_rep so its signal quality is set to '0'");
				//default values for signal quality are already set
				continue;
			}
		}

		//for history processing
		j->second->m_topoStructIndex = i->second;

		//set signal for every neighbour
		if (TopoRep.DevicesList[i->second].NeighborsList.size() != 0 && 
						j->second->NeighboursMap.size() > 0) 
		{
			SetSignalQualityForNeighbours(i->second, 
				TopoRep.DevicesList[i->second].NeighborsList.begin(),
				TopoRep.DevicesList[i->second].NeighborsList.end(), j->second);
			j++;
			continue;
		}
		
		if (TopoRep.DevicesList[i->second].NeighborsList.size() == 0)
		{
			LOG_WARN("topo_struct_fill - found dev ip = " <<  i->first.deviceIP.ToString() <<
	 				" but neighbours NOT FOUND in neighbour_health_rep so its signal quality is already set to '0'");
		}
		
		if (j->second->NeighboursMap.size() > 0)
			ClearTopoStructIndexForNeighbours(j->second);
		
		//increment here
		++j;
	}

	//now clear for remaining neighbours
	while(j != Iend)
	{
		ClearTopoStructIndexForNeighbours(j->second);
		++j;
	}

	LOG_DEBUG("topo_struct_fill--> end --------");
}


void ReportsProcessor::FillTopoStructWithJoinCount(GTopologyReport &TopoRep /*[in/out]*/, 
												const GNetworkHealthReport &NetHealthRep)
{
	LOG_INFO("topo_struct_fill -> found net_health_rep with size = " << (boost::uint32_t)NetHealthRep.DevicesMap.size() <<
					" and topo_rep with size = " << (boost::uint32_t)TopoRep.DevicesList.size());

	LOG_DEBUG("topo_struct_fill--> begin --------");
	
	std::map<IPv6, GNetworkHealthReport::NetDeviceHealth>::const_iterator j = NetHealthRep.DevicesMap.begin();

	for(TopoStructIndexT::const_iterator i = m_TopoStructIndex.begin(); i != m_TopoStructIndex.end(); ++i)
	{ //for every ip add joinCount
		
		if (j == NetHealthRep.DevicesMap.end())
		{
			LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
	 				" NOT FOUND in net_list_rep so its joinCount was set to '0'");
			//default value for join_count is already set
			continue;
		}
		if (i->first.deviceIP == j->first)
		{
			//found a match...
		}
		else 
		{
			if (j->first < i->first.deviceIP)
			{
				bool finished = false;
				do
				{
					++j;
					if (j == NetHealthRep.DevicesMap.end())
					{
						finished = true;
						break;
					}
				}while (j->first < i->first.deviceIP);
				if (finished == true)
				{
					LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in net_list_rep so its joinCount was set to '0'");
					//default value for join_count is already set
					continue; // continue 'for'
				}
				if (i->first.deviceIP < j->first)
				{
					LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in net_list_rep so its joinCount was set to '0'");
					//default value for join_count is already set
					continue;
				}
				
				//found a match...
			}
			else 
			{
				LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 				" NOT FOUND in net_list_rep so its joinCount was set to '0'");
				//default value for join_count is already set
				continue;
			}
		}

		TopoRep.DevicesList[i->second].DeviceRejoinCount = j->second.joinCount;
		LOG_DEBUG("topo_struct_fill - dev ip = " << i->first.deviceIP.ToString() <<
				" has joinCount = " <<  j->second.joinCount);

		//increment here
		++j;
	}
	LOG_DEBUG("topo_struct_fill--> end --------");

}


void ReportsProcessor::FillTopoStructWithMACAndType(GTopologyReport	&TopoRep/*[in/out]*/,
													const GDeviceListReport &DevListRep)
{
	
	LOG_INFO("topo_struct_fill -> found dev_list_rep with size = " << (boost::uint32_t)DevListRep.DevicesMap.size() <<
					" and topo_rep with size = " << (boost::uint32_t)TopoRep.DevicesList.size());
	
	LOG_DEBUG("topo_struct_fill--> begin --------");
	
	UnregisteredDev unregDev;
	m_UnregDevsList.clear();
	TopoRep.GWIndex = -1;
	std::map<IPv6, GDeviceListReport::Device>::const_iterator j = DevListRep.DevicesMap.begin();

	for(TopoStructIndexT::const_iterator i = m_TopoStructIndex.begin(); i != m_TopoStructIndex.end(); ++i)
	{ //for every ip add mac address and device type
		
		if (j == DevListRep.DevicesMap.end())
		{
			LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
	 				" NOT FOUND in dev_list_rep so it was marked as deleted");
			TopoRep.DevicesList[i->second].isMarkedDeleted = true; 
	 		continue;
		}
		if (i->first.deviceIP == j->first)
		{
			//found a match...
		}
		else 
		{
			if (j->first < i->first.deviceIP)
			{
				bool finished = false;
				do
				{
					//save unregister devices
					unregDev.ip = j->first;
					unregDev.mac = j->second.DeviceMAC;
					unregDev.DeviceManufacture = j->second.DeviceManufacture;
					unregDev.DeviceModel = j->second.DeviceModel;
					unregDev.DeviceRevision = j->second.DeviceRevision;
					unregDev.DeviceSerialNo = j->second.DeviceSerialNo;
					unregDev.DeviceTag = j->second.DeviceTag;
					unregDev.DeviceStatus = j->second.DeviceStatus;
					switch (j->second.DeviceType)
					{
					case 1:
						unregDev.DeviceType = 11/*nonrouting device*/;
						break;			
					case 2:
						unregDev.DeviceType = 10/*routing device*/;
						break;
					case 3:
						unregDev.DeviceType = 12/*routing and IO device*/;
						break;
					case 4:
						unregDev.DeviceType = 3/*backbone router*/;
						break;
					case 8:
						unregDev.DeviceType = 2/*gateway*/;
						break;
					case 16:
						unregDev.DeviceType = 1/*system manager*/;
						break;
					default:
						unregDev.DeviceType = 200/*unknown*/;
						break;
					}
					if (m_UnregDevsList.insert(unregDev).second == false)
					{
						LOG_ERROR("topo_struct_fill - unregDev:duplicated mac found = " << j->second.DeviceMAC.ToString());
					}
					
					//LOG_INFO("topo_struct_fill - unregDev: mac found = " << j->second.DeviceMAC.ToString() << "with _status" << j->second.DeviceStatus);
					
					++j;
					if (j == DevListRep.DevicesMap.end())
					{
						finished = true;
						break;
					}
				}while (j->first < i->first.deviceIP);
				if (finished == true)
				{
					LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in dev_list_rep so it was marked as deleted");	
					TopoRep.DevicesList[i->second].isMarkedDeleted = true; 
					continue; // continue 'for' to delete unmatched devices
				}
				if (i->first.deviceIP < j->first)
				{
					LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in dev_list_rep so it was marked as deleted");	
					TopoRep.DevicesList[i->second].isMarkedDeleted = true; 
					continue;
				}
				
				//found a match...
			}
			else 
			{
				LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 				" NOT FOUND in dev_list_rep so it was marked as deleted");	
				TopoRep.DevicesList[i->second].isMarkedDeleted = true;
				continue;
			}
		}

		//
		TopoRep.DevicesList[i->second].isMarkedDeleted = false;

		//fill up from "DeviceList"
		TopoRep.DevicesList[i->second].DeviceMAC = j->second.DeviceMAC;
		TopoRep.DevicesList[i->second].PowerSupplyStatus = j->second.PowerSupplyStatus; //and power supply

		switch (j->second.DeviceType)
		{
		case 1:
			TopoRep.DevicesList[i->second].DeviceType = 11/*nonrouting device*/;
			break;			
		case 2:
			TopoRep.DevicesList[i->second].DeviceType = 10/*routing device*/;
			break;
		case 3:
			TopoRep.DevicesList[i->second].DeviceType = 12/*routing and IO device*/;
			break;
		case 4:
			TopoRep.DevicesList[i->second].DeviceType = 3/*backbone router*/;
			break;
		case 8:
			TopoRep.DevicesList[i->second].DeviceType = 2/*gateway*/;
			TopoRep.GWIndex = i->second;
			break;
		case 16:
			TopoRep.DevicesList[i->second].DeviceType = 1/*system manager*/;
			break;
		default:
			TopoRep.DevicesList[i->second].DeviceType = 200/*unknown*/;
			break;
		}

		LOG_DEBUG("topo_struct_fill - dev ip = " << i->first.deviceIP.ToString() <<
						" has mac = " <<  j->second.DeviceMAC.ToString() << 
						"and type = " << (boost::int32_t)j->second.DeviceType);

		//device status = registered
		if (j->second.DeviceStatus != 20 /*registered*/)
		{
			LOG_ERROR("topo_struct_fill - it should be status = 20 for dev_mac=" << j->second.DeviceMAC.ToString() << " and dev ip = " << i->first.deviceIP.ToString());
			TopoRep.DevicesList[i->second].DeviceStatus = 20;
		}
		else
		{
			TopoRep.DevicesList[i->second].DeviceStatus = j->second.DeviceStatus; 
		}
		TopoRep.DevicesList[i->second].deviceManufacturer = j->second.DeviceManufacture;
		TopoRep.DevicesList[i->second].deviceModel = j->second.DeviceModel;
		TopoRep.DevicesList[i->second].deviceRevision = j->second.DeviceRevision;
		TopoRep.DevicesList[i->second].deviceTAG = j->second.DeviceTag;
		TopoRep.DevicesList[i->second].deviceSerialNo = j->second.DeviceSerialNo;
	
		//increment here
		++j;
	}
	LOG_DEBUG("topo_struct_fill--> end --------");


	while (j != DevListRep.DevicesMap.end())
	{

		//save unregister devices
		unregDev.ip = j->first;
		unregDev.mac = j->second.DeviceMAC;
		unregDev.DeviceManufacture = j->second.DeviceManufacture;
		unregDev.DeviceModel = j->second.DeviceModel;
		unregDev.DeviceRevision = j->second.DeviceRevision;
		unregDev.DeviceSerialNo = j->second.DeviceSerialNo;
		unregDev.DeviceTag = j->second.DeviceTag;
		unregDev.DeviceStatus = j->second.DeviceStatus;
		switch (j->second.DeviceType)
		{
		case 1:
			unregDev.DeviceType = 11/*nonrouting device*/;
			break;			
		case 2:
			unregDev.DeviceType = 10/*routing device*/;
			break;
		case 3:
			unregDev.DeviceType = 12/*routing and IO device*/;
			break;
		case 4:
			unregDev.DeviceType = 3/*backbone router*/;
			break;
		case 8:
			unregDev.DeviceType = 2/*gateway*/;
			break;
		case 16:
			unregDev.DeviceType = 1/*system manager*/;
			break;
		default:
			unregDev.DeviceType = 200/*unknown*/;
			break;
		}
		if (m_UnregDevsList.insert(unregDev).second == false)
		{
			LOG_ERROR("topo_struct_fill - unregDev:duplicated mac found = " << j->second.DeviceMAC.ToString());
		}
		++j;
	}

}

void ReportsProcessor::FillTopoStructWithDevHealth(GTopologyReport &TopoRep /*[in/out]*/, 
						std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::iterator Ibegin,
						std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::iterator Iend,
						int mapSize/* for info only*/)
{

	LOG_INFO("topo_struct_fill -> found dev_health_list_rep with size = " << mapSize <<
					" and topo_rep with size = " << (boost::uint32_t)TopoRep.DevicesList.size());
	
	LOG_DEBUG("topo_struct_fill--> begin --------");
	
	std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::iterator j = Ibegin;

	for(TopoStructIndexT::const_iterator i = m_TopoStructIndex.begin(); i != m_TopoStructIndex.end(); ++i)
	{ //for every ip add mac address and device type
		
		if (j == Iend)
		{
			LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
	 				" NOT FOUND in dev_heath_list_rep so skip it");
	 		continue;
		}
		if (i->first.deviceIP == j->first)
		{
			//found a match...
		}
		else 
		{
			if (j->first < i->first.deviceIP)
			{
				bool finished = false;
				do
				{
					//clear
					j->second->DeviceHealthList[0].topoStructIndex = -1;
	
					++j;
					if (j == Iend)
					{
						finished = true;
						break;
					}
				}while (j->first < i->first.deviceIP);
				if (finished == true)
				{
					LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in dev_list_rep so skip it");	
					continue; // continue 'for' to delete unmatched devices
				}
				if (i->first.deviceIP < j->first)
				{
					LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 					" NOT FOUND in dev_list_rep so skip it");	
					continue;
				}
				
				//found a match...
			}
			else 
			{
				LOG_WARN("topo_struct_fill - dev ip = " <<  i->first.deviceIP.ToString() <<
		 				" NOT FOUND in dev_list_rep so skip it");	
				continue;
			}
		}

		//
		j->second->DeviceHealthList[0].topoStructIndex = i->second;

		//save data
		TopoRep.DevicesList[i->second].DPDUsTransmitted = j->second->DeviceHealthList[0].DPDUsTransmitted;		// GS_DPDUs_Transmitted
		TopoRep.DevicesList[i->second].DPDUsReceived = j->second->DeviceHealthList[0].DPDUsReceived;			// GS_DPDUs_Received
		TopoRep.DevicesList[i->second].DPDUsFailedTransmission = j->second->DeviceHealthList[0].DPDUsFailedTransmission;	// GS_DPDUs_Failed_Transmission
		TopoRep.DevicesList[i->second].DPDUsFailedReception = j->second->DeviceHealthList[0].DPDUsFailedReception;		// GS_DPDUs_Failed_Reception
		
		//increment here
		++j;
	}
	LOG_DEBUG("topo_struct_fill--> end --------");

	while (j != Iend)
	{
		j->second->DeviceHealthList[0].topoStructIndex = -1;
		++j;
	}

}


/*NETWORK_HEALTH_REPORT*/

void ReportsProcessor::FillNetworkHealthRepWithDevID(GNetworkHealthReport &networkRep/*[/in/out]*/, const GTopologyReport &TopoRep)
{
	LOG_INFO("network_struct_fill -> found network_rep with size = " << (boost::uint32_t)networkRep.DevicesMap.size() <<
					" and topo_rep with size = " << (boost::uint32_t)TopoRep.DevicesList.size());

	LOG_DEBUG("network_struct_fill--> begin --------");
	
	TopoStructIndexT::const_iterator j = m_TopoStructIndex.begin();

	for(GNetworkHealthReport::DevicesMapT::iterator i = networkRep.DevicesMap.begin(); i != networkRep.DevicesMap.end(); ++i)
	{	
		if (j == m_TopoStructIndex.end())
		{
			LOG_WARN("network_struct_fill - dev ip = " <<  i->first.ToString() <<
	 				" NOT FOUND in topo_rep so its devDB_ID remains with default value");
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
					if (j == m_TopoStructIndex.end())
					{
						finished = true;
						break;
					}
				}while (j->first.deviceIP < i->first);
				if (finished == true)
				{
					LOG_WARN("network_struct_fill - dev ip = " <<  i->first.ToString() <<
		 					" NOT FOUND in topo_rep so its devDB_ID remains with default value");
					continue;
				}
				if (i->first < j->first.deviceIP)
				{
					LOG_WARN("network_struct_fill - dev ip = " <<  i->first.ToString() <<
		 					" NOT FOUND in topo_rep so its devDB_ID remains with default value");
					continue;
				}
				
				//found a match...
			}
			else 
			{
				LOG_WARN("network_struct_fill - dev ip = " <<  i->first.ToString() <<
		 				" NOT FOUND in topo_rep so its devDB_ID remains with default value");
				continue;
			}
		}

		i->second.dbDevID = TopoRep.DevicesList[j->second].device_dbID;
		LOG_DEBUG("network_struct_fill - dev ip = " << i->first.ToString() <<
				" index = " << (int)j->second <<
				" has devDB_ID = " <<  (int)TopoRep.DevicesList[j->second].device_dbID);

		//increment here
		++j;
	}
	LOG_DEBUG("network_struct_fill--> end --------");

}

}
}
