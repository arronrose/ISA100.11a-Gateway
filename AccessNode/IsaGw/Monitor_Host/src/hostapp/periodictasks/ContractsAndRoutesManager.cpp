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

#include "ContractsAndRoutesManager.h"
#include <boost/lexical_cast.hpp>

namespace nisa100 {
namespace hostapp {

void ContractsAndRoutesManager::IssueGetContractsAndRoutesCommand()
{
	Command getContractsAndRoutes;
	getContractsAndRoutes.commandCode = Command::ccGetContractsAndRoutes;
	getContractsAndRoutes.deviceID = devices.SystemManagerDevice()->id;
	getContractsAndRoutes.generatedType = Command::cgtAutomatic;
	commands.CreateCommand(getContractsAndRoutes, boost::str(boost::format("system: get contracts and routes")));

	LOG_INFO("Automatic Get ContractsAndRoutes request was made!");
}

void ContractsAndRoutesManager::Check()
{
	if (!devices.GatewayConnected())
		return; // no reason to send commands to a disconnected GW

	//added by Cristian.Guef
	if (IsGatewayReconnected == true)
		return; // no reason to send commands without a session created


	if (granularity <= nlib::NOTIME)
			return; // disabled

	//if (devices.rejoinedDevices.size() > 0)
	//	LOG_INFO("Found rejoined devices no = " << devices.rejoinedDevices.size() << "so get contracts and routes for each device...");

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
			LOG_DEBUG("SM not found or has no lease... Skipping check...");
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
		if (currentTime> lastAutomaticCommandTime + granularity)
		{
			IssueGetContractsAndRoutesCommand();
			
			lastAutomaticCommandTime = currentTime;
		}
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("Check: Getting Contracts and Routes failed! error=" << ex.what());
	}
}


}// namespace hostapp
}// namespace nisa100
