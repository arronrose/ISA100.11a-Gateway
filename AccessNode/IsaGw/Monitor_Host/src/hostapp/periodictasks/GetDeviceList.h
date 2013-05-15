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

#ifndef GETDEVICELIST_H_
#define GETDEVICELIST_H_

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


/**
 * Generates get device_list commands with specified granularity.
 */
class GetDeviceList
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.GetTopologyGenerator");
	*/

public:
	GetDeviceList(CommandsManager& commands_, DevicesManager& devices_, const ConfigApp& configApp_) :
		commands(commands_), devices(devices_), configApp(configApp_)
	{		
		devices.ResetTopology();
		//added 
		granularity = configApp.DevicesListPeriod();
		
		lastAutomaticCommandTime = nlib::CurrentUniversalTime() - configApp.DevicesListPeriod();
		LOG_INFO("ctor: GetDeviceList initialized");
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
			nlib::DateTime currentTime = nlib::CurrentUniversalTime();

			if (currentTime> lastAutomaticCommandTime + granularity)
			{
				CreateDeviceListCommand();

				lastAutomaticCommandTime = currentTime;
			}
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Check: Unhandled exception when generate automatic requests! Error:" << ex.what());
		}
	}	
private:
	void CreateDeviceListCommand()
	{
		try
		{
			Command devListCommand;
			devListCommand.commandCode = Command::ccGetDeviceList;
			devListCommand.deviceID = devices.GatewayDevice()->id;
			devListCommand.generatedType = Command::cgtAutomatic;
			commands.CreateCommand(devListCommand, "system: device_list scheduler");
			LOG_INFO("Automatic device_list request was made!");
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Error when generate automatic device_list request! Error:" << ex.what());
		}
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
