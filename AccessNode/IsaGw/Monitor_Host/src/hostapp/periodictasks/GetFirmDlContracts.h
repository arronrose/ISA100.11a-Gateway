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

#ifndef GETFIRMDLCONTRACTS_H_
#define GETFIRMDLCONTRACTS_H_

#include "../CommandsManager.h"
#include "../DevicesManager.h"
#include "../../ConfigApp.h"

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../Log.h"



namespace nisa100 {
namespace hostapp {


//added by Cristian.Guef
extern bool IsGatewayReconnected;

extern bool GetContractAndRoutesFlag();
extern void ResetContractAndRoutesFlag();
extern bool GetContractAndRoutesNowFlag();
extern void ResetContractAndRoutesNowFlag();
extern void ResetFirmDlInProgress();

/**
 * Generates get device_list commands with specified granularity.
 */
class GetFirmDlContracts
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.GetTopologyGenerator");
	*/

public:
	GetFirmDlContracts(CommandsManager& commands_, DevicesManager& devices_, const ConfigApp& configApp_) :
		commands(commands_), devices(devices_), configApp(configApp_)
	{		
		//added 
		granularity = configApp.FirmDlContractsPeriod();
		
		lastAutomaticCommandTime = nlib::CurrentUniversalTime() - configApp.FirmDlContractsPeriod();
		LOG_INFO("ctor: GetFirmDlContracts initialized");
	}

	void Check()
	{
		if (!devices.GatewayConnected())
			return; // no reason to send commands to a disconnected GW

		//added by Cristian.Guef
		if (IsGatewayReconnected == true)
			return; // no reason to send commands without a session created

		if (granularity <= nlib::NOTIME)
			return; // disabled

		try
		{
			DevicePtr systemManager = devices.SystemManagerDevice();
			/* commented by Cristian.Guef
			if (!systemManager || !systemManager->HasContract())
			*/
			//added by Cristian.Guef
			unsigned short TLDE_SAPID = 1/*apUAP1*/;
			unsigned short ObjID = 5/*SYSTEM_MONITORING_OBJECT*/;
			unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
			if (!systemManager || !systemManager->HasContract(resourceID, 0/*clientleasetype*/))
			{
				LOG_WARN("GetFirmDlContracts: SM not found or has no lease... Skipping check...");
				return;
			}


			/*
			while (devices.rejoinedDevices.size() > 0)
			{
				DevicePtr devicePtr = devices.FindDevice(devices.rejoinedDevices.front());
				if (!devicePtr)
				{
					LOG_WARN(" device_mac = " << devices.rejoinedDevices.front().ToString().c_str() << "not found in cache... Skipping check...");
				}
				else
				{
					IssueGetContractsAndRoutesCommand(*devicePtr);
				}

				devices.rejoinedDevices.pop();
			}
			*/
			nlib::DateTime currentTime = nlib::CurrentUniversalTime();
			if ((currentTime> lastAutomaticCommandTime + granularity) || GetContractAndRoutesNowFlag())
			{

				ResetContractAndRoutesNowFlag();
				if (GetContractAndRoutesFlag())
				{
					IssueGetContractsAndRoutesCommand();
				}
				
				lastAutomaticCommandTime = currentTime;
			}
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("Check: GetFirmDlContracts -> Getting Contracts and Routes failed! error=" << ex.what());
		}
	}

	void HandleGWDisconnect()
	{
		ResetContractAndRoutesFlag();
		ResetContractAndRoutesNowFlag();
		ResetFirmDlInProgress();
	}

private:
	void IssueGetContractsAndRoutesCommand()
	{
		Command getContractsAndRoutes;
		getContractsAndRoutes.commandCode = Command::ccGetContractsAndRoutes;
		getContractsAndRoutes.deviceID = devices.SystemManagerDevice()->id;
		getContractsAndRoutes.generatedType = Command::cgtAutomatic;
		getContractsAndRoutes.isFirmwareDownload = true;
		commands.CreateCommand(getContractsAndRoutes, boost::str(boost::format("system: get contracts and routes")));

		LOG_INFO("GetFirmDlContracts -> Automatic Get ContractsAndRoutes request was made!");
	}

private:
	CommandsManager& commands;
	DevicesManager& devices;
	const ConfigApp& configApp;

	nlib::DateTime lastAutomaticCommandTime;
	
	//added
	nlib::TimeSpan granularity;
};

} //namespace hostapp
} //namespace nisa100

#endif /*GETTOPOLOGYGENERATOR_H_*/
