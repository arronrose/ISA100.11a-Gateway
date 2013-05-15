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

#ifndef IGSERVICEVISITOR_H_
#define IGSERVICEVISITOR_H_

#include "AbstractGService.h"
#include "GTopologyReport.h"

//added by Cristian.Guef
#include "GDelContract.h"

#include "GContract.h"
#include "GBulk.h"

//added by Cristian.Guef
#include "GSession.h"

//added by Cristian.Guef
#include "GDeviceListReport.h"
#include "GNetworkHealthReport.h"
#include "GScheduleReport.h"
#include "GNeighbourHealthReport.h"
#include "GDeviceHealthReport.h"
#include "GNetworkResourceReport.h"
#include "GAlert_Subscription.h"
#include "GAlert_Indication.h"

#include "QueryObject.h"
#include "PublishSubscribe.h"
#include "Firmware.h"

//added by Cristian.Guef
#include "GetContractsAndRoutes.h"
#include "GISACSRequest.h"

//added
#include "GSensorFrmUpdateCancel.h"
#include "GetChannelsStatistics.h"

#include "ResetDevice.h"

#include <boost/format.hpp>

namespace nisa100 {
namespace hostapp {


class GClientServerBase : public AbstractGService
{
public:
	/* commented by Cristian.Guef
	boost::uint16_t ContractID;
	*/
	//added by Cristian.Guef
	boost::uint32_t ContractID;

	boost::uint8_t Cache;
};

template <typename ClientCommand>
class GClientServer : public GClientServerBase
{
public:
	const std::string ToString() const
	{
		return boost::str(boost::format("GClientServer[CommandID=%1% Status=%2% LeaseID=%3% Client=<%4%>]")
			% CommandID % (int)Status % (int) ContractID % Client.ToString());
	}

	bool Accept(IGServiceVisitor& visitor);

	ClientCommand Client;
};


/**
 * @brief The Visitor class.
 */
class IGServiceVisitor
{
public:
	virtual ~IGServiceVisitor()
	{
	}

	//added by Cristian.Guef
	virtual void Visit(GSession& session) = 0;

	//added by Cristian.Guef
	virtual void Visit(GDeviceListReport& deviceListReport) = 0;
	virtual void Visit(GNetworkHealthReport& networkHealthReport) = 0;
	virtual void Visit(GScheduleReport& scheduleReport) = 0;
	virtual void Visit(GNeighbourHealthReport& neighbourReport) = 0;
	virtual void Visit(GDeviceHealthReport& devHealthReport) = 0;
	virtual void Visit(GNetworkResourceReport& netResourceReport) = 0;
	virtual void Visit(GAlert_Subscription& alertSubscription) = 0;
	virtual void Visit(GAlert_Indication& alertIndication) = 0;


	//added by Cristian.Guef
	virtual void Visit(GClientServer<GetSizeForPubIndication>& getSizeForPubIndication) = 0;

	virtual void Visit(GTopologyReport& topologyReport) = 0;

	//added by Cristian.Guef
	virtual void Visit(GDelContract& contract) = 0;

	virtual void Visit(GContract& contract) = 0;
	virtual void Visit(GBulk& bulk) = 0;

	virtual void Visit(GClientServer<WriteObjectAttribute>& writeAttribute) = 0;
	
	virtual void Visit(GClientServer<ReadMultipleObjectAttributes>& multipleReadObjectAttributes) = 0;

	virtual void Visit(PublishSubscribe& publishSubscribe) = 0;
	virtual void Visit(GClientServer<Publish>& publish) = 0;
	virtual void Visit(GClientServer<Subscribe>& subscribe) = 0;
	virtual void Visit(PublishIndication& publishIndication) = 0;

	virtual void Visit(GClientServer<GetFirmwareVersion>& getFirmwareVersion) = 0;
	virtual void Visit(GClientServer<FirmwareUpdate>& startFirmwareUpdate) = 0;
	virtual void Visit(GClientServer<GetFirmwareUpdateStatus>& getFirmwareUpdateStatus) = 0;
	virtual void Visit(GClientServer<CancelFirmwareUpdate>& cancelFirmwareUpdate) = 0;

	//added by Cristian.Guef
	virtual void Visit(GClientServer<GetContractsAndRoutes>& getContractsAndRoutes) = 0;
	virtual void Visit(GClientServer<GISACSRequest>& CSRequest) = 0;

	virtual void Visit(GClientServer<ResetDevice>& resetDevice) = 0;

	//added
	virtual void Visit(GSensorFrmUpdateCancel &sensorFrmUpdateCancel) = 0;
	virtual void Visit(GClientServer<GetChannelsStatistics>& getChannels) = 0;
};

template <typename ClientCommand>
inline
bool GClientServer<ClientCommand>::Accept(IGServiceVisitor& visitor)
{
	visitor.Visit(*this);
	return true;
}

}  // namespace hostapp
}  // namespace nisa100

#endif /* IGSERVICEVISITOR_H_ */
