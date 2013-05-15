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
/// @brief Publish/Subscribe service - interface
////////////////////////////////////////////////////////////////////////////////
#ifndef PUBLISH_SUBSCRIBE_SERVICE_H
#define PUBLISH_SUBSCRIBE_SERVICE_H

#include <map>
#include "../ISA100/porting.h"
#include "Service.h"

#include "LeaseMgmtSvc.h"

/// PublishSubscribeStatus
#define PUBLISH_SUCCESS_FRESH	0	/// Success, fresh data
#define PUBLISH_SUCCESS_STALE	1	/// Success, stale data
#define PUBLISH_LEASE_EXPIRE	2	/// Failure; lease expired
#define PUBLISH_FAIL_OTHER		3	/// Failure; other

////////////////////////////////////////////////////////////////////////////////
/// @class CPublishSubscribeService
/// @brief Publish/Subscribe Service
/// @todo: erase from cache on lease expire
/// @see Service.h for details
////////////////////////////////////////////////////////////////////////////////
class CPublishSubscribeService: public CService{
	/// @brief G_Publish_INDICATION data for native access, except data CRC
	typedef struct	{
		uint32 	m_nLeaseID;			///< 		NETWORK ORDER
		uint32	m_unTxSeconds;		/// Device TX time: seconds unix time 
		uint32	m_unTxuSeconds;		/// Device TX time: micro seconds from top of the second
		uint8	m_aData[0];			/// variable-size publish data here, layout m_ucContentVersion, m_ucFreqSeqNo, data
		
		void HTON( void ) { m_nLeaseID = htonl(m_nLeaseID);  m_unTxSeconds = htonl(m_unTxSeconds); m_unTxuSeconds = htonl(m_unTxuSeconds); };
	}__attribute__ ((packed)) TPublishIndicationData;

	struct GPDUStat{
		struct timeval m_tTx;		/// Tx TAI
		struct timeval m_tCrtTAI;	/// Rx TAI

		/// TAI_OFFSET was substracted in CGwUAP::processIsaPackets to get UNIX time. Put it back, we need TAI here
		GPDUStat(const timeval* p_ptTx);

		/// TAI - (TAI % period) + ideal phase <= Crt TAI <= TAI - (TAI % period) + ideal phase + maximum delay
		/// TODO: properly compute for sub-second publish rate
		bool IsLate( int16_t p_nPublishRate, uint8_t p_unPhase, uint16_t p_unDeadline ) const;
	};
	typedef std::list< GPDUStat > GPDUStatsList;
	
public:
	/// @brief G_Subscribe_CONFIRM data for native access, except data CRC
	typedef struct	{
		byte 	m_ucStatus;			///< GS_Status
		uint32	m_unTxSeconds;		/// Device TX time: seconds unix time 
		uint32	m_unTxuSeconds;		/// Device TX time: micro seconds from top of the second
		uint8	m_aData[0];			/// variable-size publish data here, layout m_ucContentVersion, m_ucFreqSeqNo, data
	}__attribute__ ((packed)) TSubscribeConfirmData;

	/// @brief The cache key: source identifier / m_ushTSAPID
	struct PublishKey{
		uint8_t   *	m_aucAddr;		/// Endpoint (Device) address. Make sure is valid/deleted when needed
		uint16_t	m_ushTSAPID;	/// Endpoint (Device) TSAPID (source TSAPID on publish packets)

		inline PublishKey( const uint8_t * p_aucAddr, uint16_t p_ushTSAPID )
			:m_aucAddr((uint8_t *)p_aucAddr), m_ushTSAPID(p_ushTSAPID) {};
	};

	/// @brief The cache data starting with m_ucContentVersion
	struct PublishData{
		struct timeval	m_tvFirstRx;	///< Used to correctly compute GPDU stats when report is requested before m_nGPDUStatisticsPeriod
		time_t			m_tLastRcv;	///< Used to decide if data is stale

		/// *************** GPDU statistics begin **************
		int16_t			m_nPublishRate;		///< publication rate from contract, positive is seconds, negative is fractions of second
		uint8_t			m_unPhase;			///< (Ideal?) phase
		uint16_t		m_unDeadline;		///< expressed in slots (of 10 ms each)
		GPDUStatsList	m_lstGPDUStats;		///< reverse ordered because we insert in front
		/// *************** GPDU statistics end   **************

		/// make sure the pointers are always valid. Maybe use weak pointers if we cannot be sure?
		/// JUST POINTERS TO subscribers. NEVER delete a lease from this list
		LeaseList	 	m_lstSubscriberLeases;	///< list with pointers to subscribers

		unsigned		m_unRecvDelta;			///< STATS Number of messages (Publish) received since last report (k2)
		unsigned		m_unRecvCount;			///< STATS Number of messages (Publish) received from field
		unsigned		m_unSkipCount;			///< STATS Number of (detected) skip messages
		unsigned		m_unDuplicateCount;		///< STATS Number of duplicates
		unsigned		m_unOutOfOrderCount;	///< STATS Number of out-of-order messages (with weird seq number)
		unsigned		m_unDataSize;			///< Publish data size ( sizeof(TPublishIndicationData::m_aData) )
		unsigned		m_unPISize;				///< Publish indication size: the whole TPublishIndicationData + sizeof(m_aData)
		TPublishIndicationData * m_pPIData;		///< Publish indication structure

		PublishData( PUBLISH_SRVC * p_pPublish, APDU_IDTF * p_pAPDUIdtf );
		~PublishData( void )	/// DO NOT delete leases pointed by m_lstSubscriberLeases elements. Just clear the list.
			{ delete m_pPIData; m_lstSubscriberLeases.clear(); };

		/// List is reverse ordered by inserting in front.
		/// If you change the insertion point take care to change optimisations in
		/// PublishData::GPDUStatsGarbageColector / PublishData::GPDUStatsCompute
		/// @param p_ptvTxTAI: TX time, UTC, 
		inline void GPDUStatsAdd( const timeval * p_ptvTxUTC  )
			{ m_lstGPDUStats.push_front( GPDUStat(p_ptvTxUTC) ); };

		/// Remove garbage statistics - older than now - p_nGPDUStatisticsPeriod
		void GPDUStatsGarbageColector( int p_nGPDUStatisticsPeriod );

		inline void GPDUStatsSetPublishRate( int16_t p_nPublishRate, uint8_t p_unPhase, uint16_t p_unDeadline )
			{ m_nPublishRate = p_nPublishRate; m_unPhase = p_unPhase; m_unDeadline = p_unDeadline; }

		/// Compute GPDU statistics
		void GPDUStatsCompute( uint8_t& p_unGPDULatency, uint8_t& p_unGPDUPathReliability);
	};

private:
	/// @brief Functor used by TPublishMap as comparator
	struct cacheCompare {	/// cannot use memcmp, the struct is not packed
		bool operator()( PublishKey p_oK1, PublishKey p_oK2 ) const
			{	return (memcmp(p_oK1.m_aucAddr, p_oK2.m_aucAddr, 16 ) < 0)
					|| (p_oK1.m_ushTSAPID < p_oK2.m_ushTSAPID);
			};
	} ;
	
public:
	/// Map Key: publish data identifier: device address, publish object id, content version ? data size?
	/// Map Data: pointer to publish data
	typedef std::map<PublishKey, PublishData*, cacheCompare> TPublishMap;
	typedef std::pair<const PublishKey, PublishData*> TPublishPair;

	CPublishSubscribeService( const char * p_szName ):CService(p_szName)
		{ m_tOldDump = time(NULL); m_unRecvTotal = m_unSkipTotal = m_unDuplicateTotal = m_unOutOfOrderTotal = m_unPublishCount = m_unPublishWatchdogCount = m_unSubscriberTimerCount = 0; };

	/// Return true if the service is able to handle the service type received as parameter
	/// Used by (USER -> GW) flow
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;

	/// (USER -> GW) Process a user request. Call from CInterfaceObjMgr::DispatchUserRequest
	/// p_pHdr members are converted to host order. Header and data CRC are verified.
	/// p_pData members are network order, must be converted by ProcessUserRequest
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	
	/// TAKE CARE: CANNOT USE NORMAL FLOW TROUGH m_oIfaceOIDMgr.DispatchAPDU() -> ProcessAPDU
	///            because ProcessPublishAPDU needs APDU_IDTF (m_aucAddr, m_ucSrcTSAPID);
	///            p_unAppHandle does not provide enough information
	///
	/// @note THE NOTE ABOVE HAS BECOME FALSE. TODO unify with normal flow
	///
	/// (ISA -> GW) Dispatch an PUBLISH ADPU. Call from CGwUAP::processIsaPackets
	/// This class does not receive ProcessAPDU. such calls are errors and go to default handler CService::ProcessAPDU
	bool ProcessPublishAPDU( APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pPublish );
	
	/// Add a subscribed lease to cache 
	void OnLeaseAdd( lease* p_pLease );

	/// Erase subscriber lease from cache. If a cache entry becomes unused (all subscriber leases are erased),
	/// also erase from cache all entries associated with lease p_pLease
	virtual void OnLeaseDelete( lease* p_pLease );

	/// Called on USR2: Dump status to LOG
	virtual void Dump( void );

	/// (GW -> USER) Send PUBLISH (G_Publish) to al subscribers associated with p_pCacheData
	/// p_pCacheData identify the cache entry (publishing device / publish data)
	void Publish2Subscribers( PublishData * p_pCacheData, const timeval * p_ptvTxUTC );

	void UpdateSubscribers( unsigned & p_unSubscribers, unsigned & p_unIndications );
			
	inline bool GPDUStatsSetPublishRate( PublishKey p_oKey, int16_t p_nPublishRate, uint8_t p_unPhase, uint16_t p_unDeadline )
		{	TPublishMap::iterator it = m_mapCache.find(p_oKey);
			if( it !=  m_mapCache.end())
			{	it->second->GPDUStatsSetPublishRate( p_nPublishRate, p_unPhase, p_unDeadline);
				return true;
			}
			return false;
		};

	bool GPDUStatsCompute( const uint8_t * p_pNetworkAddress, uint8_t& p_unGPDULatency, uint8_t& p_unGPDUPathReliability);

	void GPDUStatsGarbageColector( int p_nGPDUStatisticsPeriod );
	
	unsigned	m_unPublishCount;			///< The number of G_Publish_Indication's sent.
	unsigned	m_unPublishWatchdogCount;	///< The number of G_Publish_Watchdog's sent.
	unsigned	m_unSubscriberTimerCount;	///< The number of G_Subscribe_Timer's sent.

	time_t		m_tOldDump;					///< Last Dump() time or program start if never Dump()
	/// Next 4 are NOT redundant: info kept in PublishData may be lost on cache entry delete, also it is time consuming to iterate whole cache
	unsigned	m_unRecvTotal;				///< STATS Total number of messages (Publish) received from field
	unsigned	m_unSkipTotal;				///< STATS Total number of (detected) skip messages
	unsigned	m_unDuplicateTotal;			///< STATS Total number of duplicates
	unsigned	m_unOutOfOrderTotal;		///< STATS Total number of out-of-order messages (with weird seq number)

private:
	///List with all TSAPID's publishing to GW, used to compute GPDU
	/// @todo possible optimisation: to keep the pair device address + tsapid
	/// because with current approach in case of unprobable distribution of many
	/// tsap id's we will end up with as many map.find()'s as the number of elements in this list
	typedef std::list<uint16_t> TTSAPIDLst;
	
	/// Forbid object copy
	CPublishSubscribeService( const CPublishSubscribeService& ):CService(""){};
	
	bool addToCache(    APDU_IDTF * p_pAPDUIdtf, PUBLISH_SRVC * p_pPublishSvc, PublishKey& p_rKey, PublishData ** p_ppPublish);
	void newCacheEntry( APDU_IDTF * p_pAPDUIdtf, PUBLISH_SRVC * p_pPublishSvc, PublishKey& p_rKey, PublishData ** p_ppPublish );
	void deleteCacheEntry( TPublishMap::iterator p_it );

	/// VALIDITY_WINDOW should be as big as 63*4 (4/second max publish freq; 63 seconds max packet lifetime)
	/// but this is too close of the whole range: nothing gets detected
	/// We choose a smaller window: at max publishing rate, some valid packets will be rejected, but no invalid packets will be accepted
	/// TODO: validity window should depend on the publishing rate (lease.nPeriod).
#define VALIDITY_WINDOW 64 ///< best match for 1-second publishing
	/// Return true if m_ucFreqSeqNo is out-of order (take care of wrap-around 255)
	bool isOutOfOrder( uint8 p_ucNew, uint8 p_ucOld)
		{ return ( p_ucNew > p_ucOld ) ? (p_ucNew - p_ucOld > VALIDITY_WINDOW) : ( p_ucOld - p_ucNew < (0xFF - VALIDITY_WINDOW) ); };

	/// (GW -> User) send G_Publish_Indication to the user
	inline bool publishIndication( lease* p_pLease, uint8* p_pMsgData, uint32 p_dwMsgDataLen )
		{ ++m_unPublishCount; return SendIndication( PUBLISH, p_pLease, p_pMsgData, p_dwMsgDataLen); };


	/// (GW -> User) send G_Subscribe_Timer to the user
	inline bool subscribeTimerIndication( lease* p_pLease, uint8* p_pMsgData, uint32 p_dwMsgDataLen )
		{ ++m_unSubscriberTimerCount; return SendIndication( SUBSCRIBE_TIMER, p_pLease, p_pMsgData, p_dwMsgDataLen); };


	/// (GW -> User) send G_Publish_Watchdog to the user
	inline bool publishWatchdogIndication( lease* p_pLease, uint8* p_pMsgData, uint32 p_dwMsgDataLen )
		{ ++m_unPublishWatchdogCount; return SendIndication( WATCHDOG_TIMER, p_pLease, p_pMsgData, p_dwMsgDataLen); };


	/// (GW -> User) send G_Subscribe_Confirm to the user
	bool subscribeConfirm( unsigned p_nSessionID, unsigned p_unTransactionID, uint8* p_pMsgData, uint32 p_dwMsgDataLen );

	TPublishMap	m_mapCache;
	TTSAPIDLst	m_lstTSAPIDs;	/// Used by GPDU Stats 
};

#endif //PUBLISH_SUBSCRIBE_SERVICE_H
