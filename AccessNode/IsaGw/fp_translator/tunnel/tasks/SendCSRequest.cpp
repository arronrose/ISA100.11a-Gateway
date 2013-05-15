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

#include "SendCSRequest.h"

#include "../TunnelIO.h"
#include "../flow/RequestProcessor.h"


namespace tunnel {
namespace comm {


SendCSRequest::SendCSRequest(CTunnelIO *pTunnelIO, RequestProcessor* reqProcessorPtr)
{
	m_pTunnelIO = pTunnelIO;
	m_reqProcessorPtr = reqProcessorPtr;
}


void SendCSRequest::DoCSRequest()
{

	CTunnelVector* pTunnelVec = m_pTunnelIO->GetTunnelVec();

	for (int i = 0; i < pTunnelVec->size(); i++)
	{

		if (m_pTunnelIO->Read_SessionID() == 0)
			return;

		if (m_pTunnelIO->Read_C_LeaseID(i) != 0 && m_pTunnelIO->Is_ReqData(i)) //send cs
		{
			Request csreq;
			csreq.sessionID = m_pTunnelIO->Read_SessionID();
			csreq.type = Request::rtClientServer_C;
			csreq.Param.ClntServer_C.leaseID = m_pTunnelIO->Read_C_LeaseID(i);
			csreq.Param.ClntServer_C.tunnNo = i;

			//not defined yet
			csreq.Param.ClntServer_C.pTransacInfo = new std::basic_string<boost::uint8_t>;
			
			csreq.Param.ClntServer_C.pReqData =new std::basic_string<boost::uint8_t>;
			m_pTunnelIO->WriteReqData(i, *csreq.Param.ClntServer_C.pReqData);

			m_reqProcessorPtr->ProcessRequest(csreq);
			
			delete csreq.Param.ClntServer_C.pTransacInfo;
			delete csreq.Param.ClntServer_C.pReqData;
		}
	}

}


}
}
