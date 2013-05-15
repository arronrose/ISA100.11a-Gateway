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

#ifndef RESPONSEPROCESSOR_H_
#define RESPONSEPROCESSOR_H_

#include "ProcessorExceptions.h"

#include "../model/Command.h"
#include "../model/Device.h"
#include "../commandmodel/IGServiceVisitor.h"
#include "../commandmodel/Nisa100ObjectIDs.h"


/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../../Log.h"
#include "PublishedDataMng.h"


namespace nisa100 {
namespace hostapp {


class CommandsProcessor;
class GClientServerBase;

class ResponseProcessor : public IGServiceVisitor
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.ResponseProcessor")
	*/

public:
	/*commented by Cristian.Guef
	void Process(AbstractGService& response, Command& command, CommandsProcessor& processor);
	*/
	//added by Cristian.Guef
	void Process(AbstractGServicePtr response, Command& command, CommandsProcessor& processor);

private:
	CommandsProcessor* processor;
	Command* command;

	//added by Cristian.Guef
	AbstractGServicePtr response;

	//IGServiceVisitor

	//added by Cristian.guef
	void Visit(GSession& session);

	//added by Cristian.Guef
	void Visit(GDeviceListReport& deviceListReport);
	void Visit(GNetworkHealthReport& networkHealthReport);
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
	void SaveSingleReadAttribute(ReadMultipleObjectAttributes::ObjectAttribute& singleRead);
	void SaveMultipleReadAttribute(ReadMultipleObjectAttributes::AttributesList& multipleRead);

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

	
	void PublishSubscribeCompleted(PublishSubscribe& publishSubscribe);
	
	DevicePtr GetDevice(int deviceID);
	void CreateCancelCommand(int deviceID, int subscriberDeviceID, int subscriberChannelID,
		int failedPublishSbscribeCommand, bool isLocalLoop);

private:
	
	//added by Cristian.Guef
	void SaveNeighbourHealthHistoryToDB(GNeighbourHealthReport& neighbourHealthReport);
	void SaveDeviceHealthHistoryToDB(GDeviceHealthReport& devHealthReport);


	//added by Cristian.Guef
	void FlushDeviceReading(int pubHandle, int devID, std::basic_string<unsigned char> &DataBuff, struct timeval &tv, bool isFirstTime, bool &isDataSaved);

	void SaveFirstPublishedData(PublishIndication& publishIndication, DevicePtr device);
	void InitPublishEnviroment(PublisherConf::COChannelListT &list, unsigned char dataVersion, DevicePtr device);

	//added by Cristian.Guef
	void ClearDataForDev(GDelContract& contract, int &DevID, IPv6 &IPAddress);


	//added by Cristian.Guef
	void IssueReportReqs();
	void SaveTopology(Command::ResponseStatus topoStatus);
	void LoadSubscribeLeaseForDel();

	//added by Cristian.Guef
	bool Is_CS_LeaseToSendGetSizeReq(DevicePtr publisherDevice, PublishIndication& publishIndication);
	void Get_CS_LeaseToSendGetSizeReq(DevicePtr publisherDevice, PublishIndication& publishIndication);
	void SendGetSizeReq(DevicePtr publisherDevice, PublishIndication& publishIndication);
	
};

} //namespace hostapp
} //namespace nisa100

#endif /*RESPONSEPROCESSOR_H_*/
