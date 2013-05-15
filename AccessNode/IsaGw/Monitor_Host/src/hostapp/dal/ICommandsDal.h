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

#ifndef ICOMMANDSDAL_H_
#define ICOMMANDSDAL_H_

#include "../model/Command.h"
#include "../model/FirmwareUpload.h"

namespace nisa100 {
namespace hostapp {

class ICommandsDal
{
public:
	virtual ~ICommandsDal()
	{
	}

	virtual void CreateCommand(Command& command) = 0;
	virtual int CreateCommandFromLink(int linkID, Command::CommandGeneratedType generatedType) = 0;
	virtual bool GetCommand(int commandID, Command& command) = 0;
	virtual void GetCommandParameters(int commandID, Command::ParametersList& commandParameters) = 0;
	virtual void GetNewCommands(CommandsList& newCommands) = 0;

	virtual void SetCommandAsSent(Command& command) = 0;
	virtual void SetCommandAsResponded(Command& command, const nlib::DateTime& responseTime, const std::string& response) = 0;
	virtual void SetCommandAsFailed(Command& command, const nlib::DateTime& failedTime, int errorCode,
			const std::string& errorReason) = 0;

	virtual void SetCommandsAsExpired(const nlib::DateTime& oldestThan, const nlib::DateTime& errorTime,
			const std::string& errorReason) = 0;


	virtual bool GetNextFirmwareUpload(FirmwareUpload& result, int notFailedForMinutes) = 0;

	//added by Cristian.Guef
	virtual void SetDBCmdID_as_new(int DBCmsID) = 0;
};

} // namespace hostapp
} // namespace nisa100


#endif /*ICOMMANDSDAL_H_*/
