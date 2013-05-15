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


#ifndef REPORTSPROCESSOR_H_
#define REPORTSPROCESSOR_H_


#include "../commandmodel/GTopologyReport.h"
#include "../commandmodel/GDeviceListReport.h"
#include "../commandmodel/GNeighbourHealthReport.h"
#include "../commandmodel/GNetworkHealthReport.h"
#include "../commandmodel/GScheduleReport.h"
#include "../commandmodel/GDeviceHealthReport.h"


namespace nisa100 {
namespace hostapp {


class ReportsProcessor
{

/*TOPOLOGY_REPORT*/
public:
	struct TopologyIndexKey	//used for ordering, searching the elements in a structure
	{
		IPv6	deviceIP;
		
		bool operator < (const TopologyIndexKey &obj)const
		{
			if(deviceIP < obj.deviceIP)
				return true;
			return false;
		}
		bool operator == (const TopologyIndexKey &obj)
		{
			return deviceIP == obj.deviceIP;
		}
	};

	typedef std::map<TopologyIndexKey, int /*indexDevice*/> TopoStructIndexT;

private:
	TopoStructIndexT	m_TopoStructIndex;	//it ensures the intersection of topology_report 
											//  with other reports (such as network_health_report,
											//	device_list_report and so on) has linear complexity
																								
private:
	void ClearSignalQualityForNeighbours(GTopologyReport &TopoRep /*[in/out]*/, int topoDevIndex);
	void SetSignalQualityForNeighbours(int devListIndex,
										GTopologyReport::Device::NeighborsListT::iterator i,
										GTopologyReport::Device::NeighborsListT::iterator iEnd,
									const boost::shared_ptr<GNeighbourHealthReport> neighboursPtr);
	void ClearTopoStructIndexForNeighbours(boost::shared_ptr<GNeighbourHealthReport> neighboursPtr/*[in/out]*/);

public:
	TopoStructIndexT* GetTopoStructIndex()	//it is not correct from OOP perspective but it works for performance
	{
		return &m_TopoStructIndex;
	}


public:
	void CreateIndexForTopoStruct(const GTopologyReport &TopoRep);	// O(mlogm), m = no. of edges
	void FillDevListIndexForNeighbours(GTopologyReport &TopoRep /*[in/out]*/);  // O(m + n)), m = no. of edges, n = no. of nodes

	void FillTopoStructWithSignalQuality(GTopologyReport &topoReport /*[in/out]*/, 
						std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::iterator Ibegin,
						std::map<IPv6, boost::shared_ptr<GNeighbourHealthReport> >::iterator Iend,
						int mapSize/* for info only*/);	// O(m + x), m = no. of edges, x = extra edges not found in topology

	void FillTopoStructWithJoinCount(GTopologyReport &topoReport /*[in/out]*/, 
							const GNetworkHealthReport &NetHealthRep); // O(n), n = no. of nodes

	void FillTopoStructWithMACAndType(GTopologyReport &TopoRep/*[in/out]*/, 
							const GDeviceListReport &DevListRep); // O(n), n = no. of nodes

	void FillTopoStructWithDevHealth(GTopologyReport &topoReport /*[in/out]*/, 
						std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::iterator Ibegin,
						std::map<IPv6, boost::shared_ptr<GDeviceHealthReport> >::iterator Iend,
						int mapSize/* for info only*/);	// O(n + y), n = no. of nodes, y = extra nodes not found in topology


/*NETWORK_HEALTH_REPORT*/
public:
	void FillNetworkHealthRepWithDevID(GNetworkHealthReport &networkRep/*[/in/out]*/, 
						const GTopologyReport &TopoRep); // O(n), n = no. of nodes

public:
	struct UnregisteredDev
	{
		MAC				mac;
		IPv6			ip;
		boost::uint16_t DeviceType;
		boost::uint8_t	PowerSupplyStatus;
		std::string		DeviceManufacture;
		std::string		DeviceModel;
		std::string		DeviceRevision;
		std::string		DeviceTag;
		std::string		DeviceSerialNo;
		int				DeviceStatus;


		bool operator < (const UnregisteredDev &obj)const
		{
			if(mac < obj.mac)
				return true;
			return false;
		}
		bool operator == (const UnregisteredDev &obj)
		{
			return mac == obj.mac;
		}
	};
	typedef std::set<UnregisteredDev> UnregDevsListT;

private:
	UnregDevsListT		m_UnregDevsList;

public:
	UnregDevsListT* GetUnregisteredDevs()
	{
		return &m_UnregDevsList;
	}
};


}
}

#endif 
