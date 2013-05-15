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

#ifndef DEVICESMANAGER_H_
#define DEVICESMANAGER_H_

#include "model/DeviceReading.h"
#include "dal/IFactoryDal.h"
#include "model/Command.h"
#include "model/FirmwareUpload.h"
#include "commandmodel/GTopologyReport.h"
#include "commandmodel/PublishSubscribe.h"
#include "NodesRepository.h"

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../Log.h"
#include "../ConfigApp.h"

#include <nlib/exception.h>
#include <boost/function.hpp> //for callback
#include <set>

//added by Cristian.Guef
#include <queue>

//added by Cristian.Guef
#include "DevicesGraph.h"
#include "commandmodel/GScheduleReport.h"
#include "commandmodel/GNetworkHealthReport.h"
#include "commandmodel/GDeviceListReport.h"


namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
class ReportsProcessor;
class PublishedDataMng;

class NoGatewayException : public nlib::Exception
{
public:
	NoGatewayException() :
		nlib::Exception("There is no gateway defined on system!")
	{
	}
};


/**
 * @brief manages devices, process topology
 */
class DevicesManager
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.DevicesManager");
	*/

public:
	DevicesManager(IFactoryDal& factoryDal, ConfigApp& p_rConfigApp);
	virtual ~DevicesManager();

public:
	/**
	 * load all devices from db, the gateway should exists
	 * @throw NoGatewayException
	 */
	void ResetTopology();

	/**
	 * Handles a topo response
	 */
	/* commented by Cristian.Guef
	void ProcessTopology(GTopologyReport& topologyReport);
	*/
	//added by Cristian.Guef
	void ProcessTopology(ReportsProcessor *pRepProcessor, GTopologyReport& topologyReport);

	/**
	 * Notify observer with list of new rejoined devices after processing a topology.
	 */
	/* commented by Cristian.Guef
	boost::function0<void> TopologyChanged;
	*/
	//added by Cristian.Guef
	boost::function1<void, bool> TopologyChanged;

	/**
	 * Save readings
	 */
	void SaveReadings(const DeviceReadingsList& readings);
	void SaveReading(const DeviceReading& reading);

	//added 
	void SaveReadings(PublishedDataMng *pPublishedDataMng);

	void CreateReading(const DeviceReading& reading);
	void CreateEmptyReadings(int deviceID);
	void CreateEmptyReading(int deviceID, int channelNo);
	void DeleteReading(int channelNo);
	void DeleteReadings(int deviceID);
	bool IsDeviceChannelInReading(int deviceID, int channelNo);

	DevicePtr FindDevice(boost::int32_t deviceID) const;
	DevicePtr FindDevice(const MAC& deviceMAC) const;

	//added by Cristian.Guef
	void UpdatePublishFlag(int deviceID, int flag);

	/**
	 * @return the gateway device object
	 * @throw NoGatewayException
	 */
	const DevicePtr GatewayDevice() const;

	/**
	 * @return the system manager object or null if not found.
	 */
	const DevicePtr SystemManagerDevice() const;

	//added
	bool IsSMDevice(int DeviceID);

	//added
	const DevicePtr BBRDevice() const;

	const NodesRepository& Devices() const;

	/* cvommented by Cristian.Guef
	void SetDeviceContract(const DevicePtr& device, int contractID);
	*/
	//added by Cristian.Guef
	void SetDeviceContract(const DevicePtr& device, unsigned int contractID,
						int resourceID, unsigned char leaseType, boost::int16_t committedBurst, 
						unsigned char &leaseTypeReplaced);


	void HandleGWConnect(const std::string& host, int port);
	void HandleGWDisconnect();
	bool const GatewayConnected();
	void ChangeGatewayStatus(Device::DeviceStatus newStatus);


	bool GetNextFirmwareUpload(int notFailedForMinutes, FirmwareUpload& result);
	bool GetFirmwareUpload(boost::uint32_t id, FirmwareUpload& result);
	void UpdateFirmware(const FirmwareUpload& upload);
	void ResetPendingFirmwareUploads();

	//added by Cristian.Guef
	void DeleteContracts();
	void ChangeContracts(const IPv6 &sourceIP, const ContractsAndRoutes::ContractsListT &infoList);
	void ChangeRoutes(const IPv6 &sourceIP, ContractsAndRoutes::RoutesListT &infoList);
	void InsertContractElements(int contractID, int sourceID, std::deque<int/*deviceID*/> & deviceIDs);
	void RemoveContractElements(int sourceID);
	void RemoveContractElements();

	//added by Cristian.Guef
	void SaveISACSConfirm(int deviceID, ISACSInfo &confirm);

	//added by Cristian.Guef
	void ChangeScheduleInfo(GScheduleReport &scheduleReport);
	
	//added by Cristian.Guef
	void ChangeNetworkHealthInfo(GNetworkHealthReport &networkHealthRep);

	//added by Cristian.Guef
	void AddDeviceHealthHistory(int deviceID, const nlib::DateTime &timestamp, const GDeviceHealthReport::DeviceHealth &devHeath);
	void UpdateDeviceHealth(int deviceID, const GDeviceHealthReport::DeviceHealth &devHeath);
	void AddNeighbourHealthHistory(int deviceID, const nlib::DateTime &timestamp, int neighbID, const GNeighbourHealthReport::NeighbourHealth &neighbHealth);

	//added by Cristian.Guef
	void SavePublishChannels(int deviceID, PublisherConf::COChannelListT &list);
	void DeletePublishChannels(const std::vector<int/*dbChannelNo*/> &list);
	void DeleteAllPublishChannels();
	void MoveChannelToHistory(int channelNo);
	void MoveChannelsToHistory(int deviceID);
	int CreateChannel(int deviceID, PublisherConf::COChannel &channel);
	void UpdateChannel(int channelNo, const std::string &channelName, const std::string &unitOfMeasure, int channelFormat, int withStatus);
	void UpdateChannelsInfo(const PublisherConf::COChannelListT &list);
	bool FindDeviceChannel(int channelNo, DeviceChannel& channel);
	bool IsDeviceChannel(int deviceID, const PublisherConf::COChannel &channel);
	bool HasDeviceChannels(int deviceID);
	void SetPublishErrorFlag(int deviceID, int flag);
	void ProcessChannelsDifference(int deviceID, 
				PublisherConf::COChannelListT &storedList, 
				const PublisherConf::ChannelIndexT &storedIndex,
				const std::vector<PublisherConf::COChannel> &channels);
	void ResetDeviceChannels();
	void ProcessDBChannels(int deviceID, PublisherConf::COChannelListT &storedList, 
											 const PublisherConf::ChannelIndexT &storedIndex);

	//publishers_info
	void DeletePublishInfo(int deviceID);
	void GetOrderPublisherMACs(std::vector<MAC> &macs);
	

	//alert
	bool GetAlertProvision(int &categoryProcessSub, int &categoryDeviceSub, int &categoryNetworkSub, int &categorySecuritySub);
	void SaveAlertIndication(int devID, int TSAPID, int objID, const nlib::DateTime &timestamp,  int milisec,
		int classType, int direction, int category, int type, int priority, std::string &data);

	//channels statistics
	void DeleteChannelsStatistics(int deviceID);
	void SaveChannelsStatistics(int deviceID, const std::string& strChannelsStatistics);

public:
	void ProcessDeviceList(boost::shared_ptr<GDeviceListReport> DevListRepPtr);

public:
	//firmwaredownloads
	enum FirmDlStatus	// status from table FirmwareDownloads field FwStatus
	{
		//FirmDlStatus_Progress = 1,
		//FirmDlStatus_Cancelling = 2,
		FirmDlStatus_Cancelled = 3,
		FirmDlStatus_Completed,
		FirmDlStatus_Failed
	};
	void SetFirmDlStatus(int deviceID, int fwType, int status/*3-cancelled, 4-completed, 5-failed*/);
	void SetFirmDlPercent(int deviceID, int fwType, int percent);
	void SetFirmDlSize(int deviceID, int fwType, int size);
	void SetFirmDlSpeed(int deviceID, int fwType, int speed);
	void SetFirmDlAvgSpeed(int deviceID, int fwType, int speed);

private:

	//added by Cristian.Guef
	void UnregisterDevices(ReportsProcessor *pRepProcessor, GTopologyReport& topologyReport, bool &doUpdateIPv6Index);
	void UpdateUnregDevices(ReportsProcessor *pRepProcessor);

	//added
	//void ProcessDeviceList(boost::shared_ptr<GDeviceListReport> DevListRepPtr);
	bool IsDevDeletedFromDB(Device& device);
	void BuildNewDevice(DevicePtr devPtr, IPv6 ip, GDeviceListReport::Device &devInfo);
	void UpdateDevice(DevicePtr devPtr, IPv6 ip, GDeviceListReport::Device &devInfo, bool &doUpdateIPv6Index);
	void UpdateGw(const MAC &mac, const IPv6 &ip, bool &doUpdateIPv6Index, bool &doUpdateIdIndex);

	void AddDevice(Device& device);

	/* commented by Cristian.Guef
	void SaveDeviceNeighbours(const DevicePtr& nodeFrom,	const GTopologyReport::Device::NeighborsListT& neighbours);
	*/
	//added by Cristian.Guef
	void SaveDeviceNeighbours(DevicesGraph &devGraph, int vertexNo);

	void SaveDeviceGraphs(const DevicePtr& nodeFrom, GTopologyReport::Device::GraphsListT& graphs);

	/* commented by Cristian.Guef
	void ComputeNodeLevels(GTopologyReport::DevicesListT& devicesList);
	*/
	//added by Cristian.Guef
	void ComputeNodeLevels(DevicesGraph &devGraph, int vertexNo, unsigned char *pTraversed, int NodeLevel);
	void StartComputeNodeLevels(DevicesGraph &devGraph, int vertex);

	//added by Cristian.Guef
	void ProcessSubnetIDsFromBBRs(DevicesGraph &devGraph, std::deque<int> &BBRVertices);
	void ComputeSubnetID(DevicesGraph &devGraph, int vertex, unsigned char *pTraversed, int subnetID);

private:
	void ResetDevicesChanges();

	bool NeedToCreateDeviceLink(DeviceLink& deviceLink);
	bool ComputeReadingValue(const DeviceReading& reading, double& computedValue);

public:
	ConfigApp& m_rConfigApp;

public:
	IFactoryDal& factoryDal;
	/*commented by Cristian.Guef
	NodesRepository repository;
	*/
	bool isGatewayConnected;

//added by Cristian.Guef
public:
	NodesRepository repository;
	
//added by Cristian.Guef
public:
	struct InfoToDelete
	{
		int				DevID;
		IPv6			IPAddress;
		unsigned int	ResourceID;
		unsigned char	LeaseType;
		unsigned int	ContractID;
	};
	std::queue<InfoToDelete>  queueToDeleteLease;

	void LoadSubscribeLeaseForDel(DevicePtr dev);

//added by Cristian.Guef
public:
	std::queue<MAC> rejoinedDevices;

//added by Cristian.Guef
private:
	DevicePtr	m_smDevPtr;
	DevicePtr	m_gwDevPtr;
	DevicePtr	m_bbrDevPtr;
};

} //namspace hostapp
} //namspace nisa100

#endif /*DEVICESMANAGER_H_*/
