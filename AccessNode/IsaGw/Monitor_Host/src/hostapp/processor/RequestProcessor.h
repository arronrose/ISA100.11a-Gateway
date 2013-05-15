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

#ifndef REQUESTPROCESSOR_H_
#define REQUESTPROCESSOR_H_

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../Log.h"

#include "ProcessorExceptions.h"

#include "../model/Command.h"
#include "../commandmodel/IGServiceVisitor.h"

namespace nisa100 {
namespace hostapp {

class CommandsProcessor;

class RequestProcessor : public IGServiceVisitor
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.RequestProcessor")
	*/

public:
	void Process(AbstractGServicePtr service_, Command& command_, CommandsProcessor& processor_);

private:
	AbstractGServicePtr service;
	Command* command;
	CommandsProcessor* processor;

	//IGServiceVisitor

	//added by Cristian.guef
	void Visit(GSession& session);

	//added by Cristian.Guef
	void Visit(GDeviceListReport& deviceListReport);
	void Visit(GNetworkHealthReport& networkListReport);
	void Visit(GScheduleReport& scheduleReport);
	void Visit(GNeighbourHealthReport& neighbourHealthReport);
	void Visit(GDeviceHealthReport& devHealthReport);
	void Visit(GNetworkResourceReport& netResourceReport);
	void Visit(GAlert_Subscription& alertSubscription);
	void Visit(GAlert_Indication& alertIndication);

	//added by Cristian.Guef
	void Visit(GClientServer<GetSizeForPubIndication>& getSizeForPubIndication);

	void Visit(GTopologyReport& topologyReport);

	//added by Cristian.Guef
	void Visit(GDelContract& contract);

	void Visit(GContract& contract);
	void Visit(GBulk& bulk);
	void Visit(GClientServer<WriteObjectAttribute>& writeAttribute);

	void Visit(GClientServer<ReadMultipleObjectAttributes>& multipleReadObjectAttributes);

	void Visit(PublishSubscribe& publishSubscribe);
	void Visit(GClientServer<Publish>& publish);
	void Visit(GClientServer<Subscribe>& subscribe);
	void Visit(PublishIndication& publishIndication);

	void Visit(GClientServer<GetFirmwareVersion>& getFirmwareVersion);
	void Visit(GClientServer<FirmwareUpdate>& startFirmwareUpdate);
	void Visit(GClientServer<GetFirmwareUpdateStatus>& getFirmwareUpdateStatus);
	void Visit(GClientServer<CancelFirmwareUpdate>& cancelFirmwareUpdate);

	//added by Cristian.Guef
	void Visit(GClientServer<GetContractsAndRoutes>& getContractsAndRoutes);
	void Visit(GClientServer<GISACSRequest>& CSRequest);

	void Visit(GClientServer<ResetDevice>& resetDevice);
	
	//added
	void Visit(GSensorFrmUpdateCancel &sensorFrmUpdateCancel);
	void Visit(GClientServer<GetChannelsStatistics>& getChannels);


	DevicePtr GetDevice(int deviceID);
	DevicePtr GetRegisteredDevice(int deviceID);
	
	/* commented by Cristian.Guef
	DevicePtr GetContractDevice(int deviceID);
	*/
	//added by Cristian.Guef
	DevicePtr GetContractDevice(unsigned int resourceID, unsigned char leaseType, int deviceID);
};

}
}

#endif /*REQUESTPROCESSOR_H_*/
