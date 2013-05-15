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

#include "GServiceOutProcessor.h"
#include "../TunnelIO.h"
#include "../flow/RequestProcessor.h"

#include <sstream>

#include "../gateway/GChannel.h"

namespace tunnel {
namespace comm {


void GServiceOutProcessor::Process(AbstractGServicePtr servicePtr, CTunnelIO* pTunnelIO,
							   RequestProcessor *pReqProcessor)
{
	m_servicePtr = servicePtr;
	m_pTunnelIO = pTunnelIO;
	m_pReqProcessor = pReqProcessor;

	servicePtr->Accept(*this);
}


void GServiceOutProcessor::Visit(GSession& session)
{
	
	assert(session.m_sessionID == m_pTunnelIO->Read_SessionID());

	m_pReqProcessor->SendRequest(m_servicePtr);

	m_pTunnelIO->Save_SessionSent(true);
}


void GServiceOutProcessor::Visit(GLease& lease)
{	
	switch(lease.m_leaseType)
	{
	case GLease::Client:
		assert(lease.m_leaseID == m_pTunnelIO->Read_C_LeaseID(lease.m_tunnelNo));
		break;
	case GLease::Server:
		assert(lease.m_leaseID == m_pTunnelIO->Read_S_LeaseID(lease.m_tunnelNo));
		break;
	default:
		assert(false);
		break;
	}

	m_pReqProcessor->SendRequest(m_servicePtr);

	//it was sent
	switch(lease.m_leaseType)
	{
	case GLease::Client:
		m_pTunnelIO->Save_C_LeaseSent(lease.m_tunnelNo, true);
		break;
	case GLease::Server:
		m_pTunnelIO->Save_S_LeaseSent(lease.m_tunnelNo, true);
		break;
	default:
		assert(false);
		break;
	}
}


void  GServiceOutProcessor::Visit(GClientServer_C& client)
{
	assert(client.m_leaseID == m_pTunnelIO->Read_C_LeaseID(client.m_tunnelNo));

	m_pReqProcessor->SendRequest(m_servicePtr);

	//it was sent
	m_pTunnelIO->PopReqData(client.m_tunnelNo);
}

void  GServiceOutProcessor::Visit(GClientServer_S& server)
{
	assert(false);
}

} // namespace comm
} // namespace tunnel
