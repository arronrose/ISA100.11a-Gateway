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


#ifndef MYSQLCOMMANDSDAL_H_
#define MYSQLCOMMANDSDAL_H_


///* commented by Cristian.Guef
//#include <nlib/log.h>
//*/
//added by Cristian.Guef
#include "../../../Log.h"

#include "MySQLDatabase.h"
#include "../ICommandsDal.h"

namespace nisa100 {
namespace hostapp {

class MySqlCommandsDal : public ICommandsDal
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.dal.MySqlCommandsDal");
	*/

public:
	MySqlCommandsDal(MySQLConnection& connection);
	virtual ~MySqlCommandsDal();

public:
	void VerifyTables();
private:
	void CreateCommand(Command& command);
	void CreateCommandParameters(int commandID, Command::ParametersList& commandParameters);
	int CreateCommandFromLink(int linkID, Command::CommandGeneratedType generatedType);

	bool GetCommand(int commandID, Command& command);
	void GetNewCommands(CommandsList& newCommands);
	void GetCommandParameters(int commandID, Command::ParametersList& commandParameters);

	void SetCommandAsSent(Command& command);
	void SetCommandAsResponded(Command& command, const nlib::DateTime& responseTime, const std::string& response);
	void SetCommandAsFailed(Command& command, const nlib::DateTime& failedTime, int errorCode, const std::string& errorReason);

	void SetCommandsAsExpired(const nlib::DateTime& oldestThan, const nlib::DateTime& errorTime,
			const std::string& errorReason);

	bool GetNextFirmwareUpload(FirmwareUpload& result, int notFailedForMinutes);

	//added by Cristian.Guef
	void SetDBCmdID_as_new(int DBCmsID);

	MySQLConnection& connection;
};

} //namespace hostapp
} //namespace nisa100

#endif /*MYSQLCOMMANDSDAL_H_*/

#endif
