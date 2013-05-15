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
/// @file GwUAP.h
/// @author Marcel Ionescu
/// @brief The Gateway UAP
////////////////////////////////////////////////////////////////////////////////
#ifndef GWUAP_H
#define GWUAP_H

#include "../ISA100/porting.h"

#include "InterfaceObjMgr.h"
#include "MsgTracker.h"
#include "GSAPList.h"	///< Client connection list
#include "Isa100Object.h"
#include "TunnelObject.h"
#include "GwUtil.h"
#include "SAPStruct.h"
#include "YDevList.h"

/// *Service classes 
#include "AlertSvc.h"
#include "BulkTransferSvc.h"
#include "ClientServerSvc.h"
#include "GwMgmtSvc.h"
#include "LeaseMgmtSvc.h"
#include "PublishSubscribeSvc.h"
#include "SystemReportSvc.h"
#include "SessionMgmtSvc.h"
#include "ServerService.h"
#include "TimeSvc.h"
#include "BulkTransferSvc.h"
#include "PublishSubscribeSvc.h"

/// @brief Return two strings fitted for printf("%s%s"), hex representation and indication of incomplete data
/// @param _p_ Pointer to data
/// @param _l_ Data length
/// @param _m_ Max bytes to return
/// @remarks First string is the hex representation of _p_, limited to _m_
/// @remarks Second string is empty if whole string was represented (_l_ <= _m_), or ".." if _p_ is represented incomplete (_l_>_m_)
/// @note Use it exclusively in *printf with "%s%s" format
#define GET_HEX_LIMITED(_ptr_, _len_, _m_) GetHex(_ptr_, _Min(_len_, _m_)), (((_len_)>(_m_)) ? ".." : "")

/// Gateway User Application Process
class CGwUAP
{
	///UAP management object attributes:
	CInterfaceObjMgr 			m_oIfaceOIDMgr;		///< Interface object manager
	CBulkTransferService		m_oBulkTransferSvc;	///< Bulk Tranfer GSAP worker
	CClientServerService		m_oClientServerSvc;	///< ClientServer GSAP worker
	CGatewayMgmtService			m_oGwMgmtSvc;		///< Gateway Management GSAP worker
	CLeaseMgmtService			m_oLeaseMgmtSvc;	///< Lease management service
	CPublishSubscribeService	m_oPublishSvc;		///< Publish GSAP worker
	CSessionMgmtService			m_oSessMgmtSvc;		///< Session management service
	CSystemReportService		m_oSystemReportSvc;	///< SystemReport GSAP worker
	CTimeService				m_oTimeSvc;			///< CTimeService will not be implemented
	CServerService				m_oServerSvc;		///< CServerService GSAP worker
	CAlertService				m_oAlertSvc;		///< Alert GSAP worker

	/// CGSAP list (client connections). Suitable for multiple protocol translators,
	/// of same or different types, each handling communication with one client
	CGSAPList 	m_oGSAPLst;		///< GSAP manager. TODO: should rename to CGSAPMgr

	CIsa100ObjectsMap	m_oIsa100ObjectsMap;
	CYDevList			m_oYDevList;	///< map tag->Addr

	/// Request identifier: passed to ASLSRVC_AddGenericObject, received back trough ASLDE_SearchOriginalRequest / UAP_DataTimeout
	uint16	m_ushAppHandle;

	/// Transaction generator, used when sending INDICATION messages to the user, received back in user CONFIRM
	/// Must be in range 0x80000000 and 0xffffffff
	uint32 m_unTransactionID;
	
	uint32 m_unJoinCount;	///< Number of join notifications
	uint32 m_unUnjoinCount;	///< Number of unjoin notifications. Greater than m_unJoinCount
	bool   m_bJoined;		///< Join status
	
	struct timeval m_tvLastRxApduUTC;		/// Rx time for last APDU received. used to avoid gettimeofday

	time_t m_nLastSMContractRequestTime;	/// GW->SM contract maintenance
	time_t m_nLastDeviceListRequestTime;	/// device list polling for YGSAP
	time_t m_nLastContractListRequestTime;	/// contract list polling for GPDU* in Network Health Report
	time_t m_nLastGPDUGarbageCollectTime;	/// GPDU Statistics garbage collect interval
public:
	CMsgTracker m_tracker;		///< Associate ISA RequestID[1]+DeviceAddr[16] with GSAP SessionID[4]+TrackingID[4]

	CGwUAP();
	void Init( void );		///< Mandatory initialisations
	void GwUAP_Task( void );///< Worker function. This must be called from the main loop of the Gateway
	void Close(void);		///< Cleanup function

	/// (ISA -> GW) Check the session validity in both SessionMgr and GSAP list. Return pointer to CGSAP if valid session is found, NULL otherwise
	/// @todo remove: Also resync session ID's in sess mgr and CGSAP list (delete)
	CGSAP * FindGSAP( uint32 p_nSessionID );

	/// (ISA -> GW)  Find the GSAP on which the request associated with p_unAppHandle was made
	CGSAP* FindGSAP( uint16 p_unAppHandle, MSG *& p_prMSG );
	
	inline void AddGSAP( CGSAP* p_pGSAP) { m_oGSAPLst.Add( p_pGSAP );}; ///< Add a new connection to m_oGSAPLst (just forward the call)

	/// (USER -> GW) Dispatch a user request (just forward the call)
	/// CALL on user requests
	inline bool DispatchUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
		{ return m_oIfaceOIDMgr.DispatchUserRequest( p_pHdr, p_pData ); };

	/// Spread the news about a new lease. For now, only to m_oPublishSvc and only about subscriber leases
	/// TODO analyse if DispatchLeaseAdd / OnLeaseAdd should be generalised to all services
	/// TODO Dispatch a lease add to all m_mapServices::CService* to allocate resources; Just a forwarder to m_oIfaceOIDMgr
	inline void DispatchLeaseAdd( lease* p_pLease )	{ m_oPublishSvc.OnLeaseAdd( p_pLease ); };

	/// Dispatch a lease deletion (explicit delete or expire) to all m_mapServices::CService* to free resources
	/// just a forwarder to m_oIfaceOIDMgr
	inline void DispatchLeaseDelete( lease* p_pLease )	{ m_oIfaceOIDMgr.DispatchLeaseDelete( p_pLease ); };
	
	/// (ISA -> GW) Dispatch a timeout
	void DispatchISATimeout( uint16 p_ushAppHandle );

	/// (DMAP -> GW) Dispatch a contract confirm (add/delete)
	void ContractNotification( uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pContract );

	/// (ISA -> GW) Join/Unjoin notification
	void OnJoin( bool p_bJoined );
		
	CIsa100Object::Ptr  GetTunnelObject (int p_nUapId, int p_nObjectId);

	/// (GW -> ISA) Generate a new application handle, send the request down ISA stack,
	/// add handle/session/transaction to tracker, return AppHhandle or 0 if it can't send to ASL
	/// Esentially: a wrapper for ASLSRVC_AddGenericObject + m_tracker.AddMessage/m_tracker.RenewMessage
	uint16 SendRequestToASL(void * p_pObj,
							uint8 p_ucObjectType,
							uint8 p_ucDstTSAPID,
							uint16 p_unContractID,
							uint32 m_nSessionID,
							uint32 m_unTransactionID,
							uint8 m_ucServiceType,
							uint16 p_unAppHandleOld = 0,
							uint8_t*	p_pucSFC  = NULL );

	/// (GW -> ISA) Search in tracker old AppHhandle, generate a new application handle, send the request down ISA stack,
	/// add new AppHhandle to tracker (preserve session/transaction), remove old AppHhandle from tracker,  return AppHhandle or 0 if it can't send to ASL
	/// Esentially: a wrapper for m_tracker.FindMessage + ASLSRVC_AddGenericObject + m_tracker.RenewMessage
	/// In other words: a wrapper for m_tracker.FindMessage + SendRequestToASL( * m_nSessionID, m_unTransactionID) (+ m_tracker.RenewMessage)
	uint16 SendRequestToASL(	void * p_pObj,
								uint8  p_ucObjectType,
								uint8  p_ucDstTSAPID,
								uint16 p_unContractID,
								uint16 p_unAppHandle );

	uint16 SendRequestToASL(	void *		p_pObj,
								uint8		p_ucObjectType,
								uint8		p_ucSrcTSAPID,
								uint8		p_ucDstTSAPID,
								uint16		p_unContractID,
								bool		p_bNoRspExpected,
								uint8_t*	p_pucSFC = NULL );

	void Dump( void );

	/// Dispatch a session delete to: CLeaseMgmtService, CSystemReportService, CMsgTracker
	void DispatchSessionDelete( unsigned  p_nSessionID )
	{	m_oSystemReportSvc.ProcessSessionDelete( p_nSessionID );
		m_oLeaseMgmtSvc.ProcessSessionDelete( p_nSessionID );
		m_tracker.ProcessSessionDelete( p_nSessionID );
	}

	/// Send confirm on SAP identified by p_nSessionID - usually needed by ProcessUserRequest
	bool SendConfirm( uint32 p_nSessionID, uint32 p_nTransactionID, uint8 p_ucServiceType, uint8* p_pMsgData, uint32 p_dwMsgDataLen, bool p_bRemoveFromTracker );

	/// Send confirm on SAP identified by p_unAppHandle - usually needed by ProcessAPDU
	bool SendConfirm( uint16 p_unAppHandle, uint8* p_pMsgData, uint32 p_dwMsgDataLen, bool p_bRemoveFromTracker );
			
	/// Application handle generator, used when adding messages to the stack.
	/// Guaranteed to return non-zero
	inline uint16 NextAppHandle( void ) { if( !(++m_ushAppHandle)) ++m_ushAppHandle; return m_ushAppHandle; };
	
	/// Transaction generator, used when sending INDICATION messages to the user.
	/// Guaranteed to return in range 0x80000000 and 0xffffffff
	inline uint32 NextTransactionId( void )
		{ if (++m_unTransactionID == 0xFFFFFFFF){ m_unTransactionID = 0x80000000;}; return m_unTransactionID; };

	/// Refresh the whole  device list m_mapDeviceList
	inline void YDevListRefresh( uint32_t p_unDevListReportSize, const SAPStruct::DeviceListRsp* p_pDevListReport )
		{ m_oYDevList.Refresh( p_unDevListReportSize, p_pDevListReport ); m_nLastDeviceListRequestTime = time( NULL );};

	/// Find a device tag in device list m_mapDeviceList and return device address
	inline uint8_t * YDevListFind( uint8_t p_uchDeviceTagSize, const uint8_t * p_aucDeviceTag )
		{ return m_oYDevList.Find( p_uchDeviceTagSize, p_aucDeviceTag ); };

	/// Find a device tag in device list m_mapDeviceList and return device address
	inline uint8_t * YDevListFind( YGSAP_IN::DeviceTag::Ptr p_pDeviceTag )
		{ return m_oYDevList.Find( p_pDeviceTag ); };

	inline uint16 SMContractID( void ) const { return m_oSystemReportSvc.SMContractID(); };

	/// Update publish interval for all devices
	/// Get data from CS SMO.GetContractsAndRoute (p_pCSData) and update it in publish cache
	void UpdatePublishRate( const CClientServerService::CSData * p_pCSData );

	inline bool GPDUStatsCompute( const uint8_t * p_pNetworkAddress, uint8_t& p_unGPDULatency, uint8_t& p_unGPDUPathReliability )
		{ return m_oPublishSvc.GPDUStatsCompute( p_pNetworkAddress, p_unGPDULatency, p_unGPDUPathReliability ); }

	/// Request contract list from SM for GS_GPDU_Latency/GS_GPDU_Data_Reliability parameters in network report
	inline 	void RequestContractList( void ) { return m_oClientServerSvc.RequestContractList(); }

	inline const struct timeval * GetLastRxApduUTC( void ) const { return &m_tvLastRxApduUTC; };

private:
	/// get an APDU from the ISA100 stack and dispatch it
	/// return true if the it should be call again (it has just processed a PUBLISH packet)
	bool processIsaPackets(int p_nUapId, int p_nTimeout );
	
	/// (GW -> DMAP) Request the GW-SM UAP contract to be used by topology and other system management services
	void requestSMContract( void );

	/// Manage GW/SM contract used by System Management Services
	void maintainSMContract( time_t p_tNow );

	///Maintain device lists system report used internally by YGSAP to map device tag to device address
	void maintainDeviceList( time_t p_tNow );

	/// Maintain contracts list used to fill GPDU* parameters in Network Health report
	void maintainContractList( time_t p_tNow );

	/// GPDU Statistics garbage collector
	void garbageCollectorGPDUStats( time_t p_tNow );
};

extern CLeaseMgmtService * 		g_pLeaseMgr;
extern CSessionMgmtService *	g_pSessionMgr;

#endif	//GWUAP_H
