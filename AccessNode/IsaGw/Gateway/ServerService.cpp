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

////////////////////////////////////////////////////////////////////////////////
/// @file 		ServerService.cpp   
/// @author 	Claudiu Hobeanu
/// @brief 		Implementation of CServerService class
////////////////////////////////////////////////////////////////////////////////
#include "../ISA100/typedef.h"
#include "../ISA100/porting.h" 	// GENERIC_ASL_SRVC
#include "../../Shared/Common.h"
#include "GwUAP.h"
#include "ServerService.h"



CServerService::~CServerService(void)
{
}

/// Return true if the service is able to handle the service type received as parameter
/// Used by (USER -> GW) flow
bool CServerService::CanHandleServiceType( uint8 p_ucServiceType ) const
{
	//do nothing for now, no FPT responses for now	
	return false;
}

/// (USER -> GW) Process a user request
/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
bool CServerService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{
	//do nothing for now, no FPT responses for now
	LOG("CServerService::ProcessUserRequest: ERROR: should not be called");
	return false;
}

/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
bool CServerService::ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq)
{
	LOG("CServerService::ProcessUserRequest: ERROR: should not be called");
	
	

	return false;
}


/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
bool CServerService::ProcessTunnelAPDU( APDU_IDTF* pAPDUIdtf, GENERIC_ASL_SRVC* p_pReq)
{
	if (p_pReq->m_ucType != SRVC_TUNNEL_REQ)
	{
		LOG("CServerService::ProcessUserRequest: ERROR: only tunnel support for now srvcType=%d", p_pReq->m_ucType );
		return false;
	}
	TUNNEL_REQ_SRVC* pTunnelReq = (TUNNEL_REQ_SRVC*)&p_pReq->m_stSRVC.m_stTunnelReq;

	lease* pLease =  g_pLeaseMgr->FindLease(LEASE_SERVER, pAPDUIdtf->m_aucAddr, 
												pAPDUIdtf->m_ucSrcTSAPID, pTunnelReq->m_unSrcOID,
												pAPDUIdtf->m_ucDstTSAPID, pTunnelReq->m_unDstOID );

	if (!pLease)
	{
		LOG("CServerService::ProcessTunnelAPDU: no lease");
		return false;
	}

	int nTmpLen = 4 + 2 + pTunnelReq->m_unLen + 16 + 16 + 2 + pLease->m_oConnectionInfo.size();
	uint8_t pTmpBuff [nTmpLen];
	uint8_t* pCrtBuff = pTmpBuff;
	

	u_int nTmp = htonl(pLease->nLeaseId);
	memcpy(pCrtBuff,&nTmp, sizeof(nTmp));
	pCrtBuff += sizeof(nTmp);

	uint16 u16Tmp = htons( pTunnelReq->m_unLen);
	memcpy(pCrtBuff, &u16Tmp, sizeof(u16Tmp));
	pCrtBuff += sizeof(u16Tmp);

	memcpy(pCrtBuff, pTunnelReq->m_pReqData, pTunnelReq->m_unLen );
	pCrtBuff += pTunnelReq->m_unLen ;

	memset(pCrtBuff, 0, 2 * 16); //Foreign_Destination_Address, Foreign_Source_Address

	pCrtBuff += 2 * 16;

	u16Tmp = htons( pLease->m_oConnectionInfo.size());
	memcpy(pCrtBuff, &u16Tmp, sizeof(u16Tmp));
	pCrtBuff += sizeof(u16Tmp);

	memcpy(pCrtBuff, &pLease->m_oConnectionInfo[0], pLease->m_oConnectionInfo.size());

	return SendIndication(CLIENT_SERVER, pLease, pTmpBuff, nTmpLen);
}

