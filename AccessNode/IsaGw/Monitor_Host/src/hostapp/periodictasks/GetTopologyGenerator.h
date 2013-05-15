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

#ifndef GETTOPOLOGYGENERATOR_H_
#define GETTOPOLOGYGENERATOR_H_

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
 * Generates get topology commands with specified granularity.
 */
class GetTopologyGenerator
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.GetTopologyGenerator");
	*/

public:
	GetTopologyGenerator(CommandsManager& commands_, DevicesManager& devices_, ConfigApp& configApp_) :
		commands(commands_), devices(devices_), configApp(configApp_)
	{		
		devices.ResetTopology();
		//devices.ResetDeviceChannels();
		//added 
		granularity = configApp.TopologyPeriod();
		
		lastAutomaticCommandTime = nlib::CurrentUniversalTime() - configApp.TopologyPeriod();
		LOG_INFO("ctor: TopologyGenerator initialized");
		
		m_loadPublishersOnce = true;
	}

	void Check()
	{
		/* commented 
		nlib::TimeSpan granularity = configApp.TopologyPeriod();
		*/

		if (!devices.GatewayConnected())
			return; // no reason to send commands to a disconnected GW

		//added by Cristian.Guef
		if (IsGatewayReconnected == true)
			return; // no reason to send commands without a session created

		if (granularity <= nlib::NOTIME)
			return; // disabled


		//send topology command when gw has made alert subscription for device_join/leave
		DevicePtr smDev = devices.SystemManagerDevice();
		DevicePtr nullDev;
		DevicePtr gwDev = devices.GatewayDevice();
		if (!gwDev->HasMadeAlertSubscription() && smDev != nullDev)
		{
			LOG_INFO("TOPOLOGY_TASK: gw hasn't made alert_subscription, so skip topology request...");
			return;
		}

		//it is done once
		if (gwDev->HasMadeAlertSubscription() && smDev != nullDev && m_loadPublishersOnce)
		{
			configApp.LoadStoredPublisherData();
			devices.ResetDeviceChannels();
			m_loadPublishersOnce = false;
		}

		try
		{
			nlib::DateTime currentTime = nlib::CurrentUniversalTime();

			if (currentTime> lastAutomaticCommandTime + granularity)
			{
				CreateTopologyCommand();

				lastAutomaticCommandTime = currentTime;
			}
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Check: Unhandled exception when generate automatic requests! Error:" << ex.what());
		}
	}	
private:
	void CreateTopologyCommand()
	{
		try
		{
			Command topologyCommand;
			topologyCommand.commandCode = Command::ccGetTopology;
			topologyCommand.deviceID = devices.GatewayDevice()->id;
			topologyCommand.generatedType = Command::cgtAutomatic;
			commands.CreateCommand(topologyCommand, "system: topolology scheduler");
			LOG_INFO("Automatic Topology request was made!");
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Error when generate automatic topology request! Error:" << ex.what());
		}
	}

private:
	CommandsManager& commands;
	DevicesManager& devices;
	ConfigApp& configApp;
	bool 		m_loadPublishersOnce;

	nlib::DateTime lastAutomaticCommandTime;
	
	//added
	nlib::TimeSpan granularity;
};

} //namespace hostapp
} //namespace nisa100

#endif /*GETTOPOLOGYGENERATOR_H_*/
