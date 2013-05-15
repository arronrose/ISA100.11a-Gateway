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

#ifndef TRACKINGMANAGER_H_
#define TRACKINGMANAGER_H_

#include <map>
#include <set>

/* commented by Cristian.Guef
#include <boost/thread.hpp>
*/

#include <boost/function.hpp> //for callback

/* commented by Cristian.Guef
#include <nlib/log.h>
*/
//added by Cristian.Guef
#include "../Log.h"

#include <nlib/datetime.h>
#include <boost/format.hpp>

#include "model/Command.h"
#include "commandmodel/AbstractGService.h"
#include "commandmodel/PublishSubscribe.h"
#include "../gateway/GeneralPacket.h"
#include "commandmodel/IGServiceVisitor.h"

//added
#include "PublishIndicationsMng.h"

namespace nisa100 {
namespace hostapp {

//TODO [nicu.dascalu] - implement expiration here

/**
 * @brief Converts CommandRequest to GService and sent to Gateway and vice versa.
 */
class TrackingManager
{
	/* commented by Cristian.Guef
	LOG_DEF("nisa100.hostapp.TrackingManager");
	*/

public:
	TrackingManager();

public:
	void SendRequest(AbstractGServicePtr request, nlib::TimeSpan timeout, int resendIfTimeoutRetryCount);
	
	/* commented by Cristian.Guef
	boost::function1<void, AbstractGServicePtr> ReceiveResponse;
	*/
	//added by Cristian.Guef
	boost::function2<void, AbstractGServicePtr, bool> ReceiveResponse;

	boost::function1<void, gateway::GeneralPacket&> SendPacket;
	void ReceivePacket(const gateway::GeneralPacket& packet);

	void HandleDisconenct();
	void CheckTimeoutRequests();

	void RegisterPublishIndication(int deviceID, boost::uint8_t channelID, boost::int32_t filterTrackingID,
	  PublishIndicationPtr indication);
	/**
	 * Unregister a publish indication
	 */
	void UnregisterPublishIndication(int deviceID, boost::uint8_t channelID);

	//added by Cristian.Guef
	void UnregisterPublishIndication2(boost::int32_t filterTrackingID, boost::uint8_t channelID, IPv6 &IPAddress);

	/**
	 * Unregister all publish indication for a device, cause the device is unregistered..
	 */
	void UnregisterPublishIndications(int deviceID);

private:
	void HandleResponse(AbstractGServicePtr, int retryCount, nlib::TimeSpan timeoutTime);
private:

	struct TrackingCommand
	{
public:
		AbstractGServicePtr GService;
		nlib::DateTime TimeoutDate;
		const nlib::TimeSpan TimeoutTime;

		int ResendIfTimeoutRetryCount;

		TrackingCommand(AbstractGServicePtr gservice_, const nlib::TimeSpan& timeout_, int resendIfTimeoutRetryCount_) :
			GService(gservice_), TimeoutTime(timeout_), ResendIfTimeoutRetryCount(resendIfTimeoutRetryCount_)
		{
			TimeoutDate = nlib::CurrentUniversalTime()+ TimeoutTime;
		}
	};

	typedef std::map<boost::int32_t, TrackingCommand> TrackingsMapT;

	struct RegisteredPublishIndication
	{
public:
		const AbstractGServicePtr GService;

		const int PublisherDeviceID;

		// There can be more than one TrackingCommand with the same TrackingID
		std::set<boost::uint8_t> PublishedChannels;

		RegisteredPublishIndication(AbstractGServicePtr gservice_, int publisherDeviceID_) :
			GService(gservice_), PublisherDeviceID(publisherDeviceID_)
		{
		}

		std::string ToString() const
		{
			std::ostringstream channels;
			for (std::set<boost::uint8_t>::const_iterator it = PublishedChannels.begin(); it != PublishedChannels.end(); it++)
			{
				channels << (int)(*it) << ", ";
			}
			return boost::str(boost::format("RegisteredPublishIndication[ DeviceID=%1%, Channels=<%2%> ]")
			    % PublisherDeviceID % channels.str());
		}
	};

	typedef std::map<boost::int32_t, RegisteredPublishIndication> RegisteredPublishIndicationsMapT;

	//holds the list of pending commands
	boost::int32_t nextTrackingID;
	TrackingsMapT pendingTrackings;

	//holds the registered publish indications
	RegisteredPublishIndicationsMapT registeredIndications;

	/* commented by Cristian.Guef
	boost::mutex mutex;
	*/

//added by Cristian.Guef
private:
	AbstractGServicePtr m_alertIndication;
public:
	void RegisterAlertIndication(AbstractGServicePtr alertInd)
	{
		m_alertIndication = alertInd;
	}
	void UnregisterAlertIndication()
	{
		m_alertIndication.reset();
	}


//added
private:
	PublishIndicationsMng m_indicationsMng;

	//helper class
	class PrintRegisteredIndications
	{
		LOG_DEF("nisa100.hostapp.TrackingManager.PrintRegisteredIndications");
public:
		PrintRegisteredIndications(TrackingManager& trackingManager);
		void Print();

private:
		TrackingManager& trackingManager;
		nlib::DateTime lastPrint;

	} logger;
};

class ManageTrackedTimeouts : public IGServiceVisitor
{
	LOG_DEF("nisa100.hostapp.TrackingManager.ManageTrackedTimeouts");
public:
	ManageTrackedTimeouts()
	{
		isTimeouted = false;
	}
	bool IsTimeoutedSpecialHandled(AbstractGService& response)
	{
		response.Accept(*this);
		return isTimeouted;
	}
private:

	//added by Cristian.Guef
	void Visit(GSession& session)
	{
		isTimeouted = false;
	}

	//added by Cristian.Guef
	void Visit(GDeviceListReport& devList)
	{
		isTimeouted = false;
	}
	//added by Cristian.Guef
	void Visit(GNetworkHealthReport& netHealth)
	{
		isTimeouted = false;
	}
	//added by Cristian.Guef
	void Visit(GScheduleReport& scheduleReport)
	{
		isTimeouted = false;
	}
	//added by Cristian.Guef
	void Visit(GNeighbourHealthReport& neighbourHealthReport)
	{
		isTimeouted = false;
	}
	//added by Cristian.Guef
	void Visit(GDeviceHealthReport& devHealthReport)
	{
		isTimeouted = false;
	}
	//added by Cristian.Guef
	void Visit(GNetworkResourceReport& netResourceReport)
	{
		isTimeouted = false;
	}
	//added by Cristian.Guef
	void Visit(GAlert_Subscription& alertSubscription)
	{
		isTimeouted = false;
	}
	//added by Cristian.Guef
	void Visit(GAlert_Indication& alertIndication)
	{
		isTimeouted = false;
	}

	void Visit(GTopologyReport& topology)
	{
		isTimeouted = false;
	}

	//added by Cristian.Guef
	void Visit(GDelContract& contract)
	{
		isTimeouted = false;
	}

	void Visit(GContract& contract)
	{
		isTimeouted = false;
	}
	void Visit(GBulk& bulk)
	{
		isTimeouted = false;
	}
	void Visit(GClientServer<WriteObjectAttribute>& writeAttribute)
	{
		if (writeAttribute.Status == Command::rsFailure_GatewayTimeout || writeAttribute.Status
		    == Command::rsFailure_HostTimeout)
		{
			isTimeouted = true;
		}
		else
		{
			isTimeouted = false;
		}
	}
	void Visit(GClientServer<ReadMultipleObjectAttributes>& multipleReadObjectAttributes)
	{
		if (multipleReadObjectAttributes.Status == Command::rsFailure_GatewayTimeout || multipleReadObjectAttributes.Status
		    == Command::rsFailure_HostTimeout)
		{
			isTimeouted = true;
		}
		else
		{
			isTimeouted = false;
		}
	}
	void Visit(PublishSubscribe& publishSubscribe)
	{
		isTimeouted = false;
	}
	void Visit(GClientServer<Publish>& publish)
	{
		if (publish.Status == Command::rsFailure_GatewayTimeout || publish.Status == Command::rsFailure_HostTimeout)
		{
			isTimeouted = true;
		}
		else
		{
			isTimeouted = false;
		}
	}
	void Visit(GClientServer<Subscribe>& subscribe)
	{
		if (subscribe.Status == Command::rsFailure_GatewayTimeout || subscribe.Status == Command::rsFailure_HostTimeout)
		{
			isTimeouted = true;
		}
		else
		{
			isTimeouted = false;
		}
	}

	//added by Cristian.Guef
	void Visit(GClientServer<GetSizeForPubIndication>& getSizeForPubIndication)
	{
		isTimeouted = false;
	}

	void Visit(PublishIndication& publishIndication)
	{
		isTimeouted = false;
	}

	void Visit(GClientServer<GetFirmwareVersion>& getFirmwareVersion)
	{
		isTimeouted = false;
	}
	void Visit(GClientServer<FirmwareUpdate>& startFirmwareUpdate)
	{
		isTimeouted = false;
	}
	void Visit(GClientServer<GetFirmwareUpdateStatus>& getFirmwareUpdateStatus)
	{
		isTimeouted = false;
	}
	void Visit(GClientServer<CancelFirmwareUpdate>& cancelFirmwareUpdate)
	{
		isTimeouted = false;
	}
	
	//added by Cristian.Guef
	void Visit(GClientServer<GetContractsAndRoutes>& getContractsAndRoutes)
	{
		isTimeouted = false;
	}
	void Visit(GClientServer<GISACSRequest>& CSRequest)
	{
		isTimeouted = false;
	}

	void Visit(GClientServer<ResetDevice>& resetDevice)
	{
		isTimeouted = false;
	}

	//added
	void Visit(GSensorFrmUpdateCancel &sensorFrmUpdateCancel)
	{
	}
	void Visit(GClientServer<GetChannelsStatistics>& getChannels)
	{
	}
	
private:
	bool isTimeouted;
};

} //namespace hostapp
} //namespace nisa100

#endif /*TRACKINGMANAGER_H_*/
