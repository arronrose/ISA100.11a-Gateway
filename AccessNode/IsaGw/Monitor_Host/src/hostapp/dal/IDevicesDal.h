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

#ifndef IDEVICESDAL_H_
#define IDEVICESDAL_H_

#include "../model/Device.h"
#include "../model/DeviceReading.h"
#include "../model/FirmwareUpload.h"

//added by Cristian.Guef
#include "../model/ContractsAndRoutes.h"
#include "../model/ISACSInfo.h"
#include "../commandmodel/GDeviceHealthReport.h"
#include "../commandmodel/GNeighbourHealthReport.h"
#include "../commandmodel/GNetworkHealthReport.h"
#include "../commandmodel/GScheduleReport.h"
#include "../../PublisherConf.h"

namespace nisa100 {
namespace hostapp {


class IDevicesDal
{
public:
	virtual ~IDevicesDal()
	{
	}
	virtual void ResetDevices(Device::DeviceStatus newStatus) = 0;
	virtual void GetDevices(DeviceList& list) = 0;

	//added by Cristian.Guef
	virtual void DeleteDevice(int id) = 0;

	//added by Cristian.Guef
	virtual bool IsDeviceInDB(int deviceID) = 0;
	
	virtual void AddDevice(Device& device) = 0;
	virtual void UpdateDevice(Device& device) = 0;

	//added by Cristian.Guef
	virtual void UpdateReadingTimeforDevice(int deviceID, const struct timeval &tv) = 0;

	virtual void AddReading(const DeviceReading& reading, bool p_bHistory = false) = 0;
	virtual void AddEmptyReadings(int DeviceID) = 0;
	virtual void AddEmptyReading(int DeviceID, int channelNo) = 0;
	virtual void UpdateReading(const DeviceReading& reading) = 0;
	virtual void DeleteReading(int p_nChannelNo) = 0;
	virtual void DeleteReadings(int deviceID) = 0;
	virtual bool IsDeviceChannelInReading(int deviceID, int channelNo) = 0;

	//added
	virtual void UpdatePublishFlag(int devID, int flag/*0-no data, 1-fresh data, 2-stale data*/) = 0;

	virtual void CreateDeviceNeighbour(int fromDevice, int toDevice, int signalQuality, int clockSource) = 0;

	virtual void CleanDeviceNeighbours() = 0;
	virtual void CreateDeviceGraph(int fromDevice, int toDevice, int graphID) = 0;
	virtual void CleanDeviceGraphs() = 0;

	virtual void ResetPendingFirmwareUploads() = 0;
	virtual bool GetNextFirmwareUpload(int notFailedForMinutes, FirmwareUpload& result) = 0;
	virtual bool GetFirmwareUpload(boost::uint32_t id, FirmwareUpload& result) = 0;
	virtual void UpdateFirmwareUpload(const FirmwareUpload& firmware) = 0;
	virtual void GetFirmwareUploads(FirmwareUploadsT& firmwaresList) = 0;

	virtual void AddDeviceConnections(int deviceID, const std::string& host, int port) = 0;
	virtual void RemoveDeviceConnections(int deviceID) = 0;
	
	//added by Cristian.Guef
	virtual void DeleteContracts(int deviceID) = 0;
	virtual void DeleteContracts() = 0;
	virtual void AddContract(int fromDevice, int toDevice, const ContractsAndRoutes::Contract &contract) = 0;

	//added by Cristian.Guef
	virtual void DeleteRoutes(int deviceID) = 0;
	virtual void AddRouteInfo(int deviceID, int routeID, int alternative, int fowardLimit, int selector, int srcAddr) = 0;
	virtual void AddRouteLink(int deviceID, int routeID, int routeIndex, int graphID) = 0;
	virtual void AddRouteLink(int deviceID, int routeID, int routeIndex, const DevicePtr neighbour) = 0;
	virtual void RemoveContractElements(int sourceID) = 0;
	virtual void RemoveContractElements() = 0;
	virtual void AddContractElement(int contractID, int sourceID, int index, int deviceID) = 0;

	//added by Cristian.Guef
	virtual void SaveISACSConfirm(int deviceID, ISACSInfo &confirm) = 0;

	//added by Cristian.Guef
	virtual void AddRFChannel(const GScheduleReport::Channel &channel) = 0;
	virtual void AddScheduleSuperframe(int deviceID, const GScheduleReport::Superframe &superFrame) = 0;
	virtual void AddScheduleLink(int deviceID, int superframeID, const GScheduleReport::Link &link) = 0;
	virtual void RemoveRFChannels() = 0;
	virtual void RemoveScheduleSuperframes() = 0;
	virtual void RemoveScheduleLinks() = 0;
	virtual void RemoveScheduleSuperframes(int deviceID) = 0;
	virtual void RemoveScheduleLinks(int deviceID) = 0;

	//added by Cristian.Guef
	virtual void AddNetworkHealthInfo(const GNetworkHealthReport::NetworkHealth &netHealth) = 0;
	virtual void AddNetHealthDevInfo(const GNetworkHealthReport::NetDeviceHealth &devHealth) = 0;
	virtual void RemoveNetworkHealthInfo() = 0;
	virtual void RemoveNetHealthDevInfo() = 0;

	//added by Cristian.Guef
	virtual void AddDeviceHealthHistory(int deviceID, const nlib::DateTime &timestamp, const GDeviceHealthReport::DeviceHealth &devHeath) = 0;
	virtual void UpdateDeviceHealth(int deviceID, const GDeviceHealthReport::DeviceHealth &devHeath) = 0;
	virtual void AddNeighbourHealthHistory(int deviceID, const nlib::DateTime &timestamp, int neighbID, const GNeighbourHealthReport::NeighbourHealth &neighbHealth) = 0;

	//added by Cristian.Guef
	virtual int CreateChannel(int deviceID, PublisherConf::COChannel &channel) = 0;
	virtual void DeleteChannel(int channelNo) = 0;
	virtual void DeleteAllChannels() = 0;
	virtual void GetOrderedChannels(int deviceID, std::vector<PublisherConf::COChannel> &channels) = 0;
	virtual void GetOrderedChannelsDevMACs(std::vector<MAC> &macs) = 0;
	virtual bool GetChannel(int channelNo, int &deviceID, PublisherConf::COChannel &channel) = 0;
	virtual void UpdateChannel(int channelNo, const std::string &channelName, const std::string &unitOfMeasure, int channelFormat, int withStatus) = 0;
	virtual bool FindDeviceChannel(int channelNo, DeviceChannel& channel) = 0;
	virtual bool IsDeviceChannel(int deviceID, const PublisherConf::COChannel &channel) = 0;
	virtual bool HasDeviceChannels(int deviceID) = 0;
	virtual void SetPublishErrorFlag(int deviceID, int flag) = 0;
	virtual void MoveChannelToHistory(int channelNo) = 0;
	virtual void MoveChannelsToHistory(int deviceID) = 0;

	//alert
	virtual bool GetAlertProvision(int &categoryProcessSub, int &categoryDeviceSub, int &categoryNetworkSub, int &categorySecuritySub) = 0;
	virtual void SaveAlertInfo(int devID, int TSAPID, int objID, const nlib::DateTime &timestamp,  int milisec,
		int classType, int direction, int category, int type, int priority, std::string &data) = 0;

	//channels statistics 
	virtual void DeleteChannelsStatistics(int deviceID) = 0;
	virtual void SaveChannelsStatistics(int deviceID, const std::string& strChannelsStatistics) = 0;

	//firmwaredownloads
	virtual void SetFirmDlStatus(int deviceID, int fwType, int status/*3-cancelled, 4-completed, 5-failed*/) = 0;
	virtual void SetFirmDlPercent(int deviceID, int fwType, int percent) = 0;
	virtual void SetFirmDlSize(int deviceID, int fwType, int size) = 0;
	virtual void SetFirmDlSpeed(int deviceID, int fwType, int speed) = 0;
	virtual void SetFirmDlAvgSpeed(int deviceID, int fwType, int speed) = 0;

};

} // namespace hostapp
} // namespace nisa100


#endif /*IDEVICESDAL_H_*/
