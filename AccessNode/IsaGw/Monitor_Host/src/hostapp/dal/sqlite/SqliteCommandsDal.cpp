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

#include <string>
#include <algorithm>
#include <list>
#include "SqliteCommandsDal.h"

namespace nisa100 {
namespace hostapp {

SqliteCommandsDal::SqliteCommandsDal(sqlitexx::Connection& connection_) :
	connection(connection_), m_oNewCommand_Compiled(connection_)
{
}

SqliteCommandsDal::~SqliteCommandsDal()
{
}

void SqliteCommandsDal::VerifyTables()
{
	LOG_DEBUG("Verify commands tables structure...");
	{
		const char* query= "SELECT CommandID, DeviceID, CommandCode, CommandStatus, TimePosted, TimeResponsed,"
			" ErrorCode, ErrorReason, Response FROM Commands WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}

	{
		const char* query= "SELECT CommandID, ParameterCode, ParameterValue FROM CommandParameters WHERE 0";
		try
		{
			sqlitexx::Command::ExecuteQuery(connection.GetRawDbObj(), query);
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Verify failed for='" << query << "' error=" << ex.what());
			throw;
		}
	}
}

void SqliteCommandsDal::CreateCommand(Command& command)
{
	LOG_DEBUG("Create command:" << command.ToString());
	sqlitexx::Command sqlCommand(connection, ""
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

void SqliteCommandsDal::CreateCommandParameters(int commandID, Command::ParametersList& commandParameters)
{
	LOG_DEBUG("Create parameters for command with ID:" << commandID);

	for (Command::ParametersList::const_iterator it = commandParameters.begin(); it != commandParameters.end(); it++)
	{
		sqlitexx::Command sqlCommand(connection, ""
			"INSERT INTO CommandParameters(ParameterCode, ParameterValue, CommandID) "
			"VALUES (?001, ?002, ?003)");

		sqlCommand.BindParam(1, (int)it->parameterCode);
		sqlCommand.BindParam(2, it->parameterValue);
		sqlCommand.BindParam(3, commandID);

		sqlCommand.ExecuteNonQuery();
	}
}

int SqliteCommandsDal::CreateCommandFromLink(int linkID, Command::CommandGeneratedType generatedType)
{
	LOG_DEBUG("Create command from link ID:" << linkID);

	sqlitexx::Command sqlCommand1(connection, ""
		"INSERT INTO Commands (DeviceID, CommandCode, CommandStatus, TimePosted, ErrorCode, Generated)"
		" SELECT DL.FromDeviceID, CASE DL.LinkType WHEN 0 THEN ?006 WHEN 1 THEN ?007 ELSE -1 END, ?001, ?002, ?003, ?004"
		"  FROM DeviceLinks DL WHERE DL.LinkID = ?005;");

	sqlCommand1.BindParam(1, (int)Command::csNew);
	sqlCommand1.BindParam(2, nlib::CurrentUniversalTime());
	sqlCommand1.BindParam(3, (int)Command::rsNoStatus);
	sqlCommand1.BindParam(4, (int)generatedType);
	sqlCommand1.BindParam(5, linkID);
	sqlCommand1.BindParam(6, (int)Command::ccPublishSubscribe);
	sqlCommand1.BindParam(7, (int)Command::ccLocalLoop);

	sqlCommand1.ExecuteNonQuery();
	int newInsertedCommandID = sqlCommand1.GetLastInsertRowID();

	sqlitexx::Command sqlCommand2(connection, ""
		"INSERT INTO CommandParameters(CommandID, ParameterCode, ParameterValue)"
		" SELECT ?001, ParameterCode, ParameterValue"
		" FROM DeviceLinkParameters WHERE LinkID = ?002;");
	sqlCommand2.BindParam(1, newInsertedCommandID);
	sqlCommand2.BindParam(2, linkID);
	sqlCommand2.ExecuteNonQuery();

	return newInsertedCommandID;
}

bool SqliteCommandsDal::GetCommand(int commandID, Command& command)
{
	LOG_DEBUG("Try to find command with ID:" << commandID);

	sqlitexx::CSqliteStmtHelper oSql (connection);


	if (oSql.Prepare(
				"SELECT DeviceID, CommandCode, CommandStatus, TimePosted, TimeResponsed, ErrorCode, ErrorReason, Response, Generated"
				" FROM Commands WHERE CommandID = ?001;") != SQLITE_OK)
	{
		return false;
	}

	oSql.BindInt(1, commandID);

	if ( oSql.Step_GetRow() != SQLITE_ROW)
	{
		return false;
	}
		
	command.commandID = commandID;
	command.deviceID = oSql.Column_GetInt(0);//itCommand->Value<int>(0);
	command.commandCode = (Command::CommandCode)oSql.Column_GetInt(1);//(Command::CommandCode)itCommand->Value<int>(1);
	command.commandStatus = (Command::CommandStatus)oSql.Column_GetInt(2);//(Command::CommandStatus)itCommand->Value<int>(2);
	command.timePosted = oSql.Column_GetDateTime(3);//itCommand->ValueDateTime(3);
	if ( !oSql.Column_IsNull(4))					//if (!itCommand->IsNull(4))
	{
		command.timeResponded = oSql.Column_GetDateTime(4);//itCommand->ValueDateTime(4);
	}
	command.errorCode =(Command::ResponseStatus)oSql.Column_GetInt(5);		//(Command::ResponseStatus)itCommand->Value<int>(5);
	if ( !oSql.Column_IsNull(6))					//if (!itCommand->IsNull(6))
	{
		command.errorReason = oSql.Column_GetText(6);// itCommand->Value<std::string>(6);
	}
	if ( !oSql.Column_IsNull(7))	 //if (!itCommand->IsNull(7))
	{
		command.response = oSql.Column_GetText(7);// itCommand->Value<std::string>(7);
	}
	command.generatedType = (Command::CommandGeneratedType)oSql.Column_GetInt(8);	//(Command::CommandGeneratedType)itCommand->Value<int>(8);
	return true;
}

void SqliteCommandsDal::GetCommandParameters(int commandID, Command::ParametersList& commandParameters)
{
	LOG_DEBUG("Get parameters for command with ID:" << commandID);

	sqlitexx::CSqliteStmtHelper oSql (connection);

	if (oSql.Prepare(
		"SELECT ParameterCode, ParameterValue"
		" FROM CommandParameters WHERE CommandID = ?001;") != SQLITE_OK)
	{
		return;
	}

	oSql.BindInt(1, commandID);

	if (oSql.Step_GetRow() != SQLITE_ROW)
	{
		return;
	}

	commandParameters.reserve(8);

	do 
	{
		CommandParameter cp;
		cp.parameterCode = (CommandParameter::ParameterCode)oSql.Column_GetInt(0); //(CommandParameter::ParameterCode)itCommandParam->Value<int>(0);
		cp.parameterValue = oSql.Column_GetText(1);//itCommandParam->Value<std::string>(1);

		commandParameters.push_back(cp);
	}
	while(oSql.Step_GetRow() == SQLITE_ROW);
}

//void SqliteCommandsDal::GetNewCommands(CommandsList& newCommands)
//{
//	LOG_DEBUG("Get all new commands from database");
//	sqlitexx::Command sqlCommand(connection,
//			"SELECT CommandID, DeviceID, CommandCode, CommandStatus, TimePosted, Generated"
//				" FROM Commands WHERE CommandStatus = ?001");
//	sqlCommand.BindParam(1, (int)Command::csNew);
//
//	sqlitexx::ResultSetPtr rsCommands = sqlCommand.ExecuteQuery();
//	newCommands.reserve(newCommands.size() + rsCommands->RowsCount());
//	LOG_DEBUG(rsCommands->RowsCount() << " new commands found!");
//	for (sqlitexx::ResultSet::Iterator itCommand = rsCommands->Begin(); itCommand != rsCommands->End(); itCommand++)
//	{
//		Command cmd;
//		cmd.commandID = itCommand->Value<int>(0);;
//		cmd.deviceID = itCommand->Value<int>(1);
//		cmd.commandCode = (Command::CommandCode)itCommand->Value<int>(2);
//		cmd.commandStatus = (Command::CommandStatus)itCommand->Value<int>(3);
//		cmd.timePosted = itCommand->ValueDateTime(4);
//		cmd.generatedType = (Command::CommandGeneratedType)itCommand->Value<int>(5);
//
//		GetCommandParameters(cmd.commandID, cmd.parameters);
//		newCommands.push_back(cmd);
//	}
//}

void SqliteCommandsDal::GetNewCommands(CommandsList& newCommands)
{
	LOG_DEBUG("Get all new commands from database");

	if (!m_oNewCommand_Compiled.GetStmt())
	{
		LOG_DEBUG("m_oNewCommand_Compiled.Prepare");
		if( m_oNewCommand_Compiled.Prepare(
			"SELECT CommandID, DeviceID, CommandCode, CommandStatus, TimePosted, Generated"
			" FROM Commands WHERE CommandStatus = ?;") != SQLITE_OK)
		{
			LOG_ERROR("m_oNewCommand_Compiled.Prepare -- SQLITE_ERROR=" << (int)SQLITE_OK);
			return;
		}

		LOG_INFO("SqliteCommandsDal::GetNewCommands -- stmt compiled");
	}

	m_oNewCommand_Compiled.BindInt(1, (int)Command::csNew);

	int nNewCmds = 0;

	std::list<Command> oCmdList;
	while ( m_oNewCommand_Compiled.Step_GetRow() == SQLITE_ROW)
	{
		Command cmd;

		m_oNewCommand_Compiled.Column_GetInt( 0, &cmd.commandID);	//		cmd.commandID = itCommand->Value<int>(0);;
		m_oNewCommand_Compiled.Column_GetInt( 1, &cmd.deviceID); //cmd.deviceID = itCommand->Value<int>(1);
		
		
		cmd.commandCode =(Command::CommandCode) m_oNewCommand_Compiled.Column_GetInt( 2); //cmd.commandCode = (Command::CommandCode)itCommand->Value<int>(2);
		cmd.commandStatus = (Command::CommandStatus)m_oNewCommand_Compiled.Column_GetInt( 3); //cmd.commandStatus = (Command::CommandStatus)itCommand->Value<int>(3);
		m_oNewCommand_Compiled.Column_GetDateTime( 4, &cmd.timePosted); //cmd.timePosted = itCommand->ValueDateTime(4);
		cmd.generatedType = (Command::CommandGeneratedType)m_oNewCommand_Compiled.Column_GetInt( 5); //cmd.generatedType = (Command::CommandGeneratedType)itCommand->Value<int>(5);

		//char szTime[256];
		//szTime[0] = 0;

		//m_oNewCommand_Compiled.Column_GetText( 4, szTime, sizeof(szTime));
		
		LOG_DEBUG("GetNewCommands " << "cmdId=" << cmd.commandID << " deviceId=" << cmd.deviceID << " code=" << (int)cmd.commandCode << " status=" << (int)cmd.commandStatus
					);

		oCmdList.push_back(cmd);
		nNewCmds++;
	}


	std::list<Command>::iterator itCmds = oCmdList.begin();
	for (;itCmds != oCmdList.end(); itCmds++)
	{
		GetCommandParameters((*itCmds).commandID, (*itCmds).parameters);

		newCommands.push_back((*itCmds));
	}

	if (nNewCmds)
	{
		LOG_DEBUG(nNewCmds << " new commands found!"); 
	}
	
}

void SqliteCommandsDal::SetCommandAsSent(Command& command)
{
	LOG_DEBUG("Update command:" << command.commandID << " as sent!");
	sqlitexx::Command sqlCommand(connection, ""
		"UPDATE Commands SET CommandStatus = ?001"
		" WHERE CommandID = ?002");
	sqlCommand.BindParam(1, (int)Command::csSent);
	sqlCommand.BindParam(2, command.commandID);

	sqlCommand.ExecuteNonQuery();

	//now update also model
	command.commandStatus = Command::csSent;
}

void SqliteCommandsDal::SetCommandAsResponded(Command& command, const nlib::DateTime& responseTime,
		const std::string& response)
{
	LOG_DEBUG("Update command:" << command.commandID << " as responded!");
	sqlitexx::Command sqlCommand(connection, ""
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

void SqliteCommandsDal::SetCommandAsFailed(Command& command, const nlib::DateTime& errorTime, int errorCode,
		const std::string& errorReason)
{
	LOG_DEBUG("Update command:" << command.commandID << " as failed");
	sqlitexx::Command sqlCommand(connection, ""
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

void SqliteCommandsDal::SetCommandsAsExpired(const nlib::DateTime& oldestThan, const nlib::DateTime& errorTime,
		const std::string& errorReason)
{
	LOG_DEBUG("Set expired commands older than:" << sqlitexx::detail::ToDbString(oldestThan));
	sqlitexx::Command sqlCommand(connection, ""
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
void SqliteCommandsDal::SetDBCmdID_as_new(int DBCmsID)
{
	LOG_DEBUG("Update DBCmdID:" << (boost::int32_t)DBCmsID << " as new");
	sqlitexx::Command sqlCommand(connection, ""
		"UPDATE Commands SET "
		" CommandStatus = ?001 "
		" WHERE CommandID = ?002");

	sqlCommand.BindParam(1, (int)Command::csNew);
	sqlCommand.BindParam(2, DBCmsID);

	sqlCommand.ExecuteNonQuery();
}


bool SqliteCommandsDal::GetNextFirmwareUpload(FirmwareUpload& result, int notFailedForMinutes)
{
	//TODO
	return false;
}


}
//hostapp
}
//nisa100
