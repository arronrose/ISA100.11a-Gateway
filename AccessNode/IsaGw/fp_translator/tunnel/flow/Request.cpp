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


#include "Request.h"


namespace tunnel {
namespace comm {


static void GetErrorCodeDescription(int responseCode, std::string& description)
{
	struct Map
	{
		int code;
		const char* description;
	};
	static Map
	    mapCodes[] = {
			{ rsSuccess, "success" },

			{ rsSuccess_SessionLowerPeriod, "session created or renewed with reduced period"},
			{ rsSuccess_LeaseLowerPeriod, "lease created or renewed with reduced period"},

			{ rsFailure_InvalidDevice, "unknown device" },

	       	{ rsFailure_ThereisNoLease, "lease does not exist to renew or delete" },
	        { rsFailure_InvalidLeaseType, "invalid lease type" },
	        { rsFailure_InvalidLeaseTypeInform, "invalid lease type information" },
			{ rsFailure_InvalidReturnedLeaseID, "invalid returned lease ID"},
			{ rsFailure_InvalidReturnedLeasePeriod, "invalid lease returned period"},
	        
			{ rsFailure_GatewayLeaseExpired, "lease expired" },
			
			{ rsFailure_GatewayNoContractsAvailable, "too many lease requests for same device" },
	        
			{ rsFailure_GatewayInvalidFailureCode, "invalid gateway failure code" },

			{ rsFailure_GatewayUnknown, "unknown gateway error" },
	        { rsFailure_HostTimeout, "timeout from host application" },
	        { rsFailure_LostConnection, "lost/no gateway connection" },
	        { rsFailure_SerializationError, "serialization error" },
	        
			{ rsFailure_InvalidReturnedSessionID, "invalid session returned ID"},
			{ rsFailure_ThereIsNoSession, "session does not exit to renew or delete" },
	        { rsFailure_SessionNotCreated, "session connot be created, no sessions available" },

			{ rsFailure_NoUnBufferedReq, "server inaccessible for unbuffered request" },
	        { rsFailure_InvalidBufferedReq, "server inaccessible and client buffer invalid for buffered request" },

			{ rsFailure_InvalidHeaderCRC, "invalid header CRC" },
			{ rsFailure_InvalidDataCRC, "invalid data CRC" },

	      };

	//look-up for description into tables
	for (unsigned int i = 0; i < sizeof(mapCodes) / sizeof(mapCodes[0]); i++)
	{
		if (mapCodes[i].code == responseCode)
		{
			description = mapCodes[i].description;
			return;
		}
	}
	
	description = "unknown application error";
	return; //not found description
}

std::string FormatResponseCode(int responseCode)
{
	std::string errorDescription;
	GetErrorCodeDescription(responseCode, errorDescription);
	
	return boost::str(boost::format("App(%1%)-%2%") % (int)responseCode % errorDescription);
}

} //namespace comm
} //namespace tunnel
