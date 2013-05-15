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

#include "NodesRepository.h"

namespace nisa100 {
namespace hostapp {

void NodesRepository::Clear()
{
	nodesByIPv6.clear();
	nodesById.clear();
	nodesByMAC.clear();

	LOG_INFO("[Cache]: all cache has been cleared.");
}

DevicePtr NodesRepository::Find(const MAC& mac) const
{
	NodesByMACT::const_iterator found = nodesByMAC.find(mac);
	if (found != nodesByMAC.end())
	{
		return found->second;
	}
	return DevicePtr();
}

DevicePtr NodesRepository::Find(const IPv6& ip) const
{
	NodesByIPv6T::const_iterator found = nodesByIPv6.find(ip);
	if (found != nodesByIPv6.end())
	{
		return found->second;
	}
	return DevicePtr();
}

DevicePtr NodesRepository::Find(const boost::int32_t deviceID) const
{
	NodesByIDT::const_iterator found = nodesById.find(deviceID);
	if (found != nodesById.end())
	{
		return found->second;
	}
	return DevicePtr();
}

/*commented by Cristian.Guef
void NodesRepository::Add(const DevicePtr& node)
{
	if (!nodesByMAC.insert(NodesByMACT::value_type(node->Mac(), node)).second)
	{
		LOG_WARN("Same MAC:" << node->Mac().ToString() << "was detected (will be ignored)!");
	}
}
*/
//added by Cristian.Guef
int NodesRepository::Add(const DevicePtr& node)
{
	if (!nodesByMAC.insert(NodesByMACT::value_type(node->Mac(), node)).second)
	{
		LOG_WARN("Same MAC:" << node->Mac().ToString() << "was detected (will be ignored)!");
		return 0;
	}
	return 1;
}


void NodesRepository::UpdateIPv6Index()
{
	nodesByIPv6.clear();

	for (NodesByMACT::const_iterator it = nodesByMAC.begin(); it!= nodesByMAC.end(); it++)
	{
		if (it->second->Status() == Device::dsRegistered)
		{
			if (!nodesByIPv6.insert(NodesByIPv6T::value_type(it->second->ip, it->second)).second)
			{
				LOG_WARN("Same IPv6:" << it->second->ip.ToString() << "was detected (will be ignored)!");
			}
		}
	}

	LOG_INFO("[Cache]: device IPs were updated");
}

void NodesRepository::UpdateIdIndex()
{
	nodesById.clear();
	for (NodesByMACT::const_iterator it = nodesByMAC.begin(); it != nodesByMAC.end(); it++)
	{
		if (!nodesById.insert(NodesByIDT::value_type(it->second->id, it->second)).second)
		{
			LOG_WARN("Same DeviceID:" << it->second->id << "was detected (will be ignored)!");
		}
	}

	LOG_INFO("[Cache]: device IDs were updated");
}

NodesRepositoryPtr NodesRepository::Clone() const
{
	return NodesRepositoryPtr(new NodesRepository(*this));
}

} //namspace monitor
} //namespace isa100
