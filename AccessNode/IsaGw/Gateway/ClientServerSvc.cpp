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
/// @file ClientServerSvc.cpp
/// @author Marcel Ionescu
/// @brief Client/server GSAP - implementation
////////////////////////////////////////////////////////////////////////////////
#include <arpa/inet.h>	// ntohs

#include "GwApp.h"
#include "ClientServerSvc.h"
#include "GwUtil.h"
////////////////////////////////////////////////////////////////////////////////
/// @class CClientServerService
/// @brief handle Client/Server generinc requests
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @class CClientServerService::CSKey
/// @brief C/S cache key
////////////////////////////////////////////////////////////////////////////////

CClientServerService::CSKey::CSKey( uint16 p_unContractID, uint8 p_ucBuffer, uint8 p_ucReqType, const uint8 * p_pData )
	: m_unContractID( p_unContractID )
	, m_ucBuffer( p_ucBuffer )
	, m_ucReqType( p_ucReqType )
	, m_tCreat( time(NULL) )
{
	switch( m_ucReqType )	///< Use p_ucReqType to determine m_ushDataLen
	{	case SRVC_TYPE_READ: 	m_ushDataLen = sizeof(TClientServerReqData_Read); break;
		case SRVC_TYPE_WRITE:	m_ushDataLen = sizeof(TClientServerReqData_Write) + ((TClientServerReqData_Write*)p_pData)->m_ushReqLen; break;
		case SRVC_TYPE_EXECUTE:	m_ushDataLen = sizeof(TClientServerReqData_Exec)  + ((TClientServerReqData_Exec*)p_pData)->m_ushReqLen;  break;
		default: m_ushDataLen = 0; m_pData = NULL;
	}
	if( m_ushDataLen)
	{
		m_pData = new uint8[ m_ushDataLen ];
		memcpy( m_pData, p_pData, m_ushDataLen);
	}
}

CClientServerService::CSKey::~CSKey()
{
	delete [] m_pData;
}

const char * CClientServerService::CSKey::Identify( void ) const
{ 	static char sBuf[ 256 ] = {0};
	char szTmp [64] = {0};
	switch( m_ucReqType ){	/// The constructor ensure there is enough data for p
		case SRVC_TYPE_READ:{ TClientServerReqData_Read *p = (TClientServerReqData_Read*)m_pData;
			sprintf(szTmp, "OID %u Attr %d Idx %d %d", p->m_ushObjectID, p->m_ushAttrID, p->m_ushAttrIdx1, p->m_ushAttrIdx2 ); }
			break;
		case SRVC_TYPE_WRITE:{ TClientServerReqData_Write *p = (TClientServerReqData_Write*)m_pData;
			sprintf(szTmp, "OID %u Attr %d Idx %d %d Len %u", p->m_ushObjectID, p->m_ushAttrID, p->m_ushAttrIdx1, p->m_ushAttrIdx2, p->m_ushReqLen ); }
			break;
		case SRVC_TYPE_EXECUTE:{ TClientServerReqData_Exec *p = (TClientServerReqData_Exec*)m_pData;
			sprintf(szTmp, "OID %u Meth %d Len %d", p->m_ushObjectID, p->m_ushMethodID, p->m_ushReqLen ); }
			break;
		default:
			sprintf(szTmp, "WARNING: TOO SMALL");
	}
	snprintf( sBuf, sizeof(sBuf), " Life %+6d C %3u %s Buf %u [%s]", (int)(m_tCreat-time(NULL)), m_unContractID, getISA100SrvcTypeName(GET_ASL_SRVC_TYPE(m_ucReqType)), m_ucBuffer, szTmp );
	sBuf[ sizeof(sBuf) - 1 ] = 0;
	return sBuf;
}

CClientServerService::CSData::CSData( uint16 p_ushDataLen)
{
	m_tLastRx    = 0;
	m_ushDataLen = p_ushDataLen;
	m_pData      = new byte[ m_ushDataLen ];
};

CClientServerService::CSData::~CSData()
{
#if 0
	if( m_ushDataLen >= sizeof(TClientServerConfirmData_W) )
	{
		TClientServerConfirmData_W * p = (TClientServerConfirmData_W*)m_pData;
		LOG("    CS_DEBUG ~CSData(Rx %d Len %u) (Stat %X Type %X:%s OID %u) %s%s", m_tLastRx, m_ushDataLen,
			p->m_ucStatus, p->m_ucReqType, getISA100SrvcTypeName(GET_ASL_SRVC_TYPE(p->m_ucReqType)), p->m_ushObjectID,
			GET_HEX_LIMITED( m_pData, m_ushDataLen, g_stApp.m_stCfg.m_nMaxCSDataLog ) );
	}
	else
		LOG("ERROR CS_DEBUG ~CSData(Rx %u Len %u)", m_tLastRx, m_ushDataLen);
#endif
	delete [] m_pData;
}

/// @note m_ucBuffer is NOT used to compare. We just need it to differentiate
/// 	CLIENTSERVER_NOT_AVAILABLE / CLIENTSERVER_BUFFER_INVALID  on GSAP or
/// 	YGS_CACHE_MISS / YGS_NOT_ACCESSIBLE on YGSAP in case of failure (ProcessISATimeout)
/// @note m_tCreat is NOT used to compare.
bool CClientServerService::cacheCompare::operator()( CSKey* p_pK1, CSKey* p_pK2 ) const
{
	if(      p_pK1->m_unContractID != p_pK2->m_unContractID ) return (p_pK1->m_unContractID < p_pK2->m_unContractID);
	else if( p_pK1->m_ucReqType    != p_pK2->m_ucReqType)     return (p_pK1->m_ucReqType    < p_pK2->m_ucReqType);
	else if( p_pK1->m_ushDataLen   != p_pK2->m_ushDataLen)    return (p_pK1->m_ushDataLen   < p_pK2->m_ushDataLen);
	else return (memcmp( p_pK1->m_pData, p_pK2->m_pData, p_pK1->m_ushDataLen) < 0);
}

CClientServerService::~CClientServerService( )
{	for(TCSMap::iterator it = m_mapCache.begin(); it != m_mapCache.end(); ++it)
	{
		delete it->first;
		delete it->second;
	}
	m_mapCache.clear();

	for(TCSPending::iterator it = m_mapPendingReq.begin(); it != m_mapPendingReq.end(); ++it)
	{
		delete it->second;
	}
	m_mapPendingReq.clear();
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Check if this worker can handle service type received as parameter
/// @param p_ucServiceType - service type tested
/// @retval true - this service can handle p_ucServiceType
/// @retval false - the service cannot handle p_ucServiceType
/// @remarks The method is called from CInterfaceObjMgr::DispatchUserRequest
/// to decide where to dispatch a user request
////////////////////////////////////////////////////////////////////////////////
bool CClientServerService::CanHandleServiceType( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case CLIENT_SERVER:
			return true;
	}
	return false;
};

template <class SizeType>
uint8_t* UnpackNetworkVariableData ( uint8_t* p_pData, int p_nDataLen, SizeType* p_pnOutLen, const char* p_szErrorTag = "" )
{
	SizeType dataSize; 

	if ( (int)sizeof(dataSize) > p_nDataLen )
	{
		LOG("UnpackNetworkVariableData: ERROR: %s: can not extract size",p_szErrorTag);
		return NULL;
	}

	memcpy (&dataSize, p_pData, sizeof(dataSize));

	switch(sizeof(SizeType))
	{
	case 2:
	{
		//dataSize = (SizeType)ntohs((uint16_t)dataSize);
		uint16_t tmp;
		tmp =  (dataSize);
		tmp = ntohs(2);
		break;
	}
	case 4:
		dataSize = (SizeType)ntohl((uint32_t)dataSize);
		break;
	default:
		break;
	}

	p_pData += sizeof (SizeType);
	p_nDataLen -= sizeof (SizeType);

	if (dataSize >  p_nDataLen)
	{
		LOG("UnpackNetworkVariableData: ERROR: %s: pack size %d > % buffer size", p_szErrorTag, dataSize, p_nDataLen);
		return NULL;
	}

	*p_pnOutLen = dataSize;
	//p_poBuffer->Set(0, p_pData, dataSize);

	return p_pData + dataSize;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Claudiu Hobeanu
/// @brief (USER -> GW) Process a tunnel user request
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param p_pData		GSAP request data (data size is in p_pHdr)
/// @param p_pLease		pointer to lease
/// @retval TBD
/// @remarks CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
/// Get the response from local cache if requested and possible (data is fresh)
/// Otherwise send the request to device
////////////////////////////////////////////////////////////////////////////////
bool CClientServerService::processUserRequestTunnel( CGSAP::TGSAPHdr * p_pHdr, void * p_pData, lease* p_pLease )
{
	uint8_t* pCrtData = (uint8_t*)p_pData + offsetof(TClientServerReqData,m_ucReqType); //
	int nCrtLen = p_pHdr->m_nDataSize - 4;

	uint16_t u16BuffDataLen;

	if (!UnpackNetworkVariableData<>(pCrtData,nCrtLen,&u16BuffDataLen, "processUserRequestTunnel: requestData" ))
	{
		return false;
	}
	pCrtData += sizeof(u16BuffDataLen);

	TUNNEL_REQ_SRVC stTunnelReq;
	stTunnelReq.m_unDstOID = p_pLease->nObjId;
	stTunnelReq.m_unSrcOID = p_pLease->m_u16LocalObjId;
	stTunnelReq.m_unLen = u16BuffDataLen;
	stTunnelReq.m_pReqData = pCrtData;

	pCrtData += u16BuffDataLen;
	nCrtLen -= (u16BuffDataLen + sizeof(u16BuffDataLen));


	if (!UnpackNetworkVariableData<>(pCrtData, nCrtLen, &u16BuffDataLen, "processUserRequestTunnel: TransactionInfo" ))
	{
		return false;
	}

	pCrtData += sizeof(u16BuffDataLen);
	p_pLease->m_oTransactionInfo.assign (pCrtData, pCrtData + u16BuffDataLen);

	pCrtData += u16BuffDataLen;
	nCrtLen -= (u16BuffDataLen + sizeof(u16BuffDataLen));


	if (		p_pLease->eProtocolType		!= PROTO_NONE
		&&	(	p_pLease->m_u16LocalObjId	!= PROMISCUOUS_TUNNEL_LOCAL_OID
			||	p_pLease->m_u16LocalTSAPID	!= PROMISCUOUS_TUNNEL_LOCAL_TSAP_ID)
		)
	{
		/// load tunnel obj if not up

		CIsa100Object::Ptr pSmartTunnel =  g_stApp.m_oGwUAP.GetTunnelObject(p_pLease->m_u16LocalTSAPID, p_pLease->m_u16LocalObjId);

		if (!pSmartTunnel)
		{
			clientServerConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : CLIENTSERVER_FAIL_OTHER, false );
			return false;
		}

		/// configure tunnel obj
		CTunnelObject* pTunnel = (CTunnelObject*)pSmartTunnel.get();
		pTunnel->m_oTransactionInfo.assign (p_pLease->m_oTransactionInfo.begin(), p_pLease->m_oTransactionInfo.end());
	}

	uint8_t ucRet;
	unsigned short ushAppHandle = g_stApp.m_oGwUAP.SendRequestToASL(&stTunnelReq, SRVC_TUNNEL_REQ, p_pLease->m_u16LocalTSAPID, p_pLease->m_ushTSAPID, p_pLease->nContractId, true, &ucRet);

	uint8_t uchStatus = p_pHdr->IsYGSAP()
		? ( (ucRet != SFC_SUCCESS) ? YGS_FAILURE : YGS_SUCCESS)
		: ( (ucRet != SFC_SUCCESS) ? CLIENTSERVER_FAIL_OTHER : CLIENTSERVER_SUCCESS);
	/// Do not attempt to remove from tracker, it's not there
	clientServerConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, uchStatus, false );

	LOG("CLIENTSERVER(H %u S %u T %u L %u) %s:%u ret=%d", ushAppHandle, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pLease->nLeaseId,
		GetHex( p_pLease->netAddr, 16), p_pLease->m_ushTSAPID, ucRet );

	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Process a user request
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @retval TBD
/// @remarks CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
/// Get the response from local cache if requested and possible (data is fresh)
/// Otherwise send the request to device
////////////////////////////////////////////////////////////////////////////////
bool CClientServerService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{	///p_pHdr->m_ucServiceType == CLIENT_SERVER
	GENERIC_ASL_SRVC stReq;
	void * pASLReq = NULL;
	TClientServerReqData* pReq = (TClientServerReqData*)p_pData;
	if( (p_pHdr->m_nDataSize < 4) || (p_pHdr->m_nDataSize - 4 < offsetof(TClientServerReqData,m_ucReqType)))
	{
		LOG("ERROR CLIENTSERVER REQ (S %u T %u) data small %d", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_nDataSize);
		clientServerConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : CLIENTSERVER_FAIL_OTHER, false );
		return true;	/// Processed
	}
	pReq->NTOH();

	lease* pLease = g_pLeaseMgr->FindLease( pReq->m_nLeaseID, p_pHdr->m_nSessionID );
	if( (!pLease) || (LEASE_CLIENT != pLease->eLeaseType && LEASE_SERVER != pLease->eLeaseType) || (!pLease->nContractId))
	{	char szExtra[64] = {0};
		if( pLease ) sprintf(szExtra, " Type %u Contract %u", pLease->eLeaseType, pLease->nContractId);
		LOG("ERROR CLIENTSERVER REQ (S %u T %u) invalid/expired lease id %u%s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pReq->m_nLeaseID, szExtra);
		clientServerConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_INVALID_LEASE : CLIENTSERVER_LEASE_EXPIRED, false );
		return true;	/// Processed
	}

	if (pLease->eProtocolType != PROTO_NONE)
	{
		/// jump to tunnel code processing
		return processUserRequestTunnel(p_pHdr, p_pData, pLease);
	}
//	pReq->m_ucBuffer = 1;	DEBUG: REQUEST buffered
	switch( pReq->m_ucReqType )
	{
	case SRVC_TYPE_READ:
		pReq->m_uData.m_stReadData.NTOH();
		stReq.m_stSRVC.m_stReadReq.m_unSrcOID = m_ushInterfaceOID;
		stReq.m_stSRVC.m_stReadReq.m_unDstOID = pReq->m_uData.m_stReadData.m_ushObjectID;
		stReq.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_ucAttrFormat = ATTR_TWO_INDEX;
		stReq.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unAttrID = pReq->m_uData.m_stReadData.m_ushAttrID;
		stReq.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unIndex1 = pReq->m_uData.m_stReadData.m_ushAttrIdx1;
		stReq.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unIndex2 = pReq->m_uData.m_stReadData.m_ushAttrIdx2;
		if(!stReq.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unIndex2)
		{
			stReq.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_ucAttrFormat = ATTR_ONE_INDEX;
			if(!stReq.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_unIndex1)
				stReq.m_stSRVC.m_stReadReq.m_stAttrIdtf.m_ucAttrFormat = ATTR_NO_INDEX;
		}
		stReq.m_ucType = SRVC_READ_REQ;
		pASLReq = &stReq.m_stSRVC.m_stReadReq;
		break;

	case SRVC_TYPE_WRITE:
		pReq->m_uData.m_stWriteData.NTOH();
		stReq.m_stSRVC.m_stWriteReq.m_unSrcOID = m_ushInterfaceOID;
		stReq.m_stSRVC.m_stWriteReq.m_unDstOID = pReq->m_uData.m_stWriteData.m_ushObjectID;
		stReq.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_ucAttrFormat = ATTR_TWO_INDEX;
		stReq.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unAttrID = pReq->m_uData.m_stWriteData.m_ushAttrID;
		stReq.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unIndex1 = pReq->m_uData.m_stWriteData.m_ushAttrIdx1;
		stReq.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unIndex2 = pReq->m_uData.m_stWriteData.m_ushAttrIdx2;
		if(!stReq.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unIndex2)
		{
			stReq.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_ucAttrFormat = ATTR_ONE_INDEX;
			if(!stReq.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_unIndex1)
				stReq.m_stSRVC.m_stWriteReq.m_stAttrIdtf.m_ucAttrFormat = ATTR_NO_INDEX;
		}
		stReq.m_stSRVC.m_stWriteReq.m_unLen = pReq->m_uData.m_stWriteData.m_ushReqLen;
		stReq.m_stSRVC.m_stWriteReq.p_pReqData = pReq->m_uData.m_stWriteData.m_aReqData;
		stReq.m_ucType = SRVC_WRITE_REQ;
		pASLReq = &stReq.m_stSRVC.m_stWriteReq;
		break;

	case SRVC_TYPE_EXECUTE:
		pReq->m_uData.m_stExecData.NTOH();
		stReq.m_stSRVC.m_stExecReq.m_unSrcOID = m_ushInterfaceOID;
		stReq.m_stSRVC.m_stExecReq.m_unDstOID = pReq->m_uData.m_stExecData.m_ushObjectID;
		stReq.m_stSRVC.m_stExecReq.m_ucMethID = pReq->m_uData.m_stExecData.m_ushMethodID;
		stReq.m_stSRVC.m_stExecReq.m_unLen = pReq->m_uData.m_stExecData.m_ushReqLen;
		stReq.m_stSRVC.m_stExecReq.p_pReqData = pReq->m_uData.m_stExecData.m_aReqData;
		stReq.m_ucType = SRVC_EXEC_REQ;
		pASLReq = &stReq.m_stSRVC.m_stExecReq;
		break;

	default:
		LOG("ERROR CLIENTSERVER REQ(S %u T %u L %u) invalid REQ type %X:%s from %s",
			p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pReq->m_nLeaseID,
			pReq->m_ucReqType, getISA100SrvcTypeName(GET_ASL_SRVC_TYPE(pReq->m_ucReqType)), pReq->m_ucBuffer ? "CACHE ":"DEVICE" );
		clientServerConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : CLIENTSERVER_FAIL_OTHER, false );
		return true;	/// Processed
	}

	/// If m_ucBuffer search in cache. Only if not present in cache ask de device
	CSKey *  pKey = new CSKey( pLease->nContractId, pReq->m_ucBuffer, pReq->m_ucReqType, (byte*)&pReq->m_uData );
	TCSMap::iterator it = m_mapCache.end();

	if( pReq->m_ucBuffer )
	{	/// Deliver from cache, if available and fresh
		it = m_mapCache.find( pKey );

		if( (it != m_mapCache.end()) && (it->second->IsValid( g_stApp.m_stCfg.m_nClientServerCacheTimeout ) )  )
		{	/// Return from cache. Use original TX time (m_unTxSeconds / m_unTxuSeconds) on cached data
			LOG("CLIENTSERVER(S %u T %u L %u) %s:%u %s get from CACHE", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pReq->m_nLeaseID,
				GetHex( pLease->netAddr, 16), pLease->m_ushTSAPID, pKey->Identify() );

			/// Do not attempt to remove from tracker: it's not there
			clientServerConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, it->second->m_pData, it->second->m_ushDataLen, false );
			delete pKey;
			return true;
		}
	}
	/// Not found in cache, or found, but expired. Send Req to device
	uint8_t ucSFC = SFC_SUCCESS;
	uint16 ushAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( pASLReq, stReq.m_ucType, pLease->m_ushTSAPID,
		pLease->nContractId, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_ucServiceType, 0, &ucSFC);

	if( !ushAppHandle )
	{	/// Cannot send to ASL, report error. Do not attempt to remove from tracker, it's not there
		clientServerConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP()
			? YGS_FAILURE
			: ((SFC_NO_CONTRACT == ucSFC) ? CLIENTSERVER_LEASE_EXPIRED : CLIENTSERVER_FAIL_OTHER), false );
		delete pKey;
		return true;	/// Processed
	}
	LOG("CLIENTSERVER(H %u S %u T %u L %u) %s:%u %s %s => ask DEVICE", ushAppHandle, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pReq->m_nLeaseID,
		GetHex( pLease->netAddr, 16), pLease->m_ushTSAPID, pKey->Identify(),
		  (!pReq->m_ucBuffer)		? "UNBUFF REQ  "
		: (it == m_mapCache.end()) 	? "CACHE MISS  "
		: /* !it->second.IsValid */   "CACHE EXPIRE");

	m_mapPendingReq.insert( std::make_pair(ushAppHandle, pKey) );
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Reacts to a lease delete: free resources, if necessary
/// @param m_nSessionID
/// @param m_nLeaseID
/// @remarks Identified services using it: ClientServer service, BulkTransfer Service, PublishSubscribe service
////////////////////////////////////////////////////////////////////////////////
void CClientServerService::OnLeaseDelete( lease* p_pLease )
{
	unsigned nCount = 0, nPending = 0;

	DMO_CONTRACT_ATTRIBUTE *pContractAttrib = DMO_FindContract( p_pLease->nContractId );
	int nUsageCount = pContractAttrib ? pContractAttrib->m_nUsageCount : -1;/// jst a weird value to differentiate

	/// delete cache/pending for a contract ONLY if contract's usage count is 0.
	for(TCSPending::iterator it = m_mapPendingReq.begin(); (0 == nUsageCount) && (it != m_mapPendingReq.end()); )
	{
		if( it->second->m_unContractID == p_pLease->nContractId )
		{	LOG("   CS OnLeaseDelete(%lu) pending %s", p_pLease->nLeaseId, it->second->Identify() );

			++nPending;
			delete it->second;
			m_mapPendingReq.erase( it++ );
		}
		else
			++it;
	}

	for( TCSMap::iterator it = m_mapCache.begin(); (0 == nUsageCount) && (it != m_mapCache.end()); )
	{
		if( it->first->m_unContractID == p_pLease->nContractId )
		{
			LOG("   CS OnLeaseDelete(%lu) %s", p_pLease->nLeaseId, it->first->Identify() );
			LOG("   CS OnLeaseDelete(%lu) LastRx %+d Data(%u) %s%s", p_pLease->nLeaseId,
				it->second->m_tLastRx - time(NULL), it->second->m_ushDataLen,
				GET_HEX_LIMITED(it->second->m_pData, it->second->m_ushDataLen, g_stApp.m_stCfg.m_nMaxCSDataLog ) );

			++nCount;
			delete it->first;
			delete it->second;
			m_mapCache.erase( it++ );
		}
		else
			++it;
	}

	if( nCount || nPending)
		LOG("   CS OnLeaseDelete(%lu contract %u used %d) deleted %u CACHE / %u pending entries",
			p_pLease->nLeaseId, p_pLease->nContractId, nUsageCount, nCount, nPending );
}
////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Process an ADPU
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_pAPDUIdtf - needed for device TX time with usec resolution
/// @param p_pRsp The response from field, unpacked
/// @param p_pReq The original request
/// @retval false - field APDU not dispatched - default handler does not do processing, just log a message
/// @note ISA standard specify in case of SFC: SRVC_READ_RSP has NO length/data, but SRVC_EXEC_RSP has length and optionally data
/// @note SRVC_READ_RSP: length is optional, missing if SFC of not SFC_SUCCESS
/// @note SRVC_EXEC_RSP: legnth is mandatory, but it may be 0
/// @note SRVC_WRITE_RSP: no length. SFC is last field
////////////////////////////////////////////////////////////////////////////////
bool CClientServerService::ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	CSData       * pData = NULL;
	const CSKey  * pKey  = NULL;
	bool bMustDelete = false;
	CMicroSec uSec;
	TCSPending::iterator itKey = m_mapPendingReq.find( p_unAppHandle );
	size_t	dataLenCS 	= 0,	// C/S data length (in GSAP)
			dataLenDev	= 0;	// Device response data length

	if( SFC_SUCCESS != p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC )	/// All response types (R/W/X) are overlapping up to m_ucSFC
	{	LOG("WARNING CLIENTSERVER ProcessAPDU(H %u Type %02X): m_ucSFC %d != SFC_SUCCESS", p_unAppHandle, p_pRsp->m_ucType, p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC );
	}
	switch( p_pRsp->m_ucType )
	{
		case SRVC_EXEC_RSP:
			dataLenDev = p_pRsp->m_stSRVC.m_stExecRsp.m_unLen;
			dataLenCS  = offsetof( TClientServerConfirmData_RX, m_aRspData ) + dataLenDev;
		break;
		case SRVC_READ_RSP:	/// device will not send m_unLen on bad SFC, avoid reading/using unitialised data size
			dataLenDev = (SFC_SUCCESS == p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC) ? p_pRsp->m_stSRVC.m_stReadRsp.m_unLen : 0;
			dataLenCS  = offsetof( TClientServerConfirmData_RX, m_aRspData ) + dataLenDev;
		break;
		case SRVC_WRITE_RSP:
		default: dataLenCS = sizeof( TClientServerConfirmData_W );
	}
	if( itKey == m_mapPendingReq.end() )
	{	/// Should NEVER happen
		LOG("WARNING CLIENTSERVER ProcessAPDU(H %u Type %02X): NOT FOUND in m_mapPendingReq. Creating substitute key/data", p_unAppHandle, p_pRsp->m_ucType );
		pData = new CSData( dataLenCS );
		pKey  = new CSKey( 0, 0, 0, NULL );
		bMustDelete = true;
	}
	else
	{
		TCSMap::iterator itCache = m_mapCache.find( itKey->second );

		if( itCache == m_mapCache.end() )
		{	/// not found in cache, ADD
			//LOG("CLIENTSERVER (H %u) ADD to cache %u bytes", p_unAppHandle, dataLenCS);
			pData = new CSData( dataLenCS );
			itCache = m_mapCache.insert( std::make_pair(itKey->second, pData) ).first;	// insert return a pair<iterator,bool>. We need the first
		}	/// else found: just UPDATE
		else
		{
			//LOG("CLIENTSERVER (H %u) UPDATE cache %u bytes", p_unAppHandle, dataLenCS);
			itCache->second->resize( dataLenCS );
			pData = itCache->second;
			delete itKey->second;
		}
		m_mapPendingReq.erase( itKey );	
		itCache->second->updateLastRx();
		pKey = itCache->first;
	}

	switch( p_pRsp->m_ucType )
	{
		case SRVC_EXEC_RSP:
		case SRVC_READ_RSP:	/// Read / eXecute structures have the same layout (m_stReadRsp == m_stWriteRsp)
		{	TClientServerConfirmData_RX * pCnfrmR = (TClientServerConfirmData_RX *) pData->m_pData;
			pCnfrmR->m_ucStatus		= CLIENTSERVER_SUCCESS;	/// same as YGS_SUCCESS
			pCnfrmR->m_unTxSeconds	= htonl(p_pAPDUIdtf->m_tvTxTime.tv_sec);
			pCnfrmR->m_unTxuSeconds	= htonl(p_pAPDUIdtf->m_tvTxTime.tv_usec);
			pCnfrmR->m_ucReqType	= p_pRsp->m_ucType;
			pCnfrmR->m_ushObjectID	= p_pRsp->m_stSRVC.m_stReadRsp.m_unSrcOID;
			pCnfrmR->m_ucSFC		= p_pRsp->m_stSRVC.m_stReadRsp.m_ucSFC;
			pCnfrmR->m_ushRspLen	= dataLenDev;
			memcpy( pCnfrmR->m_aRspData, p_pRsp->m_stSRVC.m_stReadRsp.m_pRspData, dataLenDev );	/// A single data copy, OK
			pCnfrmR->HTON();

			if( ( SRVC_EXEC_RSP == p_pRsp->m_ucType )
			&&	( SFC_SUCCESS   == p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC )
			&&	( UAP_SMAP_ID   == p_pAPDUIdtf->m_ucSrcTSAPID )
			&&	( SM_SMO_OBJ_ID == p_pReq->m_stSRVC.m_stExecReq.m_unDstOID )
			&&	( SMO_METHOD_GET_CONTRACTS_AND_ROUTES == p_pReq->m_stSRVC.m_stExecReq.m_ucMethID ) )
			{	/// Get contracts and routes, regardless if it's auto-generated or client generated
				/// Parse and save in publish class the expected publish rate from periodic contracts with GW
				g_stApp.m_oGwUAP.UpdatePublishRate( pData );
			}
			break;
		}

		case SRVC_WRITE_RSP:
		{	TClientServerConfirmData_W * pCnfrmW = (TClientServerConfirmData_W *) pData->m_pData;
			pCnfrmW->m_ucStatus		= CLIENTSERVER_SUCCESS; /// same as YGS_SUCCESS
			pCnfrmW->m_unTxSeconds	= htonl(p_pAPDUIdtf->m_tvTxTime.tv_sec);
			pCnfrmW->m_unTxuSeconds	= htonl(p_pAPDUIdtf->m_tvTxTime.tv_usec);
			pCnfrmW->m_ucReqType	= p_pRsp->m_ucType;
			pCnfrmW->m_ushObjectID	= p_pRsp->m_stSRVC.m_stWriteRsp.m_unSrcOID;
			pCnfrmW->m_ucSFC		= p_pRsp->m_stSRVC.m_stWriteRsp.m_ucSFC;
			pCnfrmW->HTON();
			break;
		}

		default:
			LOG("ERROR CLIENTSERVER ProcessAPDU(H %u): unknown response type %X", p_unAppHandle, p_pRsp->m_ucType );
			if(bMustDelete)
			{
				delete pKey;
				delete pData;
			}
			MSG   *	pMSG;
			CGSAP *	pGSAP;
			if( (pGSAP = g_stApp.m_oGwUAP.FindGSAP( p_unAppHandle, pMSG )) )
				clientServerConfirm( pMSG->m_nSessionID, pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession( pMSG->m_nSessionID ) ? YGS_FAILURE : CLIENTSERVER_FAIL_OTHER );
			else
				LOG("ERROR ProcessISATimeout(H %u): AppHandle not found in tracker, CONFIRM not sent", p_unAppHandle );
			return true;
	}
	LOG("CLIENTSERVER(H %u) %s get from DEVICE",  p_unAppHandle , pKey->Identify() );

	clientServerConfirm( p_unAppHandle, pData->m_pData, dataLenCS );

	if(bMustDelete)
	{
		delete pKey;
		delete pData;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Proces a ISA100 timeout
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_pOriginalReq the original request
/// @retval true - timeout sent to user
/// @retval true - timeout not sent to user
/// @remarks CALL whenever a timeout is received for a request generated from this class
/// (The method is called from CInterfaceObjMgr::DispatchISATimeout)
/// Derived classess should override this method.
////////////////////////////////////////////////////////////////////////////////
bool CClientServerService::ProcessISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * /*p_pOriginalReq*/ )
{	/// Response is ignored by CInterfaceObjMgr
	LOG("CLIENTSERVER ProcessISATimeout(H %u): %d requests pending", p_unAppHandle, m_mapPendingReq.size() );

	bool bBufferedReq = false;
	TCSPending::iterator itKey = m_mapPendingReq.find( p_unAppHandle );
	if( itKey == m_mapPendingReq.end() )	/// p_unAppHandle should ALWAYS be in the map
	{	// PROGRAMMER ERROR: report and go on
		LOG("PROGRAMMER ERROR in ProcessISATimeout(H %u): AppHandle not found in pending list", p_unAppHandle );
	}
	else
	{	bBufferedReq = itKey->second->m_ucBuffer;
		delete itKey->second;
		m_mapPendingReq.erase( itKey );
	}

	MSG   *	pMSG;
	CGSAP *	pGSAP;
	if( (pGSAP = g_stApp.m_oGwUAP.FindGSAP( p_unAppHandle, pMSG )) )
		clientServerConfirm( pMSG->m_nSessionID, pMSG->m_TransactionID, g_pSessionMgr->IsYGSAPSession( pMSG->m_nSessionID )
			? ( bBufferedReq ? YGS_CACHE_MISS              : YGS_NOT_ACCESSIBLE )
			: ( bBufferedReq ? CLIENTSERVER_BUFFER_INVALID : CLIENTSERVER_NOT_AVAILABLE ) );
	else
		LOG("ERROR ProcessISATimeout(H %u): AppHandle not found in tracker, CONFIRM not sent", p_unAppHandle );

	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Request contract list from SM for GS_GPDU_Latency/GS_GPDU_Data_Reliability parameters in network report
/// @note The request is a C/S to a Nivis custom method in SM SMO
/// @note use m_nSessionID == 0 / m_dwTransactionID == 0 in call
/// @note Auto-generated requests (i.e. without a corresponding SAP_IN) are tracked with m_nSessionID == 0
/// @note and m_dwTransactionID == 0. The method confirm2all does not send SAP_OUT for messages with m_nSessionID == 0
////////////////////////////////////////////////////////////////////////////////
void CClientServerService::RequestContractList( void )
{
	TClientServerReqData_Exec reqData = { m_ushObjectID:SM_SMO_OBJ_ID, m_ushMethodID:SMO_METHOD_GET_CONTRACTS_AND_ROUTES, m_ushReqLen:0 };
	CSKey *  pKey = new CSKey( g_stApp.m_oGwUAP.SMContractID(), 0, SRVC_TYPE_EXECUTE, (byte*)&reqData );

	for( TCSPending::iterator it = m_mapPendingReq.begin(); it != m_mapPendingReq.end(); ++it )
	{	if( *it->second == *pKey )	/// request already pending (by user request?)
		{	LOG("WARNING RequestContractList polling ContractsAndRoutes: request already pending, just wait");
			delete pKey;
			return;
		}
	}
	LOG("RequestContractList polling ContractsAndRoutes");
	/// EXEC SMO.getContractsAndRoutes
	EXEC_REQ_SRVC stExecReq = { m_unSrcOID:m_ushInterfaceOID, m_unDstOID:SM_SMO_OBJ_ID, m_ucReqID:0, m_ucMethID: SMO_METHOD_GET_CONTRACTS_AND_ROUTES, m_unLen:0 };
	/// Session ID 0 is registered in tracker, but does not generate SAP_OUT (@see clientServerConfirm )
	uint16 ushAppHandle = g_stApp.m_oGwUAP.SendRequestToASL( &stExecReq, SRVC_EXEC_REQ, UAP_SMAP_ID, g_stApp.m_oGwUAP.SMContractID(), 0, 0, CLIENT_SERVER);

	if( !ushAppHandle )
	{	/// Cannot send to ASL, error already logged. Do not attempt to remove from tracker, it's not there
		delete pKey;
		return;	/// Processed
	}
	LOG("CLIENTSERVER(H %u S %u T %u L %u) %s:%u %s %s => ask DEVICE", ushAppHandle, 0, 0, 0, "SM", 1, pKey->Identify(), "UNBUFF REQ  ");

	m_mapPendingReq.insert( std::make_pair(ushAppHandle, pKey) );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Dump object status to LOG
/// @remarks Called on USR2
////////////////////////////////////////////////////////////////////////////////
void CClientServerService::Dump( void )
{	unsigned char uchCounter = 0;

	LOG("%s: OID %u", Name(), m_ushInterfaceOID );
	LOG("  Pending C/S Requests: %u items", m_mapPendingReq.size() );
	for(TCSPending::const_iterator itKey = m_mapPendingReq.begin(); itKey != m_mapPendingReq.end(); ++itKey)
	{
		LOG("    AppHandle %5u %s", itKey->first, itKey->second->Identify());
	}
	time_t tNow = time( NULL );
	LOG("  C/S Cache: %u items", m_mapCache.size() );
	for(TCSMap::const_iterator it = m_mapCache.begin(); it != m_mapCache.end(); ++it)
	{	TClientServerConfirmData_W * pCnfrmW = (TClientServerConfirmData_W *) it->second->m_pData;
		LOG(" ---KEY: %s", it->first->Identify() );
		LOG("    DATA: LastRx %+4d DevTX %+d size %2u %s%s", it->second->m_tLastRx-tNow, pCnfrmW->m_unTxSeconds-tNow, it->second->m_ushDataLen,
			GET_HEX_LIMITED(it->second->m_pData, it->second->m_ushDataLen, g_stApp.m_stCfg.m_nMaxCSDataLog) );

		TClientServerReqData_Exec reqData = { m_ushObjectID:SM_SMO_OBJ_ID, m_ushMethodID:SMO_METHOD_GET_CONTRACTS_AND_ROUTES, m_ushReqLen:0 };
		CSKey oKey( g_stApp.m_oGwUAP.SMContractID(), 0, SRVC_TYPE_EXECUTE, (byte*)&reqData );
		
		if (g_stApp.m_stCfg.AppLogAllowINF() && (*it->first == oKey) )
		{	const CClientServerService::TClientServerConfirmData_RX * pCnfrmR = (const CClientServerService::TClientServerConfirmData_RX *) it->second->m_pData;
			const SAPStruct::ContractsAndRoutes * pContractsAndRoutes = (const SAPStruct::ContractsAndRoutes *)pCnfrmR->m_aRspData;
			if( !pContractsAndRoutes->VALID( pCnfrmR->m_ushRspLen ) )
			{
				LOG("ERROR: invalid ContractsAndRoutes data");
				continue;
			}
			const SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes * pD = &pContractsAndRoutes->deviceList[0];

			unsigned unTotalContracts = 0, unPeriodicContracts2GW = 0;
			for(int i = 0; i < pContractsAndRoutes->numberOfDevices; ++i)
			{	unTotalContracts += pD->numberOfContracts;
				for(int j=0; j < pD->numberOfContracts; ++j)
				{	int k = 0;
					/*LOG("DEBUG UpdatePublishRate ContractID %3u type %u %s:%u -> %s:%u Period %d",
					pD->contractTable[j].contractID, pD->contractTable[j].serviceType,
					GetHex(pD->networkAddress, 16), pD->contractTable[j].sourceSAP,
					GetHex(pD->contractTable[j].destinationAddress, 16), pD->contractTable[j].destinationSAP,
					pD->contractTable[j].period);*/

					if(	(pD->contractTable[j].serviceType == 0)	/// periodic
					&&	!memcmp( pD->contractTable[j].destinationAddress, g_stApp.m_stCfg.getGW_IPv6(), 16 ) /// with GW
					&&	(pD->contractTable[j].destinationSAP == ISA100_GW_UAP)	/// on the right TSAPID
					/// in theory it must be active, not expired, TODO check for expiry/activate also
					)
					{	++unPeriodicContracts2GW;
						char szDeadline[32] = {0};	// deadline is expresed in 10 ms slots, convert to milisec then to string
						sprintf( szDeadline, "D %3d.%03d sec", pD->contractTable[j].deadline * 10 / 1000, pD->contractTable[j].deadline * 10 % 1000 );

						LOG("\tContractID %3u %s:%u -> GW:UAP period %3d phase %3u %s", pD->contractTable[j].contractID,
							GetHex(pD->networkAddress, 16), pD->contractTable[j].sourceSAP,
							pD->contractTable[j].period, pD->contractTable[j].phase, szDeadline);

						if( ++k > 1 )
							LOG("WARNING UpdatePublishRate(%s): Multiple publish contracts", GetHex(pD->networkAddress,16));
					}
				}
				pD = pD->NEXT();
			}
			LOG("\tSMO_METHOD_GET_CONTRACTS_AND_ROUTES: total %u contracts (%u bytes), %u periodic to GW", unTotalContracts,
				unTotalContracts*sizeof(SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes::Contract), unPeriodicContracts2GW);
		}

		if ( ( ++uchCounter > 100 ) && !g_stApp.m_stCfg.AppLogAllowDBG() )
		{
			LOG("  WARN C/S Cache: too many items, give up logging. Set LOG_LEVEL_APP = 3 to list all");
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> User) send G_ClientServer_Confirm back to the user
/// @param p_nSessionID
/// @param p_unTransactionID
/// @param p_pMsgData
/// @param p_dwMsgDataLen
/// @retval true always -just a convenience for the callers to send confirm and return in the same statement
/// @remarks Calls CGSAP::SendConfirm, which erase from tracker too
/// It is being called on User Request
////////////////////////////////////////////////////////////////////////////////
void CClientServerService::clientServerConfirm(	unsigned p_nSessionID, unsigned p_unTransactionID, uint8* p_pMsgData, uint32 p_dwMsgDataLen, bool p_bRemoveFromTracker /*= true*/  )
{
	g_stApp.m_oGwUAP.SendConfirm( p_nSessionID, p_unTransactionID, CLIENT_SERVER, p_pMsgData, p_dwMsgDataLen, p_bRemoveFromTracker );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> User) send G_ClientServer_Confirm back to the user
/// @param p_nSessionID
/// @param p_unTransactionID
/// @param p_ucStatus
/// @retval true always -just a convenience for the callers to send confirm and return in the same statement
/// @remarks Calls CGSAP::SendConfirm, which erase from tracker too
////////////////////////////////////////////////////////////////////////////////
void CClientServerService::clientServerConfirm( unsigned p_nSessionID, unsigned p_unTransactionID, byte p_ucStatus, bool p_bRemoveFromTracker /*= true*/  )
{
	g_stApp.m_oGwUAP.SendConfirm( p_nSessionID, p_unTransactionID, CLIENT_SERVER, &p_ucStatus, sizeof(p_ucStatus), p_bRemoveFromTracker );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> User) send G_ClientServer_Confirm back to the user
/// @param p_unAppHandle
/// @param p_pMsgData
/// @param p_dwMsgDataLen
/// @remarks Calls CGSAP::SendConfirm, which erase from tracker too
/// It is being called on APDU/Timeout
////////////////////////////////////////////////////////////////////////////////
void CClientServerService::clientServerConfirm(	uint16 p_unAppHandle, uint8* p_pMsgData, uint32 p_dwMsgDataLen )
{
	MSG * pMSG = g_stApp.m_oGwUAP.m_tracker.FindMessage( p_unAppHandle, "CClientServerService::clientServerConfirm" );
	if(!pMSG)
		return;	/// we've got nothin' else to do

	if( 0 != pMSG->m_nSessionID )	/// Normal flow for client requests (SAP_IN)
	{
		clientServerConfirm( pMSG->m_nSessionID, pMSG->m_TransactionID, p_pMsgData, p_dwMsgDataLen, true ); ///Remove from tracker
	}
	else	///< GW-generated message, we don't want SAP_OUT for (inexistent) original request
	{		/// Just need to remove from tracker
		g_stApp.m_oGwUAP.m_tracker.RemoveMessage( p_unAppHandle );	///TAKE CARE: pMSG(p_unAppHandle) is invalid now
	}
}
