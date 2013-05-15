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

#ifndef GDEVICEHEALTHREPORT_H_
#define GDEVICEHEALTHREPORT_H_

#include "AbstractGService.h"
#include "../model/IPv6.h"
#include "../model/MAC.h"

#include <vector>


namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

/**
 * @brief Represents a GTopology Service. Holds requests & responses data.
 */
class GDeviceHealthReport : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

	std::vector<IPv6>		m_forDeviceIPs;
	std::vector<int>		m_devIDs; //it comes with db request

	struct DeviceHealth
	{
		int				   topoStructIndex; //it is set during intersection of topology_rep with device_health_rep_map
		
		IPv6				networkAddress;			// GS_Network_Address
		boost::uint32_t		DPDUsTransmitted;		// GS_DPDUs_Transmitted
		boost::uint32_t		DPDUsReceived;			// GS_DPDUs_Received
		boost::uint32_t		DPDUsFailedTransmission;	// GS_DPDUs_Failed_Transmission
		boost::uint32_t		DPDUsFailedReception;		// GS_DPDUs_Failed_Reception

		DeviceHealth()
		{
			topoStructIndex = -1; //invalid value
								 // >=0 valid value
		}
	};

	typedef std::vector<DeviceHealth> DeviceHealthListT;

public:
	GDeviceHealthReport(std::vector<int> &devIDs):m_devIDs(devIDs)
	{
	}

public:
	nlib::DateTime timestamp; //it is set during unserialization process

public:
	
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	DeviceHealthListT	DeviceHealthList;

};

} //namespace hostapp
} //namsepace nisa100

#endif /*GTOPOLOGYREPORT_H_*/
