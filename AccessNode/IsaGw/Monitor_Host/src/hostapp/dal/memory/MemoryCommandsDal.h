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

#ifndef MEMORYCOMMANDSDAL_H_
#define MEMORYCOMMANDSDAL_H_

#include "../ICommandsDal.h"

namespace nisa100 {
namespace hostapp {

class MemoryCommandsDal : public ICommandsDal
{
public:
	MemoryCommandsDal()
	{		
	}
	virtual ~MemoryCommandsDal()
	{		
	}
private:
	void CreateCommand(Command& command)
	{	
	}
	
	int CreateCommandFromLink(int linkID, Command::CommandGeneratedType generatedType)
	{
		return -1;
	}
	
	bool GetCommand(int commandID, Command& command)
	{
		return true;
	}
	
	void GetCommandParameters(int commandID, Command::ParametersList& commandParameters)
	{	
	}
	
	void GetNewCommands(CommandsList& newCommands)
	{	
	}
	
	void SetCommandAsSent(Command& command)
	{	
	}
	
	void SetCommandAsResponded(Command& command, const nlib::DateTime& responseTime, const std::string& response)
	{	
	}
	
	void SetCommandAsFailed(Command& command, const nlib::DateTime& failedTime, int errorCode, const std::string& errorReason)
	{	
	}
	
	void SetCommandsAsExpired(const nlib::DateTime& oldestThan, const nlib::DateTime& errorTime, const std::string& errorReason)
	{
	}
	
	bool GetNextFirmwareUpload(FirmwareUpload& result, int notFailedForMinutes)
	{
		return false;
	}

	//added by Cristian.Guef
	void SetDBCmdID_as_new(int DBCmsID)
	{
		//it should be defined
	}
};

}
}
#endif /*MEMORYCOMMANDSDAL_H_*/
