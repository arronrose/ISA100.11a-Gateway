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

#ifndef GNEIGHBOURHEALTHREPORT_H_
#define GNEIGHBOURHEALTHREPORT_H_

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
class GNeighbourHealthReport : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

	IPv6			m_forDeviceIP;
	int				m_devID;	//it comes with db request

	int				m_topoStructIndex; //it is set during intersection of topology_rep with neighbours_rep_map

	struct NeighbourHealth
	{
		int			topoStructIndex; //it is set during intersection of topology_rep with neighbours_rep_map

		//IPv6				networkAddress;		// GS_Network_Address -use map
		boost::uint8_t		linkStatus;			// GS_Link_Status
		boost::uint32_t		DPDUsTransmitted;		// GS_DPDUs_Transmitted
		boost::uint32_t		DPDUsReceived;			// GS_DPDUs_Received
		boost::uint32_t		DPDUsFailedTransmission;	// GS_DPDUs_Failed_Transmission
		boost::uint32_t		DPDUsFailedReception;		// GS_DPDUs_Failed_Reception
		boost::int16_t		signalStrength;			// GS_Signal_Strength
		boost::uint8_t		signalQuality;			// GS_Signal_Quality

		NeighbourHealth()
		{
			topoStructIndex = -1; //invalid value
								 // >=0 valid value
		}
	};
	
public:
	GNeighbourHealthReport(int devID):m_devID(devID)
	{
		m_topoStructIndex = -1;	//invalid value
								// >=0 valid value
	}
	GNeighbourHealthReport() 
	{
		m_topoStructIndex = -1; //invalid value
								// >=0 valid value
	}

public:
	nlib::DateTime timestamp; //it is set during unserialization process

public:
	
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	typedef std::map<IPv6, NeighbourHealth> NeighboursMapT;
	NeighboursMapT	NeighboursMap;
};

} //namespace hostapp
} //namsepace nisa100

#endif /*GTOPOLOGYREPORT_H_*/
