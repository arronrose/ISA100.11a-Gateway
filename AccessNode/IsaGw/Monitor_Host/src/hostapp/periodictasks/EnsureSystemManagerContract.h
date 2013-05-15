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

#ifndef ENSURESYSTEMMANAGERCONTRACT_H_
#define ENSURESYSTEMMANAGERCONTRACT_H_

#include "../CommandsManager.h"
#include "../DevicesManager.h"

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../Log.h"

//added by Cristian.Guef
#include "../commandmodel/Nisa100ObjectIDs.h"

namespace nisa100 {
namespace hostapp {

//added by Cristian.Guef
extern bool IsGatewayReconnected;

/**
 * This task ensure that the SM has always contract, needed for other tasks...
 */
class EnsureSystemManagerContract
{
	/* commented by Cristian
	LOG_DEF("nisa100.hostapp.EnsureSystemManagerContract");
	*/

public:
	EnsureSystemManagerContract(CommandsManager& commands_, DevicesManager& devices_)
		: commands(commands_), devices(devices_)
	{
	}

	void Check()
	{
		if (!devices.GatewayConnected())
		{
			LOG_DEBUG("No Gateway connected... Skipping check...");
			return; // no reason to send commands to a disconnected GW
		}

		//added by Cristian.Guef
		if (IsGatewayReconnected == true)
			return; // no reason to send commands without a session created


		DevicePtr systemManager = devices.SystemManagerDevice();
		if (!systemManager || !systemManager->IsRegistered())
		{
			LOG_DEBUG("SM not found or not registered!");
			return; //no SM or no registered
		}

		/* commented by Cristian.Guef
		if (!systemManager->HasContract())
		{
			commands.CreateContractFor(*systemManager, "system: create SM contract");
			return;
		}
		*/
		//added by Cristian.Guef
		//contract for GetDeviceInformation
		unsigned short TLDE_SAPID = apUAP1;
		unsigned short ObjID = SYSTEM_MONITORING_OBJECT;
		unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
		if (!systemManager->HasContract(resourceID, (unsigned char)GContract::Client))
		{
			commands.CreateContractFor(*systemManager, "system: create SM lease for getting device information", 
				resourceID, (unsigned char)GContract::Client, devices.m_rConfigApp.LeaseCommittedBurst(), -1);
			return;
		}
		
		//contract for BatteryStatus
		/*TLDE_SAPID = apUAP1;
		ObjID = PUBLISH_STATISCTICS_OBJECT;
		resourceID = (ObjID << 16) | TLDE_SAPID;
		if (!systemManager->HasContract(resourceID, (unsigned char)GContract::Client))
		{
			commands.CreateContractFor(*systemManager, "system: create SM lease for battery status",
				resourceID, (unsigned char)GContract::Client, devices.m_rConfigApp.LeaseCommittedBurst(), -1);
			return;
		}*/
		//contract for bulk transfer
		/*TLDE_SAPID = apDMAP;
		ObjID = UPLOAD_DOWNLOAD_OBJECT;
		resourceID = (ObjID << 16) | TLDE_SAPID;
		if (!systemManager->HasContract(resourceID, (unsigned char)GContract::BulkTransforClient))
		{
			commands.CreateContractFor(*systemManager, "system: create SM lease for bulk transfer",
				resourceID, (unsigned char)GContract::BulkTransforClient, devices.m_rConfigApp.LeaseCommittedBurst(), -1);
			return;
		}
		*/
	}

private:
	CommandsManager& commands;
	DevicesManager& devices;
};

}
}

#endif /* ENSURESYSTEMMANAGERCONTRACT_H_ */
