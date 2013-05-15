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

#ifndef REQUEST_H_
#define REQUEST_H_

#include <string>
#include <boost/cstdint.hpp> //used for inttypes
#include <boost/format.hpp>
#include "../log/Log.h"

namespace tunnel {
namespace comm {


struct TunnelInfo
{
	boost::uint8_t		foreignDestAddress[16];
	boost::uint8_t		foreignSrcAddress[16];
	std::basic_string<boost::uint8_t>	connInfo;
};


struct Request
{
	unsigned char	type;
	int				sessionID;

	enum RequestType
	{
		rtSession = 0,
		rtLease = 1,
		rtClientServer_C = 2
	};

	union Parameters
	{
		struct LeaseParam
		{
			int				tunnNo;
			boost::uint8_t	leaseType;
			boost::uint8_t	protocolType;
			boost::uint8_t	ipAddress[16];
			boost::uint16_t remotePort;
			boost::uint16_t remoteObjID;
			boost::uint16_t localPort;
			boost::uint16_t localObjID;
			std::basic_string<boost::uint8_t>	*pConnInfo;
		}Lease;
		struct ClntServerParam
		{
			int				tunnNo;
			boost::uint32_t	leaseID;
			std::basic_string<boost::uint8_t>	*pTransacInfo;
			std::basic_string<boost::uint8_t>	*pReqData; 
		}ClntServer_C;
	}Param;

	const std::string ToString() const
	{
		return boost::str(boost::format("[RequestType=%1%, SessionID=%2%]") % (int)type % sessionID);
	}
};



enum ResponseStatus
{
	rsNoStatus = 0,
	rsSuccess = 1,
	rsSuccess_SessionLowerPeriod = 2,
	rsSuccess_LeaseLowerPeriod = 3,

	rsFailure_InvalidDevice = -1,

	rsFailure_ThereisNoLease = -5,
	rsFailure_InvalidLeaseType = -6,
	rsFailure_InvalidLeaseTypeInform = -7,
	rsFailure_InvalidReturnedLeaseID = -8,
	rsFailure_InvalidReturnedLeasePeriod = -9,

	rsFailure_GatewayLeaseExpired = -12,
	rsFailure_GatewayNoContractsAvailable = -13,
	
	rsFailure_GatewayInvalidFailureCode = -18,
	rsFailure_GatewayUnknown = -19,
	rsFailure_HostTimeout = -20,
	rsFailure_LostConnection = -21,
	rsFailure_SerializationError = -22,
			
	rsFailure_InvalidReturnedSessionID = -28,
	
	rsFailure_ThereIsNoSession = -30,
	rsFailure_SessionNotCreated = -31,

	rsFailure_NoUnBufferedReq = -33,
	rsFailure_InvalidBufferedReq = -34,

	rsFailure_InvalidHeaderCRC = -43,
	rsFailure_InvalidDataCRC = -44,

};


std::string FormatResponseCode(int responseCode);

} // namespace comm
} // namespace tunnel

#endif
