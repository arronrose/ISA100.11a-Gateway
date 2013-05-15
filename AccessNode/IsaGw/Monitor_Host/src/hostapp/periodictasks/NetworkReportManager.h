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


#ifndef NETWORKREPORTMANAGER_H_
#define NETWORKREPORTMANAGER_H_


#include "../CommandsManager.h"
#include "../DevicesManager.h"
#include "../../ConfigApp.h"

#include "../../Log.h"

#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {


extern bool IsGatewayReconnected;


/**
 * Issue Network Health Report Request
 */
class NetworkReportManager
{
	
public:
	NetworkReportManager(CommandsManager& commands_, DevicesManager& devices_, const ConfigApp& configApp_) :
		commands(commands_), devices(devices_), configApp(configApp_)
	{
		LOG_INFO("ctor: Network Health Report Manager started.");
		lastAutomaticCommandTime = nlib::CurrentUniversalTime() - configApp.NetworkReportPeriod();
		granularity = configApp.NetworkReportPeriod();
	}

	void IssueNetworkReportCommand()
	{
		Command NetworkReportCmd;
		NetworkReportCmd.commandCode = Command::ccNetworkHealthReport;
		NetworkReportCmd.deviceID = devices.GatewayDevice()->id;
		NetworkReportCmd.generatedType = Command::cgtAutomatic;
		commands.CreateCommand(NetworkReportCmd, boost::str(boost::format("system: network health report")));

		LOG_INFO("Automatic network report request was made!");
	}

	void Check()
	{
		if (!devices.GatewayConnected())
					return; // no reason to send commands to a disconnected GW

		if (IsGatewayReconnected == true)
			return; // no reason to send commands without a session created

		
		if (granularity <= nlib::NOTIME)
			return; // disabled

		try
		{
			nlib::DateTime currentTime = nlib::CurrentUniversalTime();

			if (currentTime> lastAutomaticCommandTime + granularity)
			{
				DevicePtr dev = devices.GatewayDevice();
				if (dev->Status() != Device::dsRegistered)
					return;

				IssueNetworkReportCommand();

				lastAutomaticCommandTime = currentTime;
			}
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Check: Unhandled exception when generate automatic network health report request! Error:" << ex.what());
		}
	}
private:
	CommandsManager& commands;
	DevicesManager& devices;
	const ConfigApp& configApp;

	nlib::DateTime lastAutomaticCommandTime;
	nlib::TimeSpan granularity;
};

}//namespace hostapp
}//namespace nisa100


#endif 
