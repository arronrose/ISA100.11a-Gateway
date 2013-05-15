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

//added by Cristian.Guef
#ifndef HW_VR900


#include <string>
#include <algorithm>
#include "MySqlCommandsDal.h"


namespace nisa100 {
namespace hostapp {

MySqlCommandsDal::MySqlCommandsDal(MySQLConnection& connection_) :
	connection(connection_)
{
}

MySqlCommandsDal::~MySqlCommandsDal()
{
}

void MySqlCommandsDal::VerifyTables()
{
	LOG_DEBUG("Verify commands tables structure...");
	{
		std::string query = "SELECT CommandID, DeviceID, CommandCode, CommandStatus, TimePosted, TimeResponsed,"
			" ErrorCode, ErrorReason, Response FROM Commands WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		std::string query = "SELECT CommandID, ParameterCode, ParameterValue FROM CommandParameters WHERE 0";
		try
		{
			MySQLCommand(connection, query).ExecuteQuery();
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
}

void MySqlCommandsDal::CreateCommand(Command& command)
{
	LOG_DEBUG("Create command:" << command.ToString());
	MySQLCommand sqlCommand(connection, ""
		"INSERT INTO Commands(DeviceID, CommandCode, CommandStatus, TimePosted, ErrorCode, Generated)"
		" VALUES(?001, ?002, ?003, ?004, ?005, ?006);");
	sqlCommand.BindParam(1, command.deviceID);
	sqlCommand.BindParam(2, (int)command.commandCode);
	sqlCommand.BindParam(3, (int)command.commandStatus);
	sqlCommand.BindParam(4, command.timePosted);
	sqlCommand.BindParam(5, 0);
	sqlCommand.BindParam(6, (int)command.generatedType);
	sqlCommand.ExecuteNonQuery();
	command.commandID = sqlCommand.GetLastInsertRowID();

	CreateCommandParameters(command.commandID, command.parameters);
	//LOG_DEBUG(sqlCommand.Query());
}

void MySqlCommandsDal::CreateCommandParameters(int commandID, Command::ParametersList& commandParameters)
{
	LOG_DEBUG("Create parameters for command with ID:" << commandID);

	for (Command::ParametersList::const_iterator it = commandParameters.begin(); it != commandParameters.end(); it++)
	{
		MySQLCommand sqlCommand(connection, ""
			"INSERT INTO CommandParameters(ParameterCode, ParameterValue, CommandID) "
			"VALUES (?001, ?002, ?003)");

		sqlCommand.BindParam(1, (int)it->parameterCode);
		sqlCommand.BindParam(2, it->parameterValue);
		sqlCommand.BindParam(3, commandID);

		sqlCommand.ExecuteNonQuery();
	}
}

int MySqlCommandsDal::CreateCommandFromLink(int linkID, Command::CommandGeneratedType generatedType)
{
	LOG_DEBUG("Create command from link ID:" << linkID);

	MySQLCommand sqlCommand1(connection, ""
		"INSERT INTO Commands (DeviceID, CommandCode, CommandStatus, TimePosted, ErrorCode, Generated)"
		" SELECT DL.FromDeviceID, CASE DL.LinkType WHEN 0 THEN ?006 WHEN 1 THEN ?007 ELSE -1 END, ?001, ?002, ?003, ?004"
		"  FROM DeviceLinks DL WHERE DL.LinkID = ?005;");

	sqlCommand1.BindParam(1, (int)Command::csNew);
	//sqlCommand1.BindParam(2, nlib::CurrentLocalTime());
	sqlCommand1.BindParam(2, nlib::CurrentUniversalTime());
	sqlCommand1.BindParam(3, (int)Command::rsNoStatus);
	sqlCommand1.BindParam(4, (int)generatedType);
	sqlCommand1.BindParam(5, linkID);
	sqlCommand1.BindParam(6, (int)Command::ccPublishSubscribe);
	sqlCommand1.BindParam(7, (int)Command::ccLocalLoop);

	sqlCommand1.ExecuteNonQuery();
	int newInsertedCommandID = sqlCommand1.GetLastInsertRowID();

	MySQLCommand sqlCommand2(connection, ""
		"INSERT INTO CommandParameters(CommandID, ParameterCode, ParameterValue)"
		" SELECT ?001, ParameterCode, ParameterValue"
		" FROM DeviceLinkParameters WHERE LinkID = ?002;");
	sqlCommand2.BindParam(1, newInsertedCommandID);
	sqlCommand2.BindParam(2, linkID);
	sqlCommand2.ExecuteNonQuery();

	return newInsertedCommandID;
}

bool MySqlCommandsDal::GetCommand(int commandID, Command& command)
{
	LOG_DEBUG("Try to find command with ID:" << commandID);
	MySQLCommand sqlCommand(connection,
			"SELECT DeviceID, CommandCode, CommandStatus, TimePosted, TimeResponsed, ErrorCode, ErrorReason, Response, Generated"
				" FROM Commands WHERE CommandID = ?001");

	sqlCommand.BindParam(1, commandID);
	MySQLResultSet::Ptr rsCommands = sqlCommand.ExecuteQuery();
	if (rsCommands->RowsCount())
	{
		MySQLResultSet::Iterator itCommand = rsCommands->Begin();
		command.commandID = commandID;
		command.deviceID = itCommand->Value<int>(0);
		command.commandCode = (Command::CommandCode)itCommand->Value<int>(1);
		command.commandStatus = (Command::CommandStatus)itCommand->Value<int>(2);
		command.timePosted = itCommand->Value<nlib::DateTime>(3);
		if (!itCommand->IsNull(4))
		{
			command.timeResponded = itCommand->Value<nlib::DateTime>(4);
		}
		command.errorCode = (Command::ResponseStatus)itCommand->Value<int>(5);
		if (!itCommand->IsNull(6))
		{
			command.errorReason = itCommand->Value<std::string>(6);
		}
		if (!itCommand->IsNull(7))
		{
			command.response = itCommand->Value<std::string>(7);
		}
		command.generatedType = (Command::CommandGeneratedType)itCommand->Value<int>(8);
		return true;
	}
	return false;
}

void MySqlCommandsDal::GetCommandParameters(int commandID, Command::ParametersList& commandParameters)
{
	LOG_DEBUG("Get parameters for command with ID:" << commandID);
	MySQLCommand sqlCommand(connection, "SELECT ParameterCode, ParameterValue"
		" FROM CommandParameters WHERE CommandID = ?001");

	sqlCommand.BindParam(1, commandID);
	MySQLResultSet::Ptr rsCommandParams = sqlCommand.ExecuteQuery();
	commandParameters.reserve(commandParameters.size() + rsCommandParams->RowsCount());
	for (MySQLResultSet::Iterator itCommandParam = rsCommandParams->Begin(); itCommandParam
			!= rsCommandParams->End(); itCommandParam++)
	{
		CommandParameter cp;
		cp.parameterCode = (CommandParameter::ParameterCode)itCommandParam->Value<int>(0);
		cp.parameterValue = itCommandParam->Value<std::string>(1);

		commandParameters.push_back(cp);
	}
}

void MySqlCommandsDal::GetNewCommands(CommandsList& newCommands)
{
	LOG_DEBUG("Get all new commands from database");
	MySQLCommand sqlCommand(connection,
			"SELECT CommandID, DeviceID, CommandCode, CommandStatus, TimePosted, Generated"
				" FROM Commands WHERE CommandStatus = ?001");
	sqlCommand.BindParam(1, (int)Command::csNew);

	MySQLResultSet::Ptr rsCommands = sqlCommand.ExecuteQuery();
	newCommands.reserve(newCommands.size() + rsCommands->RowsCount());
	LOG_DEBUG(rsCommands->RowsCount() << " new commands found!");
	for (MySQLResultSet::Iterator itCommand = rsCommands->Begin(); itCommand != rsCommands->End(); itCommand++)
	{
		Command cmd;
		cmd.commandID = itCommand->Value<int>(0);;
		cmd.deviceID = itCommand->Value<int>(1);
		cmd.commandCode = (Command::CommandCode)itCommand->Value<int>(2);
		cmd.commandStatus = (Command::CommandStatus)itCommand->Value<int>(3);
		cmd.timePosted = itCommand->Value<nlib::DateTime>(4);
		cmd.generatedType = (Command::CommandGeneratedType)itCommand->Value<int>(5);

		GetCommandParameters(cmd.commandID, cmd.parameters);
		newCommands.push_back(cmd);
	}
}

void MySqlCommandsDal::SetCommandAsSent(Command& command)
{
	LOG_DEBUG("Update command:" << command.commandID << " as sent!");
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Commands SET CommandStatus = ?001"
		" WHERE CommandID = ?002");
	sqlCommand.BindParam(1, (int)Command::csSent);
	sqlCommand.BindParam(2, command.commandID);

	sqlCommand.ExecuteNonQuery();

	//now update also model
	command.commandStatus = Command::csSent;
}

void MySqlCommandsDal::SetCommandAsResponded(Command& command, const nlib::DateTime& responseTime,
		const std::string& response)
{
	LOG_DEBUG("Update command:" << command.commandID << " as responded!");
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Commands SET "
		" CommandStatus = ?001, TimeResponsed = ?002, ErrorCode = ?003, Response = ?004 "
		" WHERE CommandID = ?005");

	sqlCommand.BindParam(1, (int)Command::csResponded);
	sqlCommand.BindParam(2, responseTime);
	sqlCommand.BindParam(3, (int)Command::rsSuccess);
	sqlCommand.BindParam(4, response);
	sqlCommand.BindParam(5, command.commandID);

	sqlCommand.ExecuteNonQuery();

	//now update also model
	command.commandStatus = Command::csResponded;
	command.timeResponded = responseTime;
	command.errorCode = Command::rsSuccess;
	command.response = response;
}

void MySqlCommandsDal::SetCommandAsFailed(Command& command, const nlib::DateTime& errorTime, int errorCode,
		const std::string& errorReason)
{
	LOG_DEBUG("Update command:" << command.commandID << " as failed");
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Commands SET"
		" CommandStatus = ?001, TimeResponsed = ?002, ErrorCode = ?003, ErrorReason = ?004"
		" WHERE CommandID = ?005");

	sqlCommand.BindParam(1, (int)Command::csFailed);
	sqlCommand.BindParam(2, errorTime);
	sqlCommand.BindParam(3, errorCode);
	sqlCommand.BindParam(4, errorReason);
	sqlCommand.BindParam(5, command.commandID);

	sqlCommand.ExecuteNonQuery();

	//now update also model
	command.commandStatus = Command::csFailed;
	command.timeResponded = errorTime;
	command.errorCode = Command::rsSuccess;
	command.errorReason = errorReason;
}

void MySqlCommandsDal::SetCommandsAsExpired(const nlib::DateTime& oldestThan, const nlib::DateTime& errorTime,
		const std::string& errorReason)
{
	//LOG_DEBUG("Set expired commands older than:" << nlib::dbxx::detail::ToDbString(oldestThan));
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Commands SET"
		" CommandStatus = ?001, TimeResponsed = ?002, ErrorCode = ?003, ErrorReason = ?004"
		" WHERE TimePosted <= ?005");

	sqlCommand.BindParam(1, (int)Command::csFailed);
	sqlCommand.BindParam(2, errorTime);
	sqlCommand.BindParam(3, (int)Command::rsFailure_HostTimeout);
	sqlCommand.BindParam(4, errorReason);
	sqlCommand.BindParam(5, oldestThan);

	sqlCommand.ExecuteNonQuery();
}

//added by Cristian.Guef
void MySqlCommandsDal::SetDBCmdID_as_new(int DBCmsID)
{
	LOG_DEBUG("Update DBCmdID:" << (boost::int32_t)DBCmsID << " as new");
	MySQLCommand sqlCommand(connection, ""
		"UPDATE Commands SET "
		" CommandStatus = ?001 "
		" WHERE CommandID = ?002");

	sqlCommand.BindParam(1, (int)Command::csNew);
	sqlCommand.BindParam(2, DBCmsID);

	sqlCommand.ExecuteNonQuery();
}

bool MySqlCommandsDal::GetNextFirmwareUpload(FirmwareUpload& result, int notFailedForMinutes)
{
	//TODO
	return false;
}


}//hostapp
}//nisa100

#endif
