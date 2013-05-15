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

#include "../serialization/SerializationException.h"
#include "../serialization/GServiceSerializer.h"
#include "../serialization/GServiceUnserializer.h"

#include "../util/serialization/Serialization.h"
#include "../util/serialization/NetworkOrder.h"

#include "../gateway/GChannel.h"

#include "Request.h"
#include "RequestProcessor.h"
#include "../log/Log.h"
#include <boost/cstdint.hpp> //used for inttypes

namespace tunnel {
namespace comm {

TrackingManager::TrackingManager()
{
	m_nextTrackingID = 1;
}

void TrackingManager::SetRequestProcessor(RequestProcessor *pReqProcessor)
{
	m_pReqProcessor = pReqProcessor;
}

void TrackingManager::SetGChannel(tunnel::gateway::GChannel *pChannel)
{
	m_pChannel = pChannel;
}

void TrackingManager::SendRequest(AbstractGServicePtr servicePtr, nlib::TimeSpan timeout)
{
	boost::int32_t trackingID;
	{
		//boost::mutex::scoped_lock lk(m_mutex); //protect shared resource pendingTrackings && registeredIndications

		trackingID = m_nextTrackingID + 1;
		{
			//test in pending trackings
			TrackingsMapT::const_iterator found = m_pendingTrackings.find(trackingID);
			if (found != m_pendingTrackings.end())
			{
				LOG_ERROR("SendRequest: failed! Already exists TrackingID=" << trackingID << " ExistingTrackedCommand="
				    << found->second.GServicePtr->ToString() << " NewRequest=" << servicePtr->ToString());
				THROW_EXCEPTION1(nlib::Exception, "Duplicated TrackingID (in trackings)!");
			}
		}

		m_nextTrackingID = m_nextTrackingID + 1; //increment next id
	}

	gateway::GeneralPacket packet;
	packet.trackingID = trackingID;

	GServiceSerializer serializer;
	serializer.Serialize(*servicePtr, packet);

	try
	{
		m_pChannel->SendPacket(packet);
		{
			//boost::mutex::scoped_lock lk(m_mutex); //protect shared resource pendingTrackings && registeredCSInwards
			m_pendingTrackings.insert(TrackingsMapT::value_type(trackingID, TrackingGService(servicePtr, timeout)));
		}
		//LOG_DEBUG("SendRequest: Timeout=" << nlib::ToString(timeout) << ", Request=" << request->ToString() << " Packet=" << packet.ToString());
	}
	catch (std::exception& ex)
	{
		LOG_ERROR("SendRequest: failed! Request=" << servicePtr->ToString() << " Packet=" << packet.ToString() << " error="
				<< ex.what());
		throw;
	}
}

void TrackingManager::ReceivePacket(const gateway::GeneralPacket& packet)
{
	AbstractGServicePtr responsePtr;
	{
		//boost::mutex::scoped_lock lk(m_mutex); //protect shared resource pendingTrackings && registeredCSInwards

		if (packet.serviceType != gateway::GeneralPacket::GClientServerRequest)
		{
			TrackingsMapT::iterator foundTracking;
			if ( (foundTracking = m_pendingTrackings.find(packet.trackingID)) != m_pendingTrackings.end())
			{
				responsePtr = foundTracking->second.GServicePtr;
				m_pendingTrackings.erase(foundTracking);
			}
			else
			{
				LOG_WARN("ReceivePacket: with unknown TrackingID! Packet=" << packet.ToString());
				return;
			}
		}
		else
		{
			std::basic_istringstream<boost::uint8_t> data(packet.data);
			const util::NetworkOrder& network = util::NetworkOrder::Instance();
			int leaseID = util::binary_read<boost::uint32_t>(data, network);

			RegisteredCSInwardsMapT::const_iterator foundRegisteredCSInwards;
			if ( (foundRegisteredCSInwards = m_registeredCSInwards.find(leaseID))
				!= m_registeredCSInwards.end())
			{
				if (!(responsePtr = foundRegisteredCSInwards->second->Clone()))
				{
					LOG_ERROR("ReceivePacket: The response is not cloneable! Response="
						<< foundRegisteredCSInwards->second->ToString());
					return;
				}
			}
			else
			{
				LOG_WARN("ReceivePacket: with unknown TrackingID! Packet=" << packet.ToString());
				return;
			}
		}
	}


	//now we found the mapping GService object start parsing packet (we have all info need it)
	try
	{
		GServiceUnserializer unserializer;
		unserializer.Unserialize(*responsePtr, packet);
	}
	catch(SerializationException& ex)
	{
		LOG_ERROR("ReceivePacket: Failed to parse packet=" << packet.ToString() << ". error=" << ex.what());
		/*the response was already removed from pendingTrackings, so no timeout will occur*/
		responsePtr->m_status = rsFailure_SerializationError;
	}

	LOG_DEBUG("ReceivePacket: Response=" << responsePtr->ToString() << " Packet=" << packet.ToString());
		
	HandleResponse(responsePtr);
}

void TrackingManager::HandleResponse(AbstractGServicePtr responsePtr)
{
	try
	{
		m_pReqProcessor->ProcessResponse(responsePtr);
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


void TrackingManager::RegisterCSInwards(boost::int32_t filterTrackingID, ClientServerInwardsPtr csinwardsPtr)
{
	
	//boost::mutex::scoped_lock lk(m_mutex); //protect shared resource: pendingTrackings && registeredIndications

	//check in registered indications
	RegisteredCSInwardsMapT::iterator foundRegisteredCSInwards = m_registeredCSInwards.find(filterTrackingID);
	if (foundRegisteredCSInwards == m_registeredCSInwards.end())
	{
		m_registeredCSInwards.insert(RegisteredCSInwardsMapT::value_type(filterTrackingID, csinwardsPtr)).first;
	}
	else
	{
		LOG_ERROR("Duplicated lease detected for server_type = " << (boost::uint32_t)filterTrackingID);
		delete csinwardsPtr; //if not boost_shared_ptr
	}
}


void TrackingManager::UnregisterCSInwards(boost::int32_t filterTrackingID)
{
	//boost::mutex::scoped_lock lk(m_mutex); //protect shared resource pendingTrackings

	RegisteredCSInwardsMapT::iterator foundRegisteredCSInwards = m_registeredCSInwards.find(filterTrackingID);
	
	if (foundRegisteredCSInwards == m_registeredCSInwards.end())
	{
		LOG_WARN("UnregisterCSInwards: failed! lease =" << (boost::uint32_t)filterTrackingID << " not found.");
		return;
	}

	delete foundRegisteredCSInwards->second; //if not boost_shared_ptr
	m_registeredCSInwards.erase(foundRegisteredCSInwards);
}


void TrackingManager::GWDisconnected()
{
	LOG_WARN("HandleDisconenct: Connection lost detected. Cancelling all pending requests...");

	std::list<AbstractGServicePtr> lost;
	{
		//boost::mutex::scoped_lock lk(m_mutex); //protect shared resource pendingTrackings
		for (TrackingsMapT::iterator it = m_pendingTrackings.begin(); it != m_pendingTrackings.end(); it++)
		{
			lost.push_back(it->second.GServicePtr);
		}
		m_pendingTrackings.clear();
	}

	if (lost.begin() != lost.end())
	{
		LOG_WARN("HandleDisconenct: Canceling pending requests Count=" << lost.size());
		for (std::list<AbstractGServicePtr>::iterator it = lost.begin(); it != lost.end(); it++)
		{
			try
			{
				(*it)->m_status = rsFailure_LostConnection;
				m_pReqProcessor->ProcessResponse(*it);
			}
			catch (std::exception& ex)
			{
				LOG_WARN("HandleDisconenct: An error occured while generating lost connection for a command! error=" << ex.what());
			}
		}
	}
}

void TrackingManager::CheckTimedoutRequests()
{

	// we put all timeouted commands in a queue to avoid locking pendingTrackings
	// when notifing observers
	std::list<AbstractGServicePtr> timeouted;
	{
		//boost::mutex::scoped_lock lk(m_mutex);//protect shared resource pendingTrackings
		for (TrackingsMapT::iterator it = m_pendingTrackings.begin(); it != m_pendingTrackings.end();)
		{
			if (nlib::CurrentLocalTime()> it->second.timeoutDate)
			{
				//LOG_DEBUG("CheckTimeoutRequests: Command with TrackingID=" << it->first << " Timeouted.");
				timeouted.push_back(it->second.GServicePtr);

				TrackingsMapT::iterator oldIt = it;
				it++;
				m_pendingTrackings.erase(oldIt);
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
				(*it)->m_status = rsFailure_HostTimeout;
				m_pReqProcessor->ProcessResponse(*it);
			}
			catch (std::exception& ex)
			{
				LOG_ERROR("CheckTimeoutRequests: failed! error=" << ex.what());
			}
		}
	}
}


} //namespace comm
} //namespace tunnel
