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


#ifndef DEVICEREPMANAGER_H_
#define DEVICEREPMANAGER_H_


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
class DeviceReportManager
{

public:
	DeviceReportManager(CommandsManager& commands_, DevicesManager& devices_, const ConfigApp& configApp_) :
		commands(commands_), devices(devices_), configApp(configApp_)
	{
		granularity = configApp.DeviceReportPeriod();
		LOG_INFO("ctor: Device Health Report Manager started.");
		lastAutomaticCommandTime = nlib::CurrentUniversalTime() - configApp.DeviceReportPeriod();
	}

	void IssueDeviceReportCmd(const Device targetDevice) ;

	void Check() ;
private:
	CommandsManager& commands;
	DevicesManager& devices;
	const ConfigApp& configApp;

	nlib::DateTime lastAutomaticCommandTime;

	//added
	nlib::TimeSpan granularity;
};

}//namespace hostapp
}//namespace nisa100

#endif
