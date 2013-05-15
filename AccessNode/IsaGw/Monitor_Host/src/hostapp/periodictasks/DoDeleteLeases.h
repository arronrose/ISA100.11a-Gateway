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
#ifndef DODELETELESEASES_H_
#define DODELETELESEASES_H_

#include "../CommandsManager.h"

#include "../../Log.h"
#include <boost/lexical_cast.hpp>

#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {


extern bool IsGatewayReconnected;

/**
 * send publish_subscribe commnad for every preconfigured publisher
 */
class DoDeleteLeases
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.DoDeleteLeases");
	*/

public:
	struct InfoForDelContract
	{
		int				DevID;
		IPv6			IPAddress;
		unsigned int	ResourceID;
		unsigned char	LeaseType;
		bool			GSAPsent;
	};

public:
	DoDeleteLeases(CommandsManager& commands_,  DevicesManager& devices_) :
		commands(commands_), devices(devices_)
	{
		LOG_INFO("ctor: DoDeleteLeases started.");
	}


	void IssueDeleteContractCmd(unsigned int contractID, InfoForDelContract& infoForDelContract) ;

	void Check() ;

private:
	CommandsManager& commands;
	DevicesManager& devices;
	std::map<unsigned int, InfoForDelContract> m_InfoForDelContract;

public:
	void AddNewInfoForDelLease(unsigned int ContractID, InfoForDelContract &infoToDelLease) ;

	InfoForDelContract GetInfoForDelLease(unsigned int ContractID);

	void DelInfoForDelLease(unsigned int ContractID) ;

	void SetDeleteContractID_NotSent(unsigned int ContractID) ;
};

}//namespace hostapp
}//namespace nisa100



#endif
