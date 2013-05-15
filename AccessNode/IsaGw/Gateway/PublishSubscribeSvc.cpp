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
/// @file PublishSubscribeSvc.h
/// @author Marcel Ionescu
/// @brief Publish/Subscribe service - implementation
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include "../../Shared/Common.h"

#include "GwApp.h"
#include "PublishSubscribeSvc.h"


////////////////////////////////////////////////////////////////////////////////
/// @class CPublishSubscribeService
/// @brief Publish/Subscribe services
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
bool CPublishSubscribeService::CanHandleServiceType( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{
		case SUBSCRIBE:
			return true;
	}
	return false;
};

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Process a user request
/// @param p_pHdr	GSAP request header (has member m_nDataSize)
/// @param pData	GSAP request data (data size is in p_pHdr)
/// @return ignored, false always
/// @remarks CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
////////////////////////////////////////////////////////////////////////////////
bool CPublishSubscribeService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{
	if ( SUBSCRIBE != p_pHdr->m_ucServiceType )
	{
		LOG("ERROR CPublishSubscribeService::ProcessUserRequest: unknown/unimplemented service type %u", p_pHdr->m_ucServiceType );
		return false;	/// Caller will SendConfirm with G_STATUS_UNIMPLEMENTED
	}
	unsigned long nLeaseId = ntohl(*(unsigned long*)p_pData);
	const lease* pLease = g_pLeaseMgr->FindLease( nLeaseId, p_pHdr->m_nSessionID );
	if( !pLease )
	{
		LOG("ERROR SUBSCRIBE REQ (S %u T %u L %u) Lease not found", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, nLeaseId );
		TSubscribeConfirmData stData = { p_pHdr->IsYGSAP() ? YGS_INVALID_LEASE : PUBLISH_LEASE_EXPIRE, 0, 0 };
		return subscribeConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, (uint8*)&stData, sizeof(stData) );
	}

	TPublishMap::iterator it = m_mapCache.find( PublishKey( pLease->netAddr, pLease->TSAPID() ) );
	if( it == m_mapCache.end() )
	{	/// May be an error at user which did not provide a correct lease, even if the lease id was found
		LOG("ERROR SUBSCRIBE REQ: Data from %s:%u not found in cache", GetHex(pLease->netAddr, sizeof(pLease->netAddr)), pLease->TSAPID()  );
		TSubscribeConfirmData stData = { p_pHdr->IsYGSAP() ? YGS_FAILURE : PUBLISH_FAIL_OTHER, 0, 0 };
		return subscribeConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, (uint8*)&stData, sizeof(stData) );
	}

	size_t dataLen = offsetof( TSubscribeConfirmData, m_aData ) + it->second->m_unDataSize;
	byte aData[ dataLen ];
	TSubscribeConfirmData * pSubscribeConfirmData = (TSubscribeConfirmData *) &aData;

	pSubscribeConfirmData->m_ucStatus = ( pLease->StaleData( time(NULL)-it->second->m_tLastRcv ) )
		? ( p_pHdr->IsYGSAP() ? YGS_SUCCESS_STALE : PUBLISH_SUCCESS_STALE)
		: ( p_pHdr->IsYGSAP() ? YGS_SUCCESS: PUBLISH_SUCCESS_FRESH);
	/// Starting with m_unTxSeconds, TSubscribeConfirmData and TPublishIndicationData are identhical.
	memcpy( (void*)&pSubscribeConfirmData->m_unTxSeconds, (void*)&it->second->m_pPIData->m_unTxSeconds, dataLen - offsetof(TSubscribeConfirmData, m_unTxSeconds) );

	return subscribeConfirm( p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, aData, dataLen );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Dispatch an PUBLISH ADPU. Call from CGwUAP::processIsaPackets
/// @param p_pAPDUIdtf 	The APDU identifier, with the source device address
/// @param p_pPublish The publish APDU from field, unpacked
/// @retval false - field APDU not dispatched - default handler does not do processing, just log a message
/// @remarks This class does not receive ProcessAPDU. such calls are errors and go to default handler CService::ProcessAPDU
/// @note CANNOT USE NORMAL FLOW TROUGH m_oIfaceOIDMgr.DispatchAPDU() -> ProcessAPDU because ProcessPublishAPDU
/// needs APDU_IDTF (m_aucAddr, m_ucSrcTSAPID); p_unAppHandle does not provide enough information
/// @note THE NOTE ABOVE HAS BECOME FALSE. TODO unify with normal flow
/// @note if we ever decide to go trough normal flow, CInterfaceObjMgr::DispatchAPDU will be able to
/// get here because READ_RSP_SRVC::m_unDstOID is in the same position with PUBLISH_SRVC::m_unSubOID
/// and the dispatch is done based on READ_RSP_SRVC::m_unDstOID
/// @note the publish is sent to GSAP subscriber regardless of the validations done
///
/// @note Currently it is called dirrectly on SRVC_PUBLISH, no need to test
////////////////////////////////////////////////////////////////////////////////
bool CPublishSubscribeService::ProcessPublishAPDU( APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp )
{
	///	Data is stored in cache prepared to be sent to subscribers: packed/network ordered in TPublishIndicationData
	/// TAKE CARE: m_nLeaseID needs to be changed with each subscriber
	bool           bMustDelete = false;
	PublishKey     oKey( p_pAPDUIdtf->m_aucAddr, p_pAPDUIdtf->m_ucSrcTSAPID );
	PublishData  * pPublish = NULL;
	PUBLISH_SRVC * pAPDUPublishData = &p_pRsp->m_stSRVC.m_stPublish;

	if(	(pAPDUPublishData->m_ucContentVersion != pAPDUPublishData->m_pData[0])
	|| 	(pAPDUPublishData->m_ucFreqSeqNo      != pAPDUPublishData->m_pData[1])
	||	!pAPDUPublishData->m_ucNativePublish )
	{
		LOG("WARNING PUBLISH inconsistent: ContentVersion [%3u %3u] or FreqSeqNo [%3u %3u] NativePublish %u Size %u",
			pAPDUPublishData->m_ucContentVersion, pAPDUPublishData->m_pData[0],
			pAPDUPublishData->m_ucFreqSeqNo,      pAPDUPublishData->m_pData[1],
			pAPDUPublishData->m_ucNativePublish,  pAPDUPublishData->m_unSize);
	}
	/// Save to cache (only if element is ok - in sequence)
	bool bSavedInCache = addToCache( p_pAPDUIdtf, pAPDUPublishData, oKey, &pPublish );

	if( !bSavedInCache )
	{	/// Did not save to cache, cannot use PublishData from there. Create PublishData to be sent to user
		pPublish = new PublishData( pAPDUPublishData, p_pAPDUIdtf );
		bMustDelete = true;
	}

	/// Publish data exactly as received, regardless of it's validity
	Publish2Subscribers( pPublish, &p_pAPDUIdtf->m_tvTxTime );

	if( bMustDelete )
	{
		delete pPublish;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> USER) Send G_Subscribe_Timer/G_Publish_Watchdog to al subscribers needing it
/// @param p_unSubscribers
/// @param p_unIndications
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::UpdateSubscribers( unsigned & p_unSubscribers, unsigned & p_unIndications )
{
	time_t tNow = time(NULL);
	p_unSubscribers = p_unIndications = 0;
	
	for( TPublishMap::iterator it = m_mapCache.begin(); it != m_mapCache.end(); )
	{
		bool bHasActiveSubscribers = false;
		bool bStaleReported = false;
		for( LeaseList::iterator itLease = it->second->m_lstSubscriberLeases.begin(); itLease != it->second->m_lstSubscriberLeases.end(); ++itLease)
		{
			/// Publish only if lease parameters request it
			if((*itLease)->NotPublishYet( tNow ) )
			{
				bHasActiveSubscribers = true;	/// still got active leases, do not delete
				continue;
			}

			/// It changes with each subscriber, the value stored in cache does not matter
			it->second->m_pPIData->m_nLeaseID = htonl((*itLease)->nLeaseId);
			
			time_t tTimeDiff = tNow - it->second->m_tLastRcv;
			if ( (*itLease)->StaleData( tTimeDiff )  )
			{
				if( !(*itLease)->m_bStaleReported )
				{	/// Stale, not reported
					publishWatchdogIndication( *itLease, (uint8*)it->second->m_pPIData, it->second->m_unPISize );	///< Send G_Publish_Watchdog
					(*itLease)->m_bStaleReported = true;
					bStaleReported = true;
				}
				/// else is stale, reported already
			}
			else /// ( !pLease->StaleData() ) data fresh
			{
				subscribeTimerIndication( *itLease, (uint8*)it->second->m_pPIData, it->second->m_unPISize );	///< Send G_Subscribe_Timer
				(*itLease)->m_bStaleReported = false;
				bHasActiveSubscribers = true;	/// still got active leases, do not delete
			}
		}
		if( bStaleReported && !bHasActiveSubscribers )
		{	/// delete cache entry only if no more leases need it AND WDT was sent
			LOG("PUBLISH UpdateSubscribers (%s:%u): delete cache entry with no active subscribers", GetHex( it->first.m_aucAddr, 16), it->first.m_ushTSAPID );
			deleteCacheEntry( it++ );
		}
		else
			++it;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> USER) Send PUBLISH (G_Publish) to al subscribers associated with p_pCacheData
/// @param  p_pCacheData identify the cache entry (publishing device / publish data)
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::Publish2Subscribers( PublishData * p_pCacheData, const timeval * p_ptvTxUTC )
{
	for( LeaseList::iterator it = p_pCacheData->m_lstSubscriberLeases.begin(); it != p_pCacheData->m_lstSubscriberLeases.end(); ++it )
	{	/// It changes with each subscriber, the value stored in cache does not matter
		p_pCacheData->m_pPIData->m_nLeaseID = htonl((*it)->nLeaseId);
		/// Will also reset the lease subscription timer

		publishIndication( (*it), (uint8*)p_pCacheData->m_pPIData, p_pCacheData->m_unPISize );
		
		(*it)->m_bStaleReported = false;
	}
	/// save statistics
	p_pCacheData->GPDUStatsAdd( p_ptvTxUTC );
}

/// constructor
CPublishSubscribeService::PublishData::PublishData( PUBLISH_SRVC * p_pPublish, APDU_IDTF * p_pAPDUIdtf )
:m_nPublishRate(0), m_unPhase(0), m_unDeadline(0)
{	gettimeofday(&m_tvFirstRx, NULL);
	m_tLastRcv = m_tvFirstRx.tv_sec;
	/// PUBLISH_SRVC is unpacked, TPublishIndicationData is packed, cannot memcpy
	m_unPISize = offsetof(TPublishIndicationData, m_aData) + p_pPublish->m_unSize;
	m_pPIData = (TPublishIndicationData *) new byte [ m_unPISize ];
	m_pPIData->m_nLeaseID         = 0;	/// Even if it's stored in cache, will change with each subscriber we are publishing to
	m_pPIData->m_unTxSeconds      = p_pAPDUIdtf->m_tvTxTime.tv_sec;
	m_pPIData->m_unTxuSeconds     = p_pAPDUIdtf->m_tvTxTime.tv_usec;
	m_unDataSize                  = p_pPublish->m_unSize;
	memcpy( m_pPIData->m_aData,     p_pPublish->m_pData, p_pPublish->m_unSize);
	m_pPIData->HTON();
	m_unRecvDelta = m_unRecvCount = 1;	/// this one: one
	m_unSkipCount = m_unDuplicateCount = m_unOutOfOrderCount = 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> User) send G_Subscribe_Confirm to the user
/// @param p_oLease
/// @param p_pMsgData
/// @param p_dwMsgDataLen
/// @retval true always -just a convenience for the callers to send confirm and return in the same statement
/// @remarks Calls CGSAP::SendIndication
////////////////////////////////////////////////////////////////////////////////
bool CPublishSubscribeService::subscribeConfirm( unsigned p_nSessionID, unsigned p_unTransactionID, uint8* p_pMsgData, uint32 p_dwMsgDataLen )
{	/// Subscribe requests are not placed in tracker
	if( !g_stApp.m_oGwUAP.SendConfirm( p_nSessionID, p_unTransactionID, SUBSCRIBE, p_pMsgData, p_dwMsgDataLen, false ))
		LOG("WARN subscribeConfirm(S %u T %u): invalid session, CONFIRM not sent", p_nSessionID, p_unTransactionID );
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Dump object status to LOG
/// @remarks Called on USR2
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::Dump( void )
{
	time_t tNow = time(NULL);
	unsigned unGWTotal = m_unRecvTotal + m_unSkipTotal;
	static unsigned unOldGWTotal = 0;	/// for avg number of PUBLISH received per second
	time_t tDelta = tNow - m_tOldDump;
	unsigned unTotalExpected = 0, unTotalRecv = 0;
	unsigned unCount = 0;

	LOG("%s: OID %u Total P/ST/WD: %4u/%4u/%2u  Publish Cache: %u items", Name(), m_ushInterfaceOID,
		m_unPublishCount, m_unSubscriberTimerCount, m_unPublishWatchdogCount, m_mapCache.size());

	/// TAKE CARE:This may take a long time
	for( TPublishMap::const_iterator it = m_mapCache.begin(); it != m_mapCache.end() ; ++it )
	{	unsigned char uchCounter = 0;
		TPublishIndicationData * p = it->second->m_pPIData;
		PublishData * pd = it->second;
		unsigned unTotal = pd->m_unRecvCount+pd->m_unSkipCount;
		tDelta = tNow - m_tOldDump;	/// re-init

		if( m_tOldDump < it->second->m_tvFirstRx.tv_sec)
			tDelta = tNow - it->second->m_tvFirstRx.tv_sec;
		unsigned unExpected = 1;	/// STATS Number of packets expected
		if( it->second->m_nPublishRate > 0 ) unExpected = tDelta /   it->second->m_nPublishRate;
		else                                 unExpected = tDelta * (-it->second->m_nPublishRate);
		int nLoss = unExpected - it->second->m_unRecvDelta;
		
		if( unExpected )
		{
			unTotalExpected += unExpected;
			unTotalRecv     += it->second->m_unRecvDelta;
			++unCount;	/// it was used, must add it to error margin
		}
		/// stats since last USR2; reliable
		LOG(" ---%s:%u STATS (%d s): Expect %4u Rx %4u Loss %2d (%5.2f%%) Rte %d Ph %u Ddl %u",
			GetHex(it->first.m_aucAddr, 16), it->first.m_ushTSAPID, tDelta, unExpected, it->second->m_unRecvDelta, nLoss,
			unExpected ? 100.0*nLoss/unExpected : 0.0, it->second->m_nPublishRate, it->second->m_unPhase, it->second->m_unDeadline );
		
		it->second->m_unRecvDelta = 0;	/// Reset regardless if it was used or not

		/// stats since the cache entry exist; not reliable
		LOG("      Rx %4u (%+2d) Duplicate %3u (%4.2f%%) Skip %4u (%4.2f%%) Desync %4u (%4.2f%%) StatQue %u Ver %u Data(%u)",
			pd->m_unRecvCount, it->second->m_tLastRcv - tNow,
			pd->m_unDuplicateCount,  unTotal ? 100.0*pd->m_unDuplicateCount/unTotal : 0.0,
			pd->m_unSkipCount,       unTotal ? 100.0*pd->m_unSkipCount/unTotal      : 0.0,
			pd->m_unOutOfOrderCount, unTotal ? 100.0*pd->m_unOutOfOrderCount/unTotal: 0.0,
			pd->m_lstGPDUStats.size(), p->m_aData[0], it->second->m_unDataSize ); /// p->m_aData[0] == m_ucContentVersion

		if( g_stApp.m_stCfg.AppLogAllowDBG() )
		{
			LOG("     Seq %3u LastRx %+d DevTx %+d %s%s", p->m_aData[1],	/// m_ucFreqSeqNo
				it->second->m_tLastRcv - tNow, ntohl(it->second->m_pPIData->m_unTxSeconds) - tNow,
				GET_HEX_LIMITED( p->m_aData, it->second->m_unDataSize, (unsigned)g_stApp.m_stCfg.m_nMaxPublishDataLog ) );
			unsigned counter = 0;
			GPDUStatsList::iterator itStats =  pd->m_lstGPDUStats.begin();
			for( ;  (itStats != pd->m_lstGPDUStats.end()) && (counter < 5); ++itStats, ++counter )/// begin() has highest timestamp
			{	LOG("       GPDUStats TAI Tx %u.%03u, Rx %u.%03u Late: %s",
					itStats->m_tTx.tv_sec, itStats->m_tTx.tv_usec/1000, itStats->m_tCrtTAI.tv_sec, itStats->m_tCrtTAI.tv_usec/1000,
					itStats->IsLate( it->second->m_nPublishRate, it->second->m_unPhase, it->second->m_unDeadline ) ? "YES": "NO" );
			}
		}
		if ( ( ++uchCounter > 100 ) && !g_stApp.m_stCfg.AppLogAllowDBG() )
		{
			LOG("  WARN Publish Cache: too many items, give up logging. Set LOG_LEVEL_APP = 3 to list all");
			break;
		}
	}
   	LOG(" TOTAL Recv %6u Duplicate %3u (%4.2f%%) Skip %4u (%4.2f%%) Desync %4u (%4.2f%%) Last %d sec avg %5.2f/sec",
		m_unRecvTotal,
		m_unDuplicateTotal,  unGWTotal ? 100.0*m_unDuplicateTotal/unGWTotal : 0.0,
		m_unSkipTotal,       unGWTotal ? 100.0*m_unSkipTotal/unGWTotal      : 0.0,
		m_unOutOfOrderTotal, unGWTotal ? 100.0*m_unOutOfOrderTotal/unGWTotal: 0.0,
		tDelta, tDelta > 0 ? 1.0 * (unGWTotal - unOldGWTotal) / tDelta : 0.0);
	if( unTotalExpected && tDelta )	/// avoid SIGFPE
	{	int  nLoss = unTotalExpected - unTotalRecv;
		LOG(" TOTAL STATS/cache (over %d sec): Expect %5u Recv %5u Loss %3d (%4.2f%%). Avg %4.2f/sec expect %4.2f/sec. (+/- %5.3f%%)", tDelta,
			unTotalExpected, unTotalRecv, nLoss,  100.0*nLoss / unTotalExpected, 1.0*(unGWTotal - unOldGWTotal) / tDelta,
			1.0*unTotalExpected / tDelta, 100.0 * unCount / unTotalExpected);
	}
	m_tOldDump   = tNow;
	unOldGWTotal = unGWTotal;

	for( TTSAPIDLst::iterator itTsapID = m_lstTSAPIDs.begin(); itTsapID!= m_lstTSAPIDs.end(); ++itTsapID )
	{
		LOG(" TSAPID %u", *itTsapID );
	}
}


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Process a lease create: add it to cache
/// @param p_pLease lease whose resources must be allocated
/// @remarks Method called on Lease add
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::OnLeaseAdd( lease* p_pLease )
{
	/// CHECK if this lease is peridic, otherwise do not add it to cache.
	if( !p_pLease->IsPeriodic() )	/// Not a publish lease, nothing to do
		return;

	TPublishMap::iterator it = m_mapCache.find( PublishKey( p_pLease->netAddr, p_pLease->TSAPID() ) );
	if( it == m_mapCache.end() )	///< Not found, nothing to erase
	{
		LOG("PS OnLeaseAdd(%lu): %s:%u NO cache entry, will add when device publish",
			p_pLease->nLeaseId, GetHex(p_pLease->netAddr, sizeof(p_pLease->netAddr)), p_pLease->TSAPID() );
		return;
	}

	it->second->m_lstSubscriberLeases.push_back( p_pLease );
	LOG("PS OnLeaseAdd(%2lu) subscriber list size %u", p_pLease->nLeaseId, it->second->m_lstSubscriberLeases.size() );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Process a lease deletion: erase from cache all entries associated with p_pLease
/// @param p_pLease lease whose resources must be erased
/// @remarks Method called on Lease delete (explicit delete or expire)
/// @note if cache entry have no more leases associated, delete the entry from cache
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::OnLeaseDelete( lease* p_pLease )
{
	/// CHECK if this lease is peridic, otherwise do not erase from cache.
	if( !p_pLease->IsPeriodic() )	/// Not a publish lease, nothing to do
		return;

	TPublishMap::iterator it = m_mapCache.find( PublishKey( p_pLease->netAddr, p_pLease->TSAPID() ) );
	if( it == m_mapCache.end() )	///< Not found, nothing to erase
	{	//LOG("   PS OnLeaseDelete(%lu): %s:%u NO cache entry", p_pLease->nLeaseId, GetHex(p_pLease->netAddr, sizeof(p_pLease->netAddr)), p_pLease->TSAPID() );
		return;
	}
	it->second-> m_lstSubscriberLeases.remove( p_pLease );
	size_t uListSize = it->second->m_lstSubscriberLeases.size();
	LOG("   PS OnLeaseDelete(%lu) %s:%u subscriber list size %u", p_pLease->nLeaseId,
		GetHex(p_pLease->netAddr, sizeof(p_pLease->netAddr)), p_pLease->TSAPID(), uListSize );
	if( !uListSize )
		deleteCacheEntry( it );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief add/update a publish to cache
/// @param p_pAPDUIdtf
/// @param p_pPublishSvc
/// @param p_rKey
/// @param p_ppPublish [OUT], return pointer to publish data, only if data was added to cache (retval true)
///		if retcode is false, p_ppPublish is also changed, but it points to previously published data,
///		not current one so it cannot be used.
///		It does so only to provide access to associated statistics used for GPDU*
/// @retval true if *p_ppPublish points to a valid data which can be sent to subscribers
/// @retval false if not (something wrong with the publish: duplicate, out of order, etc)
/// @remarks return value indicate if *p_ppPublish can be used in G_PublishIndication to subscribers.
/// 	Otherwise the caller must create its own PublishData( p_pPublishSvc, p_pAPDUIdtf )
/// @note *p_ppPublish is always changed, regardless of the return value, to point to cache entry. Sometined the value id not good though
/// @note the received PUBLISH is forwarded to subscribers regardless of the verifications done here
////////////////////////////////////////////////////////////////////////////////
bool CPublishSubscribeService::addToCache(	APDU_IDTF     * p_pAPDUIdtf,
											PUBLISH_SRVC  * p_pPublishSvc,
											PublishKey    & p_rKey,
											PublishData	 ** p_ppPublish )
{
	TPublishMap::iterator it = m_mapCache.find( p_rKey );
	if( it != m_mapCache.end() )
	{
		*p_ppPublish = it->second;
		it->second->m_tLastRcv	= g_stApp.m_oGwUAP.GetLastRxApduUTC()->tv_sec;

		++m_unRecvTotal;			 ///< STATS: All received messages
		++it->second->m_unRecvCount; ///< STATS:: messages from this publisher
		++it->second->m_unRecvDelta; ///< STATS:: messages from this publisher since last Dump()
		unsigned unTotal = it->second->m_unRecvCount + it->second->m_unSkipCount;
		if(!unTotal)
			unTotal = 0xFFFFFFFF;	/// Protection against division by 0

		if( it->second->m_pPIData->m_aData[1] == p_pPublishSvc->m_ucFreqSeqNo )
		{	/// Duplicate detected. Ignore it
			++it->second->m_unDuplicateCount;	/// STATS: duplicates from this publisher
			++m_unDuplicateTotal;				/// STATS: total duplicates
			LOG("WARN PUBLISH(%s:%u) DUPLICATE SeqNo %3u, size %u. Duplicate %u (%4.2f%%)",
				GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)), p_pAPDUIdtf->m_ucSrcTSAPID,
				p_pPublishSvc->m_ucFreqSeqNo, p_pPublishSvc->m_unSize, it->second->m_unDuplicateCount,
				100.0*it->second->m_unDuplicateCount/unTotal);
			return false;	/// Do NOT add
		}
		if( isOutOfOrder(  p_pPublishSvc->m_ucFreqSeqNo, it->second->m_pPIData->m_aData[1] ) )
		{	/// out-of-order packet (some old message finally arrived).
			/// DO NOT save to cache, return current cache item which is more recent
			++it->second->m_unOutOfOrderCount;	/// STATS: out-of-order from this publisher
			++m_unOutOfOrderTotal;				/// STATS: total out-of-order 
			LOG("WARN PUBLISH(%s:%u) DESYNC SeqNo %3u -> %3u, size %u. Desync %u (%4.2f%%)",
				GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)), p_pAPDUIdtf->m_ucSrcTSAPID,
				it->second->m_pPIData->m_aData[1], p_pPublishSvc->m_ucFreqSeqNo, p_pPublishSvc->m_unSize,
				it->second->m_unOutOfOrderCount, 100.0*it->second->m_unOutOfOrderCount/unTotal );
			/// HACK uncomment to recover faster from device restart. TODO COMMENT for production
			///it->second->m_pPIData->m_aData[1]		= p_pPublishSvc->m_ucFreqSeqNo;
			/// HACK END
			return false;	/// Do NOT add
		}
		if( it->second->m_pPIData->m_aData[0] != p_pPublishSvc->m_ucContentVersion )
		{	/// Content version change. Log it and continue
			LOG("WARN PUBLISH(%s:%u) VERSION change %u -> %u seq %u -> %u, size %u -> %u",
				GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)), p_pAPDUIdtf->m_ucSrcTSAPID,
				it->second->m_pPIData->m_aData[0], 	p_pPublishSvc->m_ucContentVersion,
				it->second->m_pPIData->m_aData[1], 	p_pPublishSvc->m_ucFreqSeqNo,
				it->second->m_unDataSize,			p_pPublishSvc->m_unSize );
		}
		if( it->second->m_unDataSize != p_pPublishSvc->m_unSize )
		{	/// Publish with different size? Something must be wrong! (Probably concatenated APDU starting with a PUBLISH)
			/// Will require delete/new to accomodate the new size
			LOG("WARN PUBLISH(%s:%u) SIZE change %u -> %u, seq %u",
				GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)), p_pAPDUIdtf->m_ucSrcTSAPID,
				it->second->m_unDataSize, p_pPublishSvc->m_unSize, p_pPublishSvc->m_ucFreqSeqNo );

			delete it->second;
			PublishData * pData = new PublishData( p_pPublishSvc, p_pAPDUIdtf );
			g_pLeaseMgr->AddSubscribersToPublishCache( p_pAPDUIdtf->m_aucAddr, p_pAPDUIdtf->m_ucSrcTSAPID, &pData->m_lstSubscriberLeases );
			*p_ppPublish = it->second = pData;
			return true;	/// Add
		}
		/// Compute skip packets, take care of rollover
		int nSkip = p_pPublishSvc->m_ucFreqSeqNo - it->second->m_pPIData->m_aData[1];
		char szSkip[64]; szSkip [0] = 0;
		if( (nSkip != 1) && (nSkip != -255) )
		{	if( --nSkip < 0 )
				nSkip += 256;
			m_unSkipTotal             += nSkip;	/// STATS: total skip messages for GW
			it->second->m_unSkipCount += nSkip;	/// STATS: skip messages for this publisher
			sprintf(szSkip, " (old %3u: skip %d [%4.2f%%])", it->second->m_pPIData->m_aData[1],
				nSkip, 100.0*it->second->m_unSkipCount/(unTotal+nSkip));
		}
		/// THE REGULAR CASE: cache renewal
		if ( g_stApp.m_stCfg.AppLogAllowINF() || szSkip[0] )
		{
			LOG("PUBLISH(%s:%u) UPDATE seq %3u size %u%s",
				GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)), p_pAPDUIdtf->m_ucSrcTSAPID,
				p_pPublishSvc->m_ucFreqSeqNo, p_pPublishSvc->m_unSize, szSkip );
			it->second->m_pPIData->m_unTxSeconds	= htonl(p_pAPDUIdtf->m_tvTxTime.tv_sec);
			it->second->m_pPIData->m_unTxuSeconds	= htonl(p_pAPDUIdtf->m_tvTxTime.tv_usec);
		}
		/// Fill in-place, no need to free/alocate again. No need to modify it->second->m_pPIData->m_unDataSize either
		memcpy( it->second->m_pPIData->m_aData, p_pPublishSvc->m_pData, it->second->m_unDataSize );
		return true;	/// Add
	}
	LOG("PUBLISH(%s:%u) ADD    ver %u seq %3u size %u",
		GetHex(p_pAPDUIdtf->m_aucAddr, sizeof(p_pAPDUIdtf->m_aucAddr)), p_pAPDUIdtf->m_ucSrcTSAPID,
		p_pPublishSvc->m_ucContentVersion, p_pPublishSvc->m_ucFreqSeqNo, p_pPublishSvc->m_unSize );

	newCacheEntry( p_pAPDUIdtf, p_pPublishSvc, p_rKey, p_ppPublish);
	return true;/// Add
}


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Create a new entry in the publish cache
/// @param p_pAPDUIdtf
/// @param p_pPublishSvc
/// @param p_rKey
/// @param p_ppPublish [OUT], return pointer to publish data
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::newCacheEntry( APDU_IDTF * p_pAPDUIdtf, PUBLISH_SRVC * p_pPublishSvc, PublishKey & p_rKey, PublishData ** p_ppPublish  )
{
	uint8_t * u8Tmp = new uint8_t[ 16 ];
	memcpy( u8Tmp, p_rKey.m_aucAddr, 16);
	
	PublishData * pData = new PublishData( p_pPublishSvc, p_pAPDUIdtf );
	g_pLeaseMgr->AddSubscribersToPublishCache( p_pAPDUIdtf->m_aucAddr, p_pAPDUIdtf->m_ucSrcTSAPID, &pData->m_lstSubscriberLeases );

	*p_ppPublish = m_mapCache.insert( std::make_pair(PublishKey(u8Tmp, p_rKey.m_ushTSAPID), pData) ).first->second;	/// insert return a pair<iterator,bool>. We need the first

	TTSAPIDLst::iterator itTsapID = m_lstTSAPIDs.begin();
	for( ; itTsapID!= m_lstTSAPIDs.end(); ++itTsapID )
	{
		if( *itTsapID == p_pAPDUIdtf->m_ucSrcTSAPID)
			break;
	}
	if( itTsapID == m_lstTSAPIDs.end() )// not found
		m_lstTSAPIDs.push_back( p_pAPDUIdtf->m_ucSrcTSAPID );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Delete a publish cache entry
/// @param p_it iterator to entry to delete
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::deleteCacheEntry( TPublishMap::iterator p_it )
{
	delete p_it->first.m_aucAddr;
	delete p_it->second;
	m_mapCache.erase( p_it );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Remove garbage statistics - older than now - p_nGPDUStatisticsPeriod
/// @param p_nGPDUStatisticsPeriod oldest acceptable statistic
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::PublishData::GPDUStatsGarbageColector( int p_nGPDUStatisticsPeriod )
{	struct timeval taiNow;
	MLSM_GetCrtTaiTime( &taiNow );
	int i = 0;
	for( GPDUStatsList::iterator it = m_lstGPDUStats.begin(); it != m_lstGPDUStats.end(); )
	{
		if( ( elapsed_usec(it->m_tCrtTAI, taiNow) / MICROSEC_IN_SEC) > p_nGPDUStatisticsPeriod )
		{
			it = m_lstGPDUStats.erase( it );
		}
		else
		{
			++it;
		}
	}

	if(i) LOG("GPDUStatsGarbageColector deleted %u statistics", i);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Compute GPDU statistics
/// @param p_unGPDULatency [OUT] receive GS_GPDU_Latency
/// @param p_unGPDUPathReliability [OUT] receive GS_GPDU_Path_Reliability
/// @remarks m_nPublishRate SIGN: > 0 : x seconds, <0: 1/-x seconds
/// @remarks TIMESTAMPs are ordered, we can stop iterating at first too old
/// @note Latency is computed over expected packet number - 1
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::PublishData::GPDUStatsCompute( uint8_t& p_unGPDULatency, uint8_t& p_unGPDUPathReliability)
{
	/// Not possible, check is being done before call. However it is very important to protect against SIGFPE
	if( !m_nPublishRate )
	{	p_unGPDULatency = p_unGPDUPathReliability = 0;
		return;
	}
	struct timeval taiNow;
	MLSM_GetCrtTaiTime( &taiNow );

	unsigned unExpected;	/// Number of packets expected
	unsigned unAvailable;	/// Avail packets, less than unExpected just after a device started publishing but before passing of m_nGPDUStatisticsPeriod seconds

	if( m_nPublishRate > 0 )	/// seconds between two publish messages
	{	unExpected = g_stApp.m_stCfg.m_nGPDUStatisticsPeriod / m_nPublishRate;
		unAvailable = elapsed_usec(m_tvFirstRx, taiNow) / MICROSEC_IN_SEC / m_nPublishRate;
	}
	else	/// ( m_nPublishRate <0 )	/// publish messages per second. Rate is 1/(-m_nPublishRate)
	{	unExpected = g_stApp.m_stCfg.m_nGPDUStatisticsPeriod * ( - m_nPublishRate );
		unAvailable = (elapsed_usec(m_tvFirstRx, taiNow) / MICROSEC_IN_SEC) * ( - m_nPublishRate );
	}
	if( unExpected > unAvailable)
		unExpected = unAvailable;
	if( !unExpected )			/// May be 0 if statistics period is smaller than publish rate
		unExpected = 1;

	unsigned unRecv = 0, unLate = 0;
	GPDUStatsList::iterator it = m_lstGPDUStats.begin();
	for( ; it != m_lstGPDUStats.end(); ++it )
	{
		if( (elapsed_usec(it->m_tCrtTAI, taiNow) / MICROSEC_IN_SEC) > g_stApp.m_stCfg.m_nGPDUStatisticsPeriod )
			break;	/// Safe to stop parsing here: list is reverse ordered by inserting in front.

		if( it->IsLate(m_nPublishRate, m_unPhase, m_unDeadline) )
		{	++unLate;
		}
		++unRecv;
	}
	int nGap = (int)unExpected - (int)unRecv;
	if( nGap  > 0)
	{
		unLate += nGap;
	}
	p_unGPDULatency  = 100 * unLate / unExpected;

	p_unGPDUPathReliability = 100 * unRecv / unExpected;
	if(p_unGPDULatency > 100)			/// Because unExpected is trunc down, it may be > 100%
		p_unGPDULatency = 100;
	if(p_unGPDUPathReliability > 100)	/// Because unExpected is trunc down, it may be > 100%
		p_unGPDUPathReliability = 100;

	LOG("GPDUStats: rate %2d Expected %3u Recv %3u Late %3u LATENCY %3u RELIABILITY %3u",
		m_nPublishRate, unExpected, unRecv, unLate, p_unGPDULatency, p_unGPDUPathReliability);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Compute GPDU statistics for given device
/// @param p_pNetworkAddress device for which statistics are computed. 16 bytes
/// @param p_unGPDULatency [OUT] receive GS_GPDU_Latency
/// @param p_unGPDUPathReliability [OUT] receive GS_GPDU_Path_Reliability
/// @retval true statistics successfully computed, output parameters updated
/// @retval false statistics not computed (device not found in cache?), output parameters undefined, do not use
/// @note search all possible tsapid's. However it computes statistics only for the first found
/// TODO consider multiple publish flows from the same device and consolidate statistics from all flows
////////////////////////////////////////////////////////////////////////////////
bool CPublishSubscribeService::GPDUStatsCompute( const uint8_t * p_pNetworkAddress, uint8_t& p_unGPDULatency, uint8_t& p_unGPDUPathReliability)
{	TTSAPIDLst::iterator itTsapID = m_lstTSAPIDs.begin();
	for( ; itTsapID != m_lstTSAPIDs.end(); ++itTsapID )
	{	TPublishMap::iterator it = m_mapCache.find( PublishKey( p_pNetworkAddress, *itTsapID ) );
		if( it != m_mapCache.end() )
		{	if( !it->second->m_nPublishRate )
			{	LOG("WARNING GPDUStatsCompute(%s:%u) publish rate NOT set", GetHex(p_pNetworkAddress, 16), *itTsapID );
				p_unGPDULatency = p_unGPDUPathReliability = 0;
				continue;
			}

			it->second->GPDUStatsCompute( p_unGPDULatency, p_unGPDUPathReliability );
			return true;
		}
	}
	LOG("WARNING GPDUStatsCompute(%s) did not publish data yet", GetHex(p_pNetworkAddress, 16) );
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Remove garbage statistics - older than now - p_nGPDUStatisticsPeriod for all cache items
/// @param p_nGPDUStatisticsPeriod oldest acceptable statistic
/// @note call it periodically - only once a minute or so
////////////////////////////////////////////////////////////////////////////////
void CPublishSubscribeService::GPDUStatsGarbageColector( int p_nGPDUStatisticsPeriod )
{	for( TPublishMap::iterator it = m_mapCache.begin(); it != m_mapCache.end(); ++it )
	{	it->second->GPDUStatsGarbageColector( p_nGPDUStatisticsPeriod );
	}
}

/// TAI_OFFSET was substracted in CGwUAP::processIsaPackets to get UNIX time. Put it back, we need TAI here
CPublishSubscribeService::GPDUStat::GPDUStat(const timeval* p_ptTx)
: m_tTx( *p_ptTx )
{
	const struct timeval * pRxTv = g_stApp.m_oGwUAP.GetLastRxApduUTC();
	m_tTx.tv_sec += TAI_OFFSET + g_stDPO.m_nCurrentUTCAdjustment;
	m_tCrtTAI.tv_sec  = pRxTv->tv_sec + TAI_OFFSET + g_stDPO.m_nCurrentUTCAdjustment;
	m_tCrtTAI.tv_usec = pRxTv->tv_usec;
}

/// TAI - (TAI % period) + ideal phase <= Crt TAI <= TAI - (TAI % period) + ideal phase + maximum delay
bool CPublishSubscribeService::GPDUStat::IsLate( int16_t p_nPublishRate, uint8_t p_unPhase, uint16_t p_unDeadline ) const
{
	if( !p_nPublishRate || (p_nPublishRate < -1000) )
		return true;/// this cannot be but protect against SIGFPE anyway

	int nPhaseMsec, nDeadLineMsec = p_unDeadline * 10, nModuloMsec;
	if( p_nPublishRate > 0 ){
		nPhaseMsec = 1000 * p_nPublishRate    * p_unPhase / 100;
		nModuloMsec = (m_tTx.tv_sec % p_nPublishRate) * 1000 + ((m_tTx.tv_usec/1000) % (1000 * p_nPublishRate ));
	}
	else{
		nPhaseMsec = 1000 / (-p_nPublishRate) * p_unPhase / 100;
		nModuloMsec = ((m_tTx.tv_usec/1000) % (1000 / (-p_nPublishRate)));
	}
	/// TAI - (TAI % period) + ideal phase <= Crt TAI <= TAI - (TAI % period) + ideal phase + maximum delay
	/// =>
	/// TAI - (TAI % period) + ideal phase - Crt TAI <= 0 AND
	/// TAI - (TAI % period) + ideal phase - Crt TAI + maximum delay >=0
	/// =>
	/// Crt TAI - {TAI - (TAI % period) + ideal phase} >= 0
	/// Crt TAI - {TAI - (TAI % period) + ideal phase} <= maximum delay
	/// =>
	/// Crt TAI - TAI + (TAI % period) - ideal phase >= 0
	/// Crt TAI - TAI + (TAI % period) - ideal phase <= maximum delay
	/// use miliseconds (10^-3)
	int nTmpMiliSec = elapsed_usec( m_tTx, m_tCrtTAI) / 1000; /// Crt TAI - TAI.
	nTmpMiliSec += nModuloMsec;								 /// +(TAI % period)
	nTmpMiliSec -= nPhaseMsec;								 /// - ideal phase

#if 0
	LOG("DEBUG GPDUStats IsLate(rate %d phase %d%% deadline %d slots) CrtTai-TAI %d + modulo %d - phase %d: { 0 <= %d <= %d }: %s",
		p_nPublishRate, p_unPhase, p_unDeadline, elapsed_usec( m_tTx, m_tCrtTAI) / 1000,
		nModuloMsec, nPhaseMsec, nTmpMiliSec, nDeadLineMsec,
		(! ( (nTmpMiliSec >= 0) && (nTmpMiliSec <= nDeadLineMsec) )) ? "YES": "NO");
#endif
	
	return ! ( (nTmpMiliSec >= 0) && (nTmpMiliSec <= nDeadLineMsec) );
};
