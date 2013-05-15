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

#include <nlib/exception.h>

#include "TrackingManager.h"
#include "commandmodel/AbstractGService.h"
#include "commandmodel/QueryObject.h"

#include "serialization/SerializationException.h"
#include "serialization/GServiceSerializer.h"
#include "serialization/GServiceUnserializer.h"

//added by Cristian.Guef
#include "../util/serialization/Serialization.h"
#include "../util/serialization/NetworkOrder.h"
#include <list>
#include <../AccessNode/Shared/DurationWatcher.h>

#include <arpa/inet.h>

namespace nisa100 {
namespace hostapp {

TrackingManager::TrackingManager() :
	logger(*this)
{
	nextTrackingID = 1;
}

void TrackingManager::SendRequest(AbstractGServicePtr request, nlib::TimeSpan timeout, int resendIfTimeoutRetryCount)
{
	boost::int32_t trackingID;
	{
		/* commented by Cristian.Guef
		boost::mutex::scoped_lock lk(mutex); //protect shared resource pendingTrackings && registeredIndications
		*/

		trackingID = nextTrackingID + 1;
		{
			//test in pending trackings
			TrackingsMapT::const_iterator found = pendingTrackings.find(trackingID);
			if (found != pendingTrackings.end())
			{
				LOG_ERROR("SendRequest: failed! Already exists TrackingID=" << trackingID << " ExistingTrackedCommand="
				    << found->second.GService->ToString() << " NewRequest=" << request->ToString());
				THROW_EXCEPTION1(nlib::Exception, "Duplicated TrackingID (in trackings)!");
			}
		}

		/* commented by Cristian.Guef
		//there is no need to check in "indication pendings" because 
		//in there transactionID = leaseID
		{
			//test in pending publish trackings
			RegisteredPublishIndicationsMapT::const_iterator found = registeredIndications.find(trackingID);
			if (found != registeredIndications.end())
			{
				LOG_ERROR("SendRequest: failed! Already exists TrackingID=" << trackingID << " ExistingPublishIndication="
				    << found->second.GService->ToString() << " NewRequest=" << request->ToString());
				THROW_EXCEPTION1(nlib::Exception, "Duplicated TrackingID (in publish indications)!");
			}
		}
		*/

		nextTrackingID = nextTrackingID + 1; //increment next id
	}

	gateway::GeneralPacket packet;
	packet.trackingID = trackingID;

	GServiceSerializer serializer;
	serializer.Serialize(*request, packet);

	try
	{
		SendPacket(packet);
		{
			/* commented by Cristian.Guef
			boost::mutex::scoped_lock lk(mutex); //protect shared resource pendingTrackings && registeredIndications
			*/
			pendingTrackings.insert(TrackingsMapT::value_type(trackingID, TrackingCommand(request, timeout, resendIfTimeoutRetryCount)));
		}
		LOG_DEBUG("SendRequest: Timeout=" << nlib::ToString(timeout) << ", Request=" << request->ToString() << " Packet=" << packet.ToString());
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("SendRequest: failed! Request=" << request->ToString() << " Packet=" << packet.ToString() << " error="
				<< ex.what());
		throw;
	}
}

void TrackingManager::ReceivePacket(const gateway::GeneralPacket& packet)
{
	
	bool processNow = true;
	
	AbstractGServicePtr response;
	int savedTimeoutRetry = 0;
	nlib::TimeSpan savedTimeoutTime;
	//now there is another situation there are 2 diffrent transactionIDs
	//one for IndicationsPendinglist and the other for "anotherpendinglist"
	if (packet.serviceType != packet.GPublishIndication &&
		   packet.serviceType != packet.GSubscribeTimer &&
		   packet.serviceType != packet.GWatchdogTimer &&
		   packet.serviceType != packet.GAlertSubscriptionIndication)
	{
		TrackingsMapT::iterator foundTracking;
		if ( (foundTracking = pendingTrackings.find(packet.trackingID)) != pendingTrackings.end())
		{
			response = foundTracking->second.GService;
			savedTimeoutRetry = foundTracking->second.ResendIfTimeoutRetryCount;
			savedTimeoutTime = foundTracking->second.TimeoutTime;
			pendingTrackings.erase(foundTracking);
		}
		else
		{
			LOG_WARN("ReceivePacket: with unknown TrackingID! Packet=" << packet.ToString());
			return;
		}
			
		processNow = false;
	}
	else
	{
		if (packet.serviceType == packet.GAlertSubscriptionIndication)
		{
			if (!m_alertIndication)
			{
				LOG_WARN("Received AlertIndication! but it shoudn't. Packet=" << packet.ToString());
				return;
			}
			if (!(response = m_alertIndication->Clone()))
			{
				LOG_ERROR("Received AlertIndication! The response is not cloneable! Response="
						<< m_alertIndication->ToString());
				return;
			}
				
			processNow = false;
		}
		else
		{
			int LeaseID = htonl(*((unsigned long*)(packet.data.c_str())));
			//LOG("DEBUG TrackingManager::ReceivePacket LeaseID %d", LeaseID);
			if (!(response = m_indicationsMng.GetIndication(LeaseID)))
			{
				RegisteredPublishIndicationsMapT::const_iterator foundRegisteredIndication;
				if ( (foundRegisteredIndication = registeredIndications.find(LeaseID))
								  != registeredIndications.end())
				{
					response = foundRegisteredIndication->second.GService;
				}
				else
				{
					LOG_WARN("ReceivePacket: with unknown TrackingID! Packet=" << packet.ToString());
					return;
				}
			}
		}
	}

	//added - for publish_indication (deserialization and processing)
	//CDurationWatcher oDurationWatcher;
	//if (processNow)
	//{
	//	LOG_INFO("publish_unserializing...");
	//	WATCH_DURATION(oDurationWatcher,400, 10);
	//}
		
	//now we found the mapping GService object start parsing packet (we have all info need it)
	try
	{
		GServiceUnserializer unserializer;
		unserializer.Unserialize(*response, packet);
	}
	catch(SerializationException& ex)
	{
		LOG_ERROR("ReceivePacket: Failed to parse packet=" << packet.ToString() << ". error=" << ex.what());
		/*the response was already removed from pendingTrackings, so no timeout will occur*/
		response->Status = Command::rsFailure_SerializationError;
	}

	LOG_DEBUG("ReceivePacket: Response=" << response->ToString() << " Packet=" << packet.ToString());

	
	//added
	//if (processNow)
	//{
	//	LOG_INFO("publish_processing...");
	//	WATCH_DURATION(oDurationWatcher,400, 10);
	//}
		
	/*commented by Cristian.Guef
	HandleResponse(response, savedTimeoutRetry, savedTimeoutTime);
	*/
	//added by Cristian.Guef
	try
	{
		ReceiveResponse(response, processNow);
	}
	catch(std::exception& ex)
	{
		LOG_ERROR("ReceiveResponse: failed! error=" << ex.what());
	}
	catch(...)
	{
		LOG_ERROR("FireReceiveResponse: failed! unknown error.");
	}
	
	//added
	//if (processNow)
	//	WATCH_DURATION(oDurationWatcher,400, 10);
}

void TrackingManager::HandleResponse(AbstractGServicePtr response, int retryCount, nlib::TimeSpan timeoutTime)
{
	try
		{
			ManageTrackedTimeouts commandsTimeouted;
			if (commandsTimeouted.IsTimeoutedSpecialHandled(*response))
			{
				LOG_DEBUG("Resend request: " << (*response).ToString() << " Retry count: " << retryCount << " Timeout: " << nlib::ToString(timeoutTime));
				//send it again
				if(retryCount > 0)
				{
					int newTimeoutRetry = --retryCount;					
					(*response).Status = Command::rsNoStatus;
					SendRequest(response, timeoutTime, newTimeoutRetry);
					return; //not mark the command as responded yet 
				}
			}
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("Failed to resend the request a timeout request! error=" << ex.what());
		}
		try
		{
			/* comented by Cristian.Guef
			ReceiveResponse(response);
			*/
		}
		catch(std::exception& ex)
		{
			LOG_ERROR("ReceiveResponse: failed! error=" << ex.what());
		}
		catch(...)
		{
			LOG_ERROR("FireReceiveResponse: failed! unknown error.");
		}
}


void TrackingManager::RegisterPublishIndication(int deviceID, boost::uint8_t channelID,
  boost::int32_t filterTrackingID, PublishIndicationPtr indication)
{
	
	//added 
	if (m_indicationsMng.AddIndication(filterTrackingID, indication) == 1/*ok*/)
		return;

	//added by Cristian.Guef
	//now channelID = ConcentratorID (but be carefull that we have 1 byte = 2bytes)

	/* commented by Cristian.Guef
	boost::mutex::scoped_lock lk(mutex); //protect shared resource: pendingTrackings && registeredIndications
	*/

	{
		/*commented by Cristian.Guef - no need for this

		//test in pending trackings
		TrackingsMapT::const_iterator foundTracking = pendingTrackings.find(filterTrackingID);
		if (foundTracking != pendingTrackings.end())
		{
			LOG_ERROR("RegisterPublishIndication: failed! Already exists TrackingID=" << filterTrackingID
			    << " ExistingTrackedCommand=" << foundTracking->second.GService->ToString() << " Indication="
			    << indication->ToString());
			THROW_EXCEPTION1(nlib::Exception, "Duplicated TrackingID (in trackings)!");
		}
		*/
	}

	//check in registered indications
	RegisteredPublishIndicationsMapT::iterator foundRegisteredIndication = registeredIndications.find(filterTrackingID);
	if (foundRegisteredIndication != registeredIndications.end())
	{
		if (foundRegisteredIndication->second.PublisherDeviceID != deviceID)
		{
			LOG_WARN("RegisterPublishIndication: Detected FilterTrackingID duplicated!"
			    << " ExistingPublishIndicationDeviceID=" << foundRegisteredIndication->second.PublisherDeviceID
			    << " NewDeviceID=" << deviceID << " (so we overwrite existing one!");

			registeredIndications.erase(foundRegisteredIndication);
			foundRegisteredIndication = registeredIndications.end(); //force to recreate in next step
		}
	}

	bool createdRegisteredIndication = false;
	if (foundRegisteredIndication == registeredIndications.end())
	{
		foundRegisteredIndication = registeredIndications.insert(RegisteredPublishIndicationsMapT::value_type(
						filterTrackingID, RegisteredPublishIndication(indication, deviceID))).first;
		createdRegisteredIndication = true;
	}

	bool createdChannel = foundRegisteredIndication->second.PublishedChannels.insert(channelID).second;

	LOG_DEBUG("RegisterPublishIndication: succeeded." << " (" << (createdRegisteredIndication
	  ? "Added RegisteredIndication"
	  : "Updated RegisteredIndication") << "," << (createdChannel
	  ? " Added Channel"
	  : " Overwritten Channel") << ")" << " TrackingID=" << filterTrackingID << " DeviceID=" << deviceID << " ChannelNo="
	    << (int)channelID << " Indication=" << indication->ToString());
}

void TrackingManager::UnregisterPublishIndication(int deviceID, boost::uint8_t channelID)
{
	/*comented by Cristian.Guef
	boost::mutex::scoped_lock lk(mutex); //protect shared resource pendingTrackings
	*/

	RegisteredPublishIndicationsMapT::iterator foundRegisteredIndication = registeredIndications.begin();
	while (foundRegisteredIndication != registeredIndications.end())
	{
		if (foundRegisteredIndication->second.PublisherDeviceID == deviceID)
			break; //found it
		foundRegisteredIndication++;
	}

	if (foundRegisteredIndication == registeredIndications.end())
	{
		LOG_WARN("UnregisterPublishIndication: failed! DeviceID=" << deviceID << " not found.");
		return;
	}

	if (0 == foundRegisteredIndication->second.PublishedChannels.erase(channelID))
	{
		LOG_WARN("UnregisterPublishIndication: Removing" << " DeviceID=" << deviceID << " ChannelID=" << (int)channelID
		    << " that doesn't exist");
	}

	if (foundRegisteredIndication->second.PublishedChannels.empty())
	{
		LOG_DEBUG("UnregisterPublishIndication:" << " DeviceID=" << deviceID << " ChannelID=" << (int)channelID
		    << " No channels left in PublishSubscriber");
		registeredIndications.erase(foundRegisteredIndication);
	}
}

//added by Cristian.Guef
void TrackingManager::UnregisterPublishIndication2(boost::int32_t filterTrackingID, boost::uint8_t channelID, IPv6 &IPAddress)
{

	//added 
	if (m_indicationsMng.DeleteIndication(filterTrackingID) == 1/*ok*/)
		return;

	/*comented by Cristian.Guef
	boost::mutex::scoped_lock lk(mutex); //protect shared resource pendingTrackings
	*/

	RegisteredPublishIndicationsMapT::iterator foundRegisteredIndication = registeredIndications.find(filterTrackingID);
	
	if (foundRegisteredIndication == registeredIndications.end())
	{
		LOG_WARN("UnregisterPublishIndication: failed! DeviceIP=" << IPAddress.ToString() << " not found.");
		return;
	}

	if (0 == foundRegisteredIndication->second.PublishedChannels.erase(channelID))
	{
		LOG_WARN("UnregisterPublishIndication: Removing" << " DeviceIP=" << IPAddress.ToString() << " ChannelID=" << (int)channelID
		    << " that doesn't exist");
	}

	if (foundRegisteredIndication->second.PublishedChannels.empty())
	{
		LOG_DEBUG("UnregisterPublishIndication:" << " DeviceIP=" << IPAddress.ToString() << " ChannelID=" << (int)channelID
		    << " No channels left in PublishSubscriber");
		registeredIndications.erase(foundRegisteredIndication);
	}
}


void TrackingManager::UnregisterPublishIndications(int deviceID)
{
	/*comented by Cristian.Guef
	boost::mutex::scoped_lock lk(mutex); //protect shared resource pendingTrackings
	*/

	RegisteredPublishIndicationsMapT::iterator foundRegisteredIndication = registeredIndications.begin();
	while (foundRegisteredIndication != registeredIndications.end())
	{
		if (foundRegisteredIndication->second.PublisherDeviceID == deviceID)
			break; //found it
		foundRegisteredIndication++;
	}

	if (foundRegisteredIndication != registeredIndications.end())
	{
		LOG_DEBUG("UnregisterPublishIndications: succeeded." << " DeviceID=" << deviceID << " Indication="
		    << foundRegisteredIndication->second.ToString());
		registeredIndications.erase(foundRegisteredIndication);
	}
}

void TrackingManager::HandleDisconenct()
{
	LOG_WARN("HandleDisconenct: Connection lost detected. Cancelling all pending requests...");

	//[nicu.dascalu] - we put all lost commands in a queue to avoid locking pendingTrackings
	// when notifing observers
	std::list<AbstractGServicePtr> lost;
	{
		/*comented by Cristian.Guef
		boost::mutex::scoped_lock lk(mutex); //protect shared resource pendingTrackings
		*/
		for (TrackingsMapT::iterator it = pendingTrackings.begin(); it != pendingTrackings.end(); it++)
		{
			lost.push_back(it->second.GService);
		}
		pendingTrackings.clear();
	}

	if (lost.begin() != lost.end())
	{
		LOG_WARN("HandleDisconenct: Canceling pending requests Count=" << lost.size());
		for (std::list<AbstractGServicePtr>::iterator it = lost.begin(); it != lost.end(); it++)
		{
			try
			{
				(*it)->Status = Command::rsFailure_LostConnection;
				/* commented by Cristian.Guef
				ReceiveResponse(*it);
				*/
				//added by Cristian.Guef
				ReceiveResponse(*it, false);
			}
			catch (std::exception& ex)
			{
				LOG_WARN("HandleDisconenct: An error occured while generating lost connection for a command! error=" << ex.what());
			}
		}
	}
}

void TrackingManager::CheckTimeoutRequests()
{
	//[nicu.dascalu] - we put all timeouted commands in a queue to avoid locking pendingTrackings
	// when notifing observers
	std::list<AbstractGServicePtr> timeouted;
	{
		/*comented by Cristian.Guef
		boost::mutex::scoped_lock lk(mutex);//protect shared resource pendingTrackings
		*/
		for (TrackingsMapT::iterator it = pendingTrackings.begin(); it != pendingTrackings.end();)
		{
			if (nlib::CurrentUniversalTime()> it->second.TimeoutDate)
			{
				LOG_DEBUG("CheckTimeoutRequests: Command with TrackingID=" << it->first << " Timeouted.");
				timeouted.push_back(it->second.GService);

				TrackingsMapT::iterator oldIt = it;
				it++;
				pendingTrackings.erase(oldIt);
			}
			else
			{
				it++;
			}
		}
	}

	if (timeouted.begin() != timeouted.end())
	{
		LOG_WARN("CheckTimeoutRequests: Timeout pending requests Count=" << timeouted.size());
		for (std::list<AbstractGServicePtr>::iterator it = timeouted.begin(); it != timeouted.end(); it++)
		{
			try
			{
				(*it)->Status = Command::rsFailure_HostTimeout;
				/* commented by Cristian.Guef
				ReceiveResponse(*it);
				*/
				//added by Cristian.Guef
				ReceiveResponse(*it, false);
			}
			catch (std::exception& ex)
			{
				LOG_ERROR("CheckTimeoutRequests: failed! error=" << ex.what());
			}
		}
	}

	logger.Print();
}

//helper class

TrackingManager::PrintRegisteredIndications::PrintRegisteredIndications(TrackingManager& trackingManager_) :
	trackingManager(trackingManager_)
{
}

void TrackingManager::PrintRegisteredIndications::Print()
{
	if (!LOG_INFO_ENABLED())
		return; //disabled

	nlib::DateTime currentTime = nlib::CurrentUniversalTime();
	if ((currentTime < lastPrint + nlib::util::minutes(5)))
		return; //wait to expire period
	lastPrint = currentTime;

	/*comented by Cristian.Guef
	boost::mutex::scoped_lock lk(trackingManager.mutex);//protect shared resource pendingTrackings
	*/

	LOG_DEBUG("Print: There are RegisteredPublishIndications Count=" << trackingManager.registeredIndications.size());

	for (TrackingManager::RegisteredPublishIndicationsMapT::const_iterator it =
	    trackingManager.registeredIndications.begin(); it != trackingManager.registeredIndications.end(); it++)
	{
		LOG_DEBUG("Print: RegisteredPublishIndication FilterTrackingID=" << it->first << " Indication="
		    << it->second.ToString());
	}
}

} //namespace hostapp
} //namespace nisa100
