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


#include "../TunnelIO.h"
#include "../flow/TrackingManager.h"
#include "GServiceInProcessor.h"
#include <boost/format.hpp>

#include "../log/Log.h"

namespace tunnel {
namespace comm {


void GServiceInProcessor::Process(AbstractGService& response, CTunnelIO* pTunnelIO)
{
	m_pTunnelIO = pTunnelIO;
	response.Accept(*this);
}

void GServiceInProcessor::Visit(GSession& session)
{

	m_pTunnelIO->Save_SessionSent(false);

	if (session.m_status == rsSuccess)
	{
		
		if(session.m_sessionPeriod != -1 || session.m_sessionID == 0)
		{	
			LOG_ERROR(" invalid session period = " 
		  				<< (boost::int32_t)session.m_sessionPeriod 
		  				<< " and session ID = " 
		  				<< (boost::int32_t)session.m_sessionID <<
						"status = " << (boost::int32_t)session.m_status);

			return;
		}

		m_pTunnelIO->Save_SessionID(session.m_sessionID);
	}
	else
	{
		LOG_ERROR(" session with session period = " 
		  				<< (boost::int32_t)session.m_sessionPeriod 
		  				<< " and session ID = " 
		  				<< (boost::int32_t)session.m_sessionID <<
		  				"status = " << (boost::int32_t)session.m_status);
	}
}


void GServiceInProcessor::Visit(GLease& lease)
{

	switch (lease.m_leaseType)
	{
	case GLease::Client:
		m_pTunnelIO->Save_C_LeaseSent(lease.m_tunnelNo, false);
		break;
	case GLease::Server:
		m_pTunnelIO->Save_S_LeaseSent(lease.m_tunnelNo, false);
		break;
	default:
		assert(false);
	}

	if (lease.m_status == rsSuccess)
	{
		if(lease.m_leaseID  == 0 || lease.m_leasePeriod  != 0)
		 {
			LOG_ERROR(" -> lease for lease period = " << 
				(boost::uint32_t)lease.m_leasePeriod << 
				" with lease ID = " << (boost::uint32_t)lease.m_leaseID <<
							" for rm_obj_id = " << (boost::uint16_t)(lease.m_endPoint.remoteObjID) << 
							" and rm_tlsap_id = " << (boost::uint16_t)(lease.m_endPoint.remotePort) << 
							" for lc_obj_id = " << (boost::uint16_t)(lease.m_endPoint.localObjID) << 
							" and lc_tlsap_id = " << (boost::uint16_t)(lease.m_endPoint.localPort) << 
							" and lease_type = " << (boost::uint16_t)lease.m_leaseType <<
							"and status = " << (boost::int32_t)lease.m_status);

			return;
		}

		//check if there is a saved lease_id
		for(int i = 0; i < m_pTunnelIO->Get_TunnCount(); i++)
		{
			if (m_pTunnelIO->Read_C_LeaseID(i) == lease.m_leaseID)
			{
				m_pTunnelIO->Save_C_LeaseID(i, 0); //force to recreate it
				break;
			}
			if (m_pTunnelIO->Read_S_LeaseID(i) == lease.m_leaseID)
			{
				m_pTunnelIO->Save_S_LeaseID(i, 0); //force to recreate it
				break;
			}
		}

		switch (lease.m_leaseType)
		{
		case GLease::Client:
			m_pTunnelIO->Save_C_LeaseID(lease.m_tunnelNo, lease.m_leaseID);
			break;
		case GLease::Server:
			{
				m_pTunnelIO->Save_S_LeaseID(lease.m_tunnelNo, lease.m_leaseID);
				
				GClientServer_S *pServer = new GClientServer_S();
				pServer->m_sessionID = m_pTunnelIO->Read_SessionID();
				pServer->m_tunnelNo = lease.m_tunnelNo;
				m_pTunnelIO->Read_TrackManagerPtr()->RegisterCSInwards(lease.m_leaseID, ClientServerInwardsPtr(pServer));
			}
			break;
		default:
			assert(false);
		}

	}
	else
	{
		LOG_ERROR(" -> lease for lease period = " << 
				(boost::uint32_t)lease.m_leasePeriod << 
				" with lease ID = " << (boost::uint32_t)lease.m_leaseID <<
							" for rm_obj_id = " << (boost::uint16_t)(lease.m_endPoint.remoteObjID) << 
							" and rm_tlsap_id = " << (boost::uint16_t)(lease.m_endPoint.remotePort) << 
							" for lc_obj_id = " << (boost::uint16_t)(lease.m_endPoint.localObjID) << 
							" and lc_tlsap_id = " << (boost::uint16_t)(lease.m_endPoint.localPort) << 
							" and lease_type = " << (boost::uint16_t)lease.m_leaseType <<
							"and status = " << (boost::int32_t)lease.m_status);
	}
}



void GServiceInProcessor::Visit(GClientServer_C& client)
{

	if (client.m_status == rsSuccess)
	{
		LOG_INFO(" CS_c received");
	}
	else
	{
		if (client.m_status == rsFailure_GatewayLeaseExpired)
		{
			LOG_WARN("CS_c received -> lease expired");
			//reset leaseid
			m_pTunnelIO->Save_C_LeaseID(client.m_tunnelNo, 0);
			//resend data
			m_pTunnelIO->PushReqData(client.m_tunnelNo, client.m_reqData.c_str(), client.m_reqData.size());
		}
		else
		{
			LOG_ERROR("CS_c received -> status = " << (boost::uint32_t)client.m_status);
		}
	}
}

void GServiceInProcessor::Visit(GClientServer_S& server)
{
	if (server.m_status = rsSuccess)
	{
		LOG_INFO(" CS_s received");
		m_pTunnelIO->Save_RespData(server.m_tunnelNo, server.m_reqData);
	}
	else
	{
		LOG_ERROR("CS_s received -> status = " <<  (boost::uint32_t)server.m_status);
	}
}


} //namespace comm
} //namespace tunnel
