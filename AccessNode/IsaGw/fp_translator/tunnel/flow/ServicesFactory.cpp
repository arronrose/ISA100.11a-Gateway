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

#include "ServicesFactory.h"
#include "../services/IGServiceVisitor.h"

#include <string.h>

namespace tunnel {
namespace comm {


AbstractGServicePtr ServicesFactory::Create(Request& req)
{
	
	//LOG_DEBUG("Create: GService for request:" << req.ToString());

	switch(req.type)
	{
	case Request::rtSession:
		{
			assert(req.sessionID == 0); //we must create a session

			GSession *pSession = new GSession();

			pSession->m_sessionID = req.sessionID;
			pSession->m_sessionPeriod = -1;	//infinite
			pSession->m_networkID = 1;		//it doesn't matter right now

			return AbstractGServicePtr(pSession);
		}
		
	case Request::rtLease:
		{
			assert(req.Param.Lease.leaseType == GLease::Client || 
					req.Param.Lease.leaseType == GLease::Server); //we support just 2 leases!

			GLease *pLease = new GLease();

			pLease->m_sessionID = req.sessionID;
			pLease->m_tunnelNo = req.Param.Lease.tunnNo;

			pLease->m_leaseID = 0;		//new lease	
			pLease->m_leasePeriod = 0;	// infinite	
			pLease->m_leaseType = (GLease::GSLeaseType)req.Param.Lease.leaseType;	
			
			pLease->m_protocolType = req.Param.Lease.protocolType;
			pLease->m_endPointsNo = 1;		
			
			memcpy(pLease->m_endPoint.ipAddress, req.Param.Lease.ipAddress, sizeof(req.Param.Lease.ipAddress));
			pLease->m_endPoint.remotePort = req.Param.Lease.remotePort;
			pLease->m_endPoint.remoteObjID = req.Param.Lease.remoteObjID;
			pLease->m_endPoint.localPort = req.Param.Lease.localPort;
			pLease->m_endPoint.localObjID = req.Param.Lease.localObjID;
			
			pLease->m_parameters.transferMode = 0;
			pLease->m_parameters.updatePolicy = 0;
			pLease->m_parameters.subscriptionPeriod = 0;
			pLease->m_parameters.phase = 0;
			pLease->m_parameters.staleLimit = 0;

			pLease->m_connInfo = *req.Param.Lease.pConnInfo;

			return AbstractGServicePtr(pLease);
		}

	case Request::rtClientServer_C:
		{
			assert(req.Param.ClntServer_C.leaseID != 0);
			assert(req.Param.ClntServer_C.pReqData->size() != 0);
			
			//create
			GClientServer_C *pClient = new GClientServer_C();

			pClient->m_sessionID = req.sessionID;
			pClient->m_tunnelNo = req.Param.ClntServer_C.tunnNo;

			pClient->m_leaseID = req.Param.ClntServer_C.leaseID;			
			pClient->m_buffer = 0;			
			pClient->m_transferMode = 1;	

			pClient->m_reqData = *req.Param.ClntServer_C.pReqData;		
			pClient->m_transacInfo = *req.Param.ClntServer_C.pTransacInfo;	

			return AbstractGServicePtr(pClient);
		}

	default:
		break;
	}

	assert(false);
	return NULL;
}
	

} // namespace comm
} // namespace tunnel
