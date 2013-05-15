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


#ifndef SCHEDULEREPORTMANAGER_H_
#define SCHEDULEREPORTMANAGER_H_


#include "../CommandsManager.h"
#include "../DevicesManager.h"
#include "../../ConfigApp.h"

#include "../../Log.h"

#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {


extern bool IsGatewayReconnected;


/**
 * Query SM for contracts and routes when devices rejoin
 */
class ScheduleReportManager
{
	
public:
	ScheduleReportManager(CommandsManager& commands_, DevicesManager& devices_, const ConfigApp& configApp_) :
		commands(commands_), devices(devices_), configApp(configApp_)
	{
		LOG_INFO("ctor: Schedule Report Manager started.");
		lastAutomaticCommandTime = nlib::CurrentUniversalTime() - configApp.ScheduleReportPeriod();
		granularity = configApp.ScheduleReportPeriod();
	}

	void IssueScheduleReportCommand(const Device targetDevice)
	{
		Command ScheduleReportCmd;
		ScheduleReportCmd.commandCode = Command::ccScheduleReport;
		ScheduleReportCmd.deviceID = devices.GatewayDevice()->id;
		ScheduleReportCmd.parameters.push_back(CommandParameter(CommandParameter::ScheduleReport_DevID,
		    boost::lexical_cast<std::string>(targetDevice.id)));
		ScheduleReportCmd.generatedType = Command::cgtAutomatic;
		commands.CreateCommand(ScheduleReportCmd, boost::str(boost::format("system: schedule report for device:=%1%")
		    % targetDevice.mac.ToString()));

		LOG_INFO("Automatic schedule report request was made!"
			<< boost::str(boost::format(" DeviceID=%1%, DeviceIP=%2%") % targetDevice.id % targetDevice.ip.ToString()));
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
				if (devices.repository.nodesByIPv6.size() < 2)
					return;

				LOG_INFO("[ScheduleReport]: prepares to send for registered devices=" << devices.repository.nodesByIPv6.size());

				for (NodesRepository::NodesByIPv6T::const_iterator i = devices.repository.nodesByIPv6.begin();
						i != devices.repository.nodesByIPv6.end(); i++)
				{
					
					IssueScheduleReportCommand((const Device)*i->second);

					if (IsGatewayReconnected == true)
						return; // no reason to send commands without a session created
				}

				lastAutomaticCommandTime = currentTime;
			}
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Check: Unhandled exception when generate automatic schedule report request! Error:" << ex.what());
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
