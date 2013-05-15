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

#ifndef GTOPOLOGYREPORT_H_
#define GTOPOLOGYREPORT_H_

#include "AbstractGService.h"
#include "../model/IPv6.h"
#include "../model/MAC.h"

#include <vector>
#include <set>

namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

/**
 * @brief Represents a GTopology Service. Holds requests & responses data.
 */
class GTopologyReport : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

	struct Device
	{
		struct Neighbour
		{
			IPv6 DeviceIP;
			boost::uint8_t SignalQuality;
			
			//added by Cristian.Guef
			boost::uint8_t ClockSource;

			//added by Cristian.Guef
			int	DevListIndex;  //bound the neighbour to the coresponding device in 'DevList',
										//information used in creation of devices graph, or in getting dev_id
										//without using a searching algorithm or ...
			//added by Cristian.Guef
			Neighbour()				
			{
				DevListIndex = -1;	//default value representing no value, in fact
				SignalQuality = 0;	//default value representing no value, in fact
				ClockSource = 10;	//default value representing no value, in fact
			}

			//for std::set
			bool operator < (const Neighbour &obj)const
			{
				if(DeviceIP < obj.DeviceIP)
					return true;
				return false;
			}
			bool operator == (const Neighbour &obj)
			{
				return DeviceIP == obj.DeviceIP;
			}
		};

		/* commented by Cristian.Guef
		struct Graph
		{
			IPv6 DeviceIP;
			boost::uint16_t GraphID;
		};
		*/
		//added by Cristian.Guef
		struct Graph
		{
			struct Info
			{
				int		DevListIndex;		//used when traversing routes elements
				IPv6	neighbour;
				Info(const IPv6 & neighbour_): neighbour(neighbour_)
				{
					DevListIndex = -1;		//default value representing no value, in fact
				}
			};
			boost::uint16_t GraphID;
			std::vector<Info> infoList;
		};

		//typedef std::vector<Neighbour> NeighborsListT;
		typedef std::set<Neighbour> NeighborsListT;
		typedef std::vector<Graph> GraphsListT;

		IPv6 DeviceIP;
		MAC DeviceMAC;
		boost::uint8_t DeviceType;
		boost::uint8_t DeviceCapabilities;
		boost::uint8_t DeviceImplementationType;

		boost::uint8_t DeviceStatus;
		boost::uint16_t DeviceRejoinCount;
		
		//added by Cristian.Guef
		boost::uint8_t PowerSupplyStatus;

		//added by Cristian.Guef
		bool	isMarkedDeleted;	//we only mark a node as deleted to preserve the index (we created for topo_struct)
									//for further intersection of topology_report with other reports
		int		device_dbID;			//information saved here to skip searching for dev_id when saving schedule report 
									//in db after an intersection with topology_report

		NeighborsListT NeighborsList;
		GraphsListT GraphsList;

		//added by Cristian.Guef
		boost::uint32_t		DPDUsTransmitted;		// GS_DPDUs_Transmitted
		boost::uint32_t		DPDUsReceived;			// GS_DPDUs_Received
		boost::uint32_t		DPDUsFailedTransmission;	// GS_DPDUs_Failed_Transmission
		boost::uint32_t		DPDUsFailedReception;		// GS_DPDUs_Failed_Reception

		//added by Cristian.Guef
		std::string  deviceTAG;
		std::string  deviceModel;
		std::string  deviceRevision;
		std::string  deviceManufacturer;
		std::string	 deviceSerialNo;

		Device()	
		{
			DeviceRejoinCount = 0;  //default value representing no value, in fact
			isMarkedDeleted = true; 
			DPDUsTransmitted = 0;
			DPDUsReceived = 0;
			DPDUsFailedTransmission = 0;
			DPDUsFailedReception = 0;
			PowerSupplyStatus = 20;
			device_dbID = 0;
		}
	};

	typedef std::vector<Device> DevicesListT;

	//added by Cristian.Guef
	struct BBR
	{
		IPv6			address;
		boost::uint16_t	subnetID;
	};
	typedef std::vector<BBR> BBRsListT;

public:
	GTopologyReport()
	{
		GWIndex = -1;
	}

	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	DevicesListT DevicesList;

	//added by Cristian.Guef
	BBRsListT	m_BBRsList;

	//added by cristian.Guef
	unsigned int m_DiagID;
	
	//added by Cristian.Guef
	int GWIndex;
};

} //namespace hostapp
} //namsepace nisa100

#endif /*GTOPOLOGYREPORT_H_*/
