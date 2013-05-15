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

#include "CommandsManager.h"
#include <nlib/exception.h>


//added by Cristian.Guef
#ifdef USE_MYSQL_DATABASE
#include "./dal/mysql/MySqlFactoryDal.h"
#endif

//added by Cristian.Guef
#include <../AccessNode/Shared/Utils.h>
#include <../AccessNode/Shared/AnPaths.h>
#include <../AccessNode/Shared/DurationWatcher.h>
#include <../AccessNode/Shared/SignalsMgr.h>


namespace nisa100 {
namespace hostapp {


//added by Cristian.Guef
static bool WaitforSessionConfirm;
static bool IsGatewayConnected = false;
bool IsGatewayReconnected = true;
bool DoDeleteSubscriberLease = false;

//added by Cristian.Guef
bool DoStopProcess = false;


/* commented by Cristian.Guef
CommandsManager::CommandsManager(IFactoryDal& factoryDal_) :
	factoryDal(factoryDal_)
{
}
*/
//added by Cristian.Guef
CommandsManager::CommandsManager(IFactoryDal& factoryDal_, DevicesManager& devices_) :
	factoryDal(factoryDal_), devices(devices_)
{
}


CommandsManager::~CommandsManager()
{
}

void CommandsManager::FireReceiveCommandResponse(int commandID, Command::ResponseStatus status)
{
	for (std::vector<boost::function2<void, const int, Command::ResponseStatus> >::iterator it =
	    ReceiveCommandResponse.begin(); it != ReceiveCommandResponse.end(); it++)
	{
		try
		{
			(*it)(commandID, status);
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("FireReceiveCommandResponse: failed! error=" << ex.what());
		}
		catch (...)
		{
			LOG_ERROR("FireReceiveCommandResponse: failed! unknown error!");
		}
	}
}


void CommandsManager::ProcessRequests()
{

//added by Cristian.Guef
establish_connection:

	//added by Cristian.Guef
	if(IsGatewayConnected == false){

		//delete topology - there should be no topolgy when connection to gateway is lost
		GTopologyReport noTopolgy;
		devices.ProcessTopology(NULL, noTopolgy);

		LOG_INFO("Monitor Host connecting to Gateway...");
		while(IsGatewayConnected == false)
		{
			//stop process if asked to
			if (DoStopProcess)
				return;

			sleep(3);
			GatewayRun(2*1000*1000/*usec*/);
		}
		LOG_INFO("Monitor Host connected to Gateway.");
	}


	//added by Cristian.Guef
	while(IsGatewayReconnected == true){
		
		LOG_INFO("Monitor Host establishing session with Gateway...");
		
		CreateSession(1/*NetworkID*/, "Session for Monitor_Host");

		WaitforSessionConfirm = true;
		while(WaitforSessionConfirm == true){

			//stop process if asked to
			if (DoStopProcess)
				return;

			sleep(3);
			GatewayRun(1000/*usec*/);
			if(processResponse)
				processResponse();
			if(processTimedout)
				processTimedout();

			if (CSignalsMgr::IsRaised(SIGTERM))
			{

				LOG_INFO("CommandsThread::Run Stop requested...");
				return;
			}
#ifdef HW_VR900
			TouchPidFile(NIVIS_TMP"MonitorHost.pid");
#endif
		}

		if (IsGatewayReconnected == false)
		{
			LOG_INFO("Monitor Host established a session with Gateway.");
		}
		else
		{
			LOG_WARN("Monitor Host haven't established a session with Gateway.");
		}
	}


	CommandsList newCommands;
	try
	{
		//read new posted commands from db
		factoryDal.Commands().GetNewCommands(newCommands);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("ProcessRequests: failed! error=" << ex.what());

		//added by Cristian.Guef - try to recover from db query error
		#ifdef USE_MYSQL_DATABASE
		bool connectedToDatabase = false;
		while(!connectedToDatabase)
		{

			if (DoStopProcess == true)
				break;

			try
			{
				((MySqlFactoryDal&)factoryDal).Reconnect();
				connectedToDatabase = true;
			}
			catch(std::exception& ex)
			{
				LOG_ERROR("Cannot connect to the database. Error=" << ex.what());
				LOG_DEBUG("Retry to reconnect to the database in " << ((MySqlFactoryDal&)factoryDal).m_timeoutSeconds << " seconds");
				sleep(((MySqlFactoryDal&)factoryDal).m_timeoutSeconds);
			}
			catch(...)
			{
				LOG_ERROR("Unknown exception occured when trying to connect to the database!");
				LOG_DEBUG("Retry to reconnect to the database in " << ((MySqlFactoryDal&)factoryDal).m_timeoutSeconds << " seconds");
				sleep(((MySqlFactoryDal&)factoryDal).m_timeoutSeconds);
			}
		}
		#endif

		return;
	}

	if (!newCommands.empty())
	{
		LOG_DEBUG("ProcessRequests: CommandsCount=" << newCommands.size());

		//notify new commands
		for (CommandsList::iterator it = newCommands.begin(); it != newCommands.end(); it++)
		{

			//added by Cristian.Guef
			if (IsGatewayConnected == false || IsGatewayReconnected == true)
				goto establish_connection;

			try
			{
				LOG_INFO("FireNewCommand: Command=" << (*it).ToString());
				NewCommand(*it);
			}
			catch (std::exception& ex)
			{
				LOG_ERROR("FireNewCommand: Command=" << (*it).ToString() << " failed! error=" << ex.what());
			}
		}
	}
}

bool CommandsManager::GetCommand(int commandID, Command& command)
{
	try
	{
		/* commented by Cristian.Guef
		if (factoryDal.Commands().GetCommand(commandID, command))
		{
			factoryDal.Commands().GetCommandParameters(commandID, command.parameters);
			return true;
		}
		*/
		//added by Cristian.Guef
		//we don't check in db if it exists because if it is not no exception is thrown
		//so...
		if (commandID == Command::NO_COMMAND_ID)
			return false;
		command.commandID = commandID;
		return true;
		
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("GetCommand: CommandID=" << commandID << " failed. error=" << ex.what());
	}
	catch (...)
	{
		/* Commented by Cristian.Guef
		LOG_FATAL("GetCommand: CommandID=" << commandID << " failed. unknown error!");
		*/
		LOG_ERROR("GetCommand: CommandID=" << commandID << " failed. unknown error!");
	}
	return false;
}

void CommandsManager::SetCommandSent(Command& command)
{
	LOG_INFO("SetCommandSent: Command=" << command.ToString());

	try
	{
		factoryDal.Commands().SetCommandAsSent(command);
	}
	catch(std::exception& ex)
	{
		LOG_ERROR("SetCommandSent: Command=" << command.ToString() << " failed! error=" << ex.what());
	}
}

void CommandsManager::SetCommandResponded(Command& command, const nlib::DateTime& respondedTime,
  const std::string& response)
{
	LOG_INFO("SetCommandResponded: Command=" << command.ToString() << " response=" << response);

	FireReceiveCommandResponse(command.commandID, Command::rsSuccess);
	try
	{
		/* commented by Cristian.Guef
		factoryDal.Commands().SetCommandAsResponded(command, respondedTime, response);
		*/
		//added by Cristian.Guef
		if (command.commandID != Command::NO_COMMAND_ID)
			factoryDal.Commands().SetCommandAsResponded(command, respondedTime, response);

	}
	catch(std::exception& ex)
	{
		LOG_ERROR("SetCommandResponded: Command=" << command.ToString() << " failed! error=" << ex.what());
	}

	//added by Cristian.Guef
	if(command.commandID == Command::NO_COMMAND_ID){
		IsGatewayReconnected = false;
		WaitforSessionConfirm = false;
	}

}

void CommandsManager::SetCommandFailed(Command& command, const nlib::DateTime& failedTime,
  Command::ResponseStatus errorCode, const std::string& errorReason)
{
	LOG_WARN("SetCommandFailed: Command=" << command.ToString() << " errorCode=" << errorCode << " errorReason="
	    << errorReason);

	FireReceiveCommandResponse(command.commandID, errorCode);
	try
	{
		/* commented by Cristian.Guef
		factoryDal.Commands().SetCommandAsFailed(command, failedTime, errorCode, errorReason);
		*/
		//added by Cristian.Guef
		if (command.commandID != Command::NO_COMMAND_ID)
			factoryDal.Commands().SetCommandAsFailed(command, failedTime, errorCode, errorReason);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("SetCommandFailed: Command=" << command.ToString() << " failed! error=" << ex.what());
	}

	//added by Cristian.Guef
	if(command.commandID == Command::NO_COMMAND_ID){
		WaitforSessionConfirm = false;
	}
}

//added by Cristian.Guef
void CommandsManager::SetDBCmdID_as_new(int DBCmdID)
{
	try
	{
		factoryDal.Commands().SetDBCmdID_as_new(DBCmdID);
	}
	catch(std::exception& ex)
	{
		LOG_ERROR("SetCommandNew: DBCmdID=" << (boost::int32_t)DBCmdID << " failed! error=" << ex.what());
	}
}


void CommandsManager::CreateCommand(Command& newCommand, const std::string& createdReason)
{
	newCommand.commandStatus = Command::csNew;
	newCommand.timePosted = nlib::CurrentUniversalTime();

	/* commented by Cristian.Guef
	factoryDal.Commands().CreateCommand(newCommand);
	LOG_INFO("CreateCommand: " << newCommand.ToString() << " createdReason=" << createdReason);
	*/
	//added by Cristian.Guef
	try
	{
		factoryDal.BeginTransaction();
		factoryDal.Commands().CreateCommand(newCommand);
		factoryDal.CommitTransaction();
		LOG_INFO("CreateCommand: " << newCommand.ToString() << " createdReason=" << createdReason);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CreateCommand: failed! error=" << ex.what());
		factoryDal.RollbackTransaction();
		return;
	}
	catch (...)
	{
		LOG_ERROR("CreateCommand: failed! unknown error!");
		factoryDal.RollbackTransaction();
		return;
	}
	
	if (NewCommand)
		NewCommand(newCommand);
}

//added by Cristian.Guef
void CommandsManager::CreateCommandForAlert(Command& newCommand, const std::string& createdReason, int &currentAlertCommandId)
{
	newCommand.commandStatus = Command::csNew;
	newCommand.timePosted = nlib::CurrentUniversalTime();

	try
	{
		factoryDal.BeginTransaction();
		factoryDal.Commands().CreateCommand(newCommand);
		factoryDal.CommitTransaction();
		LOG_INFO("CreateCommand: " << newCommand.ToString() << " createdReason=" << createdReason);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CreateCommand: failed! error=" << ex.what());
		factoryDal.RollbackTransaction();
		return;
	}
	catch (...)
	{
		LOG_ERROR("CreateCommand: failed! unknown error!");
		factoryDal.RollbackTransaction();
		return;
	}
	
	currentAlertCommandId = newCommand.commandID;

	if (NewCommand)
		NewCommand(newCommand);
}

void CommandsManager::CreateCommand_for_FU(Command& newCommand, const std::string& createdReason, int &currentFirmwareCommandId)
{
	newCommand.commandStatus = Command::csNew;
	newCommand.timePosted = nlib::CurrentUniversalTime();

	/* commented by Cristian.Guef
	factoryDal.Commands().CreateCommand(newCommand);
	LOG_INFO("CreateCommand: " << newCommand.ToString() << " createdReason=" << createdReason);
	*/
	//added by Cristian.Guef
	try
	{
		factoryDal.BeginTransaction();
		factoryDal.Commands().CreateCommand(newCommand);
		factoryDal.CommitTransaction();
		LOG_INFO("CreateCommand: " << newCommand.ToString() << " createdReason=" << createdReason);
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CreateCommand: failed! error=" << ex.what());
		factoryDal.RollbackTransaction();
		return;
	}
	catch (...)
	{
		LOG_ERROR("CreateCommand: failed! unknown error!");
		factoryDal.RollbackTransaction();
		return;
	}
	
	currentFirmwareCommandId = newCommand.commandID;

	if (NewCommand)
		NewCommand(newCommand);
}

int CommandsManager::CreateCommandFromLink(int linkId, Command::CommandGeneratedType generatedType,
  const std::string& createdReason)
{
	try
	{
		factoryDal.BeginTransaction();

		int clonedCommandID = factoryDal.Commands().CreateCommandFromLink(linkId, generatedType);

		LOG_INFO("CreateCommandFromLink: LinkId=" << linkId << " into clone CommandID=" << clonedCommandID
		    << " CreatedReason='" << createdReason << "'");
		factoryDal.CommitTransaction();

		return clonedCommandID;
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CreateCommandFromLink: failed! error=" << ex.what());
		factoryDal.RollbackTransaction();
		return -1;
	}
	catch (...)
	{
		LOG_ERROR("CreateCommandFromLink: failed! unknown error!");
		factoryDal.RollbackTransaction();
		return -1;
	}
}

/* commented by Cristian.Guef
bool CommandsManager::CreateContractFor(const Device& device, const std::string& createdReason)
{
	if (device.WaitForContract())
	{
		LOG_DEBUG("CreateContractFor: Device=" << device.ToString() << "already wait for a contract... so ignored");
		return false;
	}

	Command contractCommand;
	try
	{
		contractCommand.commandCode = Command::ccCreateClientServerContract;
		contractCommand.deviceID = device.id;
		contractCommand.generatedType = Command::cgtAutomatic;
		LOG_DEBUG("CreateContractFor: Device=" << device.ToString() << " requesting...");
		CreateCommand(contractCommand, createdReason);

		return true;
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CreateContractFor: failed! Command=" << contractCommand.ToString() << " error=" << ex.what());
	}
	catch (...)
	{
		LOG_ERROR("CreateContractFor: failed! Command=" << contractCommand.ToString() << " unknown exception!");
	}

	return false;
}
*/

/* commented byb Cristian.Guef
bool CommandsManager::CreateContractFor(const Device& device, const std::string& createdReason, unsigned int resourceID)
*/
//added by Cristian.Guef
bool CommandsManager::CreateContractFor(Device& device, const std::string& createdReason, 
										unsigned int resourceID, unsigned char leaseType, boost::int16_t committedBurst,
										int commandID)
{
	/* commented by Cristian.Guef
	if (device.WaitForContract(resourceID))
	*/
	//added by Cristian.Guef
	if (device.WaitForContract(resourceID, leaseType))
	{
		LOG_DEBUG("CreateLeaseFor: Device=" << device.ToString()
					<<"for obj_id=" << (boost::uint16_t)(resourceID >> 16)
					<<"and tlsap_id=" << (boost::uint16_t)(resourceID & 0xFFFF)
					<<"and lease type=" << (boost::uint16_t)leaseType
					<<"and committedBurst=" << (boost::uint16_t)committedBurst
					<< "already wait for a lease... so ignored");
		return false;
	}

	Command contractCommand;
	try
	{
		/* commented by Cristian.Guef
		contractCommand.commandCode = Command::ccCreateClientServerContract;
		*/

		//added by Cristian.Guef -- there is no need to Command::ccCreateClientServerContract to 
		//							otherelse because leasetype is sent otherway (see in the next lines)
		contractCommand.commandCode = Command::ccCreateClientServerContract;

		contractCommand.deviceID = device.id;
		contractCommand.generatedType = Command::cgtAutomatic;

		//added by Cristian.Guef
		contractCommand.m_unResourceIDForContractCmd = resourceID;
		contractCommand.m_ucLeaseTypeForContractCmd = leaseType;
		contractCommand.m_committedBurst = committedBurst;
		contractCommand.m_dbCmdIDForContract = commandID;


		LOG_DEBUG("CreateLeaseFor: Device=" << device.ToString() 
					<<"for obj_id=" << (boost::uint16_t)(resourceID >> 16)
					<<"and tlsap_id=" << (boost::uint16_t)(resourceID & 0xFFFF)
					<<"and lease type=" << (boost::uint16_t)leaseType
					<<"and committedBurst=" << (boost::uint16_t)committedBurst
					<< " requesting...");

		CreateCommand(contractCommand, createdReason);

		return true;
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CreateLeaseFor: failed! Command=" << contractCommand.ToString() << " error=" << ex.what());
	}
	catch (...)
	{
		LOG_ERROR("CreateLeaseFor: failed! Command=" << contractCommand.ToString() << " unknown exception!");
	}

	return false;
}

//added by Cristian.Guef
bool CommandsManager::CreateSession(unsigned short networkID, const std::string& createdReason)
{

	Command sessionCommand;
	try
	{
		sessionCommand.commandCode = Command::ccSession;
		sessionCommand.deviceID = 0;
		sessionCommand.generatedType = Command::cgtAutomatic;
		sessionCommand.m_uwNetworkIDForSessionGSAP = networkID;
		
		LOG_DEBUG("CreateSession for Monitor_Host, requesting...");
		
		if (NewCommand)
			NewCommand(sessionCommand);

		return true;
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("CreateSession: failed! Command=" << sessionCommand.ToString() << " error=" << ex.what());
	}
	catch (...)
	{
		LOG_ERROR("CreateSession: failed! Command=" << sessionCommand.ToString() << " unknown exception!");
	}

	return false;
}

//added by Cristian.Guef
void CommandsManager::HandleGWConnect(const std::string& host, int port)
{
	LOG_INFO("Gateway reconnected ...");
	IsGatewayConnected = true;
	IsGatewayReconnected = true;
	DoDeleteSubscriberLease = true;
}

//added by Cristian.Guef
void CommandsManager::HandleGWDisconnect()
{
	LOG_WARN("Gateway disconnected!");
	IsGatewayConnected = false;

}



} //namespace hostapp
} // namespace nisa100
