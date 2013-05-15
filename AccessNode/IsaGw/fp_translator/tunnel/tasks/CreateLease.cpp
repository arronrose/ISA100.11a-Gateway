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

#include "CreateLease.h"

#include "../TunnelIO.h"
#include "../flow/RequestProcessor.h"
#include "../services/GLease.h"

namespace tunnel {
namespace comm {


CreateLease::CreateLease(CTunnelIO *pTunnelIO, RequestProcessor* reqProcessorPtr)
{
	m_pTunnelIO = pTunnelIO;
	m_reqProcessorPtr = reqProcessorPtr;
}

void CreateLease::DoCreateLease()
{
	CTunnelVector* pTunnelVec = m_pTunnelIO->GetTunnelVec();

	for (int i = 0; i < pTunnelVec->size(); i++)
	{
		
		if (m_pTunnelIO->Read_SessionID() == 0)
			return;

		if (m_pTunnelIO->Read_C_LeaseID(i) == 0 && m_pTunnelIO->Is_C_Lease_Sent(i) == false) //create client lease 
		{
			Request lease;
			lease.sessionID = m_pTunnelIO->Read_SessionID();
			lease.type = Request::rtLease;
			lease.Param.Lease.protocolType = (*pTunnelVec)[i]->m_nLinkType;
			lease.Param.Lease.leaseType = GLease::Client;
			lease.Param.Lease.tunnNo = i;
			memcpy(lease.Param.Lease.ipAddress, &(*pTunnelVec)[i]->m_oNetworkAddress, 16);
			lease.Param.Lease.remotePort = (*pTunnelVec)[i]->m_nTunnelRemotePort;
			lease.Param.Lease.remoteObjID = (*pTunnelVec)[i]->m_nTunnelRemoteObject;
			lease.Param.Lease.localPort = (*pTunnelVec)[i]->m_nTunnelLocalPort;
			lease.Param.Lease.localObjID = (*pTunnelVec)[i]->m_nTunnelLocalObject;

			/*
			std::basic_ostringstream<boost::uint8_t> data;
			data.write(pTunnelVec[i]->, len);
			*/
			//no conninfo defined yet
			lease.Param.Lease.pConnInfo = new std::basic_string<boost::uint8_t>;

			m_reqProcessorPtr->ProcessRequest(lease);

			delete lease.Param.Lease.pConnInfo;
		}

		if (m_pTunnelIO->Read_SessionID() == 0)
			return;


		if (m_pTunnelIO->Read_S_LeaseID(i) == 0 && m_pTunnelIO->Is_S_Lease_Sent(i) == false) //create client lease 
		{
			Request lease;
			lease.sessionID = m_pTunnelIO->Read_SessionID();
			lease.type = Request::rtLease;
			lease.Param.Lease.leaseType = GLease::Server;
			lease.Param.Lease.tunnNo = i;
			memcpy(lease.Param.Lease.ipAddress, &(*pTunnelVec)[i]->m_oNetworkAddress, 16);
			lease.Param.Lease.remotePort = (*pTunnelVec)[i]->m_nTunnelRemotePort;
			lease.Param.Lease.remoteObjID = (*pTunnelVec)[i]->m_nTunnelRemoteObject;
			lease.Param.Lease.localPort = (*pTunnelVec)[i]->m_nTunnelLocalPort;
			lease.Param.Lease.localObjID = (*pTunnelVec)[i]->m_nTunnelLocalObject;

			/*
			std::basic_ostringstream<boost::uint8_t> data;
			data.write(pTunnelVec[i]->, len);
			*/
			//no conninfo defined yet
			lease.Param.Lease.pConnInfo = new std::basic_string<boost::uint8_t>;

			m_reqProcessorPtr->ProcessRequest(lease);

			delete lease.Param.Lease.pConnInfo;
		}

	}

}


}
}
