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
/// @file SystemReportSvc.h
/// @author Marcel Ionescu
/// @brief system Report services GSAP - interface
////////////////////////////////////////////////////////////////////////////////
#ifndef SYSTEM_REPORT_SERVICE_H
#define SYSTEM_REPORT_SERVICE_H

#include <list>
#include <arpa/inet.h>	//ntoh/hton
#include <stdlib.h>	//size_t
#include <time.h> 	//time_t, time()

#include "../../Shared/MicroSec.h"

#include "Service.h"

/// SystemReportStatus
#define SYSTEM_REPORT_SUCCESS	0
#define SYSTEM_REPORT_FAILURE	1

/// System Reports Failure Respose Size without CRC

#define DEV_LIST_FAIL_RES_SIZE		3	/// Dev List:		Status(1), numberOfDevices(2)
#define TOPOLOGY_FAIL_RES_SIZE		5 	/// Topology : 		Status(1), numberOfDevices(2), numberOfBackbones(2)
#define SCHEDULE_FAIL_RES_SIZE		4	/// Schedule : 		Status(1), numberOfChannels(1), numberOfDevices(2)
#define DEVICE_HEALTH_FAIL_RES_SIZE	3	/// Device Health : 	Status(1), numberOfDevices(2)
#define NEIGHBOR_HEALTH_FAIL_RES_SIZE	3	/// Neighbor Health :	Status(1), numberOfNeighbors(2)
#define NETWORK_HEALTH_FAIL_RES_SIZE	37	/// Network Health : 	Status(1), networkHealth(34), numberOfDevices(2)
#define NETWORK_RESOURCE_FAIL_RES_SIZE	2	/// Network Resource : 	Status(1), numberOfSubnets(1)

#define SYS_REP_FAIL_RES_MAX_SIZE	NETWORK_HEALTH_FAIL_RES_SIZE	

/// @see dmap.h for SM_SMO_OBJ_ID
/// Nivis object SMO (SM_SMO_OBJ_ID) methods used by the GW
/// OBSOLETE #define SMO_METHOD_GENERATE_TOPOLOGY	6
/// OBSOLETE #define SMO_METHOD_GET_TOPOLOGY_BLOCK	7
/// OBSOLETE #define SMO_METHOD_GENERATE_DEVICE_LIST	8
/// OBSOLETE #define SMO_METHOD_GET_DEVICE_LIST_BLOCK	9
#define SMO_METHOD_GENERATE_REPORT			13
#define SMO_METHOD_GET_BLOCK				14
#define SMO_METHOD_GET_CONTRACTS_AND_ROUTES	15

////////////////////////////////////////////////////////////////////////////////
/// @class CSystemReportService
/// @brief System Report services
/// @see Service.h for details
////////////////////////////////////////////////////////////////////////////////
class CSystemReportService : public CService{

		////////////////////////////////////////////////////////////////////////////
	/// @brief GW/SM data structures
	////////////////////////////////////////////////////////////////////////////

	/// @brief Data for requests SMO.generateReport
	/// @var m_ushReportsRequested bitmap:
	/// 	Bit 0: request Dev List
	/// 	Bit 1: request Topology
	/// 	Bit 2: request Schedule
	/// 	Bit 3: request Device Health
	/// 	Bit 4: request Neighbor Health - if set, deviceNeighborHealth must be also set
	/// 	Bit 5: request Network Health
	/// 	Bit 6: request Network Resource
	typedef struct {
		uint16 m_ushReportsRequested; ///< @see the bitfield definition above
		/// device for which Neighbor Health is requested
		/// Must be set if reportsRequested & 0x0010 is true
		byte m_aucDeviceNeighborHealth_NetworkAddress[ 16 ] ;
		void HTON( void ) { m_ushReportsRequested = htons(m_ushReportsRequested); };
	} __attribute__ ((packed)) TGenerateReportCmd;

	/// @brief Response to SMO.generateReport
	typedef struct
	{	/// all members are NETWORK ORDER
		uint32 m_unMaxBlockSize;			///< common to all reports
		uint32 m_unDeviceListSize;			///< 0 if device list was not requested
		uint32 m_unDeviceListHandler;		///< 0 if device list was not requested
		uint32 m_unTopologySize;			///< 0 if topology was not requested
		uint32 m_unTopologyHandler;			///< 0 if topology was not requested
		uint32 m_unScheduleSize;			///< 0 if schedule was not requested
		uint32 m_unScheduleHandler;			///< 0 if schedule was not requested
		uint32 m_unDeviceHealthSize;		///< 0 if device health was not requested
		uint32 m_unDeviceHealthHandler;		///< 0 if device health was not requested
		uint32 m_unNeighborHealthSize;		///< 0 if neighbor health was not requested
		uint32 m_unNeighborHealthHandler;	///< 0 if neighbor health was not requested
		uint32 m_unNetworkHealthSize;		///< 0 if network health was not requested
		uint32 m_unNetworkHealthHandler;	///< 0 if network health was not requested
		uint32 m_unNetworkResourceSize;		///< 0 if network health was not requested
		uint32 m_unNetworkResourceHandler;	///< 0 if network health was not requested

		void NTOH( void ){ m_unMaxBlockSize = ntohl(m_unMaxBlockSize);
			m_unDeviceListSize		= ntohl(m_unDeviceListSize);	m_unDeviceListHandler	= ntohl(m_unDeviceListHandler);
			m_unTopologySize		= ntohl(m_unTopologySize);		m_unTopologyHandler		= ntohl(m_unTopologyHandler);
			m_unScheduleSize		= ntohl(m_unScheduleSize);		m_unScheduleHandler		= ntohl(m_unScheduleHandler);
			m_unDeviceHealthSize	= ntohl(m_unDeviceHealthSize);	m_unDeviceHealthHandler	= ntohl(m_unDeviceHealthHandler);
			m_unNeighborHealthSize	= ntohl(m_unNeighborHealthSize);m_unNeighborHealthHandler= ntohl(m_unNeighborHealthHandler);
			m_unNetworkHealthSize	= ntohl(m_unNetworkHealthSize);	m_unNetworkHealthHandler= ntohl(m_unNetworkHealthHandler);
			m_unNetworkResourceSize	= ntohl(m_unNetworkResourceSize);	m_unNetworkResourceHandler= ntohl(m_unNetworkResourceHandler);
		};
		inline uint32	ReportSize(    uint8 p_ucServiceType ) const;
		inline uint32	ReportHandler( uint8 p_ucServiceType ) const;
	} __attribute__ ((packed)) TGenerateReportRsp;

	/// @brief Data for requests SMO.getBlock
	typedef struct {
		uint32 handler;
		uint32 offset;
		uint16 size;
		void NTOH( void ){ handler = ntohl(handler); offset = ntohl(offset);  size = ntohs(size); };
		void HTON( void ){ handler = htonl(handler); offset = htonl(offset);  size = htons(size); };
	} __attribute__((packed)) TSMOGetBlockParameters;

	/// @brief Pending requests
	struct PendingRequest{
		unsigned	m_nSessionID;
		uint32		m_dwTransactionID;
		uint32		m_dwReqDataLen;
		byte *		m_pRequestData;	/// needed by SCHEDULE_REPORT/DEVICE_HEALTH_REPORT/NEIGHBOR_HEALTH_REPORT
		PendingRequest( unsigned p_nSessionID, uint32 p_dwTransactionID,  byte * p_pRequestData, uint32 p_dwReqDataLen )
			:m_nSessionID(p_nSessionID), m_dwTransactionID(p_dwTransactionID), m_dwReqDataLen(p_dwReqDataLen)
			{ 	if(p_dwReqDataLen) { m_pRequestData = new byte[ p_dwReqDataLen ]; memcpy( m_pRequestData, p_pRequestData, p_dwReqDataLen ); }
				else               { m_pRequestData = NULL; }
			};
		PendingRequest( const PendingRequest& r )
			:m_nSessionID(r.m_nSessionID), m_dwTransactionID(r.m_dwTransactionID), m_dwReqDataLen(r.m_dwReqDataLen)
			{ 	if(r.m_dwReqDataLen) { m_pRequestData = new byte[ r.m_dwReqDataLen ]; memcpy( m_pRequestData, r.m_pRequestData, r.m_dwReqDataLen ); }
				else                 { m_pRequestData = NULL;}
			};
		~PendingRequest( void ){ if(m_pRequestData) delete [] m_pRequestData; };
	};
	/// A request which cannot be sent because a request of the same type but with different parameters is pending
	/// Used for DEVICE_HEALTH_REPORT/NEIGHBOR_HEALTH_REPORT only
	struct WaitingRequest{	/// an expire mechanism is not necessary because we inwoke sendNextFromPipeline on ProcessISATimeout (we use ASL timeout)
		CGSAP::TGSAPHdr m_oHdr;
		byte * 			m_pData;

		WaitingRequest( const CGSAP::TGSAPHdr * p_pHdr, const void * p_pData)
		:m_oHdr( *p_pHdr )
		{	if(m_oHdr.m_nDataSize){ m_pData = new byte[ m_oHdr.m_nDataSize ]; memcpy( m_pData, p_pData, m_oHdr.m_nDataSize); }
			else                  { m_pData = NULL; }
		}
		WaitingRequest( const WaitingRequest& r)
		: m_oHdr( r.m_oHdr )
		{	if(m_oHdr.m_nDataSize){ m_pData = new byte[ m_oHdr.m_nDataSize ]; memcpy( m_pData, r.m_pData, m_oHdr.m_nDataSize); }
			else                  { m_pData = NULL; }
		}
		~WaitingRequest( void ) { if(m_pData) delete m_pData; };
	};

	typedef std::list<PendingRequest> TRequestList;
	typedef std::list<WaitingRequest> TRequestPipeline;

	/// @brief Report structure - all data for a specific report
	struct Report{
		uint32_t		m_nSessionID;
		uint32_t		m_unHandler;
		uint32_t		m_unSize;
		byte *			m_pBin;
		uint32_t		m_unReceived;    		///< size of partially received report
		CMicroSec 		m_uSecGen;				///< Time when cached report was generated. Used to measure RTT and freshness
		bool			m_bRequestPending;		///< true if requests are pending cannot use m_lstPendingRequests.empty() because the first req is not put in the pending list
		TRequestList	m_lstPendingRequests;	///< List with pending request
		byte *			m_pExtraData;			///< additional data: the request details, needed by DEVICE_LIST_REPORT(YGSAP)/SCHEDULE_REPORT/DEVICE_HEALTH_REPORT/NEIGHBOR_HEALTH_REPORT
		Report( void ) :m_nSessionID(0), m_unHandler(0), m_unSize(0), m_pBin(NULL), m_unReceived(0), m_bRequestPending(false), m_pExtraData(NULL) {};
		//~Report( void ) { m_lstPendingRequests.clear(); if(m_pBin) delete m_pBin; if(m_pExtraData) delete m_pExtraData; };

		/// Return the size of the next block
		size_t nextBlockSize( uint32 p_dwMaxBlockSize ) const
			{ return ((m_unSize - m_unReceived) < p_dwMaxBlockSize) ? (m_unSize - m_unReceived): p_dwMaxBlockSize; };
		//void Dump( void ) const; do not delete.
	};

	enum SRVC_IDX { SRVC_IDX_DEVICE_LIST, SRVC_IDX_TOPOLOGY, SRVC_IDX_SCHEDULE, SRVC_IDX_DEVICE_HEALTH, SRVC_IDX_NEIGHBOR_HEALTH, SRVC_IDX_NETWORK_HEALTH, SRVC_IDX_NETWORK_RESOURCE, SRVC_IDX_MAX };

	Report	m_aReport[ SRVC_IDX_MAX + 1 ];	///< The last one is only to avoid segfaults on incorrect idx() calls

	/// NEIGHBOR_HEALTH_REPORT(/DEVICE_HEALTH_REPORT?) request pipeline - requests
	/// not issued yet because a requets of the same type is pending already
	TRequestPipeline m_lstRequestPipeline;

	uint32	m_dwMaxBlockSize;	/// Max block size, negociated with SM
	uint16	m_unSysMngContractID;

	/// (USER -> GW) find the index in m_aReport function by GSAP ServiceType
	SRVC_IDX idx( byte p_ucServiceType ) const;

	bool isValid( byte p_ucServiceType, time_t p_tLifetime = 60 ) const
	{	const Report * pRep = &m_aReport[ idx(p_ucServiceType) ];
		return	pRep->m_unReceived											// non-empty
			&&	(pRep->m_unReceived == pRep->m_unSize)						// complete
			&&  ( (time_t)pRep->m_uSecGen.GetElapsedSec() <= p_tLifetime);  // fresh
	};

	/// Return the report age
	time_t age( byte p_ucServiceType ) const { return (time_t)m_aReport[ idx( p_ucServiceType ) ].m_uSecGen.GetElapsedSec(); };

	/// Return number of packets based on the size
	size_t packets( uint32 p_unSize ) const
	{	return 	 (p_unSize / m_dwMaxBlockSize) +
				((p_unSize % m_dwMaxBlockSize) ? 1 : 0);
	};

	size_t minConfirmDataSize( byte p_ucServiceType ) const;

	/// True if a report transfer from SM is in progress
	/// @note This method does not match the reqest details like network address
	bool isRequestPending( byte p_ucServiceType );

	/// (USER -> GW) Format the report for specific request details - extract only request devices from the report with all devices
	bool formatReport( uint8 p_ucServiceType, const byte *& p_pWholeReport, uint32& p_dwWholeReportLen, const byte * p_pRequestDetails, byte * p_pFormattedReport );

	/// (USER -> GW) Format the DEVICE_LIST_REPORT report for specific request details - extract one or all devices from the report with all devices
	bool formatReportDeviceList( const byte *& p_pWholeReport, uint32& p_dwWholeReportLen, const byte * p_pRequestDetails, byte * p_pFormattedReport );

	/// (USER -> GW) Format the SCHEDULE report for specific request details - extract only request devices from the report with all devices
	bool formatReportSchedule(                const byte *& p_pWholeReport, uint32& p_dwWholeReportLen, const byte * p_pRequestDetails, byte * p_pFormattedReport );

	/// (USER -> GW) Format the DEVICE_HEALTH_REPORT report for specific request details - extract only request devices from the report with all devices
	bool formatReportDeviceHealth(            const byte *& p_pWholeReport, uint32& p_dwWholeReportLen, const byte * p_pRequestDetails, byte * p_pFormattedReport );

	/// (USER -> GW) Verify the user request: service type and data size. if necessary, does NTOH()
	bool validateUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );

	/// (USER -> GW) Translate YGSAP tag into device address
	int translateYGSAP( CGSAP::TGSAPHdr * p_pHdr, const void * p_pData, byte ** pReqTranslatedData );

	/// (USER -> GW) Atempt to return the report from cache
	bool returnFromCache( CGSAP::TGSAPHdr * p_pHdr, const void * p_pData );

	/// (USER -> GW) Match REQUEST and CACHE extra data. mismatch may happen only on DEVICE_HEALTH_REPORT / NEIGHBOR_HEALTH_REPORT
	bool matchRequest2CacheData( byte p_ucServiceType, const void * p_pData);

	/// (ISA100 -> GW) send the report confirm back to all matching requests from the pipeline,
	/// to avoid an unnecessary round trip to SM
	void sendConfirmToAllInPipeline( uint8 p_ucServiceType );

	/// (ISA100 -> GW) Verify the APDU. When request of type p_ucServiceType is fully received,
	/// send next request of the same type from request pipeline.
	void processPipeline( uint8 p_ucServiceType );

	/// (ISA100 -> GW) Verify the APDU
	bool validateAPDU( uint16 p_unAppHandle, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );

	/// (GW -> ISA100) Send request to SM: SMO.generateReport
	bool requestReport( const CGSAP::TGSAPHdr * p_pHdr, const void * p_pRequestData );

	/// (ISA100 -> GW) when receiving a response to SMO.generateReport start the report transfer
	bool newReport( uint16 p_unAppHandle, byte p_ucServiceType , uint32 p_unSize, uint32 p_unHandler );

	/// (GW -> ISA100) Make a request to SM: SMO.getDevListBlock to get a block from a service
	bool reqReportBlock( uint16 p_unAppHandle,  byte p_ucServiceType );

	void clean( byte p_ucServiceType, Report * p_pRep  );

	/// (ISA100 -> GW) Add a packet to report stored locally
	bool addReportBlock( byte p_ucServiceType, uint16 p_unAppHandle, uint32 p_unHandler, uint32 p_unOffset, uint16 p_ushPacketSize, void * p_pData );

	/// Send confirm device list/device list on connection GSAP, determined from session id, if connection is valid
	bool confirm( byte p_ucServiceType, unsigned p_nSessionID, uint32 p_TransactionID, byte * p_pBin, uint32 p_unSize, bool p_bRemoveFromTracker );

	/// Send confirm device list/topology on connection GSAP determined from p_unAppHandle, if connection is valid
	bool confirm( uint16 p_unAppHandle, byte * p_pBin, uint32 p_unSize );

	/// Send a confirm with status SYSTEM_REPORT_FAILURE to client specified by p_unAppHandle
	/// Session/transaction/service type are taken from tracker entry pointed by p_unAppHandle
	void confirm_Error( byte p_ucServiceType, uint16 p_unAppHandle );

	/// Send a confirm with type p_ucServiceType and status YGS_FAILURE or SYSTEM_REPORT_FAILURE to client specified by p_nSessionID
	void confirm_Error( byte p_ucServiceType, unsigned p_nSessionID, uint32 p_TransactionID );

	/// Send a confirm with type p_ucServiceType and status p_ucStatus (MUST be error) to client specified by p_nSessionID
	void confirm_Error( byte p_ucServiceType, unsigned p_nSessionID, uint32 p_TransactionID, uint8_t p_ucStatus );

	/// Send the report received as parameter to ALL clients with pending requests including the first caller
	bool confirm2All( uint16 p_unAppHandle );

	/// Send the device list CONFIRM with SYSTEM_REPORT_FAILURE to ALL clients with pending requests
	/// including the first caller - specified by p_unAppHandle
	bool confirm2All_Error( uint8 p_ucServiceType, uint16 p_unAppHandle );

	typedef void (CSystemReportService::*TPFunc3)(byte p_ucServiceType, uint32 p_p1, uint32 p_p2, uint32 p_p3 );

	void reportFunc_clean( byte p_ucServiceType, uint32 /*p_p1*/, uint32 /*p_p2*/, uint32 /*p_p3*/ );
	void reportFunc_dump(  byte p_ucServiceType, uint32 /*p_p1*/, uint32 /*p_p2*/, uint32 /*p_p3*/ );
	void reportFunc_reset( byte p_ucServiceType, uint32 p_Bitmap, uint32 p_nSessionID, uint32 /*p_p3*/ );
	void reportFunc_sessionDelete( byte p_ucServiceType, uint32 p_nSessionID, uint32 /*p_p2*/, uint32 /*p_p3*/);
	void reportFunc_newReport( byte p_ucServiceType, uint32 p_unAppHandle, uint32 p_p2 /*TGenerateReportRsp **/, uint32 p_p3 /* MSG* */ );

	/// Iterates trough all reports and call a TPFunc3 for each
	void for_each_report( TPFunc3 p_pFunc, uint32 p_p1 = 0, uint32 p_p2 = 0, uint32 p_p3 = 0 );

	/// Return report details - number of devices in most cases
	const char * details( byte p_ucServiceType  ) const;
	/// Display the whole report
	bool dumpReport( byte p_ucServiceType ) const;

	void dumpPipeline( void );

	/// Update (fill) GPDU statistics into NETWORK_HEALTH_REPORT
	void fillGPDUStats( uint8_t* p_pData, size_t p_unSize );
public:
	CSystemReportService( const char * p_szName ) :CService(p_szName) { m_dwMaxBlockSize = 0; /*memset( m_aReport, 0, sizeof(m_aReport));*/ };
	~CSystemReportService()
	{
		for ( int i=0; i< SRVC_IDX_MAX + 1; ++i)
		{
			delete [] m_aReport[i].m_pBin ;
			delete [] m_aReport[i].m_pExtraData ;
		}
	}

	/// Return true if the service is able to handle the service type received as parameter
	/// Used by (USER -> GW) flow
	virtual bool CanHandleServiceType( uint8 p_ucServiceType ) const;

	/// (USER -> GW) Process a user request
	/// CALL on user requests (The method is called from CInterfaceObjMgr::DispatchUserRequest)
	virtual bool ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData );

	/// (ISA -> GW) Dispatch an ADPU. Call from CInterfaceObjMgr::DispatchAPDU
	virtual bool ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq );

	/// (ISA -> GW) Process a ISA100 timeout. Call from CInterfaceObjMgr::DispatchISATimeout
	virtual bool ProcessISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq );

	/// Called on USR2: Dump status to LOG
	virtual void Dump( void );

	/// Clean up pending requests generated on this session
	void ProcessSessionDelete( unsigned  p_nSessionID );

	void SetSMContractID( uint16 p_unSysMngContractID = INVALID_CONTRACTID );

	inline uint16 SMContractID( void ) const { return m_unSysMngContractID; };

	/// for YGSAP
	bool RequestDeviceList( void );
};

#endif //SYSTEM_REPORT_SERVICE_H
