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

#include "RequestProcessor.h"
#include "ServicesFactory.h"
#include "TrackingManager.h"
#include "../processor/GServiceOutProcessor.h"
#include "../processor/GServiceInProcessor.h"
#include "../gateway/GChannel.h"

#include <boost/format.hpp>

#include "../TunnelIO.h"
#include "../log/Log.h"


namespace tunnel {
namespace comm {


RequestProcessor::RequestProcessor(CTunnelIO *pTunnelIO, int timedOutSecs)
{
	m_timedOutSecs = timedOutSecs;
	m_pTunnelIO = pTunnelIO;
}

void RequestProcessor::SetTrackingManager(TrackingManager *pTrackManager)
{
	m_pTrackManager = pTrackManager;
}

void RequestProcessor::SendRequest(AbstractGServicePtr servicePtr)
{
	m_pTrackManager->SendRequest(servicePtr, nlib::util::seconds(m_timedOutSecs));
}


void RequestProcessor::ProcessRequest(Request &req)
{
	
	AbstractGServicePtr servicePtr = ServicesFactory().Create(req);
	LOG_INFO("ProcessRequest: req=" << req.ToString() << " -> Service=" << servicePtr->ToString());

	try{
		GServiceOutProcessor().Process(servicePtr, m_pTunnelIO, this);
	}
	catch (gateway::ChannelException& ex)
	{
		delete servicePtr; //if not boost_shared_ptr
		LOG_ERROR("ProcessRequest: failed! req=" << req.ToString() << " error=" << ex.what());
	}
	catch (std::exception& ex)
	{
		delete servicePtr; //if not boost_shared_ptr
		LOG_ERROR("ProcessRequest: failed! req=" << req.ToString() << " error=" << ex.what());
	}
	catch (...)
	{
		delete servicePtr; //if not boost_shared_ptr
		LOG_ERROR("ProcessRequest: failed! req=" << req.ToString() << " unknown exception!");
	}
}

void RequestProcessor::ProcessResponse(AbstractGServicePtr responsePtr)
{
	{
		//lock lk(m_monitor);
		m_responsesBuffer.push_back(responsePtr);
	}
	
}

void RequestProcessor::ProcessResponses()
{
	//lock lk(m_monitor);

	while (!m_responsesBuffer.empty())
	{
		AbstractGServicePtr responsePtr = m_responsesBuffer.front();
		m_responsesBuffer.pop_front();

		LOG_INFO("ProcessReponses: Processing... Response=" << responsePtr->ToString());

		try
		{
			GServiceInProcessor().Process(*responsePtr, m_pTunnelIO);
		}
		catch (std::exception& ex)
		{
			LOG_ERROR("ProcessResponses: failed! error=" << ex.what() << " Response=" << responsePtr->ToString());
		}
		catch (...)
		{
			LOG_ERROR("ProcessResponses: failed! unknown error. Response=" << responsePtr->ToString());
		}

		delete responsePtr; //if not boost_shared_ptr
	}
}



} // namespace comm
} // namespace tunnel
