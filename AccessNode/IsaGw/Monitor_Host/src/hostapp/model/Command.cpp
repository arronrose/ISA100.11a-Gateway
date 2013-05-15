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

#include "Command.h"

namespace nisa100 {
namespace hostapp {

const std::string Command::ToString() const
{
	return boost::str(boost::format("[CommandID=%1%, CommandCode=%2%, CommandStatus:%3%, DeviceID=%4%]") % commandID
	    % commandCode % commandStatus % deviceID);
}

bool GetErrorCodeDescription(int responseCode, std::string& description)
{
	struct Map
	{
		int code;
		const char* description;
	};
	static Map
	    mapCodes[] = {
	        { Command::rsSuccess, "success" },

			//added by Cristian.Guef
			{ Command::rsSuccess_SessionLowerPeriod, "session created or renewed with reduced period"},
			{ Command::rsSuccess_ContractLowerPeriod, "lease created or renewed with reduced period"},

	        { Command::rsFailure_InvalidDevice, "unknown device" },
	        { Command::rsFailure_InvalidCommand, "invalid command" },
	        { Command::rsFailure_DeviceHasNoContract, "device has no lease (a new lease request issued...)" },
	        { Command::rsFailure_DeviceNotRegistered, "device is not registered" },

			//added by Cristian.Guef
			{ Command::rsFailure_ThereisNoLease, "lease does not exist to renew or delete" },
	        { Command::rsFailure_InvalidLeaseType, "invalid lease type" },
	        { Command::rsFailure_InvalidLeaseTypeInform, "invalid lease type information" },
			{ Command::rsFailure_InvalidReturnedLeaseID, "invalid returned lease ID"},
			{ Command::rsFailure_InvalidReturnedLeasePeriod, "invalid lease returned period"},
	        
	        { Command::rsFailure_GatewayTimeout, "timeout from gateway" },
	        { Command::rsFailure_GatewayCommunication, "gateway reported a communication error" },
	        
			/*commented by Cristian.Guef
			{ Command::rsFailure_GatewayInvalidContract, "contract expired or invalid" },
			*/
	        //added by Cristian.Guef
			{ Command::rsFailure_GatewayContractExpired, "lease expired" },
			
			{ Command::rsFailure_GatewayNoContractsAvailable, "too many lease requests for same device" },
	        
	        //added by Cristian.Guef
			{ Command::rsFailure_ReadValueUnexpectedSize, "invalid received size for read_value" },
	        
			//added by Cristian.Guef
			{ Command::rsFailure_GatewayInvalidFailureCode, "invalid gateway failure code" },

			{ Command::rsFailure_GatewayUnknown, "gateway failure code; other" },
	        { Command::rsFailure_HostTimeout, "timeout from host application" },
	        { Command::rsFailure_LostConnection, "lost/no gateway connection" },
	        { Command::rsFailure_SerializationError, "serialization error" },
	        { Command::rsFailure_CommandNotSent, "command not sent anymore" },

			//added by Cristian.Guef
			{ Command::rsFailure_GatewayNotJoined, "gateway not joined to sm"},

			//added by Cristian.Guef
			{ Command::rsFailure_InvalidPublisherDataReceived, "invalid publisher data received"},

			//added by Cristian.Guef
			{ Command::rsFailure_InvalidReturnedSessionID, "invalid session returned ID"},
			{ Command::rsFailure_InvalidReturnedSessionPeriod, "invalid session returned period"},
			{ Command::rsFailure_ThereIsNoSession, "session does not exit to renew or delete" },
	        { Command::rsFailure_SessionNotCreated, "session connot be created, no sessions available" },

			//added by Cristian.Guef
			{ Command::rsFailure_NoUnBufferedReq, "server inaccessible for unbuffered request" },
	        { Command::rsFailure_InvalidBufferedReq, "server inaccessible and client buffer invalid for buffered request" },

			//added by Cristian.Guef
			{ Command::rsFailure_ItemExceedsLimits, "bulkOpen -> Item exceeds limits" },
			{ Command::rsFailure_UnknownResource, "bulkOpen -> Lease expired.It will be recreated..." },
			{ Command::rsFailure_InvalidMode, "bulkOpen -> Invalid mode" },
			{ Command::rsFailure_InvalidBlockSize, "bulkOpen -> Invalid block size" },
			{ Command::rsFailure_CommunicationFailed, "bulkTransfer -> Communication failed" },
			{ Command::rsFailure_TransferAborted, "bulkTransfer -> Transfer aborted" },
			{ Command::rsFailure_InvalidBlockNo, "bulkTransfer -> Invalid block no. received" },
			
			//added by Cristian.Guef
			{ Command::rsFailure_InvalidHeaderCRC, "invalid header CRC" },
			{ Command::rsFailure_InvalidDataCRC, "invalid data CRC" },

			//added by Cristian.Guef
			{ Command::rsFailure_InvalidCategory, "invalid alert category" },
			{ Command::rsFailure_InvalidIndividualAlert, "invalid individual alert" },

			{ Command::rsFailure_NoSensorUpdateInProgress, "no sensor firmware update in progress" },
			{ Command::rsFailure_AlreadySensorUpdateInProgress, "already sensor firmware update in progress" },

			{ Command::rsFailure_ContractFailed, "failed to obtain lease" },

	        { Command::rsFailure_InternalError, "internal application error (check logs for details...)" },

	        //device errors
	        { -1 + Command::rsFailure_DeviceError, "generic failure" },
	        { -2 + Command::rsFailure_DeviceError, "reason other than that listed in this enumeration" },

	        { -3 + Command::rsFailure_DeviceError, "invalid attribute to a service call" },
	        { -4 + Command::rsFailure_DeviceError, "invalid object ID" },
	        { -5 + Command::rsFailure_DeviceError, "unsupported or illegal service" },
	        { -6 + Command::rsFailure_DeviceError, "invalid attribute index" },
	        { -7 + Command::rsFailure_DeviceError, "invalid array or structure element index (or indices)" },
	        { -8 + Command::rsFailure_DeviceError, "read-only attribute" },
	        { -9 + Command::rsFailure_DeviceError, "value is out of permitted range" },
	        { -10 + Command::rsFailure_DeviceError, "process is in an inappropriate mode for the request" },
	        { -11 + Command::rsFailure_DeviceError, "value is not acceptable in current context" },
	        { -12 + Command::rsFailure_DeviceError, "value (data) not acceptable for other reason (e.g., too large, too small, invalid engineering units code)" },
	        { -13 + Command::rsFailure_DeviceError, "device internal problem" },
	        { -14 + Command::rsFailure_DeviceError, "size is not valid (may be too big or too small)" },
	        { -15 + Command::rsFailure_DeviceError, "attribute not supported in this version" },
	        { -16 + Command::rsFailure_DeviceError, "invalid method identifier" },
	        { -17 + Command::rsFailure_DeviceError, "state of object in conflict with action requested" },
	        { -18 + Command::rsFailure_DeviceError, "the content of the service requested is inconsistent" },
	        { -19 + Command::rsFailure_DeviceError, "value conveyed is not legal for method invocation" },
	        { -20 + Command::rsFailure_DeviceError, "object is not permitting access" },
	        { -21 + Command::rsFailure_DeviceError, "data not what was expected (too many or too few octets)" },
	        { -22 + Command::rsFailure_DeviceError, "device specific hardware condition prevented request from succeeding" },
	        { -23 + Command::rsFailure_DeviceError, "problem with sensor detected" },
	        { -24 + Command::rsFailure_DeviceError, "device specific software condition prevented request from succeeding" },
	        { -25 + Command::rsFailure_DeviceError, "field specific condition prevented request from succeeding" },
	        { -26 + Command::rsFailure_DeviceError, "server / sink believes its configuration conflicts with configuration	of correspondent client/source" },
	        { -27 + Command::rsFailure_DeviceError, "insufficient device resources (queue full, buffers / memory unavailable)" },
	        { -28 + Command::rsFailure_DeviceError, "value limited by device" },
	        { -29 + Command::rsFailure_DeviceError, "data warning (e.g. value has been modified due to a device specific reason" },
	        { -30 + Command::rsFailure_DeviceError, "function referenced for execution is invalid" },
	        { -31 + Command::rsFailure_DeviceError, "function referenced could not be performed due to a device specific reason" },
	        { -32 + Command::rsFailure_DeviceError, "warning" },
	        { -33 + Command::rsFailure_DeviceError, "write-only attribute" },
	        { -34 + Command::rsFailure_DeviceError, "operation accepted" },
	        { -35 + Command::rsFailure_DeviceError, "invalid block size (upload or download block size not valid)" },
	        { -36 + Command::rsFailure_DeviceError, "invalid download size" },
	        { -37 + Command::rsFailure_DeviceError, "unexpected method sequence" },
	        { -38 + Command::rsFailure_DeviceError, "timing violation" },
	        { -39 + Command::rsFailure_DeviceError, "operation incomplete" },
	        { -40 + Command::rsFailure_DeviceError, "invalid data received" },
	        { -41 + Command::rsFailure_DeviceError, "data sequence error" },
	        { -42 + Command::rsFailure_DeviceError, "operation aborted by server" },
	        { -43 + Command::rsFailure_DeviceError, "invalid block number" },
	        { -44 + Command::rsFailure_DeviceError, "block data error" },
	        { -45 + Command::rsFailure_DeviceError, "block not downloaded" },
	        { -128+ Command::rsFailure_DeviceError, "vendor defined error 128" },

	        //vendor-specific device-specific error codes, range 128  255

	        { -129 + Command::rsFailure_DeviceError, "timeout" },
	        { -130 + Command::rsFailure_DeviceError, "device not found" },
	        { -131 + Command::rsFailure_DeviceError, "duplicate (lease already exists)" },
	        { -254 + Command::rsFailure_DeviceError, "vendor defined error 254" },
	        { -255 + Command::rsFailure_DeviceError, "extension code" }
	      };

	//look-up for description into tables
	for (unsigned int i = 0; i < sizeof(mapCodes) / sizeof(mapCodes[0]); i++)
	{
		if (mapCodes[i].code == responseCode)
		{
			description = mapCodes[i].description;
			return true;
		}
	}

	return false; //not found description
}

std::string FormatResponseCode(int responseCode)
{
	std::string errorDescription;
	if (!GetErrorCodeDescription(responseCode, errorDescription))
	{
		if (responseCode <= Command::rsFailure_ExtendedDeviceError)
			errorDescription = "unknown extended device error";
		else if (responseCode <= Command::rsFailure_DeviceError)
			errorDescription = "unknown device error";
		else
			errorDescription = "unknown application error";
	}

	if (responseCode <= Command::rsFailure_ExtendedDeviceError)
	{
		//device extended error
		return boost::str(boost::format("SFCEx(%1%)-%2%") % (int)(-responseCode + Command::rsFailure_ExtendedDeviceError)
		    % errorDescription);

	}
	else if (responseCode <= Command::rsFailure_DeviceError)
	{
		//device error
		return boost::str(boost::format("SFC(%1%)-%2%") % (int)(-responseCode + Command::rsFailure_DeviceError)
		    % errorDescription);
	}
	else
	{
		//app error
		return boost::str(boost::format("App(%1%)-%2%") % (int)responseCode % errorDescription);
	}
}

} //namespace hostapp
} //namespace nisa100
