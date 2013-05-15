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


#include "./log/Log.h"

#include "TunnelIO.h"

//flow
#include "./flow/RequestProcessor.h"
#include "./flow/TrackingManager.h"
#include "./flow/TasksPool.h"

//gateway
#include "./gateway/GChannel.h"

//tasks
#include "./tasks/OpenSession.h"
#include "./tasks/CreateLease.h"
#include "./tasks/SendCSRequest.h"

#include <string>
#include <exception>



CTunnelIO::CTunnelIO(CTunnelVector* pTunnelVec, int tunnelsNo, int gwPort, const char* gwAddress, int serviceTimedOut)
{
	m_moreTunnInfoVec.resize(tunnelsNo);
	for (int i = 0; i < tunnelsNo; i++)
	{
		m_moreTunnInfoVec[i].c_Lease_ID = 0;
		m_moreTunnInfoVec[i].s_Lease_ID = 0;
		m_moreTunnInfoVec[i].c_Lease_Sent = false;
		m_moreTunnInfoVec[i].s_Lease_Sent = false;
	}

	m_sessionID = 0;
	m_sessionSent = false;
	m_pTunnelVec = pTunnelVec;

	//gateway
	std::string	str;
	str = gwAddress;
	m_pChannel = new tunnel::gateway::GChannel(str, gwPort);

	//flow
	m_pProcessor  = new tunnel::comm::RequestProcessor(this, serviceTimedOut);
	m_pTracking = new tunnel::comm::TrackingManager();
	m_pTasks = new tunnel::comm::TasksPool();

	//tasks
	m_pSession = new tunnel::comm::OpenSession(this, m_pProcessor, m_pChannel);
	m_pLease = new tunnel::comm::CreateLease(this, m_pProcessor);
	m_pCSreq = new tunnel::comm::SendCSRequest(this, m_pProcessor);


	//flow - "bind"
	m_pProcessor->SetTrackingManager(m_pTracking);
	m_pTracking->SetRequestProcessor(m_pProcessor); 
	m_pTracking->SetGChannel(m_pChannel);
		
	//tasks - "bind"
	m_pTasks->RegisterPeriodicTask(m_pChannel);
	m_pTasks->RegisterPeriodicTask(m_pSession);
	m_pTasks->RegisterPeriodicTask(m_pLease);
	m_pTasks->RegisterPeriodicTask(m_pCSreq);
	m_pTasks->RegisterPeriodicTask(m_pProcessor);
	m_pTasks->RegisterPeriodicTask(m_pTracking);

	//gateway - "bind"
	m_pChannel->SetTrackManager(m_pTracking);
	m_pChannel->m_connected.push_back(m_pSession);
	m_pChannel->m_disconnected.push_back(m_pSession);
	m_pChannel->m_disconnected.push_back(m_pTracking);

	LOG_INFO("Tunnel interface initialized.");
}

CTunnelIO::~CTunnelIO()
{
	delete m_pProcessor;
	delete m_pTracking;
	delete m_pTasks;
	
	//tasks
	delete m_pSession;
	delete m_pLease;
	delete m_pCSreq;
	//gateway
	delete m_pChannel;
}


//for users only
void CTunnelIO::PushReqData(int tunnNo, const u_char *pReqData, int len)
{
	std::basic_ostringstream<boost::uint8_t> data;
	data.write(pReqData, len);

	m_moreTunnInfoVec[tunnNo].reqData.push(data.str());
}
bool CTunnelIO::IsRespData(int tunnNo)
{
	return m_moreTunnInfoVec[tunnNo].respData.empty() == true ? false : true;
}
void CTunnelIO::WriteRespData(int tunnNo, u_char *pRespData, int *pLen)
{
	memcpy(pRespData, m_moreTunnInfoVec[tunnNo].respData.front().c_str(), 
				m_moreTunnInfoVec[tunnNo].respData.front().size());
}
void CTunnelIO::PopRespData(int tunnNo)
{
	m_moreTunnInfoVec[tunnNo].respData.pop();
}
void CTunnelIO::StartIO()
{
	m_pChannel->Start();
}

void CTunnelIO::PerformIO()
{
	m_pTasks->Run();
}
void CTunnelIO::StopIO()
{
	m_pChannel->Stop();
}


//for services only
boost::uint32_t CTunnelIO::Get_TunnCount()
{
	return m_moreTunnInfoVec.size();
}
void CTunnelIO::Save_SessionID(boost::int32_t sessionID)
{
	m_sessionID = sessionID;
}
void CTunnelIO::Save_C_LeaseID(int tunnNo, boost::uint32_t leaseID)
{
	m_moreTunnInfoVec[tunnNo].c_Lease_ID = leaseID;
}
void CTunnelIO::Save_S_LeaseID(int tunnNo, boost::uint32_t leaseID)
{
	m_moreTunnInfoVec[tunnNo].s_Lease_ID = leaseID;
}
void CTunnelIO::Save_C_LeaseSent(int tunnNo, bool val)
{
	m_moreTunnInfoVec[tunnNo].c_Lease_Sent = val;
}
void CTunnelIO::Save_S_LeaseSent(int tunnNo, bool val)
{
	m_moreTunnInfoVec[tunnNo].s_Lease_Sent = val;
}
void CTunnelIO::Save_SessionSent(bool val)
{
	m_sessionSent = val;
}
void CTunnelIO::Save_RespData(int tunnNo, std::basic_string<boost::uint8_t> &respData)
{
	m_moreTunnInfoVec[tunnNo].respData.push(respData);
}
boost::uint32_t CTunnelIO::Read_SessionID()
{
	return m_sessionID;
}
boost::uint32_t CTunnelIO::Read_C_LeaseID(int tunnNo)
{
	return m_moreTunnInfoVec[tunnNo].c_Lease_ID;
}
boost::uint32_t CTunnelIO::Read_S_LeaseID(int tunnNo)
{
	return m_moreTunnInfoVec[tunnNo].s_Lease_ID;
}
bool CTunnelIO::Is_C_Lease_Sent(int tunnNo)
{
	return m_moreTunnInfoVec[tunnNo].c_Lease_Sent;
}
bool CTunnelIO::Is_S_Lease_Sent(int tunnNo)
{
	return m_moreTunnInfoVec[tunnNo].s_Lease_Sent;
}
bool CTunnelIO::Is_SessionSent()
{
	return m_sessionSent;
}
bool CTunnelIO::Is_ReqData(int tunnNo)
{
	return m_moreTunnInfoVec[tunnNo].reqData.empty() == true ? false : true;
}
void CTunnelIO::WriteReqData(int tunnNo, std::basic_string<boost::uint8_t> &reqData)
{
	reqData = m_moreTunnInfoVec[tunnNo].reqData.front();
}
void CTunnelIO::PopReqData(int tunnNo)
{
	m_moreTunnInfoVec[tunnNo].reqData.pop();
}
CTunnelVector* CTunnelIO::GetTunnelVec()
{
	return m_pTunnelVec;
}
tunnel::comm::TrackingManager* CTunnelIO::Read_TrackManagerPtr()
{
	return m_pTracking;
}

