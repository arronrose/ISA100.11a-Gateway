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
/// @brief Lease Management Service - interface
////////////////////////////////////////////////////////////////////////////////
#ifndef LEASE_MGMT_SVC_H
#define LEASE_MGMT_SVC_H

#include <list>
#include <time.h>
#include <vector>

#include "../ISA100/porting.h"
#include "Service.h"
#include "../../Shared/EasyBuffer.h"

//#include "PublishSubscribeSvc.h"

enum LeaseTypes{ LEASE_CLIENT = 0, LEASE_SERVER, LEASE_PUBLISHER, LEASE_SUBSCRIBER, LEASE_BULK_CLIENT, LEASE_BULK_SERVER, LEASE_ALERT, LEASE_TYPE_NO };
enum ProtocolTypes{ PROTO_NONE = 0, PROTO_HART, PROTO_FFH1, PROTO_MODBUS, PROTO_PROFIBUS, PROTO_CIP };
enum UpdatePolicies{ POLICY_PERIODIC = 0, POLICY_CHANGE_OF_STATE };

/// LeaseStatus
#define LEASE_SUCCESS				0	// Success, new lease created, renewed or deleted
#define LEASE_SUCCESS_REDUCED		1	// Success, new lease created or renewed with reduced period
#define LEASE_NOT_EXIST				2	// Failure; lease does not exist to renew or delete
#define LEASE_NOT_AVAILABLE			3	// Failure; no additional leases available
#define LEASE_NO_DEVICE				4	// Failure; no device exists at network address
#define LEASE_INVALID_TYPE			5 	// Failure; invalid lease type
#define LEASE_INVALID_INFORMATION	6 	// Failure; invalid lease type information
#define LEASE_FAIL_OTHER			7	// Failure; other

#define TARGET_SELECTOR_DEVICE_TAG 0
typedef struct lease{
	unsigned long nLeaseId;			///< GS_Lease_ID
	time_t		nExpirationTime;
	time_t		nCreationTime;
	LeaseTypes	eLeaseType;
	ProtocolTypes	eProtocolType;
	uint8		netAddr[16];		///< GS_Network_Address
	uint16		m_ushTSAPID;		///< GS_Resource
	uint16		nObjId;				///< GS_Resource [marcel]: will not be used on protocol type 0-None
									///< GS_Resource [claudiu]: only for protocol type != 0-None

	uint16		m_u16LocalTSAPID;	///< GS_Resource [claudiu]: only for protocol type != 0-None
	uint16		m_u16LocalObjId;	///< GS_Resource [claudiu]: only for protocol type != 0-None

	uint8		nTransferMode;		///
	UpdatePolicies eUpdatePolicy;	///< for publish 0 - Periodic 1- ChangeOfState - TODO in phase 1 when we implement PUBLISH
	short		shPeriod_or_CommittedBurst;	/// P/S: data publication period
											/// C/S: Committed_Burst
	uint16		nPhase;				///< for publish/subscribe (ideal publication phase in tens of milliseconds)
	uint8		nStaleLimit;		///< for publish/subscribe (number of seconds)
	std::vector<uint8_t>	m_oConnectionInfo;
	std::vector<uint8_t>	m_oTransactionInfo;
	//uint8*	pConnectionInfo;	//used only for foreign protocol tunnels
	//void*		pWirelessParams;	//unused

	/// 1. passed in confirm after a lease is completed
	/// 2. Used to match session period update/ session delete
	unsigned	nSessionId;			///< Session owning this lease
	/// 1. Passed in confirm after a lease is completed
	/// 1. Passed in confirm after a contract is erased following a leade delete requested by used
	long		nTransactionId;		///< transaction id
	uint16		nContractId;		///< Contract ID
		
	/// Time of last transmission to user. Currently used by CPublishSubscribeService : subscribe timer and watchdog timer
	///
	/// TODO mark start time on CClientServerService/CBulkTransferService and any other services doing TX to user
	/// Maybe marking should be done on CService (ignoring non-lease services as CSessionMgmtService, CLeaseMgmtService, CSystemReportService, are there others? )
	CMicroSec	m_oLastTx;
	bool 		m_bStaleReported;

	/// Must call CMicroSec::Init because default constructor read current time
	lease() { memset(this, 0, sizeof(*this)); struct timeval tmInit = {0,0}; m_oLastTx.Init( tmInit ); m_bStaleReported = false; };
	
	unsigned char TSAPID( void ) const { return m_ushTSAPID; };///< Keep here. Maybe in the future we go back to UDP ports
	bool		IsPeriodic( void ) const { return (eLeaseType == LEASE_PUBLISHER) || (eLeaseType == LEASE_SUBSCRIBER); }

	/// Return update period in miliseconds (sec/1000)
	uint32 GetPeriodMSec( void ) const
		{	if(!IsPeriodic())		return 0;
			if(shPeriod_or_CommittedBurst > 0)	return shPeriod_or_CommittedBurst * MILISEC_IN_SEC;
			if(shPeriod_or_CommittedBurst < 0)	return MILISEC_IN_SEC / (-shPeriod_or_CommittedBurst);
			return 0;
		};

	/// Return true if data is to be forwarded to the user with either SUBSCRIPTION_TIMER or WATCHDOG_TIMER.
	/// Have meaning on publish leases, but will work on any lease type
	/// Use direct access to tv_sec instead of GetElapsedMSec to avoid one gettimeofday call
	inline bool NotPublishYet( time_t p_tNow ) { time_t tDiff = p_tNow - ((struct timeval*)m_oLastTx)->tv_sec; return (tDiff < (int)(GetPeriodMSec()/MILISEC_IN_SEC)) && !StaleData( tDiff ); }

	/// TODO: TBD: can we use nStaleLimit for both lease stale detection and DMO_CONTRACT_BANDWIDTH?
	inline bool StaleData( time_t p_tDiff ) const { return p_tDiff > nStaleLimit; };
}lease;

/// TODO: change lease list to a multimap with key (netAddr/TSAPID) and data the rest of lease details
/// @see comments in CPublishSubscribeService::UpdateSubscriber
typedef std::list<lease*> LeaseList;

/// Timeout waiting for a pending lease to complete. It's about twice the lifetime of an ISA packet
/// @see ContractNotification, this is no longer necessary and probaly dead code if ISA stack behaves and return timeout on all failed contract requests
#define PENDING_LEASE_TIMEOUT 300

////////////////////////////////////////////////////////////////////////////////
/// @class CLeaseMgmtService
/// @brief Lease Management Service
/// @remarks Lease Management serrvices does not use Tracker: contract requests are done trough DMAP
/// which will call CLeaseMgmtService::ContractNotification trough a wrapper in porting.cpp: ContractNotification
/// to indicate contract create/erase
///
/// This class does not have standard ISA -> GW flow: it does not receive APDU/timeouts;
/// (ProcessAPDU / ProcessISATimeout) are NOT used by the lease manager therefore
/// the class does not implement overrides
/// @see Service.h and porting.cpp for details
////////////////////////////////////////////////////////////////////////////////
class CLeaseMgmtService :public CService
{
	/// @brief Lease Management REQ parameters for 0-None protocol type and 3-"Subscriber lease" type also valid for 0-Client lease type
	typedef struct {
		uint8	m_ucTransferMode;	///< also valid for 0-Client lease type and 0-None protocol type
		uint8	m_ucUpdatePolicy;
		short 	m_shPeriod_or_CommittedBurst;	///< NETWORK ORDER periodic: period. aperiodic: committed_burst Positive number 'x' means 'x seconds' Negative number '-x' means '1/x seconds'.
		uint8	m_ucPhase;
		uint8	m_ucStaleLimit;
		uint16	m_u16ConnectionInfoLen;
		void NTOH () 
		{	m_shPeriod_or_CommittedBurst = ntohs(m_shPeriod_or_CommittedBurst);
			m_u16ConnectionInfoLen       = ntohs(m_u16ConnectionInfoLen);
		};
	}__attribute__ ((packed)) TLeaseMgmtReqData_Parameters;

	/// @brief YGSAP Lease Management REQ data, except data CRC
	struct YLeaseMgmtReqData{
		uint32_t 	m_nLeaseID;			///< 		NETWORK ORDER
		uint32_t 	m_nLeasePeriod;		///< 		NETWORK ORDER
		uint8_t		m_ucLeaseType;		///<		LeaseTypes
		uint8_t		m_ucProtocolType;	///<		ProtocolTypes
		uint8_t		m_ucNumberOfEndpoints;	///< must be 1

		//GSAP/YGSAP difference starts here
		uint8_t		m_ucTargetSelector;		//0 - Device Tag 1 - Network Address
		uint8_t		m_unDeviceTagSize;		// 0-16
		uint8_t		m_aucDeviceTag[16];		// considered only for m_ucTargetSelector 0
		uint8_t		m_aucEndpointAddr[16];
		uint16_t	m_ushEndpointTSAPID;	///< 		NETWORK ORDER
		uint16_t	m_ushEndpointOID;		///< 		NETWORK ORDER
		uint16_t	m_ushLocalTSAPID;		///< 		NETWORK ORDER
		uint16_t	m_ushLocalOID;			///< 		NETWORK ORDER

		TLeaseMgmtReqData_Parameters m_stParameters;	/// Lease Management REQ parameters for 3-Subscriber lease type, also valid for 0-Client lease type
		/// 0-Client lease type and 0-None protocol type : uint8	m_ucTransferMode
		/// 3-Subscriber lease type and 0-None protocol type: m_stParameters
		/// 4-Bulk transfer client lease type and 0-None protocol type: No bytes will be present.
		///	5-Bulk transfer server lease type and 0-None protocol type: No bytes will be present.
		void NTOH( void ) 
		{	m_nLeaseID = ntohl(m_nLeaseID); m_nLeasePeriod = ntohl(m_nLeasePeriod);
			m_ushEndpointTSAPID = ntohs(m_ushEndpointTSAPID); m_ushEndpointOID = ntohs(m_ushEndpointOID);
			m_ushLocalTSAPID = ntohs(m_ushLocalTSAPID); m_ushLocalOID = ntohs(m_ushLocalOID);
			m_stParameters.NTOH(); 
		};
		bool IsTypeClient(void)     { return (LEASE_CLIENT==m_ucLeaseType) && (PROTO_NONE==m_ucProtocolType); }
		bool IsTypeSubscriber(void) { return (LEASE_SUBSCRIBER==m_ucLeaseType) && (PROTO_NONE==m_ucProtocolType); }
	}__attribute__ ((packed));

public:	

	/// @brief Lease Management REQ data, except data CRC
	typedef struct	{
		uint32_t 	m_nLeaseID;			///< 		NETWORK ORDER
		uint32_t 	m_nLeasePeriod;		///< 		NETWORK ORDER
		uint8_t		m_ucLeaseType;		///<		LeaseTypes
		uint8_t		m_ucProtocolType;	///<		ProtocolTypes
		uint8_t		m_ucNumberOfEndpoints;	///< must be 1

		//GSAP/YGSAP difference starts here
		uint8_t		m_aucEndpointAddr[16];
		uint16_t	m_ushEndpointTSAPID;	///< 		NETWORK ORDER
		//uint16	m_ushEndpointPort;	///< 		NETWORK ORDER
		uint16_t	m_ushEndpointOID;	///< 		NETWORK ORDER
		uint16_t	m_ushLocalTSAPID;	///< 		NETWORK ORDER
		//uint16	m_ushEndpointPort;	///< 		NETWORK ORDER
		uint16_t	m_ushLocalOID;	///< 		NETWORK ORDER

		TLeaseMgmtReqData_Parameters m_stParameters;	/// Lease Management REQ parameters for 3-"Subscriber lease" type, also valid for 0-Client lease type
		/// 0-Client lease type and 0-None protocol type : uint8	m_ucTransferMode
		/// 3-Subscriber lease type and 0-None protocol type: m_stParameters
		/// 4-Bulk transfer client lease type and 0-None protocol type: No bytes will be present.
		///	5-Bulk transfer server lease type and 0-None protocol type: No bytes will be present.
		void NTOH( void ) 
		{	m_nLeaseID = ntohl(m_nLeaseID); m_nLeasePeriod = ntohl(m_nLeasePeriod);
			m_ushEndpointTSAPID = ntohs(m_ushEndpointTSAPID); m_ushEndpointOID = ntohs(m_ushEndpointOID);
			m_ushLocalTSAPID = ntohs(m_ushLocalTSAPID); m_ushLocalOID = ntohs(m_ushLocalOID);
			m_stParameters.NTOH(); 
		};
		bool IsTypeClient(void)     { return (LEASE_CLIENT==m_ucLeaseType) && (PROTO_NONE==m_ucProtocolType); }
		bool IsTypeSubscriber(void) { return (LEASE_SUBSCRIBER==m_ucLeaseType) && (PROTO_NONE==m_ucProtocolType); }
	}__attribute__ ((packed)) TLeaseMgmtReqData;
	
	/// @brief Lease Management CONFIRM data, except data CRC
	typedef struct	{
		byte 	m_ucStatus;			///< GS_Status
		uint32 	m_nLeaseID;			///< 		NETWORK ORDER
		uint32 	m_nLeasePeriod;		///< 		NETWORK ORDER
		void HTON( void ){ 	m_nLeaseID = htonl(m_nLeaseID);	m_nLeasePeriod = htonl(m_nLeasePeriod);};
	}__attribute__ ((packed)) TLeaseMgmtConfirmData;

	CLeaseMgmtService( const char * p_szName );
	/// ProcessAPDU / ProcessISATimeout are NOT used by the lease manager.
	/// No need to implement overrides: DMAP will call ContractNotification trough a wrapper in porting.cpp

	/// Return true if the service is able to handle the service type received as parameter
	/// Used by (USER -> GW) flow
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;
	
	/// (USER -> GW) Process a user request
	/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
	/// p_pHdr members are converted to host order. Header and data CRC are verified.
	/// p_pData members are network order, must be converted by ProcessUserRequest
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );
	
	/// Process session delete (expire or explicit delete) - deleting associated leases
	/// Only some derived classess will implement, the rest will use base class implementation which does nothing
	/// Identified services using it: CSystemReportService
	virtual void ProcessSessionDelete( unsigned  p_nSessionID );

	/// Called on USR2: Dump status to LOG
	virtual void Dump( void );

	/// Notifies us of the asynchronous completion of the resources allocation (contracts' establishment/deletion)
	void ContractNotification( uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pContract);

	/// On session update (renewal) - update expiration on associated leases, only if expiration get shorter
	void ProcessSessionUpdate( unsigned p_nSessionID, time_t p_nExpTime );

	lease *FindLease( unsigned p_nLeaseId, unsigned p_nSessionID = 0);
	
	/// find lease that match the request id 
	lease *FindLease( LeaseTypes p_eLeaseType, const uint8* p_pPeerIPv6, 
							uint16 p_u16PeerTSapID, uint16 p_u16PeerObjectID, uint16 p_u16LocalTSapID, uint16 p_u16LocalObjectID );

	/// (ISA -> GW) Add to publish cache pointers to subscriber leases matching the pair p_pNetAddr / p_ushTSAPID
	void AddSubscribersToPublishCache( uint8_t * p_pNetAddr, uint16_t p_ushTSAPID, LeaseList * p_pSubscriberLeaseList );

	/// Expire active/pending leases. Pending leases expire if they don't get contract back in 2 minutes
	/// Active leases expire when their period ends
	/// must be called periodically
	void CheckExpireLeases( time_t p_tNow );

	/// Delete all leases on unjoin. Currently does nothing on join
	void OnJoin( bool p_bJoined );

	private:
	bool validateUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pReq );

	/// Walks the list of active leases and returns the wanted one or NULL
	LeaseList::iterator findLease( unsigned p_nLeaseId, unsigned p_nSessionID =0 );
	
	/// Creates a new lease for a specific session and requests a new contract from the DMAP if necessary. May call leaseConfirm
	bool createLease( uint8_t p_ucVersion,
					unsigned p_nSessionID,
					unsigned p_unTransactionID,
					void * p_pReq, 
					uint32_t p_u32ReqLen);

	/// Deletes a lease.
	/// If requested and necessary, also ask DMAP to delete the underlying contract
	/// If requested and necessary, store the lease in m_lstLeasesPendingDelete to be deleted on contract delete notification
	/// return iterator to next list element, to help parsing
	LeaseList::iterator deleteActiveLease( LeaseList::iterator p_itLease, bool p_bDeleteContract, bool * p_pbPendingContractDelete = NULL );

	/// Deletes a subscriber lease. Return iterator to next list element, to help parsing
	LeaseList::iterator deleteActiveSubscriberLease( LeaseList::iterator p_itLease );
	
	/// Renews a lease, within the session time boundary. Return (Y)lease status 
	byte renewLease( uint8_t p_ucVersion, LeaseList::iterator &p_itLease, unsigned long p_nLeasePeriod );

	/// (GW -> User) send G_LeaseConfirm back to the user
	bool leaseConfirm( unsigned p_nSessionID, unsigned p_unTransactionID, byte p_ucStatus, unsigned p_nLeaseId, unsigned long p_nLeasePeriod );

	const char * getLeaseTypeName( uint8 p_ucLeaseType );
	
	LeaseList		m_lstLeasesInProgress;		///< list of leases undergoing establishment
	LeaseList	 	m_lstActiveLeases;			///< list of established leases, other than subscriber leases
	LeaseList	 	m_lstSubscriberLeases;		///< list of established leases, other than subscriber leases
	LeaseList		m_lstLeasesPendingDelete;	///< list of leases waiting contract delete notification (TERMINATE contract was sent for them)
	unsigned long 	m_nLeaseIndTop; 			///< the lease ID generator
};

#define GET_NEW_ID(_id_top_) ( (!_id_top_) && (++_id_top_), _id_top_++)
#define GET_NEW_LEASE_ID GET_NEW_ID(m_nLeaseIndTop)

#endif	//LEASE_MGMT_SVC_H
