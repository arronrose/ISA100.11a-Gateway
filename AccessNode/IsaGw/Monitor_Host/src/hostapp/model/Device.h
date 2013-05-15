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

#ifndef DEVICE_H_
#define DEVICE_H_

#include <vector>
#include <map>
#include <nlib/datetime.h>

#include "MAC.h"
#include "IPv6.h"

#include <iostream>

#include <queue>
#include <string>



namespace nisa100 {
namespace hostapp {

const int TIMEOUT_WAIT_CONTRACT = 60; //in seconds
const std::string strNOTAVAILABLE = "N/A";

struct DeviceLink;
typedef std::vector<DeviceLink> DevicesLinksT;

struct DeviceLink
{
	int FromDevice;
	int ToDevice;
	int LinkType;
	int LinkID;
	int FromChannelNo;
};

class ChannelValue
{
public:
	enum DataType
	{
		cdtUInt8 = 0,
		cdtUInt16 = 1,
		cdtUInt32 = 2,
		cdtInt8 = 3,
		cdtInt16 = 4,
		cdtInt32 = 5,
		cdtFloat32 = 6
	};
	DataType dataType;// data type for each channel
	union AttributeVal
	{
		boost::uint8_t uint8;
		boost::int8_t int8;
		boost::uint16_t uint16;
		boost::int16_t int16;
		boost::uint32_t uint32;
		boost::int32_t int32;
		float float32;
	} value; //mybe not
public:
	ChannelValue();

	double GetDoubleValue();
	
	//added
	float GetFloatValue();
	uint32_t GetIntValue();
	const std::string ToString() const;
};

class DeviceChannel
{
public:
	enum ChannelType
	{
		ctAnalog = 0,
		ctDigital = 1
	};

	std::string channelName;
	int channelNumber;
	int minRawValue;
	int maxRawValue;
	double minValue;
	double maxValue;

	//leave as int in case somebody decides to put another value in it, other than the enum values
	int channelType;

	ChannelValue::DataType channelDataType;
	int mappedTSAPID;
	int mappedObjectID;
	int mappedAttributeID;

	int withStatus;

	const std::string ToString() const;
};

class Device;
typedef std::vector<Device> DeviceList;
typedef boost::shared_ptr<Device> DevicePtr;
typedef std::vector<DevicePtr> DevicesPtrList;

class Device
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.model.Device");
	*/

public:
	enum DeviceType
	{
		dtUnknown = -1,
		dtSystemManager = 1,
		dtGateway = 2,
		dtBackboneRouter = 3,
		dtRoutingDeviceNode = 10,
		dtNonRoutingDeviceNode = 11,
		dtIORoutingDeviceNode = 12
	};

	enum DeviceApplicationType
	{
		datNotAvailable = 0,
		datPressureSensor = 1,
		datPumpControl = 2,
		datTempPressureDewSensor = 3,
		datEvalKit = 4,
		datHartISAAdapter = 5
	};

	enum DeviceStatus
	{
		dsUnregistered = 1,
		dsRegistered = 20,
		dsSecJoinReqReceived = 4,
		dsSecJoinRespSent = 5,
		dsSMJoinReceived = 6,
		dsSMJoinRespSent = 7,
		dsSMContractJoinReceived = 8,
		dsSMContractJoinRespSent = 9,
		dsSecConfirmReceived = 10,
		dsSecConfirmRespSent = 11,
		
		//no need
		/*
		dsRegistering = 2,
		*/
		dsNotConnected = 50, //when the GW is not connected
		dsNotAvailable = 51 //when we don't know the real state of the device couse we don't get a succeed topo
	};

//added by Cristian.Guef
public:
	int m_subnetID;
public:
	void SetSubnetID(int subnetID)
	{
		if (m_subnetID != subnetID)
		{
			m_subnetID = subnetID;
			isChanged = true;
		}
	}
	int GetSubnetID()const
	{
		return m_subnetID;
	}


public:

	DeviceStatus Status() const
	{
		return status;
	}

	bool IsRegistered() const
	{
		return Device::dsRegistered == Status();
	}

	void Status(DeviceStatus newStatus)
	{
		if (newStatus != status)
		{
			if (newStatus == Device::dsRegistered)
			{
				//ups maybe the rejoined count is same with the cached one (when SM is restarted)
				isRejoined = true;
			}
			/*
			else if (newStatus == Device::dsUnregistered)
			{
				//commented by Cristian.Guef
				//ip = IPv6::NONE();
			}
			*/
			status = newStatus;
			isChanged = true;	//it needs to be set even if 'isStatusChanged' is set
			isStatusChanged = true;
			changedStatus.push(nlib::ToString(nlib::CurrentUniversalTime()));
		}
	}

	bool Rejoined() const
	{
		return isRejoined;
	}

	int RejoinCount() const
	{
		return rejoinCount;
	}

	void RejoinCount(int rejoinCount_)
	{
		if (rejoinCount_ != rejoinCount)
		{
			rejoinCount = rejoinCount_;
			isRejoined = true;
			isChanged = true;
		}
	}

	bool Changed() const
	{
		return isChanged;
	}

	void ResetChanged()
	{
		isChanged = false;
		isRejoined = false;
		isStatusChanged = false;
	}
	

	int Level() const
	{
		return deviceLevel;
	}

	void Level(int level)
	{
		if (level != deviceLevel)
		{
			deviceLevel = level;
			isChanged = true;
		}
	}

	//added by Cristian.Guef
	void SetVertexNo(int no)
	{
		m_vertexNo = no;
	}
	int GetVertexNo()
	{
		return m_vertexNo;
	}

	//added by Cristian.Guef
	void SetPublishHandle(int publishHandle)
	{
		m_publishHandle = publishHandle;
	}
	int GetPublishHandle()
	{
		return m_publishHandle;
	}

	MAC Mac() const
	{
		return mac;
	}
	void Mac(const MAC& mac_)
	{
		if (mac_ != mac)
		{
			mac = mac_;
			isChanged = true;
		}
	}

	IPv6 IP() const
	{
		return ip;
	}
	void IP(const IPv6& ip_)
	{
		if (ip_ != ip)
		{
			ip = ip_;
			isChanged = true;
		}
	}

	DeviceType Type() const
	{
		return deviceType;
	}
	void Type(DeviceType type_)
	{
		if (type_ != deviceType)
		{
			deviceType = type_;
			isChanged = true;
		}
	}

	//added by Cristian.Guef
	boost::uint8_t PowerStatus() const
	{
		return powerStatus;
	}
	void PowerStatus(boost::uint8_t status)
	{
		if (powerStatus != status)
		{
			powerStatus = status;
			isChanged = true;
		}
	}

	void DeviceCapabilities(bool hasAquisitionBoardType_, bool hasBatteryPower_)
	{
		if (hasAquisitionBoardType_ != hasAquisitionBoardType || hasBatteryPower_ != hasBatteryPower)
		{
			hasAquisitionBoardType = hasAquisitionBoardType_;
			hasBatteryPower = hasBatteryPower_;
			isChanged = true;
		}
	}

	void DeviceImplType(boost::uint8_t implType_)
	{
		if (implType_ != implType)
		{
			implType = implType_;
			isChanged = true;
		}
	}

	boost::uint8_t DeviceImplType() const
	{
		return implType;
	}

	/* commented by Cristian.Guef
	boost::uint16_t ContractID() const
	{
		return contractID;
	}
	*/
	//added by Cristian.Guef
	boost::uint32_t ContractID(int resourceID, unsigned char leaseType) const
	{
		std::map<unsigned int, boost::uint32_t>::const_iterator i = m_ResourceIDToContractID[leaseType].find(resourceID);
		if(i != m_ResourceIDToContractID[leaseType].end()){
			return i->second;
		}
		return 0;
	}

	/*comented by Cristian.Guef
	void ContractID(boost::uint16_t contractID_)
	{
		LOG_DEBUG("Set contract ID:" << contractID_ << " on device" << mac.ToString());
		WaitForContract(false);
		if (contractID != contractID_)
		{
			contractID = contractID_;
			isChanged = true;
		}
	}
	*/
	//added by Cristian.Guef
	void ContractID(int resourceID, unsigned char leaseType, boost::uint32_t contractID_)
	{
		LOG_DEBUG("Set lease ID:" << contractID_ << " on device" << mac.ToString());
		WaitForContract(resourceID, leaseType, false);

		std::map<unsigned int, boost::uint32_t>::iterator i = m_ResourceIDToContractID[leaseType].find(resourceID);
		if(i != m_ResourceIDToContractID[leaseType].end()){
			if (i->second != contractID_)
			{
				i->second = contractID_;
			}
		}
		else{
			m_ResourceIDToContractID[leaseType][resourceID] = contractID_;
		}
		
	}

	//for lease committed burst
	//std::vector<std::map<unsigned int, boost::int16_t> >::ite
	void LeaseCommittedBurst(int resourceID, unsigned char leaseType, boost::int16_t committedBurst)
	{
		std::pair<std::map<unsigned int, boost::int16_t>::iterator, bool> pair;
		pair = m_ResourceIDToCommittedBurst[leaseType].insert(
			std::map<unsigned int, boost::int16_t>::value_type(resourceID, committedBurst));

		if (pair.second == false)
		{
			pair.first->second = committedBurst;
		}
	}
	bool IsCommittedBurstGreater(int resourceID, unsigned char leaseType, boost::int16_t committedBurst)
	{
		std::map<unsigned int, boost::int16_t>::iterator i = 
					m_ResourceIDToCommittedBurst[leaseType].find(resourceID);
		if (i != m_ResourceIDToCommittedBurst[leaseType].end())
		{
			if (i->second == 0/*see doc -> gw-ul uses the default*/ && committedBurst != 0 && committedBurst > -15)
				return true;

			if (committedBurst > i->second)
				return true;
			return false;
		}
		return true;
	}
	boost::int16_t LeaseCommittedBurst(int resourceID, unsigned char leaseType)
	{
		std::map<unsigned int, boost::int16_t>::iterator i = 
					m_ResourceIDToCommittedBurst[leaseType].find(resourceID);
		if (i != m_ResourceIDToCommittedBurst[leaseType].end())
		{
			return i->second;
		}
		return 0;
	}

	/**
	 * Just reset existing contract.
	 */
	/*comented by Cristian.Guef
	void ResetContractID()
	{
		//Here we don't clear the waitingContractID flag, maybe is one in pending
		if (contractID != 0)
		{
			contractID = 0;
			isChanged = true;
		}
	}
	*/
	//added by Cristian.Guef
	void ResetContractID()
	{
		//Here we don't clear the waitingContractID flag, maybe is one in pending
		
		unsigned int leaseTypes = m_ResourceIDToContractID.size();
		for(unsigned int k = 0; k < leaseTypes; k++){
			m_ResourceIDToContractID[k].clear();
		}
	}

	//added by Cristian.Guef
	void ResetContractID(int resourceID, unsigned char leaseType)
	{
		std::map<unsigned int, boost::uint32_t>::iterator i = m_ResourceIDToContractID[leaseType].find(resourceID);
		if(i != m_ResourceIDToContractID[leaseType].end()){
			m_ResourceIDToContractID[leaseType].erase(i);
		}
	}

	/* commented by Cristian.Guef
	bool HasContract() const
	{		
		if (deviceType == Device::dtGateway)
			return true; //GW has always contractID == 0.
		
		if (status != Device::dsRegistered)
					return false;

		return contractID> 0;
	}
	*/
	//added by Cristian.Guef
	bool HasContract(int resourceID, unsigned char leaseType) const
	{	
		/* comented by Cristian.Guef
		if (deviceType == Device::dtGateway)
			return true; //GW has always contractID == 0.
		*/
		//added by Cristian.Guef
		if (leaseType != 6 /*AlertSubscription*/ && deviceType == Device::dtGateway)
			return true;

		/* commented by Cristian.Guef - check to see if device is registered before calling this function
		if (status != Device::dsRegistered)
					return false;
		*/

		std::map<unsigned int, boost::uint32_t>::const_iterator i = m_ResourceIDToContractID[leaseType].find(resourceID);
		if(i != m_ResourceIDToContractID[leaseType].end()){
			return true;
		}
		return false;
	}

	/* commneted by Cristian.Guef
	bool WaitForContract() const
	{
		if (waitingContractID &&
				(issueContractRequestAt + nlib::util::seconds(TIMEOUT_WAIT_CONTRACT) < nlib::CurrentLocalTime()))
		{
			// wait for contract expired, probably will never comes
			waitingContractID = false;
		}
		return waitingContractID;
	}
	*/
	//added by Cristian.Guef
	bool WaitForContract(int resourceID, unsigned char leaseType)
	{

		std::map<unsigned int, bool>::iterator waitingIter = m_ResourceIDToWaitingContractID[leaseType].find(resourceID);
		std::map<unsigned int, nlib::DateTime>::const_iterator issueIter = m_ResourceIDToIssueContractRequestAt[leaseType].find(resourceID);
		bool waiting = false;

		if(waitingIter != m_ResourceIDToWaitingContractID[leaseType].end()){
			waiting = waitingIter->second ;
		}
		
		/* no need for that because we ask for infinite period contract
		if(waiting == true){
			if((issueIter->second + nlib::util::seconds(TIMEOUT_WAIT_CONTRACT)) < nlib::CurrentLocalTime()){
				waitingIter->second = false;
			}
		}
		*/
		return waiting;
	}


	/* commented by Cristian.Guef
	void WaitForContract(bool wait)
	{
		if (waitingContractID != wait)
		{
			waitingContractID = wait;
		}

		if (waitingContractID)
			issueContractRequestAt = nlib::CurrentLocalTime();
	}
	*/
	//added by Cristian.Guef
	void WaitForContract(int resourceID, unsigned char leaseType, bool wait)
	{
		std::map<unsigned int, bool>::iterator waitingIter = m_ResourceIDToWaitingContractID[leaseType].find(resourceID);
		
		if(waitingIter != m_ResourceIDToWaitingContractID[leaseType].end()){
			if(waitingIter->second != wait){
				waitingIter->second = wait;
			}
			if(waitingIter->second == true){
				(m_ResourceIDToIssueContractRequestAt[leaseType])[resourceID] = nlib::CurrentUniversalTime();
			}
		}
		else{
			(m_ResourceIDToWaitingContractID[leaseType])[resourceID] = wait;
			if(wait == true){
				(m_ResourceIDToIssueContractRequestAt[leaseType])[resourceID] = nlib::CurrentUniversalTime();
			}
		}
	}

	//alert subscription
	bool HasMadeAlertSubscription()
	{
		return m_hasMadeAlertSubscription;
	}
	void HasMadeAlertSubscription(bool val)
	{
		m_hasMadeAlertSubscription = val;
	}

	bool BatteryPowerOperated() const
	{
		return hasBatteryPower;
	}
public:
	int id;
	IPv6 ip;
	MAC mac;
	DeviceStatus status;
	DeviceType deviceType;
	int sensorTypeID;
	int deviceLevel;
	int rejoinCount;
	
	boost::uint8_t implType;
	nlib::DateTime lastReading;
	//capabilities
	bool hasAquisitionBoardType;
	bool hasBatteryPower;

	//added by Cristian.Guef
	boost::uint8_t powerStatus;

	//alert subscription
	bool m_hasMadeAlertSubscription;

private:
	/*
	- commented by Cristian.Guef
	- these were replaced due to the added ResourceID (see definition in "GContract.h")
	boost::uint16_t contractID;
	mutable bool waitingContractID;
	nlib::DateTime issueContractRequestAt; //used to handle expiration of waiting contract
	*/


	bool isChanged;
	bool isRejoined;
	bool isStatusChanged;
public:
	std::queue<std::string> changedStatus;
	bool IsStatusChanged()
	{
		return isStatusChanged;
	}

public:
	Device()
	{
		id = -1;
		deviceType = dtUnknown;
		sensorTypeID = dtUnknown;
		implType = 0;
		
		status = dsUnregistered;

		ip = IPv6::NONE();
		rejoinCount = 0;
		deviceLevel = -1;

		hasAquisitionBoardType = false;
		hasBatteryPower = false;

		ResetChanged();

		/* commented by Cristian.Guef
		contractID = 0;
		waitingContractID = false;
		*/

		//added by Cristian.Guef
		m_ResourceIDToContractID.resize(7 /*leasetypes*/);
		m_ResourceIDToWaitingContractID.resize(7 /*leasetypes*/);
		m_ResourceIDToIssueContractRequestAt.resize(7 /*leasetypes*/);

		//lease committed burst
		m_ResourceIDToCommittedBurst.resize(7 /*leasetypes*/);

		//added by Cristian.Guef
		issuePublishSubscribeCmd = true;

		//added by Cristian.Guef
		m_vertexNo = -1;
		m_subnetID = -1;
		powerStatus = 8; //invalid value
		m_publishHandle = -1; //invalid value

		//alert subscription
		m_hasMadeAlertSubscription = false;
		isStatusChanged = false;
	}

	//added by Cristian.Guef
	int m_publishHandle;

	const std::string ToString() const;

	//added by Cristian.Guef
	//regarding resourceID, see definition in "GContract.h" file
	//it is transmited when throwing exception (not good from design point of view -> it wouldn't belong to this class)
	unsigned int	m_unThrownResourceID;
	unsigned char	m_unThrownLeaseType;
	
	//added by Cristian.Guef -- need it in the periodic task "DoPreconfiguredPublish.."
	bool issuePublishSubscribeCmd;

	//added by Cristian.Guef
	//a device may have more than one contract so
	// we have a map for each obtained ContractID
	//leasetype 0 - 6
	std::vector<std::map<unsigned int, boost::uint32_t> > m_ResourceIDToContractID;
	std::vector<std::map<unsigned int, bool> > m_ResourceIDToWaitingContractID;
	std::vector<std::map<unsigned int, nlib::DateTime> > m_ResourceIDToIssueContractRequestAt;

	//lease committed burst
	std::vector<std::map<unsigned int, boost::int16_t> > m_ResourceIDToCommittedBurst;

	//added by Cristian.Guef -used in graph processing level
	int		m_vertexNo;

public:
	//added by Cristian.Guef
	std::string m_deviceTAG; 
	std::string m_deviceModel;
	std::string m_deviceRevision;
	std::string m_deviceManufacturer;
	std::string m_deviceSerialNo;

public:
	void SetTAG(const std::string &tag)
	{
		if (m_deviceTAG != tag)
		{
			m_deviceTAG = tag;
			isChanged = true;
		}
	}
	const std::string GetTAG()const 
	{
		return m_deviceTAG.size() == 0 ? strNOTAVAILABLE : m_deviceTAG;
	}
	void SetModel(const std::string & model)
	{
		if (m_deviceModel != model)
		{
			m_deviceModel = model;
			isChanged = true;
		}
	}
	const std::string GetModel()const
	{
		return m_deviceModel.size() == 0 ? strNOTAVAILABLE : m_deviceModel;
	}
	void SetRevision(const std::string & revision)
	{
		if (m_deviceRevision != revision)
		{
			m_deviceRevision = revision;
			isChanged = true;
		}
	}
	const std::string GetRevision()const
	{
		return m_deviceRevision.size() == 0 ? strNOTAVAILABLE : m_deviceRevision;
	}
	void SetManufacturer(const std::string &manufacturer)
	{
		if (m_deviceManufacturer != manufacturer)
		{
			m_deviceManufacturer = manufacturer;
			isChanged = true;
		}
	}
	const std::string GetManufacturer()const
	{
		return m_deviceManufacturer.size() == 0 ? strNOTAVAILABLE : m_deviceManufacturer;
	}
	void SetSerialNo(const std::string &serialNo)
	{
		if (m_deviceSerialNo != serialNo)
		{
			m_deviceSerialNo = serialNo;
			isChanged = true;
		}
	}
	const std::string GetSerialNo()const
	{
		return m_deviceSerialNo.size() == 0 ? strNOTAVAILABLE : m_deviceSerialNo;
	}
public:
	boost::uint32_t		m_DPDUsTransmitted;		// GS_DPDUs_Transmitted
	boost::uint32_t		m_DPDUsReceived;			// GS_DPDUs_Received
	boost::uint32_t		m_DPDUsFailedTransmission;	// GS_DPDUs_Failed_Transmission
	boost::uint32_t		m_DPDUsFailedReception;		// GS_DPDUs_Failed_Reception
public:
	void SetDPDUsTransmitted(boost::uint32_t val)
	{
		if (m_DPDUsTransmitted != val)
		{
			m_DPDUsTransmitted = val;
			isChanged = true;
		}
	}
	boost::uint32_t GetDPDUsTransmitted()const
	{
		return m_DPDUsTransmitted;
	}
	void SetDPDUsReceived(boost::uint32_t val)
	{
		if (m_DPDUsReceived != val)
		{
			m_DPDUsReceived = val;
			isChanged = true;
		}
	}
	boost::uint32_t GetDPDUsReceived()const
	{
		return m_DPDUsReceived;
	}
	void SetDPDUsFailedTransmission(boost::uint32_t val)
	{
		if (m_DPDUsFailedTransmission != val)
		{
			m_DPDUsFailedTransmission = val;
			isChanged = true;
		}
	}
	boost::uint32_t GetDPDUsFailedTransmission()const
	{
		return m_DPDUsFailedTransmission;
	}
	void SetDPDUsFailedReception(boost::uint32_t val)
	{
		if (m_DPDUsFailedReception != val)
		{
			m_DPDUsFailedReception = val;
			isChanged = true;
		}
	}
	boost::uint32_t GetDPDUsFailedReception()const
	{
		return m_DPDUsFailedReception;
	}
};

} // namespace hostapp
}
// namespace nisa100

#endif /*DEVICE_H_*/
