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

#ifndef NODESLIST_H_
#define NODESLIST_H_

#include "model/Device.h"

#include <map>

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../Log.h"

namespace nisa100 {
namespace hostapp {

class NodesRepository;
typedef boost::shared_ptr<NodesRepository> NodesRepositoryPtr;

class NodesRepository
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.runtime.NodesRepository");
	*/

public:
	typedef std::map<MAC, DevicePtr> NodesByMACT;
	typedef std::map<IPv6, DevicePtr> NodesByIPv6T;
	typedef std::map<boost::int32_t, DevicePtr> NodesByIDT;

public:
	DevicePtr Find(const MAC& macAddress) const;
	DevicePtr Find(const IPv6& ipv6Address) const;
	DevicePtr Find(const boost::int32_t deviceID) const;

	void Clear();

	/*comented by Cristian.Guef
	void Add(const DevicePtr& node);
	*/
	//added by Cristian.Guef
	int Add(const DevicePtr& node);	//0 - fail
									//1 -ok

	void UpdateIPv6Index();
	void UpdateIdIndex();

	NodesRepositoryPtr Clone() const;

public:
	NodesByMACT nodesByMAC;
	NodesByIDT nodesById;
	NodesByIPv6T nodesByIPv6;
};

} //namespace hostapp
} //namespace nisa100

#endif /*NODESLIST_H_*/
