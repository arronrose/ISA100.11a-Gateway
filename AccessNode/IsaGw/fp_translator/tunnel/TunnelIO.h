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

#ifndef GTUNNELIO_H_
#define GTUNNELIO_H_

#include "../FptTypes.h"

#include <vector>
#include <queue>
#include <string>

#include <boost/cstdint.hpp> //used for inttypes
//#include <boost/thread.hpp> //thread_group


//inside things
extern bool SetLogLevel(int level);
namespace tunnel{
	namespace comm
	{
		//flow
		class RequestProcessor;
		class TrackingManager;
		class TasksPool;
		//tasks
		class OpenSession;
		class CreateLease;
		class SendCSRequest;
	}
	namespace gateway
	{
		class GChannel;
	}
}

//tunnel
class CTunnelIO
{
public:
	typedef boost::shared_ptr<CTunnelIO> Ptr;

private:
	struct MoreTunnInfo
	{
		boost::uint32_t	c_Lease_ID;		// =0 - no client lease_id
										// >0 - client lease_id obtained
		boost::uint32_t	s_Lease_ID;		// =0 - no server lease_id
										// >0 - server lease_id obtained
		bool	c_Lease_Sent;
		bool	s_Lease_Sent;
		std::queue<std::basic_string<boost::uint8_t> >	reqData;
		std::queue<std::basic_string<boost::uint8_t> >	respData;	 
	};

	CTunnelVector*				m_pTunnelVec;
	std::vector<MoreTunnInfo>	m_moreTunnInfoVec;	//index = tunnelNo in 'm_pTunnelVec'
	unsigned int				m_sessionID;		// =0 - no session_id obtained
													// >0 - session_id obtained
	bool						m_sessionSent;

private:
	CTunnelIO();
	CTunnelIO(const CTunnelIO &);

public:
	CTunnelIO(CTunnelVector* pTunnelVec, int tunnelsNo, int gwPort, const char* gwAddress, int serviceTimedOut/*secs*/);
	~CTunnelIO();

public:
	//for users only
	void PushReqData(int tunnNo, const u_char *pReqData, int len);
	bool IsRespData(int tunnNo);
	void WriteRespData(int tunnNo, u_char *pRespData, int *pLen);
	void PopRespData(int tunnNo);
	void StartIO();
	void PerformIO();
	void StopIO();
	
public:
	//for requests and services only
	boost::uint32_t Get_TunnCount();
	void Save_SessionID(boost::int32_t sessionID);
	void Save_C_LeaseID(int tunnNo, boost::uint32_t leaseID);
	void Save_S_LeaseID(int tunnNo, boost::uint32_t leaseID);
	void Save_C_LeaseSent(int tunnNo, bool val);
	void Save_S_LeaseSent(int tunnNo, bool val);
	void Save_SessionSent(bool val);
	void Save_RespData(int tunnNo, std::basic_string<boost::uint8_t> &respData);
	boost::uint32_t Read_SessionID();
	boost::uint32_t Read_C_LeaseID(int tunnNo);
	boost::uint32_t Read_S_LeaseID(int tunnNo);
	bool Is_C_Lease_Sent(int tunnNo);
	bool Is_S_Lease_Sent(int tunnNo);
	bool Is_SessionSent();
	bool Is_ReqData(int tunnNo);
	void WriteReqData(int tunnNo, std::basic_string<boost::uint8_t> &reqData);
	void PopReqData(int tunnNo);
	CTunnelVector* GetTunnelVec();
	tunnel::comm::TrackingManager* Read_TrackManagerPtr();

private:
	//flow
	tunnel::comm::RequestProcessor	*m_pProcessor;
	tunnel::comm::TrackingManager	*m_pTracking;
	tunnel::comm::TasksPool			*m_pTasks;
	//tasks
	tunnel::comm::OpenSession	*m_pSession;
	tunnel::comm::CreateLease	*m_pLease;
	tunnel::comm::SendCSRequest	*m_pCSreq;
	//gateway
	tunnel::gateway::GChannel	*m_pChannel;
	//thread
	//boost::thread_group m_threadGroup;
};


#endif
