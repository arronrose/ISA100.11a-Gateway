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

//#include <boost/thread.hpp>

#include <nlib/datetime.h>
#include <boost/format.hpp>

#include "../services/AbstractGService.h"
#include "../gateway/GeneralPacket.h"

#include "../interfaces/TaskRun.h"
#include "../interfaces/GWTrigger.h"


namespace tunnel {
	namespace gateway{
	class GChannel;
	}

namespace comm {

class RequestProcessor;

class TrackingManager:public ITaskRun, public IGWTrigger
{

public:
	TrackingManager();

public:
	void SetRequestProcessor(RequestProcessor *pReqProcessor);
	void SetGChannel(tunnel::gateway::GChannel *pChannel);
		
private:
	RequestProcessor *m_pReqProcessor;
	tunnel::gateway::GChannel *m_pChannel;

public:
	void SendRequest(AbstractGServicePtr servicePtr, nlib::TimeSpan timeout);
	void ReceivePacket(const gateway::GeneralPacket& packet);
	virtual void operator()(){CheckTimedoutRequests();} 
	
private: 
	void CheckTimedoutRequests();

public:
	void RegisterCSInwards(boost::int32_t filterTrackingID, ClientServerInwardsPtr csinwardsPtr);
	void UnregisterCSInwards(boost::int32_t filterTrackingID);
	
public:
	virtual void GWDisconnected();
	virtual void GWConnected(const std::string&, int){assert(false);};

private:
	void HandleResponse(AbstractGServicePtr);

private:
	struct TrackingGService
	{
		AbstractGServicePtr GServicePtr;
		nlib::DateTime timeoutDate;
		
		TrackingGService(AbstractGServicePtr GServicePtr_, const nlib::TimeSpan& timeout_) :
			GServicePtr(GServicePtr_)
		{
			timeoutDate = nlib::CurrentLocalTime()+ timeout_;
		}
	};
	typedef std::map<boost::int32_t, TrackingGService> TrackingsMapT;
	//holds the list of pending tunnle requests
	TrackingsMapT m_pendingTrackings;

private:
	typedef std::map<boost::int32_t, AbstractGServicePtr> RegisteredCSInwardsMapT;
	//holds the registered client_server_inwards
	RegisteredCSInwardsMapT m_registeredCSInwards;

private:
	//boost::mutex m_mutex;
	boost::int32_t m_nextTrackingID;
};


} //namespace comm
} //namespace tunnel

#endif
