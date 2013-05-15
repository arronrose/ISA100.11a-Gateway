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
#ifndef _DOPRECONFIGURESUBSCRIBERLEASE_H_
#define _DOPRECONFIGURESUBSCRIBERLEASE_H_

#include "../CommandsManager.h"
#include "../DevicesManager.h"
#include "../../ConfigApp.h"
#include "DoDeleteLeases.h"

#include "../../Log.h"

#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {


extern bool IsGatewayReconnected;
extern bool DoDeleteSubscriberLease;


/**
 * send publish_subscribe commnad for every preconfigured publisher
 */
class DoPreconfigureSubscriberLease_
{

public:
	DoPreconfigureSubscriberLease_(CommandsManager& commands_, DevicesManager& devices_,
		ConfigApp& configApp_, DoDeleteLeases & doDeleteLeases_) :
		commands(commands_), devices(devices_), configApp(configApp_),
			doDeleteLeases(doDeleteLeases_)
	{
		LOG_INFO("ctor: DoPreconfigureSubscriberLease started.");
	}

private:

	void IssuePublishSubscribeCmd(const MAC& targetDevice, int devID, const PublisherConf::COData &coData, PublisherConf::COChannelListT &list) ;
	void IssuePublishSubscribeCmd(const MAC& targetDevice, int devID, const PublisherConf::COData &coData, 
		PublisherConf::COChannelListT &list, PublisherConf::ChannelIndexT &index) ;

private:
	void ReloadPublishersWithSignal();
	void DeleteLeases();
	void ConfigurePublishers(int &pubHasSubLease);
	bool IsAnyLeaseSent();
	int DoDeleteLease(const DevicePtr devPtr, unsigned short tsapID, unsigned short objID);
	int GetChannelsDifference(const PublisherConf::COChannelListT &storedList, const PublisherConf::ChannelIndexT &storedIndex,
							PublisherConf::COChannelListT &loadedList, const PublisherConf::ChannelIndexT &loadedIndex);
	// rez
	// 0 - do not update channels list in cache
	// 1 - do update channels list in cache
	int ProcessChannelsDifference(int loadedDeviceID,
				const PublisherConf::COChannelListT &storedList, const PublisherConf::ChannelIndexT &storedIndex,
				PublisherConf::COChannelListT &loadedList, const PublisherConf::ChannelIndexT &loadedIndex);
public:
	void Check() ;
private:
	CommandsManager& commands;
	DevicesManager& devices;
	ConfigApp& configApp;
	DoDeleteLeases &doDeleteLeases;
};

}//namespace hostapp
}//namespace nisa100



#endif
