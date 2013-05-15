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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <nlib/exception.h>
#include <nlib/datetime.h>
#include <boost/cstdint.hpp>


//added by Cristian.Guef
#include "./PublisherConf.h"

namespace nisa100 {

class InvalidConfigAppException : public nlib::Exception
{
public:
	InvalidConfigAppException(const std::string& message) :
		nlib::Exception(message)
	{
	}
};

class ConfigApp
{
public:
	ConfigApp();

	void Init(int argc, char* argv[]);

	void DoPrintHelp();
	void Load();

private:
	const char* appVersion;
	std::string configFilePath;
	std::string logConfigPath;
	std::string databasePath;

	std::string databaseServer;
	std::string databaseName;
	std::string databaseUser;
	std::string databasePassword;

	int databaseTimeout;

	std::string gatewayHost;
	int gatewayPort;
	int gatewayPacketVersion;
	bool gatewayListenMode;

	int threadCheckPeriod;
	int commandsTimeout;
	int topologyPool;
	int devicesListPool;
	int firmDlContractsPeriod;
	int readingsBulkSaverPeriod;
	int readingsBulkSaverMaxReadings;

	int databaseVacuumPeriodMinutes;
	int databaseRemoveEntriesCheckPeriodMinutes;
	int databaseRemoveOlderEntriesThanMinutes;
	int databaseRemoveOlderAlarmEntriesThanDays;

	std::string pathToFirmwareFiles;
	int delayPeriodBeforeFirmwareRetry;
	int bulkDataTransferRate;
	int retryCountIfTimeout;

	//added by Cristian.Guef
	std::string strPubConfigFile;
	int ScheduleReportPool;
	int DeviceReportPool;
	int NetworkReportPool;
	int NeighbourReportPool;
	int ContractsAndRoutesPool;

	int m_nDeviceReadingsHistoryEnable;

	int m_UseEncryption;
	std::string m_sslClientCertifFile;
	std::string m_sslClientKeyFile;
	std::string m_sslCACertFile;

	//for contracts
	int	m_leasePeriod;         /*sec*/
	int	m_leaseCommittedBurst;  /*see doc*/

	//for publishing separate flow
	unsigned int m_savePublishPeriod; /*msec*/

public:
	bool runApplication;

public:
	const std::string ApplicationVersion() const { return appVersion; }
	const std::string ConfigurationFilePath() const { return configFilePath; }
	const std::string LogConfigurationFilePath() const { return logConfigPath; }
	const std::string DatabaseFilePath() const { return databasePath ; }

	const std::string DatabaseServer() const { return databaseServer; }
	const std::string DatabaseName() const { return databaseName; }
	const std::string DatabaseUser() const { return databaseUser; }
	const std::string DatabasePassword() const { return databasePassword; }

	const int DatabaseTimeout() const { return databaseTimeout ; }

	const std::string GatewayHost() const { return gatewayHost; }
	const int GatewayPort() const { return gatewayPort; }
	const int GatewayPacketVersion() const { return gatewayPacketVersion; }
	const bool GatewayListenMode() const { return gatewayListenMode; }

	const int InternalTasksPeriod() const { return threadCheckPeriod; }
	int	IsDeviceReadingsHistoryEnable() const { return m_nDeviceReadingsHistoryEnable; }

	const nlib::TimeSpan CommandsTimeout() const { return nlib::util::seconds(commandsTimeout); }
	const nlib::TimeSpan ReadingsSaverPeriod() const { return nlib::util::seconds(readingsBulkSaverPeriod); }
	const int ReadingsMaxEntriesSaver() const { return readingsBulkSaverMaxReadings; }

	const nlib::TimeSpan TopologyPeriod() const {return nlib::util::seconds(topologyPool);}

	const nlib::TimeSpan DevicesListPeriod() const {return nlib::util::seconds(devicesListPool);}
	const nlib::TimeSpan FirmDlContractsPeriod() const {return nlib::util::seconds(firmDlContractsPeriod);}

	//added by Cristian.Guef
	const nlib::TimeSpan ScheduleReportPeriod() const { return nlib::util::seconds(ScheduleReportPool);}
	const nlib::TimeSpan DeviceReportPeriod() const   { return nlib::util::seconds(DeviceReportPool); }
	const nlib::TimeSpan NetworkReportPeriod() const  { return nlib::util::seconds(NetworkReportPool); }
	const nlib::TimeSpan NeighbourReportPeriod() const{ return nlib::util::seconds(NeighbourReportPool); }
	const nlib::TimeSpan ContractsAndRoutesPeriod() const{ return nlib::util::seconds(ContractsAndRoutesPool); }

	const nlib::TimeSpan DatabaseVacuumPeriod() const { return nlib::util::minutes(databaseVacuumPeriodMinutes); }
	const nlib::TimeSpan DatabaseRemoveEntriesPeriod() const { return nlib::util::minutes(databaseRemoveEntriesCheckPeriodMinutes); }
	const nlib::TimeSpan DatabaseRemoveEntriesOlderThan() const { return nlib::util::minutes(databaseRemoveOlderEntriesThanMinutes);}
	const nlib::TimeSpan DatabaseRemoveAlarmEntriesOlderThan() const { return nlib::util::hours(databaseRemoveOlderAlarmEntriesThanDays*24);}

	const std::string PathToFirmwareFiles() const { return pathToFirmwareFiles; }
	const int DelayPeriodBeforeFirmwareRetry() const { return delayPeriodBeforeFirmwareRetry; }
	const int BulkDataTransferRate()const { return bulkDataTransferRate; }

	const int SocketReconnectTimeout() const { return 3;/*seconds*/ }

	const int DeviceLinks_MaxRetryCounter() const { return 5; }
	const int DeviceLinks_MaxContractRetryCounter() const { return 5; }
	const nlib::TimeSpan DeviceLinks_MaxSleepTillRetry() const	{	return nlib::util::seconds(20);	}
	const int RetryCountIfTimeout() const { return retryCountIfTimeout; }
	const int ConnectToDatabaseRetryInterval() const { return 60; } //seconds

	//added
	const int UseEncryption() const { return m_UseEncryption; }
	const std::string SSLClientCertifFile() const { return m_sslClientCertifFile; }
	const std::string SSLClientKeyFile() const { return m_sslClientKeyFile; }
	const std::string SSLCACertFile() const { return m_sslCACertFile; }
	
	//lease
	const unsigned int LeasePeriod() const { return m_leasePeriod; }
	const short LeaseCommittedBurst() const { return m_leaseCommittedBurst; }

	//for publishing separate flow
	unsigned int SavePublishPeriod() const { return m_savePublishPeriod; }

//added by cristian.Guef
	void ResetLeaseFlagForPub(nisa100::hostapp::MAC mac);
	PublisherConf::PublisherInfoMAP_T   PublishersMapStored;
	PublisherConf::PublisherInfoMAP_T	PublishersMapLoaded;
	bool processSignal2;
	bool signal2Issued;
	void LoadPublisherData();
	void LoadStoredPublisherData();

};

} //namespace nisa100

#endif /*SETTINGS_H_*/
