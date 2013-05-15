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

#ifndef FIRMWAREUPLOADMANAGER_H_
#define FIRMWAREUPLOADMANAGER_H_

#include "../CommandsManager.h"
#include "../DevicesManager.h"
#include "../model/FirmwareUpload.h"
#include "../model/Command.h"
#include "../../ConfigApp.h"

#define DEFAULT_RETRIES_COUNT 5


//added by Cristian.Guef
#include "../commandmodel/Nisa100ObjectIDs.h"

namespace nisa100 {
namespace hostapp {

/**
 * Uploads a single firmaware one at time.
 */
class FirmwareUploadManager
{
	LOG_DEF("nisa100.hostapp.FirmwareUploadManager");
public:
	enum FirmwareUploadManagerState
	{
		WaitingForNewFirmwareUpload = 1,
		WaitingForFirmwareResponse = 2
	};

	FirmwareUploadManager(CommandsManager& commands_, DevicesManager& devices_, const ConfigApp& configApp_) :
	commands(commands_), devices(devices_), configApp(configApp_), state(WaitingForNewFirmwareUpload)
	{
		devices.ResetPendingFirmwareUploads();
		commands.ReceiveCommandResponse.push_back(boost::bind(&FirmwareUploadManager::HandleRespondedCommand, this, _1, _2));

		//added
		sleepPeriodBeforeRetry = configApp.DelayPeriodBeforeFirmwareRetry();
		
		LOG_INFO("ctor: FirmwareUploadManager initialized.");
	}

	void Check()
	{
		/* commented 
		sleepPeriodBeforeRetry = configApp.DelayPeriodBeforeFirmwareRetry();
		*/
		
		LOG_DEBUG("Checking for new firmwares to upload: state=" << state);
		if (!devices.GatewayConnected())
		{
			LOG_DEBUG("No Gateway connected... Skipping check...");
			return; // no reason to send commands to a disconnected GW
		}

		DevicePtr systemManager = devices.SystemManagerDevice();

		/* commented by Cristian.Guef
		if (!systemManager || !systemManager->HasContract())
		*/
		//added by Cristian.Guef
		unsigned short TLDE_SAPID = apSMAP;
		unsigned short ObjID = UPLOAD_DOWNLOAD_OBJECT;
		unsigned int resourceID = (ObjID << 16) | TLDE_SAPID;
		/*if (!systemManager || !systemManager->HasContract(resourceID, 
				(unsigned char)GContract::BulkTransforClient))
		{
			LOG_DEBUG("SM not found or has no lease... Skipping check...");
			return;
		}*/
		if (!systemManager)
		{
			LOG_DEBUG("[firmwares to upload]: SM not found Skipping check...");
			return;
		}

		try
		{
			DoSteps(systemManager);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("DoSteps: failed! error=" << ex.what());
		}
		catch(...)
		{
			LOG_ERROR("DoSteps: failed! unknown error!");
		}
	}

	void DoSteps(DevicePtr& systemManager)
	{
		if (state == WaitingForNewFirmwareUpload)
		{
			if (devices.GetNextFirmwareUpload(sleepPeriodBeforeRetry, currentFirmware))
			{
				LOG_INFO("New firmware waiting to upload. Firmware=<" << currentFirmware.ToString() << ">.");
				Command command;
				command.commandCode = Command::ccFirmwareUpload;
				command.commandStatus = Command::csNew;
				command.deviceID = systemManager->id;
				command.generatedType = Command::cgtAutomatic;
				command.parameters.push_back(CommandParameter(CommandParameter::FirmwareUpload_FileName,
								currentFirmware.FileName));

				/* commented by Cristian.Guef
				commands.CreateCommand(command, std::string("firmware upload"));
				currentFirmwareCommandId = command.commandID;
				*/
				
				state = WaitingForFirmwareResponse;
				
				if (currentFirmware.Status == FirmwareUpload::fusNew)
				{
					currentFirmware.RetriesCount = DEFAULT_RETRIES_COUNT;
				}

				currentFirmware.Status = FirmwareUpload::fusUploading;
				devices.UpdateFirmware(currentFirmware);

				//added by Cristian.Guef
				commands.CreateCommand_for_FU(command, std::string("firmware upload"), currentFirmwareCommandId);
				
				LOG_INFO("Waiting for Command=" << command.ToString() << " to complete for firmware="
						<< currentFirmware.ToString());
			}
		}
	}

	void HandleRespondedCommand(const int commandID, Command::ResponseStatus status)
	{
		//added by Cristian.Guef
		/*LOG_INFO("Firmware Upload ->HandleRespondedCommand with commandID="
					<< (boost::int32_t)commandID 
					<< "wtih currentFirmwareCommandId="
					<< (boost::int32_t)currentFirmwareCommandId
					<< "and state=" 
					<< (boost::int32_t)state);*/
					
		if (commandID != currentFirmwareCommandId)
			return;

		if (state != WaitingForFirmwareResponse)
			return;

		if (status != Command::rsSuccess)
		{
			currentFirmware.RetriesCount--;
			LOG_INFO("Firmware Upload failed... Firmware=<" << currentFirmware.ToString() << ">.");

			currentFirmware.Status = (currentFirmware.RetriesCount <= 0) ?
					FirmwareUpload::fusFailed : FirmwareUpload::fusWaitRetry;

			currentFirmware.LastFailedUploadTime = nlib::CurrentUniversalTime();
			devices.UpdateFirmware(currentFirmware);
		}
		else
		{
			currentFirmware.Status = FirmwareUpload::fusSuccess;
			LOG_INFO("Firmware Upload succeeded... Firmware=<" << currentFirmware.ToString() << ">.");

			devices.UpdateFirmware(currentFirmware);

		}

		state = WaitingForNewFirmwareUpload;
	}


private:
	CommandsManager& commands;
	DevicesManager& devices;
	const ConfigApp& configApp;

	FirmwareUploadManagerState state;
	FirmwareUpload currentFirmware;
	int currentFirmwareCommandId;

	int sleepPeriodBeforeRetry;
};

}
}

#endif /*FIRMWAREUPLOADMANAGER_H_*/
