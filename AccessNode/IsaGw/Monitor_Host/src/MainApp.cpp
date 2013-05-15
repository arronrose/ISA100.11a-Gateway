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

#include "MainApp.h"

#include <string>
#include <exception>
#include <signal.h>

#include <boost/bind.hpp> //for binding to function
#include "util/process/Signals.h"
#include <../AccessNode/Shared/app.h>
#include <../AccessNode/Shared/log.h>
#include <../AccessNode/Shared/SignalsMgr.h>
#include "hostapp/dal/sqlite/SqliteFactoryDal.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

//added by Cristian.Guef
#ifndef HW_VR00
#include "hostapp/dal/mysql/MySqlFactoryDal.h"
#endif

#include "hostapp/dal/memory/MemoryFactoryDal.h"

#include "hostapp/CommandsManager.h"
#include "hostapp/DevicesManager.h"
#include "hostapp/CommandsProcessor.h"
#include "hostapp/TrackingManager.h"
#include "gateway/GChannel.h"

#include "hostapp/LeaseTrackingMng.h"

#include "hostapp/periodictasks/GetTopologyGenerator.h"
#include "hostapp/periodictasks/GetDeviceList.h"
#include "hostapp/periodictasks/DeviceReadingsBulkSaver.h"
#include "hostapp/periodictasks/DatabaseVacuum.h"
#include "hostapp/periodictasks/FirmwareUploadManager.h"
#include "hostapp/periodictasks/EnsureSystemManagerContract.h"

//added by Cristian.Guef
#include "hostapp/periodictasks/DoPreconfigureSubscriberLease.h"
#include "hostapp/periodictasks/DoDeleteLeases.h"
#include "hostapp/periodictasks/ContractsAndRoutesManager.h"
#include "hostapp/periodictasks/ScheduleReportManager.h"
#include "hostapp/periodictasks/DeviceReportManager.h"
#include "hostapp/periodictasks/NetworkReportManager.h"
#include "hostapp/periodictasks/NeighbourReportManager.h"
#include "hostapp/periodictasks/AlertGenerator.h"
#include "hostapp/periodictasks/GetFirmDlContracts.h"


#include "hostapp/CommandsThread.h"

#include "util/process/Signals.h"
#include "hostapp/model/MAC.h"
#include "hostapp/model/IPv6.h"



namespace nisa100 {
namespace detail {

void testGChannel();

} // namespace detail


int MainApp::Run(int argc, char* argv[])
{
	//test for instances
		char szLockFile[] = "/tmp/MonitorHost.flock";
		int m_nSyncFd = open( szLockFile, O_RDWR | O_CREAT, 0666 );
		if( flock( m_nSyncFd, LOCK_EX | LOCK_NB ) )
		{
			//LOG_ERROR( "Process MonitorHost try to start but another instance of program is running ");
			return 0;
		}

	int resultCode = 0;
	try
	{
		g_stLog.OpenStdout();
		config.Init(argc, argv);
		if (!config.runApplication)
		{
			//nicu.dascalu - do not start application (maybe just display help info)
			config.DoPrintHelp();
			return resultCode;
		}

		LoadConfig(false);

		LOG_INFO("Starting Monitor_Host(version=" << config.ApplicationVersion() << ") ...");

#ifdef USE_SQLITE_DATABASE
		hostapp::SqliteFactoryDal factoryDal;
		factoryDal.Open(config.DatabaseFilePath(), config.DatabaseTimeout());
#elif USE_MYSQL_DATABASE
		hostapp::MySqlFactoryDal factoryDal;
		bool connectedToDatabase = false;
		while(!connectedToDatabase)
		{
			try
			{
				factoryDal.Open(config.DatabaseServer(), config.DatabaseUser(), config.DatabasePassword(), config.DatabaseName());
				connectedToDatabase = true;
			}
			catch(std::exception& ex)
			{
				LOG_ERROR("Cannot connect to the database. Error=" << ex.what());
				LOG_DEBUG("Retry to reconnect to the database in " << config.ConnectToDatabaseRetryInterval() << " seconds");
				sleep(config.ConnectToDatabaseRetryInterval());
			}
			catch(...)
			{
				LOG_ERROR("Unknown exception occured when trying to connect to the database!");
				LOG_DEBUG("Retry to reconnect to the database in " << config.ConnectToDatabaseRetryInterval() << " seconds");
				sleep(config.ConnectToDatabaseRetryInterval());
			}
		}
#else
		hostapp::MemoryFactoryDal factoryDal;
#endif

		

		hostapp::DevicesManager devices(factoryDal, config );

		//added by Cristian.Guef
		hostapp::CommandsManager commands(factoryDal, devices);

		hostapp::LeaseTrackingMng leaseTrackMng(commands, devices);

		hostapp::CommandsProcessor processor(commands, devices, factoryDal, config, leaseTrackMng);
		hostapp::TrackingManager tracking;

		//added by Cristian.Guef
		commands.processResponse = boost::bind(&hostapp::CommandsProcessor::ProcessResponses, &processor);
		commands.processTimedout = boost::bind(&hostapp::TrackingManager::CheckTimeoutRequests, &tracking);
		
		gateway::GChannel gateway(config);

		//added by Cristian.Guef
		processor.GatewayRun = boost::bind(&gateway::GChannel::Run, &gateway, _1);
		commands.GatewayRun = boost::bind(&gateway::GChannel::Run, &gateway, _1);

		hostapp::GetTopologyGenerator topologyGenerator(commands, devices, config);
		
		//added
		hostapp::GetDeviceList getDeviceList(commands, devices, config);
		hostapp::GetFirmDlContracts getFirmDLContracts(commands, devices, config);
		
		hostapp::FirmwareUploadManager firmwareManager(commands, devices, config);
		hostapp::EnsureSystemManagerContract ensureSMContract(commands, devices);

		//added by Cristian.Guef
		hostapp::DoDeleteLeases doDeleteLeases(commands, devices);
		hostapp::DoPreconfigureSubscriberLease_ doSubscriberLease(commands, devices, config, doDeleteLeases);

		//added by Cristian.Guef
		hostapp::ContractsAndRoutesManager contractsAndRoutesManager(commands, devices, config);
		hostapp::ScheduleReportManager scheduleRepMng(commands, devices, config);
		hostapp::DeviceReportManager deviceRepMng(commands, devices, config);
		hostapp::NetworkReportManager networkRepMng(commands, devices, config);
		hostapp::NeighbourReportManager neighbourRepMng(commands, devices, config);
		hostapp::AlertGenerator alertGenerator(commands, devices);

		hostapp::DatabaseVacuum vacuum(factoryDal, config, devices);
		hostapp::DeviceReadingsBulkSaver bulkSaver(devices, config);

		gateway.Connected.push_back(boost::bind(&hostapp::DevicesManager::HandleGWConnect, &devices, _1, _2));
		gateway.Disconnected.push_back(boost::bind(&hostapp::DevicesManager::HandleGWDisconnect, &devices));

		gateway.Connected.push_back(boost::bind(&hostapp::AlertGenerator::HandleGWConnect, &alertGenerator, _1, _2));
		gateway.Disconnected.push_back(boost::bind(&hostapp::AlertGenerator::HandleGWDisconnect, &alertGenerator));

		gateway.Disconnected.push_back(boost::bind(&hostapp::GetFirmDlContracts::HandleGWDisconnect, &getFirmDLContracts));

		//BEGIN COMMANDS FLOW

		//db(requests) -> processor
		commands.NewCommand = boost::bind(&hostapp::CommandsProcessor::ProcessRequest, &processor, _1);

		//added by Cristian.Guef
		gateway.Connected.push_back(boost::bind(&hostapp::CommandsManager::HandleGWConnect, &commands, _1, _2));
		gateway.Disconnected.push_back(boost::bind(&hostapp::CommandsManager::HandleGWDisconnect, &commands));


		//processor -> tracking
		processor.SendRequestCallback = boost::bind(&hostapp::TrackingManager::SendRequest, &tracking, _1, _2, _3);

		processor.RegisterPublishIndication = boost::bind(&hostapp::TrackingManager::RegisterPublishIndication,
				&tracking, _1, _2, _3, _4);
		processor.CancelPublishIndication = boost::bind(&hostapp::TrackingManager::UnregisterPublishIndication,
				&tracking, _1, _2);

		//added by Cristian.Guef
		processor.CancelPublishIndication2 = boost::bind(&hostapp::TrackingManager::UnregisterPublishIndication2,
				&tracking, _1, _2, _3);

		//added by Cristian.Guef
		processor.RegisterAlertIndication = boost::bind(&hostapp::TrackingManager::RegisterAlertIndication, &tracking, _1);
		processor.UnregisterAlertIndication = boost::bind(&hostapp::TrackingManager::UnregisterAlertIndication, &tracking);

		processor.CancelPublishIndications = boost::bind(&hostapp::TrackingManager::UnregisterPublishIndications,
				&tracking, _1);
//		processor.AddDeviceReading = boost::bind(&hostapp::DeviceReadingsBulkSaver::AddReading, &bulkSaver, _1);

		//added by Cristian.Guef
		processor.AddNewInfoForDelLease = boost::bind(&hostapp::DoDeleteLeases::AddNewInfoForDelLease,
								&doDeleteLeases, _1, _2);
		processor.GetInfoForDelLease = boost::bind(&hostapp::DoDeleteLeases::GetInfoForDelLease,
								&doDeleteLeases, _1);
		processor.DelInfoForDelLease = boost::bind(&hostapp::DoDeleteLeases::DelInfoForDelLease,
								&doDeleteLeases, _1);
		processor.SetDeleteContractID_NotSent = boost::bind(&hostapp::DoDeleteLeases::SetDeleteContractID_NotSent,
								&doDeleteLeases, _1);

		//tracking -> channel
		tracking.SendPacket = boost::bind(&gateway::GChannel::SendPacket, &gateway, _1);

		//channel -> tracking
		gateway.ReceivePacket = boost::bind(&hostapp::TrackingManager::ReceivePacket, &tracking, _1);

		//channel.Disconnected -> tracking.HandleDisconenct
		gateway.Disconnected.push_back(boost::bind(&hostapp::TrackingManager::HandleDisconenct, &tracking));

		//tracking -> processor
		/*commented by Cristian.Guef
		tracking.ReceiveResponse = boost::bind(&hostapp::CommandsProcessor::ProcessResponse, &processor, _1);
		*/
		//added by Cristian.Guef
		tracking.ReceiveResponse = boost::bind(&hostapp::CommandsProcessor::ProcessResponse, &processor, _1, _2);
		
		//processor -> db(response|readings) ??

		//END COMMANDS FLOW

		
		hostapp::CommandsThread commandsThread(processor, config.InternalTasksPeriod(),config);

		//register automatic tasks
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::CommandsManager::ProcessRequests, &commands));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::TrackingManager::CheckTimeoutRequests, &tracking));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::LeaseTrackingMng::CheckLeaseTimedOut, &leaseTrackMng));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::EnsureSystemManagerContract::Check, &ensureSMContract));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::GetTopologyGenerator::Check, &topologyGenerator));
		
		//added
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::GetDeviceList::Check, &getDeviceList));
		
		//it was added a separate flow
		//commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::DeviceReadingsBulkSaver::CheckForSave, &bulkSaver));
		processor.m_SaveReadings = boost::bind(&hostapp::DeviceReadingsBulkSaver::SaveReadingsWithoutSignal, &bulkSaver);

		//added by Cristian.Guef
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::DoDeleteLeases::Check, &doDeleteLeases));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::DoPreconfigureSubscriberLease_::Check, &doSubscriberLease));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::ContractsAndRoutesManager::Check, &contractsAndRoutesManager));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::ScheduleReportManager::Check, &scheduleRepMng));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::DeviceReportManager::Check, &deviceRepMng));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::NetworkReportManager::Check, &networkRepMng));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::NeighbourReportManager::Check, &neighbourRepMng));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::AlertGenerator::Check, &alertGenerator));


		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::FirmwareUploadManager::Check, &firmwareManager));


		//added by Cristian.Guef
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::DatabaseVacuum::Vacuum, &vacuum));
		commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::GetFirmDlContracts::Check, &getFirmDLContracts));


		
		//commandsThread.RegisterPeriodicTask(boost::bind(&MainApp::RunStep, this,commandsThread));
		//end registration

		gateway.Start();

		/* commented by Cristian.Guef
		boost::thread_group threadGroup;
		threadGroup.create_thread(
				detail::ExceptionHandler("GW", boost::bind(&gateway::GChannel::Run, &gateway)));
		*/
		LOG_INFO("Started (all threads), waiting for SIGINT to stop...");

		//added by Cristian.Guef
		//util::Signals signals;
		//signals.RegisterSignalHandler(SIGTERM, boost::bind(&hostapp::CommandsThread::Stop, &commandsThread));
		//signals.RegisterSignalHandler(SIGHUP, boost::bind(&nisa100::ConfigApp::LoadPublisherData, &config));

		//for publishing separate flow
		//signals.RegisterSignalHandler(SIGALRM, boost::bind(&hostapp::DeviceReadingsBulkSaver::SaveReadingsWithSignal, &bulkSaver));
		//signals.RegisterSignalHandler(SIGVTALRM, boost::bind(&hostapp::DeviceReadingsBulkSaver::SaveReadingsWithSignal, &bulkSaver));
		//bulkSaver.SetConfigAlarmTimer();
		//commandsThread.RegisterPeriodicTask(boost::bind(&hostapp::DeviceReadingsBulkSaver::SaveReadingsWithoutSignal, &bulkSaver));
		
		
		//signals.Ignore(SIGPIPE);
		
		///NEVER USE CSignalsMgr TO HANDLE SIGABRT/SIGSEGV

		/* commented by Cristian.Guef -not in prerelease
		util::Signals signals;
		signals.RegisterSignalHandler(SIGINT, boost::bind(&hostapp::CommandsThread::Stop, &commandsThread));
		signals.RegisterSignalHandler(SIGHUP, boost::bind(&MainApp::ReloadConfig, this));
		*/

		commandsThread.Run();
		LOG_INFO("Stopped commands thread ...");

		gateway.Stop();
		LOG_INFO("Stopped gateway thread ...");

		/* commented by Cristian.Guef
		threadGroup.interrupt_all();
		threadGroup.join_all(); // won't join throw interrupted_exception ?
		*/

		LOG_INFO("Stopped (all threads) ...");

		resultCode = 0;
	}
	catch(InvalidConfigAppException& ex)
	{
		//nicu.dascalu - the log is not yet initializated
		//LOG_ERROR(ex.Message() << std::endl << " exception=" << ex.what());
		std::cout << "FATAL: " << ex.Message() << std::endl << " exception=" << ex.what();
		resultCode = 3;
	}
	catch (gateway::ChannelException& ex)
	{
		LOG_ERROR("Unable to create communications channel. Reason=" << ex.Message());
		resultCode = 3;
	}
	catch(std::exception& ex)
	{
		LOG_ERROR("Unhandled exception=" << ex.what() << " in MainApp::Run");
		resultCode = 1;
	}
	catch(...)
	{
		LOG_ERROR("Unknown exception in MainApp::Run");
		resultCode = 2;
	}

	LOG_INFO("Stopped.");
	return resultCode;
}

void MainApp::RunStep(hostapp::CommandsThread& p_rCommandsThread)
{

	//signals.RegisterSignalHandler(SIGTERM, boost::bind(&hostapp::CommandsThread::Stop, &commandsThread));
	//signals.RegisterSignalHandler(SIGHUP, boost::bind(&nisa100::ConfigApp::LoadPublisherData, &config));


	//if (CSignalsMgr::IsRaised(SIGTERM))
	//{
	//	//p_rCommandsThread.Stop();
	//}

}

void MainApp::LoadConfig(bool reloadConfiguration)
{
	config.Load();
	if (!reloadConfiguration)
	{
		/* commented by Cristian.Guef
		LOG_INIT(config.LogConfigurationFilePath());
		*/
	}
	else
	{
		/* commented by Cristian.Guef
		LOG_REINIT(config.LogConfigurationFilePath());
		*/
	}
	CSignalsMgr::Install(SIGTERM);
	CSignalsMgr::Install(SIGHUP);
	CSignalsMgr::Install(SIGUSR2);
	CSignalsMgr::Ignore(SIGPIPE);


#ifndef HW_PC
	signal( SIGABRT, HandlerFATAL );/// do NOT use delayed processing with HandlerFATAL. Stack trace must be dumped on event
	signal( SIGSEGV, HandlerFATAL );/// do NOT use delayed processing with HandlerFATAL. Stack trace must be dumped on event
	signal( SIGFPE,  HandlerFATAL );/// do NOT use delayed processing with HandlerFATAL. Stack trace must be dumped on event
#endif



	LOG_INFO("Configuration loaded");
	LOG_INFO("" << boost::str(boost::format("Configuration file: '%1%' parsed successfull!"
							" DatabasePath='%2%', DatabaseTimeout='%3%'"
							" DatabaseVacuumPeriod:%4%, DatabaseRemoveEntriesCheckPeriod:%5%, DatabaseRemoveOlderEntriesThan:%6%,"
							" Gateway=%7%:%8% (ListenMode=%9%, PacketVersion=%10%),"
							" InternalCheckPeriod:%11%,"
							" CommandTimeout:%12%,"
							" TopologyPooling:%13%,") % config.ConfigurationFilePath() % config.DatabaseFilePath()
					% config.DatabaseTimeout() % nlib::ToString(config.DatabaseVacuumPeriod())
					% nlib::ToString(config.DatabaseRemoveEntriesPeriod()) % nlib::ToString(config.DatabaseRemoveEntriesOlderThan())
					% config.GatewayHost() % config.GatewayPort() % config.GatewayListenMode() % config.GatewayPacketVersion()
					% config.InternalTasksPeriod() % nlib::ToString(config.CommandsTimeout())
					% nlib::ToString(config.TopologyPeriod())));

}

void MainApp::ReloadConfig()
{
	try
	{
		LOG_DEBUG("Configuration reloading ...");
		LoadConfig(true);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Failed to reload config. error=" << ex.what());
	}
	catch (...)
	{
		LOG_ERROR("Failed to reload config. unknown error.");
	}
}

}

