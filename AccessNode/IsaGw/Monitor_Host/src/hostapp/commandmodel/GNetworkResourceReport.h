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

#ifndef GNETWORKRESOURCEREPORT_H_
#define GNETWORKRESOURCEREPORT_H_



#include "AbstractGService.h"
#include "../model/IPv6.h"

#include <vector>


namespace nisa100 {
namespace hostapp {

class IGServiceVisitor;

/**
 * @brief Represents a GTopology Service. Holds requests & responses data.
 */
class GNetworkResourceReport : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

	struct NetReport
	{
		IPv6 			BBRAddress;			// GS_Network_Address
		boost::uint16_t	SubnetID;			// GS_Subnet_ID associated with BBR 
		boost::uint32_t	SlotLength;			// GS_Slot_Length - Length of
											// timeslot, reported in µSeconds.
		boost::uint16_t	SlotsOccupied;		// GS_Number_Slots_Occupied 
		boost::uint16_t	AperiodicData_X;		// GS_Slots_Linktype_0_X
		boost::uint16_t	AperiodicData_Y;		// GS_Slots_Linktype_0_Y
		boost::uint16_t	AperiodicMgmt_X;		// GS_Slots_Linktype_1_X
		boost::uint16_t	AperiodicMgmt_Y;		// GS_Slots_Linktype_1_Y
		boost::uint16_t	PeriodicData_X;		// GS_Slots_Linktype_2_X
		boost::uint16_t	PeriodicData_Y;		// GS_Slots_Linktype_2_Y
		boost::uint16_t	PeriodicMgmt_X;		// GS_Slots_Linktype_3_X
		boost::uint16_t	PeriodicMgmt_Y;		// GS_Slots_Linktype_3_Y
	};

	typedef std::vector<NetReport> NetReportListT;

public:
	
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	NetReportListT	NetReportList;

};

} //namespace hostapp
} //namsepace nisa100


#endif
