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

#ifndef COMMANDSMANAGER_H_
#define COMMANDSMANAGER_H_

#include "dal/IFactoryDal.h"
#include "model/Command.h"
#include "model/Device.h"

#include <vector>

/* commented by Cristian.Guef
#include <boost/thread.hpp> //for condition & mutex
*/

#include <boost/function.hpp> //for callback

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../Log.h"

//adde by Cristian.Guef
#include "DevicesManager.h"

namespace nisa100 {
namespace hostapp {

typedef int ResponseT;

/**
 * @brief Checks for new commands in db, and notiy new commands
 * Process command response updating db.
 */
class CommandsManager
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.CommandsManager");
	*/

public:
	/* commented by Cristian.Guef
	CommandsManager(IFactoryDal& factoryDal);
	*/
	//added by Cristian.Guef
	CommandsManager(IFactoryDal& factoryDal, DevicesManager& devices);
	
	
	virtual ~CommandsManager();


	boost::function1<void, Command&> NewCommand;

	//added by Cristian.Guef
	boost::function0<void>  processResponse;
	boost::function0<void>  processTimedout;

	std::vector<boost::function2<void, const int, Command::ResponseStatus> > ReceiveCommandResponse;
	void FireReceiveCommandResponse(int commandID, Command::ResponseStatus status);

	void CreateCommand(Command& newCommand, const std::string& createdReason);

	//added by Cristian.Guef
	void CreateCommandForAlert(Command& newCommand, const std::string& createdReason, int &currentAlertCommandId);
	void CreateCommand_for_FU(Command& newCommand, const std::string& createdReason, int &currentFirmwareCommandId);

	int CreateCommandFromLink(int commandID, Command::CommandGeneratedType generatedType, const std::string& createdReason);

	/**
	 * Try to create a contract for device if no other contract request is pending for this device.
	 */

	/*
	//commented by Cristian.Guef
	bool CreateContractFor(const Device& device, const std::string& createdReason);
	*/
	//added by Cristian.Guef
	bool CreateContractFor(Device& device, const std::string& createdReason, 
		unsigned int resourceID, unsigned char leaseType, boost::int16_t committedBurst, int commandID);

	//added by Cristian.Guef
	bool CreateSession(unsigned short networkID, const std::string& createdReason);

	//added by Cristian.Guef
	void HandleGWConnect(const std::string& host, int port);
	void HandleGWDisconnect();

	bool GetCommand(int commandID, Command& command);
	void ProcessRequests();

	void SetCommandSent(Command& command);
	void SetCommandResponded(Command& command, const nlib::DateTime& respondedTime, const std::string& response);
	void SetCommandFailed(Command& command, const nlib::DateTime& failedTime, Command::ResponseStatus errorCode,
			const std::string& errorReason);

	//added by Cristian.Guef
	void SetDBCmdID_as_new(int DBCmdID);

public:
	IFactoryDal& factoryDal;
	
	//added by Cristian.Guef
	DevicesManager& devices;

//added by Cristian.Guef
public:
	boost::function1<void, int> GatewayRun;
};

} //namespace hostapp
} //namespace nisa100

#endif /*COMMANDSMANAGER_H_*/
