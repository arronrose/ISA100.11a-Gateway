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

#ifndef SQLITEDEVICESDAL_H_
#define SQLITEDEVICESDAL_H_

#include <Shared/SqliteUtil/Connection.h>

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../../Log.h"

#include "../IDevicesDal.h"

namespace nisa100 {
namespace hostapp {

class SqliteDevicesDal : public IDevicesDal
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.dal.SqliteDevicesDal");
	*/

public:
	SqliteDevicesDal(sqlitexx::Connection& connection);
	virtual ~SqliteDevicesDal();

public:
	void VerifyTables();

private:
	void ResetDevices(Device::DeviceStatus newStatus);
	void GetDevices(DeviceList& list);

	//added by Cristian.Guef
	void DeleteDevice(int id);

	//added by Cristian.Guef
	bool IsDeviceInDB(int deviceID);

	void AddDevice(Device& device);
	void UpdateDevice(Device& device);

	//added by Cristian.Guef
	void UpdateReadingTimeforDevice(int deviceID, const struct timeval &tv);

	/* commented by Cristian.Guef
	void CreateDeviceNeighbour(int fromDevice, int toDevice, int signalQuality);
	*/
	//added by Cristian.Guef
	void CreateDeviceNeighbour(int fromDevice, int toDevice, int signalQuality, int clockSource);

	void CleanDeviceNeighbours();
	void CreateDeviceGraph(int fromDevice, int toDevice, int graphID);
	void CleanDeviceGraphs();

	void AddReading(const DeviceReading& reading, bool p_bHistory = false);
	void AddEmptyReadings(int DeviceID);
	void AddEmptyReading(int DeviceID, int channelNo);
	void UpdateReading(const DeviceReading& reading);
	void DeleteReading(int p_nChannelNo);
	void DeleteReadings(int deviceID);
	bool IsDeviceChannelInReading(int deviceID, int channelNo);

	//added
	void UpdatePublishFlag(int devID, int flag/*0-no data, 1-fresh data, 2-stale data*/);

	void ResetPendingFirmwareUploads();
	bool GetNextFirmwareUpload(int notFailedForMinutes, FirmwareUpload& result);
	void UpdateFirmwareUpload(const FirmwareUpload& firmware);
	bool GetFirmwareUpload(boost::uint32_t id, FirmwareUpload& result);
	void GetFirmwareUploads(FirmwareUploadsT& firmwaresList);

	void AddDeviceConnections(int deviceID, const std::string& host, int port);
	void RemoveDeviceConnections(int deviceID);
	
	//added by Cristian.Guef
	void DeleteContracts(int deviceID);
	void DeleteContracts();
	void AddContract(int fromDevice, int toDevice, const ContractsAndRoutes::Contract &contract);

	//added by Cristian.Guef
	void DeleteRoutes(int deviceID);
	void AddRouteInfo(int deviceID, int routeID, int alternative, int fowardLimit, int selector, int srcAddr);
	void AddRouteLink(int deviceID, int routeID, int routeIndex, int graphID);
	void AddRouteLink(int deviceID, int routeID, int routeIndex, const DevicePtr neighbour);
	void RemoveContractElements(int sourceID);
	void RemoveContractElements();
	void AddContractElement(int contractID, int sourceID, int index, int deviceID);

	//added by Cristian.Guef
	void SaveISACSConfirm(int deviceID, ISACSInfo &confirm);

	//added by Cristian.Guef
	void AddRFChannel(const GScheduleReport::Channel &channel);
	void AddScheduleSuperframe(int deviceID, const GScheduleReport::Superframe &superFrame);
	void AddScheduleLink(int deviceID, int superframeID, const GScheduleReport::Link &link);
	void RemoveRFChannels();
	void RemoveScheduleSuperframes();
	void RemoveScheduleLinks();
	void RemoveScheduleSuperframes(int deviceID);
	void RemoveScheduleLinks(int deviceID);

	//added by Cristian.Guef
	void AddNetworkHealthInfo(const GNetworkHealthReport::NetworkHealth &netHealth);
	void AddNetHealthDevInfo(const GNetworkHealthReport::NetDeviceHealth &devHealth);
	void RemoveNetworkHealthInfo();
	void RemoveNetHealthDevInfo();

	//added by Cristian.Guef
	void AddDeviceHealthHistory(int deviceID, const nlib::DateTime &timestamp, const GDeviceHealthReport::DeviceHealth &devHeath);
	void UpdateDeviceHealth(int deviceID, const GDeviceHealthReport::DeviceHealth &devHeath);
	void AddNeighbourHealthHistory(int deviceID, const nlib::DateTime &timestamp, int neighbID, const GNeighbourHealthReport::NeighbourHealth &neighbHealth);

	//added by Cristian.Guef
	int CreateChannel(int deviceID, PublisherConf::COChannel &channel);
	void DeleteChannel(int channelNo);
	void DeleteAllChannels();
	void GetOrderedChannels(int deviceID, std::vector<PublisherConf::COChannel> &channels);
	void GetOrderedChannelsDevMACs(std::vector<MAC> &macs);
	bool GetChannel(int channelNo, int &deviceID, PublisherConf::COChannel &channel);
	void UpdateChannel(int channelNo, const std::string &channelName, const std::string &unitOfMeasure, int channelFormat, int withStatus);
	bool FindDeviceChannel(int channelNo, DeviceChannel& channel);
	bool IsDeviceChannel(int deviceID, const PublisherConf::COChannel &channel);
	bool HasDeviceChannels(int deviceID);
	void SetPublishErrorFlag(int deviceID, int flag);
	void MoveChannelToHistory(int channelNo);
	void MoveChannelsToHistory(int deviceID);

	//alert
	bool GetAlertProvision(int &categoryProcessSub, int &categoryDeviceSub, int &categoryNetworkSub, int &categorySecuritySub);
	void SaveAlertInfo(int devID, int TSAPID, int objID, const nlib::DateTime &timestamp, int milisec, 
		int classType, int direction, int category, int type, int priority, std::string &data);

	//channels statistics
	void DeleteChannelsStatistics(int deviceID);
	void SaveChannelsStatistics(int deviceID, const std::string& strChannelsStatistics);


	//firmwaredownloads
	void SetFirmDlStatus(int deviceID, int fwType, int status/*3-cancelled, 4-completed, 5-failed*/);
	void SetFirmDlPercent(int deviceID, int fwType, int percent);
	void SetFirmDlSize(int deviceID, int fwType, int size);
	void SetFirmDlSpeed(int deviceID, int fwType, int speed);
	void SetFirmDlAvgSpeed(int deviceID, int fwType, int speed);

private:
	sqlitexx::Connection& connection;

	sqlitexx::CSqliteStmtHelper m_oUpdateDeviceReadings_Prepare;
	sqlitexx::CSqliteStmtHelper m_oAddDeviceReadings_compiled;
	sqlitexx::CSqliteStmtHelper m_oUpdatePublishFlag_compiled;
	sqlitexx::CSqliteStmtHelper m_oUpdateFirmDl_compiled_status;
	sqlitexx::CSqliteStmtHelper m_oUpdateFirmDl_compiled_percent;
	sqlitexx::CSqliteStmtHelper m_oUpdateFirmDl_compiled_size;
	sqlitexx::CSqliteStmtHelper m_oUpdateFirmDl_compiled_speed;
	sqlitexx::CSqliteStmtHelper m_oUpdateFirmDl_compiled_avgspeed;
	sqlitexx::CSqliteStmtHelper m_oUpdateFirmDl_compiled_completed;
};

}
}

#endif /*SQLITEDEVICESDAL_H_*/
