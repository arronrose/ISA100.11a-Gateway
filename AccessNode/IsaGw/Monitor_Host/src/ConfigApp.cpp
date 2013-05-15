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

#include "ConfigApp.h"
#include "Version.h"

#include <boost/program_options.hpp>
#include <boost/format.hpp>

//added by Cristian.Guef
#include "Log.h"
#include "Config.h"
#include "../AccessNode/Shared/AnPaths.h"

#ifndef HW_PC
# define MHOST_CONF	NIVIS_PROFILE "Monitor_Host.conf"
# define MHOST_LOG_INI	NIVIS_PROFILE "Monitor_Host_Log.ini"
# define MHOST_DB3 	NIVIS_TMP "Monitor_Host.db3"
# define MHOST_PUBLISH  NIVIS_PROFILE "Monitor_Host_Publisher.conf"
#else
#ifdef MINI_PC
# define MHOST_CONF	"etc/Monitor_Host.conf"
# define MHOST_LOG_INI	"etc/Monitor_Host_Log.ini"
# define MHOST_DB3 	"etc/Monitor_Host.db3"
# define MHOST_PUBLISH  "etc/Monitor_Host_Publisher.conf"
#else
# define MHOST_CONF	"/home/cristi/workspace/AccessNode/IsaGw/Monitor_Host/etc/Monitor_Host.conf"
# define MHOST_LOG_INI	"/home/cristi/workspace/AccessNode/IsaGw/Monitor_Host/etc/Monitor_Host_Log.ini"
# define MHOST_DB3 	"/home/cristi/workspace/AccessNode/IsaGw/Monitor_Host/etc/Monitor_Host.db3"
# define MHOST_PUBLISH  "/home/cristi/workspace/AccessNode/IsaGw/Monitor_Host/etc/Monitor_Host_Publisher.conf"
#endif
#endif


int history_status_size = 6;


namespace nisa100 {

ConfigApp::ConfigApp()
{
	
#ifdef VERSION
	if (!strcmp(VERSION, "local"))
		appVersion = M_H_VERSION;
	else
		appVersion = VERSION;
#else
		appVersion = M_H_VERSION;
#endif
	
	configFilePath	= MHOST_CONF;
	databasePath	= MHOST_DB3;
	logConfigPath	= MHOST_LOG_INI ;

	databaseServer	= "127.0.0.1";
	databaseName	= "Monitor_Host";
	databaseUser	= "root";
	databasePassword= "";

	databaseTimeout	= 40;

	gatewayHost	= "127.0.0.1";
	gatewayPort	= 4900;
	gatewayPacketVersion = 1;
	gatewayListenMode = false;

	threadCheckPeriod = 2;
	commandsTimeout	= 60;
	topologyPool	= 10;
	devicesListPool	= 10;
	readingsBulkSaverPeriod = 20;
	readingsBulkSaverMaxReadings = 1000;

	databaseVacuumPeriodMinutes = 0;
	databaseRemoveEntriesCheckPeriodMinutes = 30;
	databaseRemoveOlderEntriesThanMinutes = 30;
	pathToFirmwareFiles = "/usr/local/NISA/Data/DeviceFirmwares/";
	delayPeriodBeforeFirmwareRetry = 1;

	//added by Cristian.Guef
	strPubConfigFile    = MHOST_DB3 ;
	ScheduleReportPool  = 20;
	DeviceReportPool    = 20;
	NetworkReportPool   = 20;
	NeighbourReportPool = 20;
	ContractsAndRoutesPool = 20;

	runApplication	= true;
	processSignal2	= false;
	signal2Issued	= false;

	m_nDeviceReadingsHistoryEnable = 0;

	firmDlContractsPeriod = 10;
}


void ConfigApp::Init(int argc, char* argv[])
{
}

void ConfigApp::DoPrintHelp()
{
	printf( "Usage: MonitorHost [--config <path-to-config-file>] [--help]\n"
		"\tMonitor_Host daemon version %s\n"
		,appVersion
		);
}

void ConfigApp::Load()
{
	CConfigExt oConf ;
	if ( ! oConf.Load(configFilePath.c_str()) )
	{	char szLocal[ 128 ];
		snprintf( szLocal, sizeof(szLocal), "Unable to load config file[%s]\n", configFilePath.c_str());
		szLocal[sizeof(szLocal) - 1] = 0;
		THROW_EXCEPTION1(InvalidConfigAppException, szLocal );
	}
	oConf.GetVar( "LogConfigPath", logConfigPath, MHOST_LOG_INI );
	oConf.GetVar( "DatabasePath", databasePath,   MHOST_DB3 );
	oConf.GetVar( "DatabaseServer", databaseServer, "127.0.0.1" );
	oConf.GetVar( "DatabaseName", databaseName, "Monitor_Host" );
	oConf.GetVar( "DatabaseUser", databaseUser, "root" );
	oConf.GetVar( "DatabasePassword", databasePassword, "" );
	oConf.GetVar( "DatabaseTimeout", databaseTimeout, 10 );
	oConf.GetVar( "DatabaseVacuumPeriodMinutes", databaseVacuumPeriodMinutes, 30 );
	oConf.GetVar( "DatabaseRemoveEntriesCheckPeriodMinutes", databaseRemoveEntriesCheckPeriodMinutes, 10 );
	oConf.GetVar( "DatabaseRemoveEntriesOlderThanMinutes", databaseRemoveOlderEntriesThanMinutes, 30 );
	
	oConf.GetVar( "DatabaseRemoveAlarmEntriesOlderThanDays", databaseRemoveOlderAlarmEntriesThanDays , 4 );

	oConf.GetVar( "GatewayHost", gatewayHost, "127.0.0.1" );
	oConf.GetVar( "GatewayPort", gatewayPort, 4900 );
	oConf.GetVar( "GatewayListenMode", gatewayListenMode, false );
	oConf.GetVar( "GatewayPacketVersion", gatewayPacketVersion, 1 );
	oConf.GetVar( "CommandsTimeout", commandsTimeout, 60 );
	oConf.GetVar( "CommandsRetryCountIfTimeout", retryCountIfTimeout, 3 );
	oConf.GetVar( "TopologyPooling", topologyPool, 500 );
	oConf.GetVar( "DevicesListPooling", devicesListPool, 500 );
	oConf.GetVar( "firmDlContractsPooling", firmDlContractsPeriod, 30 );
	oConf.GetVar( "ScheduleReportPooling", ScheduleReportPool, 500 );
	oConf.GetVar( "DeviceHealthReportPooling", DeviceReportPool, 500 );
	oConf.GetVar( "NetworkHealthReportPooling", NetworkReportPool, 500 );
	oConf.GetVar( "NeighbourHealthReportPooling", NeighbourReportPool, 500 );
	oConf.GetVar( "ContractsAndRoutesPooling", ContractsAndRoutesPool, 500 );
	oConf.GetVar( "CommandsCheckPeriod", threadCheckPeriod, 2 );
	oConf.GetVar( "UseReadingsHistory", m_nDeviceReadingsHistoryEnable, 0 );
	oConf.GetVar( "ReadingsSavePeriod", readingsBulkSaverPeriod, 10 );
	oConf.GetVar( "ReadingsMaxEntriesBeforeSave", readingsBulkSaverMaxReadings, 10 );
	oConf.GetVar( "BulkDataTransferRate", bulkDataTransferRate, 85 );
	oConf.GetVar( "DelayPeriodBeforeFirmwareRetry", delayPeriodBeforeFirmwareRetry, 1 );
	oConf.GetVar( "PathToFiles", pathToFirmwareFiles, "/usr/local/NISA/Data/DeviceFirmwares/" );
	oConf.GetVar( "PubConfigPath",strPubConfigFile, "" );

	oConf.GetVar( "HistoryStatusSize", history_status_size, 6);

	//security
	oConf.GetVar( "UseEncryption", m_UseEncryption, 0 );
	oConf.GetVar( "ClientCertifFile", m_sslClientCertifFile, "/access_node/activity_files/clientcert.pem" );
	oConf.GetVar( "ClientKeyFile", m_sslClientKeyFile, "/access_node/activity_files/clientkey.pem" );
	oConf.GetVar( "CACertFile", m_sslCACertFile, "/access_node/activity_files/cacert.pem" );


	oConf.GetVar( "LeasePeriod", m_leasePeriod, 15*60 /* 15 minutes */);
	oConf.GetVar( "LeaseCommittedBurst", m_leaseCommittedBurst, 0 /*default for gw*/ );

	oConf.GetVar( "SavePublishPeriod", (int&)m_savePublishPeriod, 2000 /*msec default*/ );

	//added by Cristian.Guef
	if ( InitLogEnv(logConfigPath.c_str()) == false)
	{
		char szLocal[ 128 ];
		snprintf( szLocal, sizeof(szLocal), "Unable to load LOG config file[%s]\n", logConfigPath.c_str());
		szLocal[sizeof(szLocal) - 1] = 0;
		THROW_EXCEPTION1(InvalidConfigAppException,	szLocal);
	}
	
	//added by Cristian.Guef read publisher list -it is delayed now because there should be done alert_subscription
	//PublisherConf().LoadPublishers(strPubConfigFile.c_str(), PublishersMapStored);
	//processSignal2 = true;
}

//added by Cristian.Guef
void ConfigApp::ResetLeaseFlagForPub(nisa100::hostapp::MAC mac)
{
	PublisherConf::PublisherInfoMAP_T::iterator i = PublishersMapStored.find(mac);
	if (i != PublishersMapStored.end())
	{
		i->second.coData.HasLease = false;
		return;
	}
	
	LOG_WARN("device with mac=" <<  mac.ToString() << "hasn't been found in stored publishers map ");
		
}
void ConfigApp::LoadPublisherData()
{
	if (processSignal2 == true)
	{
		PublisherConf().LoadPublishers(strPubConfigFile.c_str(), PublishersMapLoaded);
		signal2Issued = true;
	}//processSignal2 = true;
}

void ConfigApp::LoadStoredPublisherData()
{
	PublisherConf().LoadPublishers(strPubConfigFile.c_str(), PublishersMapStored);
	processSignal2 = true;
}

}// namespace nisa100
