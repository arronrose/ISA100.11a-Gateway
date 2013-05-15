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

#include "OpenSession.h"

#include "../TunnelIO.h"
#include "../flow/RequestProcessor.h"
#include "../gateway/GChannel.h"

#include "../log/Log.h"

namespace tunnel {
namespace comm {


OpenSession::OpenSession(CTunnelIO *pTunnelIO, RequestProcessor* reqProcessorPtr, tunnel::gateway::GChannel *pChannel)
{
	m_pTunnelIO = pTunnelIO;
	m_reqProcessorPtr = reqProcessorPtr;
	m_pChannel = pChannel;
	m_isConnectionToGW = false;
}

void OpenSession::DoOpenSession()
{
	static bool bSessionStarted = false;

	if (m_isConnectionToGW == false)
		return;

	if (m_pTunnelIO->Read_SessionID() == 0 && m_pTunnelIO->Is_SessionSent() == false)	
	{
		if (bSessionStarted == true)
		{
			bSessionStarted = false;
			LOG_WARN("Monitor Host haven't established a session with Gateway.");
		}

		Request session;
		session.sessionID = 0; 
		session.type = Request::rtSession;
		
		m_reqProcessorPtr->ProcessRequest(session);

		if (m_pTunnelIO->Is_SessionSent())
		{
			bSessionStarted = true;
			LOG_INFO("establishing session with Gateway...");
		}
	}

	
	if (m_pTunnelIO->Read_SessionID() != 0 && bSessionStarted == true)
	{
		bSessionStarted = false;
		LOG_INFO("Monitor Host established a session with Gateway.");
	}

}


void OpenSession::GWDisconnected()
{
	m_isConnectionToGW = false;
	m_pTunnelIO->Save_SessionID(0);
}

void OpenSession::GWConnected(const std::string& host, int port)
{
	m_isConnectionToGW = true;
}


}
}

