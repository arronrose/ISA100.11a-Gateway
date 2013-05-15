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

#ifndef COMMAND_H_
#define COMMAND_H_

#include <string>
#include <vector>
#include <nlib/datetime.h>
#include <boost/format.hpp>

//added by Cristian.Guef
#include "../../PublisherConf.h"


namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
class DevicesManager;

class Command;
typedef std::vector<Command> CommandsList;

class CommandParameter
{
public:
	enum ParameterCode
	{
		ReadValue_Channel = 10,
		PublishSubscribe_PublisherChannelNo = 30,
		PublishSubscribe_Frequency = 31,
		PublishSubscribe_FrequencyFaze = 32,
		PublishSubscribe_SubscriberChannelNo = 33,
		PublishSubscribe_SubscriberDeviceID = 34,

		//added by Cristian.Guef
		PublishSubscribe_Concentrator_id = 35,
		PublishSubscribe_Concentrator_tlsap_id = 36,
		PublishSubscribe_StaleLimit = 37,

		//added by Cristian.Guef
		DelContract_ContractID = 38,	//parameter wich represents the contract_id 
										//in the map containing data to delete lease

		LocalLoop_PublisherChannelNo = 40,
		LocalLoop_Frequency = 41,
		LocalLoop_FrequencyFaze = 42,
		LocalLoop_SubscriberChannelNo = 43,
		LocalLoop_SubscriberDeviceID = 44,
		LocalLoop_SubscriberLowThreshold = 45,
		LocalLoop_SubscriberHighThreshold = 46,

		DigitalOutOn_Channel = 50,
		DigitalOutOff_Channel = 60,

		//added by Cristian.Guef
		ScheduleReport_DevID = 61,
		NeighbourHealthReport_DevID = 62,
		DevHealthReport_DevIDs = 63,

		//for alert_subscription
		Alert_Category_Subscribe = 64,
		Alert_Category_Enable = 65,
		Alert_Category_Category = 66,
		Alert_NetAddr_Subscribe = 67,
		Alert_NetAddr_Enable = 68,
		Alert_NetAddr_DevID = 69,
		Alert_NetAddr_ObjID = 70,
		Alert_NetAddr_TLSAPID = 71,
		Alert_NetAddr_AlertType = 72,


		//added by Cristian.Guef
		ISACSRequest_TSAPID = 80,
		ISACSRequest_ReqType = 81,
		ISACSRequest_ObjID = 82,
		ISACSRequest_ObjResourceID = 83,
		ISACSRequest_AttrIndex1 = 84,
		ISACSRequest_AttrIndex2 = 85,
		ISACSRequest_DataBuffer = 86,
		ISACSRequest_ReadAsPublish = 87,

		GetFirmware_DeviceAddress = 1030,
		FirmwareUpdate_DeviceAddress = 1040,
		FirmwareUpdate_FileName = 1041,

		GetFirmwareUpdateStatus_DeviceAddress = 1050,
		CancelFirmware_DeviceAddress = 1060,

		GetDeviceInformation_DeviceID = 1070,

		ResetDevice_DeviceID = 1080,
		
		//added by Cristian.Guef
		ResetDevice_RestartType = 1081,

		//added by Cristian.Guef
		GetContractsAndRoutes_DeviceID = 1080,

		FirmwareUpload_FileName = 1100,
		FirmwareUpload_FirmwareID = 1101,

		//added
		SensorFirmwareUpdate_DeviceID = 1200,
		SensorFirmwareUpdate_Filename = 1201,
		SensorFirmwareUpdate_Port = 1202,
		SensorFirmwareUpdate_ObjID = 1203,
				
		//added
		CancelSensorFirmwareUpdate_DeviceID = 1210,

		//added
		GetChannelsStatistics_DeviceID = 1220,

		//for bulk and c/s requests
		LeaseCommittedBurst	= 1230

	};

	ParameterCode parameterCode;
	std::string parameterValue;

	CommandParameter()
	{
	}
	CommandParameter(ParameterCode code, const std::string& value)
	{
		parameterCode = code;
		parameterValue = value;
	}
};


class Command
{
public:
	typedef std::vector<CommandParameter> ParametersList;

	enum CommandCode
	{
		ccGetTopology = 0,
		ccReadValue = 1,
		ccPublishSubscribe = 3,
		ccLocalLoop = 4,
		ccDigitalOutputOn = 5,
		ccDigitalOutputOff = 6,
		ccCancelPublishSubscribe = 7,
		ccCancelLocalLoop = 8,

		//added by Cristian.Guef
		ccISAClientServerRequest = 10,

		ccCreateClientServerContract = 100,
		ccGetDevicesBatteryStatistics = 101,
		ccResetDevicesBatteryStatistics = 102,
		ccGetFirmwareVersion = 103,
		ccFirmwareUpdate = 104,
		ccGetFirmwareUpdateStatus = 105,
		ccCancelFirmwareUpdate = 106,
		
		ccGetDeviceInformation = 107,
		ccResetDevice = 108,
		ccAcqBoardSWVersion = 109,

		ccFirmwareUpload = 110,

		//added by Cristian.Guef
		ccSession = 111,

		//added by Cristian.Guef
		ccDelContract = 112,

		//added by Cristian.Guef
		ccNetworkHealthReport = 113,
		ccScheduleReport = 114,
		ccNeighbourHealthReport = 115,
		ccDevHealthReport = 116,
		ccNetResourceReport = 117,
		ccAlertSubscription = 118,

		//added by Cristian.Guef
		ccGetContractsAndRoutes = 119,

		//added
		ccGetDeviceList = 120,

		//added
		ccSensorBoardFirmwareUpdate = 121,
		ccCancelBoardFirmwareUpdate = 122,
		ccGetChannelsStatistics = 123
	};
	
	//added by Cristian.Guef
	//used in generating contracts (lease)
	unsigned char	m_ucLeaseTypeForContractCmd;

	//added by Cristian.Guef used in generating contracts (lease)
	//of new Type (GSAP - 2009.02.04)
	//see definition of resourceID in "GContract.h" file
	unsigned int	m_unResourceIDForContractCmd;

	//
	boost::int16_t	m_committedBurst;
	int				m_dbCmdIDForContract;

	//added by Cristian.Guef
	//we have to break the command ccReadValue into pieces (because of new GSAP)
	struct MultipleReadDBCmd
	{
		int IndexforFragmenting;		//  ">=" do fragmenting
										//  "<0" stop fragmenting 
		unsigned int	DiagID;
	} m_MultipleReadDBCmd;

	//added by Cristian.Guef
	unsigned short m_uwNetworkIDForSessionGSAP;

	//added by Cristian.Guef - generate contracts before sending client/server commands
	DevicesManager*		 devicesManager;

	//added by Cristian.Guef - for new subscribe leases
	PublisherConf::COChannelListT	*pcoChannelList;
	PublisherConf::ChannelIndexT	*pcoChannelIndex;
	unsigned char					dataContentVer;
	unsigned char					interfaceType;

	//for del_lease
	boost::uint8_t	ContractType;
	IPv6			IPAddress;
	unsigned int	ResourceID;

	//for contract_and_routes cmd
	bool			isFirmwareDownload;

	enum ResponseStatus
	{
		rsNoStatus = 0,
		rsSuccess = 1,
		rsSuccess_SessionLowerPeriod = 2,
		rsSuccess_ContractLowerPeriod = 3,

		rsFailure_InvalidDevice = -1,
		rsFailure_InvalidCommand = -2,
		rsFailure_DeviceHasNoContract = -3,
		rsFailure_DeviceNotRegistered = -4,

		//added by Cristian.Guef
		rsFailure_ThereisNoLease = -5,
		rsFailure_InvalidLeaseType = -6,
		rsFailure_InvalidLeaseTypeInform = -7,
		rsFailure_InvalidReturnedLeaseID = -8,
		rsFailure_InvalidReturnedLeasePeriod = -9,

		rsFailure_GatewayTimeout = -10,
		rsFailure_GatewayCommunication = -11,

		/* commented by Cristian.Guef
		rsFailure_GatewayInvalidContract = -12,
		*/
		//added by Cristian.Guef
		rsFailure_GatewayContractExpired = -12,
		
		rsFailure_GatewayNoContractsAvailable = -13,
		
		//added by Cristian.Guef
		rsFailure_ReadValueUnexpectedSize = -17,
		
		//added by Cristian.Guef
		rsFailure_GatewayInvalidFailureCode = -18,
		
		rsFailure_GatewayUnknown = -19,

		rsFailure_HostTimeout = -20,
		rsFailure_LostConnection = -21,
		rsFailure_SerializationError = -22,
				
		rsFailure_CommandNotSent = -23,

		//added by Cristian.Guef
		rsFailure_GatewayNotJoined = -24,

		//added by Cristian.Guef
		rsFailure_InvalidPublisherDataReceived = -26,

		//added by Cristian.Guef
		rsFailure_InvalidReturnedSessionID = -28,
		rsFailure_InvalidReturnedSessionPeriod = -29,
		rsFailure_ThereIsNoSession = -30,
		rsFailure_SessionNotCreated = -31,

		//added by Cristian.Guef
		rsFailure_NoUnBufferedReq = -33,
		rsFailure_InvalidBufferedReq = -34,

		//added by Cristian.Guef
		rsFailure_ItemExceedsLimits = -35,
		rsFailure_UnknownResource = -36,
		rsFailure_InvalidMode = -37,
		rsFailure_InvalidBlockSize = -38,
		rsFailure_CommunicationFailed = -39,
		rsFailure_TransferAborted = -40,
		rsFailure_InvalidBlockNo = -41,
		
		//added by Cristian.Guef
		rsFailure_InvalidHeaderCRC = -43,
		rsFailure_InvalidDataCRC = -44,

		//added by Cristian.Guef
		rsFailure_InvalidCategory = -45,
		rsFailure_InvalidIndividualAlert = -46,


		rsFailure_NoSensorUpdateInProgress = -47,
		rsFailure_AlreadySensorUpdateInProgress = -48,


		rsFailure_ContractFailed = -49,

		rsFailure_InternalError = -100,

		rsFailure_DeviceError = -100000,
		rsFailure_ExtendedDeviceError = rsFailure_DeviceError - 65280,

		rsWarning_PublishSubscribeNotFound = rsFailure_DeviceError - 3
	};

	enum CommandStatus
	{
		csNew = 0,
		csSent = 1,
		csResponded = 2,
		csFailed = 3
	};

	enum CommandGeneratedType
	{
		cgtManual = 0,
		cgtAutomatic = 1
	};

	static const int NO_COMMAND_ID = -1;

public:
	Command()
	{
		commandID = NO_COMMAND_ID;
		deviceID = -1;
		errorCode = rsNoStatus;
		generatedType = cgtManual; //manual by default
		isFirmwareDownload = false;
	}
public:
	int commandID;
	int deviceID;
	CommandCode commandCode;
	CommandStatus commandStatus;

	nlib::DateTime timePosted;
	nlib::DateTime timeResponded;
	ResponseStatus errorCode;
	std::string errorReason;

	std::string response;

	ParametersList parameters;

	CommandGeneratedType generatedType;
public:
	const std::string ToString() const;
};

std::string FormatResponseCode(int responseCode);

} // namespace hostapp
} // namespace nisa100

#endif /*COMMAND_H_*/
