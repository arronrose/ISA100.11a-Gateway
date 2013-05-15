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

#ifndef GSCHEDULEREPORT_H_
#define GSCHEDULEREPORT_H_

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
class GScheduleReport : public AbstractGService
{
public:
	enum ConfirmCommunicationStatus
	{
		Success = 0,
		Failure = 1
	};

	IPv6			m_forDeviceIP;
	int				m_devID;

	struct Channel
	{
		boost::uint8_t 	channelNumber;	// GS_Channel_Number
		boost::uint8_t 	channelStatus;	// GS_Channel_Status
	};
	typedef std::vector<Channel>		ChannelListT;

	struct Link
	{
		IPv6			networkAddress;	// GS_Network_Address
		int				devDB_ID;
		boost::uint16_t	slotIndex;		// GS_Slot_Index
		boost::uint16_t	linkPeriod;
		boost::uint16_t	slotLength;		// GS_Slot_Length
		boost::uint8_t 	channelNumber;	// GS_Channel
		boost::uint8_t 	direction;		// GS_Direction
		boost::uint8_t 	linkType;		// GS_Link_Type
	};
	typedef std::vector<Link>			LinkListT;

	struct Superframe
	{
		boost::uint16_t 	superframeID;		// GS_Superframe_ID
		boost::uint16_t		timeSlotsCount;		// GS_Num_Time_Slots
		nlib::DateTime		startTime;			
		LinkListT			linkList;			// GS_Link_List
	};
	typedef	std::vector<Superframe>		SuperframeListT;

	struct DeviceSchedule
	{
		IPv6				networkAddress;		// GS_Network_Address
		SuperframeListT		superframeList;		// GS_Superframe_List [];	
	};
	typedef std::vector<DeviceSchedule> DeviceScheduleListT;

public:
	GScheduleReport(int devID):m_devID(devID)
	{
	}

public:
	
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	ChannelListT		channelList;		// GS_Channel_List
	DeviceScheduleListT DeviceScheduleList;

};

} //namespace hostapp
} //namsepace nisa100

#endif /*GTOPOLOGYREPORT_H_*/
