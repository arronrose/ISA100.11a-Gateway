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

//added by Cristian.Guef

#ifndef GNETWORKHEALTHREPORT_H_
#define GNETWORKHEALTHREPORT_H_


#include "AbstractGService.h"
#include "../model/IPv6.h"
#include "../model/MAC.h"

#include <map>

namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

/**
 * @brief Represents a DeviceListReport Service. Holds requests & responses data.
 */
class GNetworkHealthReport : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

	struct TAI
	{
		boost::uint32_t Seconds;
		boost::uint16_t FractionOfSeconds;
	};
	struct NetworkHealth
	{
		boost::uint32_t	networkID;			// GS_Network_ID
		boost::uint8_t	networkType;		// GS_Network_Type
		boost::uint16_t	deviceCount;		// GS_Device_Count
		nlib::DateTime 	startDate;			
		nlib::DateTime	currentDate;		// GS_Current_Date
		boost::uint32_t	DPDUsSent;			// GS_DPDUs_Sent
		boost::uint32_t	DPDUsLost;			// GS_DPDUs_Lost
		boost::uint8_t	GPDULatency;		// GS_GPDU_Latency
		boost::uint8_t	GPDUPathReliability;	// GS_GPDU_Path_Reliability
		boost::uint8_t	GPDUDataReliability;	// GS_GPDU_Data_Reliability
		boost::uint32_t	joinCount;			// GS_Join_Count
	}m_NetworkHealth;

	struct NetDeviceHealth{

		//added by Cristian.Guef
		int	dbDevID;

		nlib::DateTime	startDate;
		nlib::DateTime currentDate;
		boost::uint32_t	DPDUsSent;
		boost::uint32_t DPDUsLost;
		boost::uint8_t GPDULatency;
		boost::uint8_t GPDUPathReliability;
		boost::uint8_t GPDUDataReliability;
		boost::uint32_t joinCount;

		NetDeviceHealth()
		{
			dbDevID = 0; 
		}
	};

public:

	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	typedef std::map<IPv6, NetDeviceHealth> DevicesMapT;
	DevicesMapT DevicesMap;

};


} //namespace hostapp
} //namsepace nisa100
#endif
