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


#include "LeaseTrackingMng.h"

#include <boost/lexical_cast.hpp>

namespace nisa100 {
namespace hostapp {



LeaseTrackingMng::LeaseTrackingMng(CommandsManager &commands_, DevicesManager  &devices_):commands(commands_), devices(devices_)
{
	
}

void LeaseTrackingMng::AddLease(int leaseID, DevicePtr dev, int resourceID, int leaseType, nlib::TimeSpan timeoutTime)
{
	
	TrackingItem itm;
	itm.dev = dev;
	itm.resourceID = resourceID;
	itm.leaseType = leaseType;
	itm.timedOut = nlib::CurrentUniversalTime()+ timeoutTime;
	m_tracking[leaseID] = itm;
	
	LOG_DEBUG("LeaseTracking: leaseID=" << leaseID << " with type=" << leaseType 
					<< " for objID=" << (resourceID >> 16) 
					<< " tsapID=" << (resourceID & 0xFFFF)
	 				<< " on deviceID=" << dev->id << "added to the list" );
}

void LeaseTrackingMng::RemoveLease(int leaseID)
{
	TrackingsMapT::iterator i = m_tracking.find(leaseID);
	if (i != m_tracking.end())
	{
		LOG_DEBUG("LeaseTracking: leaseID=" << leaseID << "removed from the list");
		m_tracking.erase(i);
		return;
	}

	LOG_WARN("LeaseTracking: no leaseID =" << leaseID << " found in pending list");
}


void LeaseTrackingMng::CheckLeaseTimedOut()
{
	for (TrackingsMapT::iterator it = m_tracking.begin(); it != m_tracking.end();)
	{
		if (nlib::CurrentUniversalTime() > it->second.timedOut)
		{
			LOG_DEBUG("LeaseTracking:  leaseID" << it->first << " Timeouted.");
			
			Command DoDeleteLease;
			DoDeleteLease.commandCode = Command::ccDelContract;
			DoDeleteLease.deviceID = devices.GatewayDevice()->id;
			DoDeleteLease.generatedType = Command::cgtAutomatic;
			DoDeleteLease.ContractType = it->second.leaseType;
			DoDeleteLease.IPAddress = it->second.dev->IP();
			DoDeleteLease.ResourceID = it->second.resourceID;

			//parameters
			int contractID = it->first;
			DoDeleteLease.parameters.push_back(CommandParameter(CommandParameter::DelContract_ContractID,
				boost::lexical_cast<std::string>(contractID)));

			
			it->second.dev->ResetContractID(it->second.resourceID, it->second.leaseType);

			commands.CreateCommand(DoDeleteLease, boost::str(boost::format("system: do delete lease_id = %1% with type = %2% for ObjID = %3% and TLSAPID = %4% for device with ip = %5% and dev_id = %6%")
					% (boost::uint32_t) contractID
					% (boost::uint8_t) it->second.leaseType
					% (boost::uint16_t) (it->second.resourceID >> 16)
					% (boost::uint16_t) (it->second.resourceID & 0xFFFF)
					% it->second.dev->IP().ToString()
					% it->second.dev->id));
			
			TrackingsMapT::iterator currIt = it++;
			m_tracking.erase(currIt);
			continue;
		}
		++it;
	}
}


}
}
