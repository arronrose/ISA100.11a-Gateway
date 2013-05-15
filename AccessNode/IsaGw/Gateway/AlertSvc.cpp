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
/// @file AlertSvc.cpp
/// @author Marcel Ionescu
/// @brief Alert service - implementation
////////////////////////////////////////////////////////////////////////////////
#include "../../Shared/Common.h"
#include "../ISA100/dmap_armo.h"

#include "GwApp.h"
#include "AlertSvc.h"

#include <functional>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
/// @class CAlertService
/// @brief Alert service
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Check if this worker can handle service type received as parameter
/// @param p_ucServiceType - service type tested
/// @retval true - this service can handle p_ucServiceType
/// @retval false - the service cannot handle p_ucServiceType
/// @remarks The method is called from CInterfaceObjMgr::DispatchUserRequest
/// to decide where to dispatch a user request
////////////////////////////////////////////////////////////////////////////////
bool CAlertService::CanHandleServiceType( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case ALERT_SUBSCRIBE:
		//case ALERT_NOTIFY:
			return true;
	}
	return false;
};

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Process a user request
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @retval TBD
/// @remarks CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
////////////////////////////////////////////////////////////////////////////////
bool CAlertService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void  *p_pData )
{

	if ( p_pHdr->m_ucServiceType != ALERT_SUBSCRIBE)
	{
		/// PROGRAMMER ERROR: We should never get to default
		LOG("ERROR CAlertService::ProcessUserRequest(S %u T %u): unknown/unimplemented service type %u",
			p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_ucServiceType );
	}

	TAlertSubscriptionReqInfo *pAlertSubscReq = (TAlertSubscriptionReqInfo *)p_pData;

	if (p_pHdr->m_nDataSize - 4  < sizeof(TAlertSubscriptionReqInfo))
	{
		LOG("ERROR CAlertService::ProcessUserRequest(S %u T %u): data size to small",
			p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_nDataSize );
		return false;
	}
	pAlertSubscReq->NTOH();
	
	lease* pLease = g_pLeaseMgr->FindLease( pAlertSubscReq->m_unLeaseID, p_pHdr->m_nSessionID );
	if( !pLease || (LEASE_ALERT != pLease->eLeaseType) /*|| (!pLease->nContractId)*/)
	{	char szExtra[64] = {0};
		if( pLease ) sprintf(szExtra, " Contract %u", pLease->nContractId);
		LOG("ERROR CAlertService::ProcessUserRequest(S %u T %u) invalid/expired lease id %u%s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pAlertSubscReq->m_unLeaseID, szExtra);
		return confirm( p_pHdr, YGS_INVALID_LEASE, ALERT_FAIL_OTHER, NULL, 0);
	}

	uint8_t* pCrt = (uint8_t*) (pAlertSubscReq + 1); 
	int nLeftLen = p_pHdr->m_nDataSize - (pCrt - (uint8_t*)p_pData) - 4;

	for(int i = 0;; ++i)
	{
		TSubscCateg *pSubscCateg = (TSubscCateg *)pCrt;

		// sizeof(TSubscCateg) <= sizeof(TSubscNetAddr)
		if (	(nLeftLen < (int)sizeof(TSubscCateg)) 
			||	(	(pSubscCateg->m_ucSubscType == SubscriptionTypeAddress)
				&&	(	( nLeftLen < (int)sizeof(TSubscNetAddr))	// !p_pHdr->IsYGSAP()
					||	( p_pHdr->IsYGSAP() && (nLeftLen < (int)sizeof(YGSAP_IN::YSubscNetAddr)))
					)
				)
		   )
		{	// will log error about misaligned later
			break;
		}

		if (pSubscCateg->m_ucSubscType == SubscriptionTypeCategory)
		{
			LOG("ALERT_Request(S %u T %u L %u): %s/%s category=%d|%s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,
				pAlertSubscReq->m_unLeaseID, pSubscCateg->m_ucSubscribe ? "  SUBSCRIBE" : "UNSUBSCRIBE",
				pSubscCateg->m_ucEnable ? "enable " : "disable", pSubscCateg->m_ucCategory, GetAlertCategoryName(pSubscCateg->m_ucCategory));

			if (pSubscCateg->m_ucCategory >= ALERT_CAT_NO)
			{
				LOG("ERROR CAlertService::ProcessUserRequest(S %u T %u): invalid category %d ",
					p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pSubscCateg->m_ucCategory);
				return confirm( p_pHdr, YGS_INVALID_CATEGORY, ALERT_INVALID_CATEGORY, pCrt, sizeof(TSubscCateg));
			}
			// unsubscribe or subscribe: remove first  
			m_lstSubscCateg.remove(std::make_pair < uint8, uint32 >(pSubscCateg->m_ucCategory, pAlertSubscReq->m_unLeaseID)); 
			if (pSubscCateg->m_ucSubscribe/* ==1 */)
			{	
				m_lstSubscCateg.push_back(std::make_pair < uint8, uint32 >(pSubscCateg->m_ucCategory, pAlertSubscReq->m_unLeaseID));
			}
			pCrt     += sizeof(TSubscCateg);
			nLeftLen -= sizeof(TSubscCateg);
		}
		else if (pSubscCateg->m_ucSubscType == SubscriptionTypeAddress)
		{
			TSubscNetAddr *pSubscAddr = translateYAlertReq( p_pHdr, pCrt );
			
			if( !pSubscAddr )
			{
				LOG("ERROR CAlertService::ProcessUserRequest(S %u T %u): invalid alert nr %d at offset %u ",
					p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, i, pCrt - (uint8_t*) (pAlertSubscReq + 1));
				LOG_HEX("ERROR CAlertService::ProcessUserRequest: ",  (uint8_t*)p_pData, p_pHdr->m_nDataSize);
				
				return confirm( p_pHdr, YGS_INVALID_ALERT, ALERT_INVALID_ALERT, pCrt, _Max(sizeof(TSubscNetAddr), sizeof(YGSAP_IN::YSubscNetAddr)) );
			}

			pSubscAddr->NTOH();

			LOG("ALERT_Request(S %u T %u L %u): %s/%s addr %s:%u:%-2u type %d ", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,
				pAlertSubscReq->m_unLeaseID, pSubscCateg->m_ucSubscribe ? "  SUBSCRIBE" : "UNSUBSCRIBE",
				pSubscCateg->m_ucEnable ? "enable " : "disable", GetHex (pSubscAddr->m_aucNetAddress, sizeof(pSubscAddr->m_aucNetAddress)),
				pSubscAddr->m_ushEndpointPort, pSubscAddr->m_ushObjId, pSubscAddr->m_u8AlertType);

			TNetAddrId stNetAddrId;
			stNetAddrId.m_unLeaseID			= pAlertSubscReq->m_unLeaseID;
			stNetAddrId.m_u8Type			= pSubscAddr->m_u8AlertType;
			stNetAddrId.m_ushEndpointObjId	= pSubscAddr->m_ushObjId;
			stNetAddrId.m_ushEndpointPort	= pSubscAddr->m_ushEndpointPort;
			memcpy (stNetAddrId.m_aucNetAddress, pSubscAddr->m_aucNetAddress, sizeof(pSubscAddr->m_aucNetAddress));

			// unsubscribe or subscribe: remove first  
			m_lstSubscNetAddr.remove(stNetAddrId);

			if (pSubscAddr->m_ucSubscribe)
			{
				m_lstSubscNetAddr.push_back(stNetAddrId);
			}

			size_t unJump = p_pHdr->IsYGSAP() ? sizeof(YGSAP_IN::YSubscNetAddr) : sizeof(TSubscNetAddr);
			pCrt     += unJump;
			nLeftLen -= unJump;
		}
		else
		{	/// cannot include a subscription; no known type-> no known size
			LOG("ERROR CAlertService::ProcessUserRequest(S %u T %u): unknown Subscription type %d hdr_len %d",
				p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pSubscCateg->m_ucSubscType, p_pHdr->m_nDataSize);
			LOG_HEX("ERROR CAlertService::ProcessUserRequest: ",  (uint8_t*)p_pData, p_pHdr->m_nDataSize);
			
			return confirm( p_pHdr,  YGS_INVALID_ALERT, ALERT_FAIL_OTHER, NULL, 0);
		}
		
	}
	
	if (nLeftLen > 0)
	{
		LOG("ERROR CAlertService::ProcessUserRequest(S %u T %u): hdr_len=%d last subscription is partial",
			p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_nDataSize);
		LOG_HEX("ERROR CAlertService::ProcessUserRequest: ", (uint8_t*)p_pData, p_pHdr->m_nDataSize);

		return confirm( p_pHdr, YGS_INVALID_ALERT, ALERT_FAIL_OTHER, NULL, 0);
	}
	return confirm( p_pHdr, YGS_SUCCESS, ALERT_SUCCESS, NULL, 0);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Translate YGSAP subscription type address request into normal GSAP subscription type address
/// @param p_pHdr	The service request header
/// @param p_pData
/// @param p_pDeviceAddr translated address (or addresses in case of DEVICE_HEALTH_REPORT) are stored here.
///						User must make sure to have enough space allocated
/// @return pointer to valid req details: either translated req, or original req if translation is not necessary
/// @retval NULL: translation fail, caller must return error to user
////////////////////////////////////////////////////////////////////////////////
TSubscNetAddr * CAlertService::translateYAlertReq( CGSAP::TGSAPHdr * p_pHdr, uint8_t * p_pData  ) const
{
	static TSubscNetAddr oAddrReq;
	if( !p_pHdr->IsYGSAP() )
		return (TSubscNetAddr * )p_pData;
	
	YGSAP_IN::YSubscNetAddr * pSubscribeReq = (YGSAP_IN::YSubscNetAddr *)p_pData;
	YGSAP_IN::YDeviceSpecifier* p = &pSubscribeReq->m_oDevice;
	uint8_t * pDeviceAddr = NULL;
	
	// status not verified
	p->ExtractDeviceAddress(&pDeviceAddr);

	if( !pDeviceAddr )
	{
		return NULL;
	}
	oAddrReq.m_ucSubscType     = pSubscribeReq->m_ucSubscType;
	oAddrReq.m_ucSubscribe     = pSubscribeReq->m_ucSubscribe;
	oAddrReq.m_ucEnable        = pSubscribeReq->m_ucEnable;
	memcpy(oAddrReq.m_aucNetAddress, pDeviceAddr, 16);
	oAddrReq.m_ushEndpointPort = pSubscribeReq->m_ushEndpointPort;
	oAddrReq.m_ushObjId        = pSubscribeReq->m_ushObjId;
	oAddrReq.m_u8AlertType     = pSubscribeReq->m_u8AlertType;

	return &oAddrReq;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Dump alert details
/// @param p_pAlert	The alert
/// @return nothing
/// @remarks currently logged: SM application alerts join-type and UDO-type
////////////////////////////////////////////////////////////////////////////////
void CAlertService::dumpSMAlertDetails( ALERT_REP_SRVC* pAlert )
{
	/// category 3 == application TODO have #defines for categories
	if(pAlert->m_stAlertInfo.m_ucCategory == 3 )									/// application alerts
	{
		uint8_t * pValue =  (uint8_t*) pAlert->m_pAlertValue;
		
		if(	(pAlert->m_stAlertInfo.m_unDetObjTLPort == 0xF0B1)	/// SMAP
		&&	(pAlert->m_stAlertInfo.m_unDetObjID == SM_SMO_OBJ_ID) )
		{	/// Join-type alerts & topology alerts
			switch( pAlert->m_stAlertInfo.m_ucType )
			{
				case 0:
					{
						const SAPStruct::DeviceListRsp* pDev = (const SAPStruct::DeviceListRsp*) pValue;
						const SAPStruct::DeviceListRsp::Device * pD = (const SAPStruct::DeviceListRsp::Device*) &pDev->deviceList[0];
						if(!pDev->VALID( pAlert->m_stAlertInfo.m_unSize ))
						{
							LOG("WARNING ALERT: DEVICE Join %u devices - cannot parse", ntohs(pDev->numberOfDevices));
							break;
						}
						for( int i = 0; i < ntohs(pDev->numberOfDevices); ++i )
						{
							LOG("ALERT: DEVICE Join OK   %s IPv6 %s", GetHex(pD->uniqueDeviceID, 8), GetHex(pD->networkAddress, 16));
							pD = pD->NEXT();
						}
					}
					break;
				case 1:
					while( (pValue - pAlert->m_pAlertValue + sizeof(SAPStruct::TValAlertJoinFail)) <= pAlert->m_stAlertInfo.m_unSize )
					{	const SAPStruct::TValAlertJoinFail * p = (const SAPStruct::TValAlertJoinFail *)pValue;
						LOG("ALERT: DEVICE Join Fail %s Phase %u|%s Reason %u|%s", GetHex(p->eui64, 8), p->u8Phase,
							getJoinStatusText(p->u8Phase), p->u8Reason, getJoinFailAlertReasonText(p->u8Reason) );
						pValue += sizeof(SAPStruct::TValAlertJoinFail);
					}
					break;
				case 2:
					while( (pValue - pAlert->m_pAlertValue + sizeof(SAPStruct::TValAlertJoinLeave)) <= pAlert->m_stAlertInfo.m_unSize )
					{	const SAPStruct::TValAlertJoinLeave * p = (const SAPStruct::TValAlertJoinLeave *)pValue;
						LOG("ALERT: DEVICE Leave     %s Reason %u:%s", GetHex(p->eui64, 8), p->u8Reason, getJoinLeaveAlertReasonText(p->u8Reason) );
						pValue += sizeof(SAPStruct::TValAlertJoinLeave);
					}
					break;
				/// topology alerts
				case 4:
				case 5:
					LOG("WARNING ALERT: TOPOLOGY not implemented");
					break;
				default:
					LOG("WARNING ALERT: DEVICE unknown");
			}
		}
		else if(	(pAlert->m_stAlertInfo.m_unDetObjTLPort == 0xF0B1)	/// SMAP
		&&			(pAlert->m_stAlertInfo.m_unDetObjID == SM_UDO_OBJ_ID) )
		{	/// UDO-type alerts
			switch( pAlert->m_stAlertInfo.m_ucType )
			{
				case 0:
					while( (pValue - pAlert->m_pAlertValue + sizeof(SAPStruct::TValAlertUdoStart)) <= pAlert->m_stAlertInfo.m_unSize )
					{	const SAPStruct::TValAlertUdoStart * p = (const SAPStruct::TValAlertUdoStart *)pValue;
						LOG("ALERT: UDO Transfer start    %s Bytes per packet %u Total packets %u total bytes %u", GetHex(p->eui64, 8),
							ntohs(p->u16BytesPerPacket), ntohs(p->u16totalPackets), ntohl(p->u32totalBytes) );
						pValue += sizeof(SAPStruct::TValAlertUdoStart);
					}
					break;
				case 1:
					while( (pValue - pAlert->m_pAlertValue + sizeof(SAPStruct::TValAlertUdoTransfer)) <= pAlert->m_stAlertInfo.m_unSize )
					{	const SAPStruct::TValAlertUdoTransfer * p = (const SAPStruct::TValAlertUdoTransfer *)pValue;
						LOG("ALERT: UDO Transfer progress %s Packets %3u", GetHex(p->eui64, 8), ntohs(p->u16PacketsTransferred) );
						pValue += sizeof(SAPStruct::TValAlertUdoTransfer);
					}
					break;
				case 2:
					while( (pValue - pAlert->m_pAlertValue + sizeof(SAPStruct::TValAlertUdoEnd )) <= pAlert->m_stAlertInfo.m_unSize )
					{	const SAPStruct::TValAlertUdoEnd * p = (const SAPStruct::TValAlertUdoEnd *)pValue;
						LOG("ALERT: UDO Transfer end      %s errcode %u|%s", GetHex(p->eui64, 8), p->u8ErrCode, getUDOTransferEndAlertReasonText(p->u8ErrCode) );
						pValue += sizeof(SAPStruct::TValAlertUdoEnd);
					}
					break;
				default:
					LOG("WARNING ALERT: UDO unknown");
			}
		}
		else if(	(pAlert->m_stAlertInfo.m_unDetObjTLPort == 0xF0B1)	/// SMAP
		&&			(pAlert->m_stAlertInfo.m_unDetObjID == SM_SCO_OBJ_ID) )
		{	/// contract alerts
			LOG("WARNING ALERT: CONTRACT not implemented");
		}
		/// else the alert is unknown
		else
		{
			LOG("WARNING ALERT: from SM category:application type:unknown");
		}
	}
}

bool CAlertService::ProcessAlertAPDU( APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pReq )
{
	if( SRVC_ALERT_REP != p_pReq->m_ucType )
	{
		LOG("ERROR CAlertService::ProcessAlertAPDU: invalid ADPU type %X from %s:%u", p_pReq->m_ucType,
			GetHex( p_pAPDUIdtf->m_aucAddr,sizeof(p_pAPDUIdtf->m_aucAddr)), p_pAPDUIdtf->m_ucSrcTSAPID );
		return false;
	}

	ALERT_REP_SRVC* pAlert = &p_pReq->m_stSRVC.m_stAlertRep; //just for clarity
	
	LOG("ALERT from %s:%d:%u -> %d:%-2u id %3d DETect %d:%-2u categ %u|%s type %u",
		GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)), p_pAPDUIdtf->m_ucSrcTSAPID, pAlert->m_unSrcOID,
		p_pAPDUIdtf->m_ucDstTSAPID, pAlert->m_unDstOID, pAlert->m_stAlertInfo.m_ucID, pAlert->m_stAlertInfo.m_unDetObjTLPort-ISA100_START_PORTS,
		pAlert->m_stAlertInfo.m_unDetObjID, pAlert->m_stAlertInfo.m_ucCategory,
		GetAlertCategoryName(pAlert->m_stAlertInfo.m_ucCategory), pAlert->m_stAlertInfo.m_ucType);
	if(  g_stApp.m_stCfg.AppLogAllowDBG() )
	{
		LOG("ALERT_UTC %lu.%-2u NextSendTAI %u class %u dir %u prio %u size %u %s",
			pAlert->m_stAlertInfo.m_stDetectionTime.m_ulSeconds - (TAI_OFFSET + g_stDPO.m_nCurrentUTCAdjustment),
			(pAlert->m_stAlertInfo.m_stDetectionTime.m_unFract * 1000000) >> 16,
			pAlert->m_stAlertInfo.m_ulNextSendTAI, pAlert->m_stAlertInfo.m_ucClass,
			pAlert->m_stAlertInfo.m_ucDirection, pAlert->m_stAlertInfo.m_ucPriority,
			pAlert->m_stAlertInfo.m_unSize, GetHex(pAlert->m_pAlertValue, pAlert->m_stAlertInfo.m_unSize) );
	}
	if( !memcmp(p_pAPDUIdtf->m_aucAddr, g_stApp.m_stCfg.getSM_IPv6(), IPv6_ADDR_LENGTH))	/// from SM
	{
		dumpSMAlertDetails( pAlert );
	}

	TSubscCategLst::iterator itCat = m_lstSubscCateg.begin();
	for ( ;itCat != m_lstSubscCateg.end(); itCat++)
	{
		if (itCat->first != pAlert->m_stAlertInfo.m_ucCategory)
		{
			continue;
		}

		sendAlertNotification (p_pAPDUIdtf, pAlert, itCat->second);
	}

	TSubscNetAddrLst::iterator itNetAddr = m_lstSubscNetAddr.begin();
	for ( ;itNetAddr != m_lstSubscNetAddr.end(); itNetAddr++)
	{
		//if not equal continue
		if	(	memcmp( itNetAddr->m_aucNetAddress, p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)) != 0
				|| itNetAddr->m_ushEndpointObjId	!=	pAlert->m_stAlertInfo.m_unDetObjID
				|| itNetAddr->m_ushEndpointPort		!=	pAlert->m_stAlertInfo.m_unDetObjTLPort
				|| itNetAddr->m_u8Type				!=	pAlert->m_stAlertInfo.m_ucType
			)
		{
			continue;
		}

		sendAlertNotification (p_pAPDUIdtf, pAlert, itNetAddr->m_unLeaseID);
	}

	//send alert ack

	ALERT_ACK_SRVC stAlertAck = {pAlert->m_unDstOID, pAlert->m_unSrcOID, pAlert->m_stAlertInfo.m_ucID };

	DMO_CONTRACT_ATTRIBUTE * pContract = DMO_GetContract (p_pAPDUIdtf->m_aucAddr, 
																ISA100_START_PORTS + p_pAPDUIdtf->m_ucSrcTSAPID, 
																ISA100_START_PORTS + p_pAPDUIdtf->m_ucDstTSAPID, SRVC_APERIODIC_COMM);

	if (pContract)
	{
		LOG("ALERT_ACK  %s:%d:%u <- %d:%-2u id %3d Contract %u", GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)),
			p_pAPDUIdtf->m_ucSrcTSAPID, pAlert->m_unSrcOID, p_pAPDUIdtf->m_ucDstTSAPID, pAlert->m_unDstOID, 
			pAlert->m_stAlertInfo.m_ucID, pContract->m_unContractID);

		/// TODO: ADD pPendingAlertAck->m_u8Priority as packet priority
		g_stApp.m_oGwUAP.SendRequestToASL( &stAlertAck, SRVC_ALERT_ACK, p_pAPDUIdtf->m_ucDstTSAPID, p_pAPDUIdtf->m_ucSrcTSAPID, pContract->m_unContractID, true);
		return true;
	}

	//no contract
	TPendingAlertAck::Ptr pPendingAlertAck (new TPendingAlertAck);
	
	memcpy ( &pPendingAlertAck->m_stAlertAckSrvc, &stAlertAck, sizeof(stAlertAck));
	pPendingAlertAck->m_u16PortDst = ISA100_START_PORTS + p_pAPDUIdtf->m_ucSrcTSAPID;
	pPendingAlertAck->m_u16PortSrc = ISA100_START_PORTS + p_pAPDUIdtf->m_ucDstTSAPID;
	memcpy( pPendingAlertAck->m_u8Dest, p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr));

	pPendingAlertAck->m_nEndOfLife = time(NULL) + PENDING_ALERT_ACK_TIMEOUT;


	LOG( "Alert ACK REQUEST NEW contract for %s:%d:%-2u <- %d", GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)),
		pPendingAlertAck->m_u16PortDst-(uint16_t)ISA100_START_PORTS, pAlert->m_unSrcOID, pPendingAlertAck->m_u16PortSrc-(uint16_t)ISA100_START_PORTS);

	DMO_CONTRACT_BANDWIDTH stBandwidth;
	stBandwidth.m_stAperiodic.m_nComittedBurst	= -15;
	stBandwidth.m_stAperiodic.m_nExcessBurst	= 1;
	stBandwidth.m_stAperiodic.m_ucMaxSendWindow	= 5;

	if (DMO_RequestNewContract ( pPendingAlertAck->m_u8Dest, pPendingAlertAck->m_u16PortDst, pPendingAlertAck->m_u16PortSrc,  0xFFFFFFFF,
				SRVC_APERIODIC_COMM, 3, ( !memcmp(pPendingAlertAck->m_u8Dest, g_stApp.m_stCfg.getSM_IPv6(), IPv6_ADDR_LENGTH) ) ? 1252 : 80,
				0, &stBandwidth ) == SFC_SUCCESS)
	{	///TODO: determine which alerts have contract priority 3-network control, and which have 0-best effort queued
		m_oPendingAlertAckList.push_back(pPendingAlertAck);
	}

	
	return true;
}

void CAlertService::OnLeaseDelete( lease* p_pLease )	
{ 
	TSubscCategLst::iterator itNextCat = m_lstSubscCateg.begin();
	for (;itNextCat != m_lstSubscCateg.end(); )
	{
		if (itNextCat->second != p_pLease->nLeaseId)
		{
			++itNextCat;
			continue;
		}
		LOG("ALERT OnLeaseDelete(%lu) cat=%d|%s",p_pLease->nLeaseId, itNextCat->first, GetAlertCategoryName(itNextCat->first));
		itNextCat = m_lstSubscCateg.erase(itNextCat);
	}

	TSubscNetAddrLst::iterator itNextAddr = m_lstSubscNetAddr.begin();
	for (;itNextAddr != m_lstSubscNetAddr.end(); )
	{
		if (itNextAddr->m_unLeaseID != p_pLease->nLeaseId)
		{
			++itNextAddr;
			continue;
		}

		LOG("ALERT OnLeaseDelete(%lu) %s:%u:%-2u type %d", p_pLease->nLeaseId,
					GetHex(itNextAddr->m_aucNetAddress, sizeof(itNextAddr->m_aucNetAddress)),
					itNextAddr->m_ushEndpointPort, itNextAddr->m_ushEndpointObjId, itNextAddr->m_u8Type );

		itNextAddr = m_lstSubscNetAddr.erase(itNextAddr);
	}		
}

///     p_pContract != NULL && p_nContractId != 0 contract created
///     p_pContract != NULL && p_nContractId == 0 contract creation failed
///     p_pContract == NULL && p_nContractId 1= 0 contract deleted
///     p_pContract == NULL && p_nContractId == 0 INVALID combination
void CAlertService::ContractNotification (uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pContract)
{
	if (!p_pContract || !p_nContractId)
	{	return;
	}

	CPendingAlertAckList::iterator itNext = m_oPendingAlertAckList.begin();

	for (;itNext != m_oPendingAlertAckList.end(); )
	{
		CPendingAlertAckList::iterator itCrt = itNext++;
		TPendingAlertAck::Ptr pPendingAlertAck = *itCrt;

		if (		pPendingAlertAck->m_u16PortDst != p_pContract->m_unDstTLPort
				||	pPendingAlertAck->m_u16PortSrc != p_pContract->m_unSrcTLPort
				||	SRVC_APERIODIC_COMM != p_pContract->m_ucServiceType
				||	memcmp(pPendingAlertAck->m_u8Dest, p_pContract->m_aDstAddr128, sizeof(p_pContract->m_aDstAddr128))
			)
		{
			continue;
		}

		LOG("CAlertService::ContractNotification: use NEW Contract %u for AlertId %d", p_nContractId, (*itCrt)->m_stAlertAckSrvc.m_ucAlertID );
		LOG("ALERT_ACK  %s:%d:%u <- %d:%-2u id %3d Contract %u", GetHex((*itCrt)->m_u8Dest, sizeof((*itCrt)->m_u8Dest)),
			(*itCrt)->m_u16PortDst-(uint16)ISA100_START_PORTS, (*itCrt)->m_stAlertAckSrvc.m_unDstOID,
			(*itCrt)->m_u16PortSrc-(uint16)ISA100_START_PORTS, (*itCrt)->m_stAlertAckSrvc.m_unSrcOID,
			(*itCrt)->m_stAlertAckSrvc.m_ucAlertID, p_pContract->m_unContractID);
		//send
		///TODO: ADD pPendingAlertAck->m_u8Priority as packet priority
		g_stApp.m_oGwUAP.SendRequestToASL( &pPendingAlertAck->m_stAlertAckSrvc, SRVC_ALERT_ACK,
			pPendingAlertAck->m_u16PortSrc-(uint16)ISA100_START_PORTS,
			pPendingAlertAck->m_u16PortDst-(uint16)ISA100_START_PORTS, p_pContract->m_unContractID, true);

		//erase message
		m_oPendingAlertAckList.erase(itCrt);
	}
}

//find lease and send 
bool CAlertService::sendAlertNotification (APDU_IDTF* p_pAPDUIdtf, ALERT_REP_SRVC* p_pAlert, uint32 p_u32LeaseId )
{
	lease* pLease = g_pLeaseMgr->FindLease(p_u32LeaseId);

	if (!pLease)
	{
		LOG("WARNING CAlertService::sendAlertNotification: lease %d not found, ?expired ", p_u32LeaseId);
		return false;
	}

	std::auto_ptr<uint8_t> pAlertData(new uint8_t[sizeof(TAlertNotification) + p_pAlert->m_stAlertInfo.m_unSize]);
	TAlertNotification* pAlertNotif = (TAlertNotification*)pAlertData.get();

	memcpy( pAlertNotif->m_aucNetAddress, p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr));
	pAlertNotif->m_ushEndpointPort = p_pAlert->m_stAlertInfo.m_unDetObjTLPort;
	pAlertNotif->m_ushEndpointObjId = p_pAlert->m_stAlertInfo.m_unDetObjID;

	pAlertNotif->m_u32Seconds = p_pAlert->m_stAlertInfo.m_stDetectionTime.m_ulSeconds;
	pAlertNotif->m_u16Fract = p_pAlert->m_stAlertInfo.m_stDetectionTime.m_unFract;
	

	pAlertNotif->m_ucCategory	= p_pAlert->m_stAlertInfo.m_ucCategory;
	pAlertNotif->m_ucClass		= p_pAlert->m_stAlertInfo.m_ucClass;
	pAlertNotif->m_ucDirection	= p_pAlert->m_stAlertInfo.m_ucDirection;
	pAlertNotif->m_u8Type		= p_pAlert->m_stAlertInfo.m_ucType;
	
	pAlertNotif->m_u8Priority	= p_pAlert->m_stAlertInfo.m_ucPriority;
	pAlertNotif->m_ushDataLen	=  p_pAlert->m_stAlertInfo.m_unSize;

	pAlertNotif->HTON();
	memcpy(pAlertNotif+1, p_pAlert->m_pAlertValue, p_pAlert->m_stAlertInfo.m_unSize);

	return SendIndication(ALERT_NOTIFY, pLease, pAlertData.get(), sizeof(TAlertNotification) + p_pAlert->m_stAlertInfo.m_unSize);
}


void CAlertService::DropExpired ()
{
	time_t nNow = time(NULL);

	for (;;)
	{
		CPendingAlertAckList::iterator it = m_oPendingAlertAckList.begin();
		if (it == m_oPendingAlertAckList.end())
		{
			break;
		}

		//list is order asc by time
		if ((*it)->m_nEndOfLife > nNow)
		{
			break;
		}

		LOG("WARNING CAlertService::DropExpired: %s:%d -> %d alertId=%d",
			GetHex((*it)->m_u8Dest, sizeof((*it)->m_u8Dest)), (*it)->m_u16PortSrc-(uint16)ISA100_START_PORTS,
			(*it)->m_u16PortDst-(uint16)ISA100_START_PORTS, (*it)->m_stAlertAckSrvc.m_ucAlertID );

		m_oPendingAlertAckList.erase(it);
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send confirm on connection GSAP, determined from session id, if connection is valid
/// @param p_pHdr - to get p_nSessionID, p_TransactionID, IsYGSAP()
/// @retval true - confirm was sent to client
/// @retval false - confirm was not sent to client
/// @remarks 
////////////////////////////////////////////////////////////////////////////////
bool CAlertService::confirm( CGSAP::TGSAPHdr * p_pHdr, uint8_t p_unYGSAPStatus, uint8_t p_unGSAPStatus, uint8_t * p_pConfirmData, size_t p_unConfirmDataSize )
{	uint8 tmp[ sizeof(uint8_t) + p_unConfirmDataSize ];
	tmp[ 0 ] = p_pHdr->IsYGSAP() ? p_unYGSAPStatus : p_unGSAPStatus;	///< GS_Status on first byte
	if( p_unConfirmDataSize ) memcpy( tmp+1, p_pConfirmData, p_unConfirmDataSize );	// grr, data copy, should be avoided
	return g_stApp.m_oGwUAP.SendConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, ALERT_SUBSCRIBE, tmp, sizeof(tmp), false );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Dump object status to LOG
/// @remarks Called on USR2
////////////////////////////////////////////////////////////////////////////////
void CAlertService::Dump( void )
{
	LOG("%s: OID %u. Subscriptions: Category %u/Addr %u. Alerts pending ACK %u", Name(),
		m_ushInterfaceOID, m_lstSubscCateg.size(), m_lstSubscNetAddr.size(), m_oPendingAlertAckList.size() );

	for( TSubscCategLst::iterator it = m_lstSubscCateg.begin(); it != m_lstSubscCateg.end(); ++it)
	{
		LOG( "\tCategory %u|%s -> LeaseID %u", it->first, GetAlertCategoryName(it->first), it->second);
	}
	
	for( TSubscNetAddrLst::iterator it = m_lstSubscNetAddr.begin(); it != m_lstSubscNetAddr.end(); ++it)
	{
		LOG( "\tAddr %s:%u:%u Type %u -> LeaseID %u", GetHex( it->m_aucNetAddress, 16), it->m_ushEndpointPort, it->m_ushEndpointObjId, it->m_u8Type, it->m_unLeaseID);
	}
	
	for( CPendingAlertAckList::iterator it = m_oPendingAlertAckList.begin(); it != m_oPendingAlertAckList.end(); ++it)
	{
		LOG( "\tPending %s EOL %u", GetHex( (*it)->m_u8Dest, 16), (*it)->m_nEndOfLife);
	}
}

/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
bool CAlertService::ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* /*p_pAPDUIdtf*/, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	
	return false;//tmp
}

/// (ISA -> GW) Process a ISA100 timeout. Call from CInterfaceObjMgr::DispatchISATimeout
bool CAlertService::ProcessISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq )
{
	return false;//tmp
}

