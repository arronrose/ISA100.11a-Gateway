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

#ifndef GLEASE_H_
#define GLEASE_H_

#include "AbstractGService.h"

namespace tunnel {
namespace comm {

class IGServiceVisitor;

class GLease : public AbstractGService
{
public:

	enum GSLeaseType
	{
		Client = 0,
		Server = 1,
		Publisher = 2,
		Subscriber = 3,
		BulkTransforClient = 4,
		BulkTransforServer = 5,
		Alert_Subscription = 6
	};

public:
	virtual bool Accept(IGServiceVisitor& visitor);
	const std::string ToString() const;

public:
	boost::int32_t m_tunnelNo;

public:

	boost::uint32_t m_leaseID;			// req/resp
	boost::uint32_t m_leasePeriod;		// req/resp	
	GSLeaseType		m_leaseType;		// req
	
	boost::uint8_t	m_protocolType;		// req
	boost::uint8_t	m_endPointsNo;		// req
	

	struct Network_Address_List_Item
	{
		boost::uint8_t	ipAddress[16];
		boost::uint16_t	remotePort;
		boost::uint16_t	remoteObjID;
		boost::uint16_t localPort;
		boost::uint16_t	localObjID;
	} m_endPoint;						// req
	
	struct Lease_Parameters
	{
		boost::uint8_t	transferMode;
		boost::uint8_t	updatePolicy;
		boost::uint16_t	subscriptionPeriod;
		boost::uint8_t	phase;
		boost::uint8_t	staleLimit;
	}m_parameters;						// req

	std::basic_string<boost::uint8_t>	m_connInfo; // req
};

} //namespace comm
} //namespace tunnel

#endif
