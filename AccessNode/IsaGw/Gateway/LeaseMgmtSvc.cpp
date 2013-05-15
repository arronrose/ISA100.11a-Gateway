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
/// @file LeaseMgmtSvc.cpp
/// @author
/// @brief Lease Management Service - implementation
////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include "../ISA100/porting.h"	// NLME_FindContract*
#include "LeaseMgmtSvc.h"
#include "GwApp.h"
#include "TunnelObject.h"
#include "GwUtil.h"
////////////////////////////////////////////////////////////////////////////////
/// @class CLeaseMgmtService
/// @brief Lease Management services
////////////////////////////////////////////////////////////////////////////////
CLeaseMgmtService::CLeaseMgmtService( const char * p_szName )
:CService(p_szName), m_nLeaseIndTop(1)
{
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief (DMAP -> GW) Notifies us of the asynchronous completion of contracts' establishment/deletion
/// @param p_nContractId - the contract id
/// @param p_pContract - pointer to contract attributes, if contract is created, or NULL if contract is erased
/// @remarks Two valid cases:
///		p_pContract != NULL : contract created.
///		Move leases from m_lstLeasesInProgress to m_lstActiveLeases/m_lstSubscriberLeases, send G_Lease_Confirm(success) to user
///
///		p_pContract == NULL : contract deleted.
///		Forcibly expire all leases associated with this contractId
///
///     p_pContract != NULL && p_nContractId == 0 contract creation failed
///     p_pContract == NULL && p_nContractId == 0 INVALID combination
///     Nothing to do
///
/// The timeout case (Erase leases from m_lstLeasesInProgress, send G_Lease_Confirm(fail) to user)
/// is not notified, so we must time-out the LeaseInProgress after a while
////////////////////////////////////////////////////////////////////////////////
void CLeaseMgmtService::ContractNotification(uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pContract)
{	LeaseList::iterator it;	
	if ( p_pContract && p_nContractId )	/// Contract created
	{
		for ( it = m_lstLeasesInProgress.begin(); it != m_lstLeasesInProgress.end(); )
		{	if(	( !(*it)->IsPeriodic() )														///< aperiodic LEASE only
			&&	!memcmp( (*it)->netAddr, p_pContract->m_aDstAddr128, sizeof( (*it)->netAddr))	///< adress match
			&&	( ISA100_START_PORTS + (*it)->TSAPID() == p_pContract->m_unDstTLPort )			///< device TSAPID match
			&&  ( ISA100_START_PORTS + ISA100_GW_UAP == p_pContract->m_unSrcTLPort )			///< Source (GW) SAPID match
			&&	( p_pContract->m_ucServiceType == SRVC_APERIODIC_COMM) )						///< aperiodic CONTRACTS only. This is always true
			{	/// My contract request
				if( p_nContractId )	/// Contract created
				{
					++p_pContract->m_nUsageCount;
					(*it)->nContractId = p_nContractId;
					(*it)->nLeaseId = GET_NEW_LEASE_ID;
					/// this is not necessary inconsistency. 
					LOG( "LEASE Request(S %u T %u) ==> L %u USE NEW contractID %u (used %u times)%s",
						(*it)->nSessionId, (*it)->nTransactionId, (*it)->nLeaseId, (*it)->nContractId, p_pContract->m_nUsageCount,
						(1 != p_pContract->m_nUsageCount) ? " WARNING inconsistency" : "");
				
					///TODO: we should use pContractAttrib->m_ulAssignedExpTime for lease duration
					/// LEASE_SUCCESS is the same as YGS_SUCCESS
					leaseConfirm( (*it)->nSessionId, (*it)->nTransactionId, LEASE_SUCCESS, (*it)->nLeaseId, (*it)->nExpirationTime ? (*it)->nExpirationTime - (*it)->nCreationTime : 0 );
					LeaseList::iterator itSplice = it++;	///< advance it here, (postadvance)
					m_lstActiveLeases.splice( m_lstActiveLeases.end(), m_lstLeasesInProgress, itSplice);
				}
				else	/// !p_nContractId: -> Contract creation failed: contract timeout
				{
					LOG( "LEASE Request(S %u T %u) ==> FAILED", (*it)->nSessionId, (*it)->nTransactionId );
					/// The device was reported in topology, MH requested contract but SM did not provide a contract back.
					/// We should retry: LEASE_FAIL_OTHER ensures retry at MH level, while LEASE_NO_DEVICE does not (MH will just expect device re-join)
					leaseConfirm((*it)->nSessionId, (*it)->nTransactionId, g_pSessionMgr->IsYGSAPSession((*it)->nSessionId) ? YGS_FAILURE: LEASE_FAIL_OTHER, (*it)->nLeaseId, (*it)->nExpirationTime ? (*it)->nExpirationTime - (*it)->nCreationTime : 0 );
					delete *it;
					it = m_lstLeasesInProgress.erase( it );	 ///< Advance it here
				}
			}
			else
			{
				++it;	///< advance it here
			}
		}
	}
	else if (!p_pContract)	/// Contract deleted : /// search in m_lstActiveLeases, DispatchLeaseDelete and erase the lease
	{	/// no need to search in subscriber leases
		for( it = m_lstActiveLeases.begin(); it != m_lstActiveLeases.end(); )
		{
			if ((*it)->nContractId == p_nContractId)
			{
				it = deleteActiveLease( it, false );	/// Do not req contract delete, it is already erased.
			}
			else
			{
				++it;
			}
		}
		for( it = m_lstLeasesPendingDelete.begin(); it != m_lstLeasesPendingDelete.end(); )
		{
			if ((*it)->nContractId == p_nContractId)
			{
				LOG( "LEASE Request(S %u T %u) Lease %lu C %u deleted OK", (*it)->nSessionId, (*it)->nTransactionId, (*it)->nLeaseId, p_nContractId );
				leaseConfirm( (*it)->nSessionId, (*it)->nTransactionId, YGS_SUCCESS /*: LEASE_SUCCESS*/, (*it)->nLeaseId, 0 );
				delete *it;
				it = m_lstLeasesPendingDelete.erase( it );
			}
			else
			{
				++it;
			}
		}
	}
	//else if ( !p_pContract && !p_nContractId ) invalid combination
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief On session update (renewal) - update expiration on associated leases, only if expiration get shorter
/// @param p_nSessionID The session id
/// @param p_nExpTime new expiration time (absolute value)
/// @remarks Call on session update to eventually update the associated leases
/// Update expiration on both active and pending leases
////////////////////////////////////////////////////////////////////////////////
void CLeaseMgmtService::ProcessSessionUpdate(unsigned p_nSessionID, time_t p_nExpTime)
{	LOG("CLeaseMgmtService::ProcessSessionUpdate in %u active / %u subscriber / %u pending leases",
		m_lstActiveLeases.size(), m_lstSubscriberLeases.size(), m_lstLeasesInProgress.size() );
	/// Update active leases
	for( LeaseList::iterator it = m_lstActiveLeases.begin(); it != m_lstActiveLeases.end(); ++it)
	{
		if ( (*it)->nSessionId == p_nSessionID && (*it)->nExpirationTime > p_nExpTime )
		{
			LOG("CLeaseMgmtService::ProcessSessionUpdate: active leaseID %lu exp %u -> %u", (*it)->nLeaseId, (*it)->nExpirationTime, p_nExpTime );
			(*it)->nExpirationTime = p_nExpTime;
		}
	}
	/// Update active subscriber leases
	for( LeaseList::iterator it = m_lstSubscriberLeases.begin(); it != m_lstSubscriberLeases.end(); ++it)
	{
		if ( (*it)->nSessionId == p_nSessionID && (*it)->nExpirationTime > p_nExpTime )
		{
			LOG("CLeaseMgmtService::ProcessSessionUpdate: subscriber leaseID %lu exp %u -> %u", (*it)->nLeaseId, (*it)->nExpirationTime, p_nExpTime );
			(*it)->nExpirationTime = p_nExpTime;
		}
	}
	/// Update pending leases
	for( LeaseList::iterator it = m_lstLeasesInProgress.begin(); it != m_lstLeasesInProgress.end(); ++it )
	{
		if ((*it)->nSessionId==p_nSessionID && (*it)->nExpirationTime > p_nExpTime)
		{
			LOG("CLeaseMgmtService::ProcessSessionUpdate: update pending %u -> %u", (*it)->nExpirationTime, p_nExpTime );
			(*it)->nExpirationTime = p_nExpTime;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Process session delete (expire or explicit delete) - deleting associated leases
/// @param p_nSessionID The session id
/// @remarks Call on session delete to erase the associated leases
////////////////////////////////////////////////////////////////////////////////
void CLeaseMgmtService::ProcessSessionDelete( unsigned p_nSessionID )
{	LOG("CLeaseMgmtService::ProcessSessionDelete(%u) search %u active / %u subscriber / %u pending leases",
		p_nSessionID, m_lstActiveLeases.size(), m_lstSubscriberLeases.size(), m_lstLeasesInProgress.size() );
	/// Delete from active leases list
	for( LeaseList::iterator it = m_lstActiveLeases.begin(); it != m_lstActiveLeases.end(); )
	{
		if( (*it)->nSessionId == p_nSessionID )
		{
			/// This will generate some errors on contract notification: lease delete confirm will fail
			/// but we need pending del active
			/// TODO maybe have a flag to be validated on ContractNotification which does not sent lease delete confirm when the flag is set
			bool bPendingContractDelete = false; /// needed to activate pending del
			it = deleteActiveLease( it, true, &bPendingContractDelete );/// DO erase contract
		}
		else
		{
			++it;
		}
	}
	/// Delete from active subscriber leases list
	for( LeaseList::iterator it = m_lstSubscriberLeases.begin(); it != m_lstSubscriberLeases.end(); )
	{
		if( (*it)->nSessionId == p_nSessionID )
		{
			it = deleteActiveSubscriberLease( it );
		}
		else
		{
			++it;
		}
	}
	/// Delete from pending leases list
	for( LeaseList::iterator it = m_lstLeasesInProgress.begin(); it!=m_lstLeasesInProgress.end(); )
	{
		if( (*it)->nSessionId==p_nSessionID )
		{
			LOG("CLeaseMgmtService::ProcessSessionDelete pending %s:%u OID %u",
				GetHex((*it)->netAddr, sizeof((*it)->netAddr) ), (*it)->m_ushTSAPID, (*it)->nObjId );
			delete *it;
			it = m_lstLeasesInProgress.erase( it );
		}
		else
		{
			++it;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Claudiu Hobeanu
/// @brief find lease that match the request id 
/// @param 
/// @return  lease* or NULL if not found
/// @remarks 
////////////////////////////////////////////////////////////////////////////////
lease *CLeaseMgmtService::FindLease( LeaseTypes p_eLeaseType, const uint8* p_pPeerIPv6, 
					   uint16 p_u16PeerTSapID, uint16 p_u16PeerObjectID, uint16 p_u16LocalTSapID, uint16 p_u16LocalObjectID )
{
	for( LeaseList::iterator it = m_lstActiveLeases.begin(); it != m_lstActiveLeases.end(); ++it )
	{
		lease *pLease = *it;

		if (		pLease->eLeaseType == p_eLeaseType 
				&&	memcmp(p_pPeerIPv6, pLease->netAddr, sizeof(pLease->netAddr)) == 0
				&&	p_u16PeerTSapID  == pLease->m_ushTSAPID		 &&	p_u16PeerObjectID  == pLease->nObjId
				&&	p_u16LocalTSapID == pLease->m_u16LocalTSAPID && p_u16LocalObjectID == pLease->m_u16LocalObjId			
			)
		{
			return pLease;
		}
	}
	for( LeaseList::iterator it = m_lstSubscriberLeases.begin(); it != m_lstSubscriberLeases.end(); ++it )
	{
		lease *pLease = *it;

		if (		pLease->eLeaseType == p_eLeaseType 
				&&	memcmp(p_pPeerIPv6, pLease->netAddr, sizeof(pLease->netAddr)) == 0
				&&	p_u16PeerTSapID  == pLease->m_ushTSAPID		 &&	p_u16PeerObjectID  == pLease->nObjId
				&&	p_u16LocalTSapID == pLease->m_u16LocalTSAPID && p_u16LocalObjectID == pLease->m_u16LocalObjId			
			)
		{
			return pLease;
		}
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Find a lease based on lease id
/// @param p_nLeaseId the lease ID to search for
/// @param p_nSessionID the lease must be allocated to this session id.
///  If 0, do not match session ID (search for lease ID only)
/// @retval lease* or NULL if not found or invalid session or lease/session mismatch
/// @remarks we rely on lease id being unique over all sessions and all lease types
////////////////////////////////////////////////////////////////////////////////
lease *CLeaseMgmtService::FindLease(unsigned p_nLeaseId, unsigned p_nSessionID /*=0*/)
{
	LeaseList::iterator it = findLease( p_nLeaseId, p_nSessionID);
	return ( it == (LeaseList::iterator)NULL ) ? NULL : (lease *)*it;
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Find a lease based on lease id
/// @param p_nLeaseId
/// @return iterator in LeaseList if found
/// @return NULL if lease ID not found
/// @remarks we rely on lease id being unique over all sessions and all lease types
////////////////////////////////////////////////////////////////////////////////
LeaseList::iterator CLeaseMgmtService::findLease(unsigned p_nLeaseId, unsigned p_nSessionID /*=0*/)
{
	for( LeaseList::iterator it = m_lstActiveLeases.begin(); it != m_lstActiveLeases.end(); ++it )
	{
		if ((*it)->nLeaseId == p_nLeaseId)
		{
			return ( p_nSessionID && (p_nSessionID != (*it)->nSessionId )) ? (LeaseList::iterator)NULL : it;
		}
	}
	for( LeaseList::iterator it = m_lstSubscriberLeases.begin(); it != m_lstSubscriberLeases.end(); ++it )
	{
		if ((*it)->nLeaseId == p_nLeaseId)
		{
			return ( p_nSessionID && (p_nSessionID != (*it)->nSessionId )) ? (LeaseList::iterator)NULL : it;
		}
	}
	return (LeaseList::iterator)NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Creates a new lease for a specific session and requests a new contract from DMAP if necessary
/// @param 
/// @return always true
/// @remarks May call leaseConfirm. If contractId exis its UsageCount is incremented.
////////////////////////////////////////////////////////////////////////////////
bool CLeaseMgmtService:: createLease( uint8_t p_ucVersion, unsigned p_nSessionID, unsigned p_unTransactionID, void * p_pReq, uint32_t p_u32ReqLen )
{
	time_t nSessExpTime;
	TLeaseMgmtReqData * pReqGSAP  = (TLeaseMgmtReqData*)p_pReq;
	YLeaseMgmtReqData * p_pReqYGSAP = (YLeaseMgmtReqData*)p_pReq;
	
	uint32_t nLeasePeriod      = pReqGSAP->m_nLeasePeriod;	/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	uint16_t ushEndpointTSAPID = IsYGSAP(p_ucVersion) ? p_pReqYGSAP->m_ushEndpointTSAPID : pReqGSAP->m_ushEndpointTSAPID;
	uint16_t ushLocalTSAPID    = IsYGSAP(p_ucVersion) ? p_pReqYGSAP->m_ushLocalTSAPID : pReqGSAP->m_ushLocalTSAPID;
	uint16_t ushEndpointOID    = IsYGSAP(p_ucVersion) ? p_pReqYGSAP->m_ushEndpointOID : pReqGSAP->m_ushEndpointOID;
	uint16_t ushLocalOID       = IsYGSAP(p_ucVersion) ? p_pReqYGSAP->m_ushLocalOID : pReqGSAP->m_ushLocalOID;
	TLeaseMgmtReqData_Parameters * pstParameters = IsYGSAP(p_ucVersion) ? &p_pReqYGSAP->m_stParameters : &pReqGSAP->m_stParameters;
			
	time_t nNow        = time(NULL);
	lease* pLease      = new lease;	/// TAKE CARE: delete before return if not added to any list
	pLease->nSessionId = p_nSessionID;

	if( !g_pSessionMgr->GetSessionExpTime( p_nSessionID, nSessExpTime ) )
	{	/// PROGRAMMER ERROR: SESSION sHould be verified prior to this call
		LOG("ERROR LEASE REQ (S %u T %u) expired session", p_nSessionID, p_unTransactionID);
		delete pLease;
		return leaseConfirm( p_nSessionID, p_unTransactionID, IsYGSAP(p_ucVersion) ? YGS_FAILURE : LEASE_FAIL_OTHER, 0, nLeasePeriod );
	}
	/// Request finite lease, session is also finite, do not allocate more than the session lifetime
	if(	nLeasePeriod && nSessExpTime && ( (unsigned)nSessExpTime < nNow + nLeasePeriod) )
	{
		LOG("WARNING LEASE REQ (S %u T %u) reduced lifetime SessExpTime = %lu, nNow = %lu, lease_life %lu -> %lu",
			p_nSessionID, p_unTransactionID, nSessExpTime, nNow, nLeasePeriod, nSessExpTime - nNow);
		pLease->nExpirationTime = nSessExpTime;
		///TODO: MARK AS REDUCED LEASE_SUCCESS_REDUCED to confirm later 
	}
	else
	{
		pLease->nExpirationTime = nLeasePeriod ? nNow + nLeasePeriod : 0;
		///TODO: MARK AS full LEASE_SUCCESS to confirm later 
	}

	pLease->eLeaseType    = (LeaseTypes)pReqGSAP->m_ucLeaseType; /// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	pLease->eProtocolType = (ProtocolTypes)pReqGSAP->m_ucProtocolType; /// TLeaseMgmtReqData/YLeaseMgmtReqData overlap

	/// Main YGSAP work: associate device tag with device address
	byte * pEndpointAddr = pReqGSAP->m_aucEndpointAddr;/// must be initialised for alert type leases
	if( IsYGSAP(p_ucVersion) )
	{
		if( TARGET_SELECTOR_DEVICE_TAG == p_pReqYGSAP->m_ucTargetSelector )// select device tag
		{
			if( LEASE_ALERT != p_pReqYGSAP-> m_ucLeaseType )
			{	
				/// tag size cannot be 0
				if (!p_pReqYGSAP->m_unDeviceTagSize)
				{
					delete pLease;
					return leaseConfirm( p_nSessionID, p_unTransactionID, YGS_FAILURE, 0, nLeasePeriod );/// this is YGSAP
				}
				/// alert type leases ignore the address field
				pEndpointAddr = g_stApp.m_oGwUAP.YDevListFind( p_pReqYGSAP->m_unDeviceTagSize, p_pReqYGSAP->m_aucDeviceTag );
				if( !pEndpointAddr)	/// there is no EUI64 to match the device tag provided
				{
					delete pLease;
					return leaseConfirm( p_nSessionID, p_unTransactionID, YGS_TAG_NOT_FOUND, 0, nLeasePeriod );/// this is YGSAP
				}
			}
		}
		else
			pEndpointAddr = p_pReqYGSAP->m_aucEndpointAddr;
	}
	memcpy(pLease->netAddr, pEndpointAddr, sizeof(pLease->netAddr));
	
	pLease->nCreationTime	= nNow;
	pLease->m_ushTSAPID		= ushEndpointTSAPID;
	pLease->nObjId			= ushEndpointOID;

	pLease->m_u16LocalTSAPID	= ushLocalTSAPID;
	pLease->m_u16LocalObjId		= ushLocalOID;

	if( pReqGSAP->IsTypeClient() )	/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	{
		pLease->nTransferMode	= pstParameters->m_ucTransferMode;
	}
	pLease->shPeriod_or_CommittedBurst	= pstParameters->m_shPeriod_or_CommittedBurst;	/// regardless of periodic/aperiodic
	if( pReqGSAP->IsTypeSubscriber() || (pReqGSAP->m_ucProtocolType != PROTO_NONE) ) /// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	{
		pLease->nTransferMode				= pstParameters->m_ucTransferMode;
		pLease->eUpdatePolicy				= (UpdatePolicies)pstParameters->m_ucUpdatePolicy;
		pLease->nPhase						= pstParameters->m_ucPhase;
		pLease->nStaleLimit					= pstParameters->m_ucStaleLimit;
	}

	if (	pstParameters->m_u16ConnectionInfoLen > 0 )
	{
		if (pstParameters->m_u16ConnectionInfoLen + sizeof(TLeaseMgmtReqData) <= p_u32ReqLen  )
		{
			LOG("ERROR LEASE REQ (S %u T %u) connection info len invalid %u ",
				p_nSessionID, p_unTransactionID, pstParameters->m_u16ConnectionInfoLen);
			delete pLease;
			return leaseConfirm( p_nSessionID, p_unTransactionID, IsYGSAP(p_ucVersion) ? YGS_INVALID_TYPE_INFO : LEASE_INVALID_INFORMATION, 0, nLeasePeriod );
		}
		else
		{	uint8_t* pConnectionInfo = IsYGSAP(p_ucVersion) ? (uint8_t*)(p_pReqYGSAP + 1) : (uint8_t*)(pReqGSAP + 1);
			pLease->m_oConnectionInfo.assign ( pConnectionInfo, pConnectionInfo + pstParameters->m_u16ConnectionInfoLen);
		}
	}

	//void*		pWirelessParams;//unused
	pLease->nSessionId		= p_nSessionID;
	pLease->nTransactionId	= p_unTransactionID;

	if (	pReqGSAP->m_ucProtocolType != PROTO_NONE	/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
		&&	(	pLease->m_u16LocalObjId != PROMISCUOUS_TUNNEL_LOCAL_OID 
			||	pLease->m_u16LocalTSAPID != PROMISCUOUS_TUNNEL_LOCAL_TSAP_ID) 
		)
	{
		/// load tunnel obj if not up
		CIsa100Object::Ptr pSmartTunnel =  g_stApp.m_oGwUAP.GetTunnelObject(pLease->m_u16LocalTSAPID, pLease->m_u16LocalObjId);
		if (!pSmartTunnel)
		{	LOG("ERROR LEASE REQ (S %u T %u) cannot get tunnel(%u, %u)", p_nSessionID, p_unTransactionID, ushLocalTSAPID, pLease->m_u16LocalTSAPID, pLease->m_u16LocalObjId);
			delete pLease;
			return leaseConfirm( p_nSessionID, p_unTransactionID, IsYGSAP(p_ucVersion) ? YGS_INVALID_TYPE_INFO : LEASE_INVALID_INFORMATION, 0, nLeasePeriod );
		}
		/// configure tunnel obj
		CTunnelObject* pTunnel = (CTunnelObject*)pSmartTunnel.get();
		
		//pTunnel->m_u8Status
		
		pTunnel->m_u8Protocol		= pReqGSAP->m_ucProtocolType; /// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
		pTunnel->m_u8NumPeerTunnels = 1;
		pTunnel->m_u8UpdatePolicy	= pstParameters->m_ucUpdatePolicy;
		pTunnel->m_i16Period		= pstParameters->m_shPeriod_or_CommittedBurst;
		pTunnel->m_u16Phase			= pstParameters->m_ucPhase;
		pTunnel->m_u8StaleLimit		= pstParameters->m_ucStaleLimit;

		pTunnel->m_u8FlowType		= CTunnelObject::FLOW_TYPE_2PART;
		uint8_t* pConnectionInfo = IsYGSAP(p_ucVersion) ? (uint8_t*)(p_pReqYGSAP + 1) : (uint8_t*)(pReqGSAP + 1);
		pTunnel->m_oConnectionInfo.assign ( pConnectionInfo, pConnectionInfo +  pstParameters->m_u16ConnectionInfoLen);
		
		pTunnel->m_u8Status			= CTunnelObject::STATUS_CONFIG;
	}

	if( (pLease->eLeaseType == LEASE_SERVER) || (pLease->eLeaseType == LEASE_ALERT) )
	{
		/// server leases get immediate response
		pLease->nLeaseId = GET_NEW_LEASE_ID;

		LOG( "LEASE Request(S %u T %u) ==> L %u server/alert, no contract needed", p_nSessionID, p_unTransactionID, pLease->nLeaseId);
		pLease->nContractId = 0;
		m_lstActiveLeases.push_back( pLease );
		leaseConfirm( pLease->nSessionId, p_unTransactionID,  IsYGSAP(p_ucVersion) ? YGS_SUCCESS : LEASE_SUCCESS, pLease->nLeaseId, pLease->nExpirationTime ? pLease->nExpirationTime - pLease->nCreationTime : 0);
	}
	else if( pLease->IsPeriodic() )	
	{	/// Subscriber leases get immediate response /// TODO what about publish leases?
		pLease->nLeaseId = GET_NEW_LEASE_ID;
		
		LOG( "LEASE Request(S %u T %u) ==> L %u periodic, no contract needed", p_nSessionID, p_unTransactionID, pLease->nLeaseId);
		pLease->nContractId = 0;
		m_lstSubscriberLeases.push_back( pLease );

		g_stApp.m_oGwUAP.DispatchLeaseAdd( pLease );

		leaseConfirm( pLease->nSessionId, p_unTransactionID, IsYGSAP(p_ucVersion) ? YGS_SUCCESS : LEASE_SUCCESS, pLease->nLeaseId, pLease->nExpirationTime ? pLease->nExpirationTime - pLease->nCreationTime : 0);
		/// TODO: In phase 1: add code for publish leases
	}
	else 
	{	DMO_CONTRACT_ATTRIBUTE *pContractAttrib = DMO_GetContract( pEndpointAddr, ISA100_START_PORTS + pLease->TSAPID(), ISA100_START_PORTS + ISA100_GW_UAP, SRVC_APERIODIC_COMM);
		if (pContractAttrib)
		{
			for( LeaseList::iterator it = m_lstLeasesPendingDelete.begin(); it != m_lstLeasesPendingDelete.end(); ++it )
			{	///  CANNOT re-use a contract pending delete. Refuse the request; the client should re-try few (10-20) seconds later
				if(	!memcmp( (*it)->netAddr, pLease->netAddr, 16) &&			/// same address128. At tis point the contract is guaranteed aperiodic
					( (*it)->TSAPID()         == pLease->TSAPID()) &&			/// same destination TSAP
					( (*it)->m_u16LocalTSAPID == pLease->m_u16LocalTSAPID ) )	/// same source TSAP
				{
					LOG( "LEASE Request(S %u T %u) contract %u pending DELETE, cannot allocate lease. Try again LATER",  p_nSessionID, p_unTransactionID, (*it)->nContractId );
					delete pLease;
					return leaseConfirm( p_nSessionID, p_unTransactionID, IsYGSAP(p_ucVersion) ? YGS_LIMIT_EXCEEDED : LEASE_NOT_AVAILABLE, 0, nLeasePeriod );
				}
			}

			pLease->nContractId = pContractAttrib->m_unContractID;
			++pContractAttrib->m_nUsageCount;
					
			pLease->nLeaseId = GET_NEW_LEASE_ID;
			m_lstActiveLeases.push_back( pLease );
			///TODO: we should use pContractAttrib->m_ulAssignedExpTime for lease duration
			LOG("LEASE Request(S %u T %u) ==> L %u re-use contractID %u (used %u times)%s",
				p_nSessionID, p_unTransactionID, pLease->nLeaseId, pLease->nContractId, pContractAttrib->m_nUsageCount,
				(pContractAttrib->m_nUsageCount<2) ? " WARNING inconsistency" : "" );
			leaseConfirm( pLease->nSessionId, p_unTransactionID, IsYGSAP(p_ucVersion) ? YGS_SUCCESS : LEASE_SUCCESS, pLease->nLeaseId, pLease->nExpirationTime ? pLease->nExpirationTime - pLease->nCreationTime : 0 );
		}
		else
		{	DMO_CONTRACT_BANDWIDTH stBandwidth;
			pLease->nTransactionId = p_unTransactionID;	///< pLease->nContractId will be completed on ContractNotification
			if( pLease->IsPeriodic() )
			{	/// TAKE CARE: DEAD CODE - We never get here: we have only subscriber leases.
				/// Only publish leases would use these parameters
				stBandwidth.m_stPeriodic.m_nPeriod		= pLease->shPeriod_or_CommittedBurst;

				///TODO: FIXME: CANNOT USE nStaleLimit (sec) dirrectly as assignment to m_unDeadline (slots 10^-2 sec)
				stBandwidth.m_stPeriodic.m_unDeadline	= pLease->nStaleLimit;	///TODO: FIXME not good; deadline is expressed in slots (of 10 ms each)
				
				stBandwidth.m_stPeriodic.m_ucPhase		= pLease->nPhase;
			}
			else
			{	/// Use default: one at 15 sec if no value requested
				stBandwidth.m_stAperiodic.m_nComittedBurst	= pLease->shPeriod_or_CommittedBurst ? pLease->shPeriod_or_CommittedBurst : -15;
				stBandwidth.m_stAperiodic.m_nExcessBurst	= 1;
				stBandwidth.m_stAperiodic.m_ucMaxSendWindow	= 5;
			}
			m_lstLeasesInProgress.push_back( pLease );

			char szExpirationTime[ 64 ] = {0};
			int nExpirationTime = ( pLease->nExpirationTime == 0 ) ? 0xFFFFFFFF : (int)(pLease->nExpirationTime - time(NULL));
		
			if( pLease->nExpirationTime == 0 )
				strcpy( szExpirationTime, "never");
			else
				sprintf( szExpirationTime, "%d(%+d)", (int)pLease->nExpirationTime, nExpirationTime);

			LOG( "LEASE Request(S %u T %u) REQUEST NEW contract, %s %3d expiration: %s", p_nSessionID, p_unTransactionID,
				 pLease->IsPeriodic() ?  "Period " : "CdtBrst",
									pLease->IsPeriodic() ? stBandwidth.m_stPeriodic.m_nPeriod : stBandwidth.m_stAperiodic.m_nComittedBurst, szExpirationTime);

			DMO_RequestNewContract( pEndpointAddr, ISA100_START_PORTS+pLease->TSAPID(), ISA100_APP_PORT, nExpirationTime,
									pLease->IsPeriodic() ? SRVC_PERIODIC_COMM : SRVC_APERIODIC_COMM, 0,
											( !memcmp(pEndpointAddr, g_stApp.m_stCfg.getSM_IPv6(), IPv6_ADDR_LENGTH) ) ? 1252 : 82,
											   0, &stBandwidth );
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Decrease usage count for a contract. If usage count reach 0, request DMAP to erase the contract. Then delete lease.
/// @param p_oLease iterator within the list pointing to the lease whose contract is to be deleted
/// @param p_bDeleteContract should we send TERMINATE CONTRACT ?
/// @param p_pbPendingContractDelete [IN/OUT] indicates if a TERMINATE contract was sent
/// @remarks if p_pbPendingContractDelete is NULL, the caller wants immediate lease delete regardless if a contratc delete request was sent or not.
/// @remarks if p_pbPendingContractDelete is not NULL, the caller would wait contract delete notification
/// @remarks if p_pbPendingContractDelete pointer is not NULL, the caller wants to handle lese delete later, and wants status
////////////////////////////////////////////////////////////////////////////////
LeaseList::iterator CLeaseMgmtService::deleteActiveLease( LeaseList::iterator p_itLease, bool p_bDeleteContract, bool * p_pbPendingContractDelete /* = NULL */)
{
	LOG("LEASE delete %lu", (*p_itLease)->nLeaseId);
		
	DMO_CONTRACT_ATTRIBUTE *pContractAttrib = DMO_FindContract( (*p_itLease)->nContractId );
	if( pContractAttrib )
		--pContractAttrib->m_nUsageCount;

	if( p_pbPendingContractDelete )
		*p_pbPendingContractDelete = false;


	/// MUST be after decreasing usage count, CS::OnLeaseDelete will read it
	g_stApp.m_oGwUAP.DispatchLeaseDelete( *p_itLease );

	if(	p_bDeleteContract							/// requested
	&&	pContractAttrib								/// no segfaults on next line
	&&	( pContractAttrib->m_nUsageCount <= 0 )		/// no other leases use it. TODO m_nUsageCount should be signed...
	&&	( (*p_itLease)->nContractId )				/// existing contracts only (exception: server/alert leases do not have underlying contracts)
	&&	( g_stApp.m_oGwUAP.SMContractID() != (*p_itLease)->nContractId) ) /// Do not terminate GW->SM contract.
	{	LOG("  Request TERMINATE contract %u (%s:%u)", (*p_itLease)->nContractId, GetHex((*p_itLease)->netAddr,16), (*p_itLease)->m_ushTSAPID);

		DMO_RequestContractTermination( (*p_itLease)->nContractId, 0 );	//operation 0/1/2: termination/deactivation/reactivation
		
		if( p_pbPendingContractDelete )
		{
			*p_pbPendingContractDelete = true;
			m_lstLeasesPendingDelete.push_back( *p_itLease );/// wait contract delete notification before erasing lease/sending SAP_OUT
			return m_lstActiveLeases.erase( p_itLease );
		}
	}

	delete *p_itLease;
	return m_lstActiveLeases.erase( p_itLease );
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Delete a subscriber lease (dispatch the lease delete prior to that)
/// @param p_oLease iterator within the list pointing to the lease to delete
/// @remarks
////////////////////////////////////////////////////////////////////////////////
LeaseList::iterator CLeaseMgmtService::deleteActiveSubscriberLease( LeaseList::iterator p_itLease )
{
	LOG("LEASE delete %lu (subscriber)", (*p_itLease)->nLeaseId);

	g_stApp.m_oGwUAP.DispatchLeaseDelete( *p_itLease );

	delete *p_itLease;
	return m_lstSubscriberLeases.erase( p_itLease );
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Renews a lease, within the session time boundary.
/// @param p_oLease iterator in m_lstActiveLeases
/// @param p_nLeasePeriod the new lease period
/// @return LeaseStatus
/// @remarks
////////////////////////////////////////////////////////////////////////////////
byte CLeaseMgmtService::renewLease(uint8_t p_ucVersion, LeaseList::iterator &p_itLease, unsigned long p_nLeasePeriod)
{	time_t nSessExpTime;
	if ( !g_pSessionMgr->GetSessionExpTime( (*p_itLease)->nSessionId, nSessExpTime ) )
	{	/// PROGRAMMER ERROR: SESSION sHould be verified prior to this call
		LOG("ERROR CLeaseMgmtService::renewLease (S %u T %u): expired session", (*p_itLease)->nSessionId, (*p_itLease)->nTransactionId);
		return IsYGSAP(p_ucVersion) ? YGS_FAILURE : LEASE_FAIL_OTHER;
	}
	LOG("CLeaseMgmtService::renewLease(%lu) nSessExpTime = %lu", (*p_itLease)->nLeaseId, nSessExpTime);

	/// TODO: CHECK LEASE INFINITE/FINITE CHANGES
	if( nSessExpTime && (unsigned)nSessExpTime < ( (*p_itLease)->nCreationTime + p_nLeasePeriod ) )
	{
		(*p_itLease)->nExpirationTime = nSessExpTime;
		return IsYGSAP(p_ucVersion) ? YGS_SUCCESS_REDUCED : LEASE_SUCCESS_REDUCED;
	}
	else
	{
		(*p_itLease)->nExpirationTime = (*p_itLease)->nCreationTime + p_nLeasePeriod;
		return LEASE_SUCCESS;	
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author 
/// @brief Delete expired leases from both lists (m_lstActiveLeases/m_lstLeasesInProgress)
/// @remarks Pending leases expire if they don't get contract back in 2 minutes
/// Active leases expire when their period ends
/// Must be called periodically
////////////////////////////////////////////////////////////////////////////////
void CLeaseMgmtService::CheckExpireLeases( time_t p_tNow )
{	int nCount = 0;
	/// Expire established leases with finite period
	for( LeaseList::iterator it = m_lstActiveLeases.begin(); it != m_lstActiveLeases.end(); )
	{
		if ((*it)->nExpirationTime && ((*it)->nExpirationTime <= p_tNow))/// 0 means it does not expire
		{
			LOG("CheckExpireLeases: delete lease id %lu: (exp %d <= now %d)", (*it)->nLeaseId, (*it)->nExpirationTime,  p_tNow);
			++nCount;
			it = deleteActiveLease( it, true );	/// DO erase the contract
		}
		else
			++it;
	}
	/// Expire established subscriber leases with finite period
	for( LeaseList::iterator it = m_lstSubscriberLeases.begin(); it != m_lstSubscriberLeases.end(); )
	{
		if ((*it)->nExpirationTime && ((*it)->nExpirationTime <= p_tNow))/// 0 means it does not expire
		{
			LOG("CheckExpireLeases: delete subscriber lease id %lu: (exp %d <= now %d)", (*it)->nLeaseId, (*it)->nExpirationTime,  p_tNow);
			++nCount;
			it = deleteActiveSubscriberLease( it );	/// Do erase the contract
		}
		else
			++it;
	}
	if( nCount )
	{
		LOG("CheckExpireLeases: %d leases deleted", nCount);
	}
	/// Pending leases will NOT receive timeout: it's necessary to periodically check expiration
	/// THIS IS NO longer true. We do receive timeout on contract creation failure
	/// @todo This is probably dead code. It is executed, but yeld no effects because
	///	supposedly all contract requests receive an answer from the stack, either success of failure.
	/// @see ContractNotification
#if 1	/// test if this is true
	for( LeaseList::iterator it = m_lstLeasesInProgress.begin(); it != m_lstLeasesInProgress.end(); )
	{
		if( ((*it)->nCreationTime + PENDING_LEASE_TIMEOUT) < p_tNow )
		{
			LOG("ERROR LEASE Expire(S %u T %u L %u) Created %u + %u < Now %u. This should NOT happen",
				(*it)->nSessionId, (*it)->nTransactionId, (*it)->nLeaseId, (*it)->nCreationTime, PENDING_LEASE_TIMEOUT, p_tNow );
			/// The device was reported in topology, MH requested contract but SM did not provide a contract back.
			/// We should retry: LEASE_FAIL_OTHER ensures retry at MH level, while LEASE_NO_DEVICE does not (MW will just expect device re-join)
			leaseConfirm((*it)->nSessionId, (*it)->nTransactionId, g_pSessionMgr->IsYGSAPSession((*it)->nSessionId)?YGS_FAILURE:LEASE_FAIL_OTHER, (*it)->nLeaseId, (*it)->nExpirationTime ? (*it)->nExpirationTime - (*it)->nCreationTime : 0 );
			delete *it;
			it = m_lstLeasesInProgress.erase( it );	 ///< Advance it here
		}
		else
			++it;
	}
#endif
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
bool CLeaseMgmtService::CanHandleServiceType( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case LEASE_MANAGEMENT:
			return true;
	}
	return false;
};


/// used in Dump
static void prepareLog( LeaseList::iterator it, time_t tNow, char * szExpirationTime, char * szCreationTime, char *szLastTx, char * szExtraParam, unsigned * unCounters)
{
	szExtraParam[ 0 ] = 0;
		
	if( (*it)->nExpirationTime == 0 )
		strcpy( szExpirationTime, "never");
	else
		sprintf( szExpirationTime, "%d(%+d)", (int)(*it)->nExpirationTime, (int)((*it)->nExpirationTime - tNow));
 
	sprintf( szCreationTime, "%+d", (int)((*it)->nCreationTime - tNow));
		
	if( ((LEASE_CLIENT == (*it)->eLeaseType) || (LEASE_BULK_CLIENT == (*it)->eLeaseType)) && (PROTO_NONE == (*it)->eProtocolType) )
	{	sprintf( szExtraParam, "CdtBrst %d", (*it)->shPeriod_or_CommittedBurst);
	}
	else if( (LEASE_SUBSCRIBER == (*it)->eLeaseType) && (PROTO_NONE == (*it)->eProtocolType) )
	{	sprintf( szExtraParam, "Period %d Ph %u Stale %u",
				 (*it)->shPeriod_or_CommittedBurst, (*it)->nPhase, (*it)->nStaleLimit);
	}
	if( ! ((timeval*)(*it)->m_oLastTx)->tv_sec )
			sprintf( szLastTx, "never");
		else
		sprintf( szLastTx, "%+d sec", (int)-(*it)->m_oLastTx.GetElapsedSec() );

	switch( (*it)->eLeaseType )
	{
		case LEASE_CLIENT: 		++unCounters[0]; break;
		case LEASE_SERVER:	 	++unCounters[1]; break;
		// publisher
		case LEASE_SUBSCRIBER: 	++unCounters[3]; break;
		case LEASE_BULK_CLIENT: ++unCounters[4]; break;
		// bulk server
		case LEASE_ALERT: 		++unCounters[6]; break;
		default:				++unCounters[7];
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Dump object status to LOG
/// @remarks Called on USR2
////////////////////////////////////////////////////////////////////////////////
void CLeaseMgmtService::Dump( void )
{
	// CLIENT, SERVER, /*PUBLISHER,*/ SUBSCRIBER, BULK_CLIENT, /*BULK_SERVER,*/ ALERT, OTHER
	unsigned unCounters[ 8 ] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	char szExpirationTime[ 64 ] = {0};
	char szCreationTime[ 64 ]   = {0};
	char szLastTx[ 64 ]         = {0};
	char szExtraParam[ 128 ]    = {0};
	time_t tNow = time(NULL);

	LOG("%s: OID %u. Pending %u Active %u Subscriber %u Pending contract DELETE %u", Name(), m_ushInterfaceOID, m_lstLeasesInProgress.size(), m_lstActiveLeases.size(), m_lstSubscriberLeases.size(), m_lstLeasesPendingDelete.size() );
	for( LeaseList::iterator it = m_lstLeasesPendingDelete.begin(); it != m_lstLeasesPendingDelete.end(); ++it)
	{	LOG( " Pending DELETE LeaseID %lu (S %u, T %ld, C %3u)",
			(*it)->nLeaseId, (*it)->nSessionId, (*it)->nTransactionId, (*it)->nContractId );
	}

	for( LeaseList::iterator it = m_lstLeasesInProgress.begin(); it != m_lstLeasesInProgress.end(); ++it)
	{	unsigned char uchCounter = 0;
		prepareLog( it, tNow, szExpirationTime, szCreationTime, szLastTx, szExtraParam, unCounters );
		if ( ( ++uchCounter > 100 ) && !g_stApp.m_stCfg.AppLogAllowDBG() )
		{
			LOG("  WARN Pending Lease list: too many items, give up logging. Set LOG_LEVEL_APP = 3 to list all");
			break;
		}
		LOG( " PendingLease(S %u, T %ld) %s:%u:%u Type %u:%s %s",
			(*it)->nSessionId, (*it)->nTransactionId, GetHex((*it)->netAddr, sizeof((*it)->netAddr) ),
			(*it)->m_ushTSAPID, (*it)->nObjId, (*it)->eLeaseType, getLeaseTypeName((*it)->eLeaseType), szExtraParam[0] ? szExtraParam : "" );
		LOG( "   Proto %u Creat: %s Exp: %s", (*it)->eProtocolType, szCreationTime, szExpirationTime);
	}
	LOG( " ---Total PendingLeases: Client %u Server %u Subscriber %u BulkClient %u Alert %u Other %u",
		unCounters[0], unCounters[1], unCounters[3], unCounters[4], unCounters[6], unCounters[7] );
	// fresh counters
	unCounters[0] = unCounters[1] = unCounters[3] = unCounters[4] = unCounters[6] = unCounters[7] = 0;

	LeaseList * pList = (LeaseList*)NULL;
	do
	{	if( pList == (LeaseList*)NULL)			pList = &m_lstActiveLeases;
		else if( pList == &m_lstActiveLeases )	pList = &m_lstSubscriberLeases;

		for( LeaseList::iterator it = pList->begin(); it != pList->end(); ++it)
		{	unsigned char uchCounter = 0;
			prepareLog( it, tNow, szExpirationTime, szCreationTime, szLastTx, szExtraParam, unCounters );
			if ( ( ++uchCounter > 100 ) && !g_stApp.m_stCfg.AppLogAllowDBG() )
			{	LOG("  WARN Lease list: too many items, give up logging. Set LOG_LEVEL_APP = 3 to list all");
				break;
			}
			LOG( " LeaseID %2lu (S %u C %3u) %s:%u:%u %s %s", (*it)->nLeaseId, (*it)->nSessionId, (*it)->nContractId,
				GetHex((*it)->netAddr, sizeof((*it)->netAddr) ), (*it)->m_ushTSAPID, (*it)->nObjId, getLeaseTypeName((*it)->eLeaseType), szExtraParam[0] ? szExtraParam : "");
			LOG( "   Creat: %s Exp: %s LastTX: %s", szCreationTime, szExpirationTime, szLastTx);
		}
	} while (pList != &m_lstSubscriberLeases );

	LOG( " ---Total ActiveLeases:  Client %u Server %u Subscriber %u BulkClient %u Alert %u Other %u",
		unCounters[0], unCounters[1], unCounters[3], unCounters[4], unCounters[6], unCounters[7] );

}

bool CLeaseMgmtService::validateUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pReq )
{	// the two brances are pretty much the same, just pReq is either TLeaseMgmtReqData or YLeaseMgmtReqData. do a macro?
	TLeaseMgmtReqData* pReqGSAP  = (TLeaseMgmtReqData*)p_pReq;
	YLeaseMgmtReqData* pReqYGSAP = (YLeaseMgmtReqData*)p_pReq;
	time_t nSessExpTime;
	size_t unMinSize = p_pHdr->IsYGSAP() ? sizeof(YLeaseMgmtReqData) : sizeof(TLeaseMgmtReqData);
	
	if (p_pHdr->m_nDataSize - 4 < unMinSize )
	{	LOG("ERROR LEASE REQ (S %u T %u) data size too small %d < %d-> drop",
			p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_nDataSize - 4, unMinSize);
		return false;
	}

	if( p_pHdr->IsYGSAP() )	pReqYGSAP->NTOH();
	else					pReqGSAP->NTOH();

	uint32_t nLeaseID          = pReqGSAP->m_nLeaseID;				/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	uint32_t nLeasePeriod      = pReqGSAP->m_nLeasePeriod;			/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	uint8_t	 ucLeaseType       = pReqGSAP->m_ucLeaseType;			/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	uint8_t  ucProtocolType    = pReqGSAP->m_ucProtocolType;		/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	uint8_t  ucNumberOfEndpoints = pReqGSAP->m_ucNumberOfEndpoints;	/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
	uint16_t ushEndpointTSAPID = p_pHdr->IsYGSAP() ? pReqYGSAP->m_ushEndpointTSAPID : pReqGSAP->m_ushEndpointTSAPID;
	uint16_t ushLocalTSAPID    = p_pHdr->IsYGSAP() ? pReqYGSAP->m_ushLocalTSAPID : pReqGSAP->m_ushLocalTSAPID;
	uint16_t ushEndpointOID    = p_pHdr->IsYGSAP() ? pReqYGSAP->m_ushEndpointOID : pReqGSAP->m_ushEndpointOID;
	uint16_t ushLocalOID       = p_pHdr->IsYGSAP() ? pReqYGSAP->m_ushLocalOID : pReqGSAP->m_ushLocalOID;
	TLeaseMgmtReqData_Parameters * pstParameters = p_pHdr->IsYGSAP() ? &pReqYGSAP->m_stParameters : &pReqGSAP->m_stParameters;

	if( !g_pSessionMgr->GetSessionExpTime( p_pHdr->m_nSessionID, nSessExpTime ) )
	{	/// PROGRAMMER ERROR: SESSION sHould be verified prior to this call
		LOG("ERROR LEASE REQ (S %u T %u) expired session", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID);
		return leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_FAILURE : LEASE_FAIL_OTHER, 0, nLeasePeriod );
	}
	if ( ushEndpointTSAPID > 15)
	{
		LOG("ERROR LEASE REQ (S %u T %u) remote tsap %u outside accepted range [%u %u]",
			p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, ushEndpointTSAPID, 0, 15);
		return leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP()?YGS_INVALID_TYPE_INFO:LEASE_INVALID_INFORMATION, 0, nLeasePeriod );
	}
	if ( !IsUAPLocalValid(ushLocalTSAPID))
	{
		LOG("ERROR LEASE REQ (S %u T %u) local tsap %u not supported",
			p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, ushLocalTSAPID);
		return leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP()?YGS_INVALID_TYPE_INFO:LEASE_INVALID_INFORMATION, 0, nLeasePeriod );
	}
	if(	(ucProtocolType == PROTO_NONE)
	&&	(	(ucLeaseType == LEASE_CLIENT)
		||	(ucLeaseType == LEASE_BULK_CLIENT)) )
	{
		if( ( pstParameters->m_shPeriod_or_CommittedBurst > 10 ) )
		{
			LOG("ERROR LEASE REQ (S %u T %u) incorrect bandwidth requested: %d",
				p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pstParameters->m_shPeriod_or_CommittedBurst);
			return leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP()?YGS_INVALID_TYPE_INFO:LEASE_INVALID_INFORMATION, 0, nLeasePeriod );
		}
	}
	if (	ucProtocolType != PROTO_NONE	/// TLeaseMgmtReqData/YLeaseMgmtReqData overlap
		&&	(	ushLocalOID != PROMISCUOUS_TUNNEL_LOCAL_OID
			||	ushLocalTSAPID != PROMISCUOUS_TUNNEL_LOCAL_TSAP_ID)
		&&	ushLocalTSAPID != ISA100_GW_UAP
		&&	ushLocalTSAPID != TUNNEL_EXTRA_UAP
		)
	{	LOG("ERROR LEASE REQ (S %u T %u) local tsap %u not supported for protocol %u", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, ushLocalTSAPID, ucProtocolType);
		return leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->IsYGSAP() ? YGS_INVALID_TYPE_INFO : LEASE_INVALID_INFORMATION, 0, nLeasePeriod );
	}

	//additional size tests based on lease type and protocol type
	if( ucNumberOfEndpoints != 1 )
	{
		LOG("ERROR LEASE REQ (S %u T %u) NumberOfEndpoints %d NOT SUPPORTED", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, ucNumberOfEndpoints);
		return leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, YGS_INVALID_TYPE_INFO, nLeaseID, nLeasePeriod );
	}
	if( !(ucLeaseType < LEASE_TYPE_NO) )
	{	//LEASE TYPE NOT SUPPORTED
		LOG("ERROR LEASE REQ: LeaseType %d NOT SUPPORTED", ucLeaseType);
		return leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, YGS_INVALID_TYPE, nLeaseID, nLeasePeriod );
	}
	if (!(ucProtocolType == PROTO_NONE || ucLeaseType == LEASE_CLIENT || ucLeaseType == LEASE_SERVER))
	{	//COMBINATION NOT SUPPORTED
		LOG("ERROR LEASE REQ: LeaseType %d / ProtocolType %d NOT SUPPORTED", ucLeaseType, ucProtocolType );
		return leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, YGS_INVALID_TYPE_INFO, nLeaseID, nLeasePeriod );
	}

	char szExtraParam[ 128 ];
	szExtraParam[0] = 0;
	if( ( (LEASE_CLIENT == (LeaseTypes)ucLeaseType) || (LEASE_BULK_CLIENT == (LeaseTypes)ucLeaseType)) && (PROTO_NONE == (ProtocolTypes)ucProtocolType))
	{	sprintf( szExtraParam, "TransferMode %u CdtBrst %d", pstParameters->m_ucTransferMode, pstParameters->m_shPeriod_or_CommittedBurst);
	}
	else if( (PROTO_NONE != (ProtocolTypes)ucProtocolType) || (LEASE_SUBSCRIBER == (LeaseTypes)ucLeaseType) )
	{	sprintf( szExtraParam, "TransferMode %u UpdPolicy %u, SubscrPeriod %d, Ph %u, Stale %u",
			pstParameters->m_ucTransferMode, (UpdatePolicies)pstParameters->m_ucUpdatePolicy,
			pstParameters->m_shPeriod_or_CommittedBurst, pstParameters->m_ucPhase, pstParameters->m_ucStaleLimit );
	}
	LOG("LEASE Request(S %u T %u L %u) --- Period %d Type %d (%s) Proto %d ---", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,
		nLeaseID, nLeasePeriod, ucLeaseType, getLeaseTypeName(ucLeaseType), ucProtocolType );

	char szEndpoint[ 64 ];
	if( p_pHdr->IsYGSAP() )
	{	char szTarget[ 33 ]; // 17 for ygsap, 33 for eui64 (1 byte for null-terminator)
		if( (TARGET_SELECTOR_DEVICE_TAG == pReqYGSAP->m_ucTargetSelector))
		{	uint8 unTargetsize = _Min(pReqYGSAP->m_unDeviceTagSize, 16);
			/// Shall we add protection against malformed messages with non-printable chars in tag?
			/// Nope: will not break the program, just log some binary chars....
			memcpy(szTarget, pReqYGSAP->m_aucDeviceTag, unTargetsize);
			szTarget[ unTargetsize ] = 0;	// this is correct, NO need to substract 1. Tag have up to 16 chars, put null terminator on 17th
		}
		else
		{	snprintf(szTarget, sizeof(szTarget), "%s", GetHex(pReqYGSAP->m_aucEndpointAddr, 16));
			szTarget[ sizeof(szTarget)-1 ] = 0;
		}
		sprintf(szEndpoint, "[%s:%s]", (TARGET_SELECTOR_DEVICE_TAG == pReqYGSAP->m_ucTargetSelector) ? "Tag": "EUI64", szTarget);
	}
	else
	{
		sprintf(szEndpoint, "%s", GetHex(pReqGSAP->m_aucEndpointAddr, 16) );
	}

	LOG("LEASE Request(S %u T %u) for %s:%u:%u leasePeriod %d", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,
		szEndpoint, ushEndpointTSAPID, ushEndpointOID, nLeasePeriod );
	if( szExtraParam[0] )
	{
		LOG( "LEASE Request(S %u T %u) %s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, szExtraParam );
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Process a user request
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @retval TBD
/// @remarks CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
/// p_pHdr members are converted to host order. Header and data CRC are verified.
/// p_pData members are network order, must be converted by ProcessUserRequest
////////////////////////////////////////////////////////////////////////////////
bool CLeaseMgmtService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{	///p_pHdr->m_ucServiceType == LEASE_MANAGEMENT
	if( !validateUserRequest( p_pHdr, p_pData ))
		return false;

	uint32 nLeaseID     = ((TLeaseMgmtReqData*)p_pData)->m_nLeaseID;
	uint32 nLeasePeriod = ((TLeaseMgmtReqData*)p_pData)->m_nLeasePeriod;

	if (!nLeaseID)
	{	/// May send G_LeaseConfirm internally
		createLease( p_pHdr->m_ucVersion, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pData, p_pHdr->m_nDataSize );
	}
	else
	{	/// Does send G_LeaseConfirm
		byte ucStatus = p_pHdr->IsYGSAP() ? YGS_FAILURE : LEASE_FAIL_OTHER;
		
		LeaseList::iterator oLease = findLease( nLeaseID, p_pHdr->m_nSessionID );
		if (oLease == (LeaseList::iterator)NULL )
		{
			ucStatus = p_pHdr->IsYGSAP() ? YGS_INVALID_LEASE : LEASE_NOT_EXIST;
		}
		else if( !nLeasePeriod )
		{
			if( (*oLease)->IsPeriodic() )
				deleteActiveSubscriberLease( oLease );
			else
			{
				bool bPendingContractDelete = false;
				deleteActiveLease( oLease, true, &bPendingContractDelete );	/// DO erase contract.
				if( bPendingContractDelete )
				{	/// so we know which transaction id to confirm
					(*oLease)->nTransactionId = p_pHdr->m_unTransactionID;
					return true;/// Do not confirm right now; wait contract notification
				}
			}
			ucStatus = p_pHdr->IsYGSAP() ? YGS_SUCCESS : LEASE_SUCCESS;
		}
		else
		{
			ucStatus = renewLease( p_pHdr->m_ucVersion, oLease, nLeasePeriod );
		}
		leaseConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, ucStatus, nLeaseID, nLeasePeriod );
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Add to publish cache pointers to subscriber leases matching the pair p_pNetAddr / p_ushTSAPID
/// @param p_pNetAddr Network address generating publish
/// @param p_ushTSAPID tsap generating publish
/// @param p_pSubscriberLeaseList destination list
/// @remarks Iterate trough all subscriber leases and call CPublishSubscribeService::AddSubscriber()
/// @remarks	for each appropriate lease. Needed to create the subscriber list 
/// @remarks	on the cache entry corresponding to a new publisher
/// @note caller should pass LeaseList as CPublishSubscribeService::PublishData::m_lstSubscriberLeases
////////////////////////////////////////////////////////////////////////////////
void CLeaseMgmtService::AddSubscribersToPublishCache( uint8_t * p_pNetAddr, uint16_t p_ushTSAPID, LeaseList * p_pSubscriberLeaseList )
{
	for( LeaseList::iterator it = m_lstSubscriberLeases.begin(); it!=m_lstSubscriberLeases.end(); ++it )
	{
		if( !memcmp( p_pNetAddr, (*it)->netAddr, 16) && ( p_ushTSAPID == (*it)->m_ushTSAPID ) )
		{
			p_pSubscriberLeaseList->push_back( (*it) );
			LOG("AddSubscribersToPublishCache: lease %2lu publisher %s:%u (%u)",
				(*it)->nLeaseId, GetHex(p_pNetAddr, 16), p_ushTSAPID, p_pSubscriberLeaseList->size() );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Delete all leases on unjoin. Currently does nothing on join
/// @brief The operator will send G_Publish_Indication if the lease is associated with source address and session is still valid
/// @param p_pPublish The publish service requesting iteration
/// @remarks
////////////////////////////////////////////////////////////////////////////////
void CLeaseMgmtService::OnJoin( bool p_bJoined )
{
	if( !p_bJoined )
	{	/// No need to delete pending leases - will expire anyway
		LOG("CLeaseMgmtService::OnJoin(UNJOIN): deleting %d leases", m_lstActiveLeases.size() );
		for( LeaseList::iterator it = m_lstActiveLeases.begin(); it != m_lstActiveLeases.end(); )
		{	/// delete leases, except SUBSCRIBER leases, which do not have underlying contract
			if((*it)->IsPeriodic())
			{
				LOG(" CLeaseMgmtService::OnJoin: lease     %lu is periodic, do not delete", (*it)->nLeaseId );
				++it;
			}
			else
			{
				it = deleteActiveLease( it, false ); /// Do NOT erase contract
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> User) send G_LeaseConfirm back to the user
/// @param p_nSessionID
/// @param p_unTransactionID
/// @param p_nLeaseId
/// @param p_nLeasePeriod
/// @param p_ucStatus
/// @retval false always -just a convenience for the callers to send confirm and return in the same statement
/// @remarks Calls CGSAP::SendConfirm, which erase from tracker too
////////////////////////////////////////////////////////////////////////////////
bool CLeaseMgmtService::leaseConfirm(	unsigned      p_nSessionID,
										unsigned      p_unTransactionID,
										byte          p_ucStatus,
										unsigned      p_nLeaseId,
										unsigned long p_nLeasePeriod )
{
	TLeaseMgmtConfirmData stConfirm = { m_ucStatus:p_ucStatus, m_nLeaseID:p_nLeaseId, m_nLeasePeriod:p_nLeasePeriod };
	stConfirm.HTON();
	if( !g_stApp.m_oGwUAP.SendConfirm( p_nSessionID, p_unTransactionID, LEASE_MANAGEMENT, (byte*)&stConfirm, sizeof(stConfirm), false ) )
		LOG("WARN leaseConfirm(S %u T %u Lease %u) Stat %u: invalid session, CONFIRM not sent", p_nSessionID, p_unTransactionID, p_nLeaseId, p_ucStatus);
	return false;
}

const char * CLeaseMgmtService::getLeaseTypeName( uint8 p_ucLeaseType )
{	static const char * sTypeName[] = {
		"CLIENT",		//LEASE_CLIENT
		"SERVER",		//LEASE_SERVER",
		"PUBLISHER",	//LEASE_PUBLISHER",
		"SUBSCRIBER",	//LEASE_SUBSCRIBER",
		"BULK_CLIENT",	//LEASE_BULK_CLIENT",
		"BULK_SERVER",	//LEASE_BULK_SERVER",
		"ALERT",		//LEASE_ALERT"
		"UNKNOWN" };
	uint8 u8UnkIdx = (sizeof(sTypeName) / sizeof(sTypeName[0])) - 1;
	return sTypeName[ ( p_ucLeaseType >= u8UnkIdx )  ? u8UnkIdx : p_ucLeaseType ];
}
