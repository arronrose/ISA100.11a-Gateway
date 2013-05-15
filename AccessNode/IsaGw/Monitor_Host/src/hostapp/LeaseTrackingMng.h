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

#ifndef LEASETRACKINGMNG_H_
#define LEASETRACKINGMNG_H_


#include <map>
#include "../Log.h"
#include <nlib/datetime.h>
#include <boost/format.hpp>

#include "CommandsManager.h"
#include "DevicesManager.h"


namespace nisa100 {
namespace hostapp {


class LeaseTrackingMng
{
public:
	LeaseTrackingMng(CommandsManager &commands, DevicesManager  &devices);

private:
	struct TrackingItem
	{
		DevicePtr		dev;
		int				resourceID;
		int				leaseType;
		nlib::DateTime	timedOut;
	};
	typedef std::map<int/*leaseID*/, TrackingItem> TrackingsMapT;
	TrackingsMapT m_tracking;

public:
	void AddLease(int leaseID, DevicePtr dev, int resourceID, int leaseType, nlib::TimeSpan timeoutTime);
	void RemoveLease(int leaseID);
	
public:
	void CheckLeaseTimedOut();

private:
	CommandsManager &commands;
	DevicesManager  &devices;

};


}
}

#endif
