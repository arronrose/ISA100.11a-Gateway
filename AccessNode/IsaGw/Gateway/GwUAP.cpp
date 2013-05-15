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
/// @author
/// @brief The Gateway UAP - implementation
////////////////////////////////////////////////////////////////////////////////

#include <arpa/inet.h>	// ntoh
#include "../../Shared/Utils.h"
#include "../../Shared/MicroSec.h"
#include "GwUAP.h"
#include "GwApp.h"	// g_stApp

/// UGLY. consider recode
CSessionMgmtService * g_pSessionMgr;	/// Used in CLeaseMgmtService
CLeaseMgmtService	* g_pLeaseMgr;


CGwUAP::CGwUAP()
:  m_oBulkTransferSvc( 	"Bulk Transfer Service" )
, m_oClientServerSvc( 	"Client/Server Service" )
, m_oGwMgmtSvc( 		"Gateway Management Service" )
, m_oLeaseMgmtSvc( 		"Lease Management Service" )
, m_oPublishSvc( 		"Publish Service" )
, m_oSystemReportSvc( 	"System Reports Service" )
, m_oTimeSvc( 			"Time Service" )
, m_oServerSvc (		"Server Service" )
, m_oAlertSvc( 			"Alert Service" )
, m_ushAppHandle(0)
, m_unTransactionID(0x80000000)

{
	m_unJoinCount = m_unUnjoinCount = 0;
	m_nLastSMContractRequestTime = m_nLastDeviceListRequestTime = m_nLastContractListRequestTime = m_nLastGPDUGarbageCollectTime = 0;
	g_pSessionMgr 	= &m_oSessMgmtSvc;
	g_pLeaseMgr		= &m_oLeaseMgmtSvc;
}

void CGwUAP::Init( void )
{
	m_oAlertSvc.SetInterfaceOID(		m_oIfaceOIDMgr.AllocateOID( &m_oAlertSvc ) );
	m_oBulkTransferSvc.SetInterfaceOID(	m_oIfaceOIDMgr.AllocateOID( &m_oBulkTransferSvc ) );
	m_oClientServerSvc.SetInterfaceOID(	m_oIfaceOIDMgr.AllocateOID( &m_oClientServerSvc) );
	m_oGwMgmtSvc.SetInterfaceOID(		m_oIfaceOIDMgr.AllocateOID( &m_oGwMgmtSvc ) );
	m_oLeaseMgmtSvc.SetInterfaceOID(	m_oIfaceOIDMgr.AllocateOID( &m_oLeaseMgmtSvc ) );

	/// The publisher does not receive ProcessAPDU as the rest of the services
	/// GwUAP calls explicitly ProcessPublishAPDU. On the other hand, USER->GW flow is handled normally
	/// trough DispatchUserRequest / CanHandleServiceType / ProcessUserRequest
	m_oPublishSvc.SetInterfaceOID(		m_oIfaceOIDMgr.AllocateOID(	&m_oPublishSvc, SO_OID) );	/// Dispersion OID is well-known: 5

	m_oSystemReportSvc.SetInterfaceOID(	m_oIfaceOIDMgr.AllocateOID( &m_oSystemReportSvc ) );
	m_oTimeSvc.SetInterfaceOID(			m_oIfaceOIDMgr.AllocateOID( &m_oTimeSvc ) );
	m_oServerSvc.SetInterfaceOID(		m_oIfaceOIDMgr.AllocateOID( &m_oServerSvc, ARO_OID ) );
	/// Session management service does not get registered. It's a special type of service, not derived from CService
}


/// After a join, do not request teh contract immediately. instead, allow SM to
/// set the contract itself, and request it only if that did not happen in reasonable time
#define WAIT_BEFORE_REQUESTING_SM_CONTRACT 3
////////////////////////////////////////////////////////////////////////////////
/// @author
/// @brief UAP task. Handle
/// @param p_ushAppHandle - the application handle, used to find the original request
/// @remarks there are two ASLDE_SearchOriginalRequest, used on timeouts / responses
///
/// @note ProcessIsaTimeout does not need GENERIC_ASL_SRVC, p_ushAppHandle is enough
/// But m_oIfaceOIDMgr.DispatchISATimeout needs GENERIC_ASL_SRVC::object id.
/// @todo: keep the object ID in tracker and eliminate parameter GENERIC_ASL_SRVC from m_oIfaceOIDMgr.DispatchISATimeout
/// in order to get rid of tho stack calls: ASLDE_SearchOriginalRequest / ASLSRVC_GetGenericObject
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::GwUAP_Task(void)
{
	static time_t tLastLeaseCheck = 0;
	static CMicroSec uSec;

	/// (USER -> GW -> ISA) Check all client connections, read data ckunks, assmeble messages, dispatch messages to stack
	/// if a full message was processed, reduce timeout on ISA socket so we can eventually read a new message from GSAP
	unsigned unISATimeoutMultiplier = m_oGSAPLst.ProcessAll() ? 10 : 50 ;
	
	/// (ISA -> GW -> USER) Get APDU from stack, validate and dispatch on correct CGSAP (connection)
	/// Use 50*GETAPDU_MSEC to wait 50 ms for UDP packets.
	/// Greatly reduce processing. It should not limit the number of incoming packets because it return on packet arrival
	/// Unfortunately, it does reduce the number of SAP_IN packets processed. @see comments on CGSAPList::ProcessAll
	/// this is why on full messages from GSAP, the timeout is decreased
	uint8_t nAPDUs = 0;
	while( (nAPDUs < 25) && processIsaPackets(ISA100_GW_UAP, unISATimeoutMultiplier * GETAPDU_MSEC))
	{
		++nAPDUs;
	}
	if( nAPDUs > 20)
	{
		LOG("GwUAP_Task processed %2d APDUs", nAPDUs);
	}

	processIsaPackets(TUNNEL_EXTRA_UAP, 0);

	/// Check 4 TIMES per second for publish data
	/// TODO: check more often. >20 may even be 300
	if(uSec.GetElapsedMSec() > 1000 /*250*/)
	{
		uSec.MarkStartTime();	/// Mark before parsing, to generate as little drift as possible

		unsigned unSubscribers, unIndications;
		m_oPublishSvc.UpdateSubscribers( unSubscribers, unIndications );

		if( g_stApp.m_stCfg.AppLogAllowINF() && unIndications)
		{	LOG("STATS:   UpdateSubscribers(%3u/%u). Total P/ST/WD: %2u/%2u/%2u",
				unIndications, unSubscribers, m_oPublishSvc.m_unPublishCount,
				m_oPublishSvc.m_unSubscriberTimerCount, m_oPublishSvc.m_unPublishWatchdogCount );
		}
	}

	/// Various routines need the current time. Get it only once.
	/// TODO:  make tNow a public member in CGwUAP and get it instead of time(NULL) which is expensive
	/// TODO: GET NOW (tNow) FROM PORTING.CPP, IT'S UPDATED 10 TIMES PER SECOND.
	time_t tNow = time( NULL );

	/// 1 sec tasks
	/// Check expired leases, drop expired alerts, drop expired tracker entries
	if( tNow != tLastLeaseCheck )
	{
		tLastLeaseCheck = tNow;

		maintainSMContract( tNow );
		maintainContractList( tNow );
		garbageCollectorGPDUStats( tNow );

		if( g_stApp.m_stCfg.getYGSAP_TCPPort() )
		{	/// Poll device list ONLY if YGSAP enabled
			maintainDeviceList( tNow );
		}

		m_oSessMgmtSvc.CheckExpireSessions();	
		m_oLeaseMgmtSvc.CheckExpireLeases( tNow );
		m_oAlertSvc.DropExpired();
		m_tracker.DropExpired();
	}
}

/// Manage GW/SM contract used by System Management Services
void CGwUAP::maintainSMContract( time_t p_tNow )
{
	if( g_ucJoinStatus != DEVICE_JOINED )
	{
		m_nLastSMContractRequestTime = 0;
	}else{
		if( !m_nLastSMContractRequestTime )
		{
			m_nLastSMContractRequestTime = p_tNow; //this happens immediately after join.
		}
		if( INVALID_CONTRACTID == m_oSystemReportSvc.SMContractID() )
		{
			if( p_tNow - m_nLastSMContractRequestTime > WAIT_BEFORE_REQUESTING_SM_CONTRACT ) // allow the SM to set us up before asking the contract ourselves
			{
				m_nLastSMContractRequestTime = p_tNow;
				requestSMContract();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Maintain device lists system report used internally by YGSAP to map device tag to device address
/// @param p_tNow - now
/// @remarks It pools the device list at YGSAP_DEVICE_LIST_REFRESH seconds
/// @remarks Device list requested by the user is copied here too.
/// @remarks Auto-requested device list is available to user requests also.
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::maintainDeviceList( time_t p_tNow )
{
	if( (g_ucJoinStatus != DEVICE_JOINED) || (INVALID_CONTRACTID == m_oSystemReportSvc.SMContractID()))
	{
		m_nLastDeviceListRequestTime = 0;
	}else
	{
		if( p_tNow - m_nLastDeviceListRequestTime > g_stApp.m_stCfg.m_nDeviceListRefresh )
		{
			m_nLastDeviceListRequestTime = p_tNow;
			m_oSystemReportSvc.RequestDeviceList();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Maintain contracts list used to fill GPDU* parameters in Network Health report
/// @param p_tNow - now
/// @remarks It pools the contracts list at CONTRACT_LIST_REFRESH seconds
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::maintainContractList( time_t p_tNow )
{
	if( (g_ucJoinStatus != DEVICE_JOINED) || (INVALID_CONTRACTID == m_oSystemReportSvc.SMContractID()))
	{
		m_nLastContractListRequestTime = p_tNow;/// delay req, allow device join/contracts establish
	}else
	{
		if( p_tNow - m_nLastContractListRequestTime > g_stApp.m_stCfg.m_nContractListRefresh )
		{
			m_nLastContractListRequestTime = p_tNow;
			m_oClientServerSvc.RequestContractList();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Maintain contracts list used to fill GPDU* parameters in Network Health report
/// @param p_tNow - now
/// @remarks Garbage collect interval is hardcoded - 5 minutes
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::garbageCollectorGPDUStats( time_t p_tNow )
{	if( (p_tNow - m_nLastGPDUGarbageCollectTime) > 300 )	// 5 minutes?
	{
		m_nLastGPDUGarbageCollectTime = p_tNow;
		m_oPublishSvc.GPDUStatsGarbageColector( g_stApp.m_stCfg.m_nGPDUStatisticsPeriod );
	}
}

void CGwUAP::Close(void)
{}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Extracts one packet from the ISA100 application queue which is destined to the host application,
/// @brief and dispatches it according to its type: solicited or unsolicited data
/// @remarks
////////////////////////////////////////////////////////////////////////////////
bool CGwUAP::processIsaPackets( int p_nUapId, int p_nTimeout )
{
	bool bRet = false;
	APDU_IDTF stAPDUIdtf;
	/// Get an APDU. Wait for UDP up to 1 ms, only if APDU buffer is empty
	const uint8 * pAPDUStart = ASLDE_GetMyAPDU( p_nUapId, p_nTimeout, &stAPDUIdtf);

	if( !pAPDUStart ) /// Nothing to do
		return bRet;

	GENERIC_ASL_SRVC stRecvGenSrvc;
	const uint8 * pNext = pAPDUStart;
	const uint8 * pCurrent;
	unsigned unObjectsInAPDU = 0;
	gettimeofday( &m_tvLastRxApduUTC, NULL);
	stAPDUIdtf.m_tvTxTime.tv_sec -= TAI_OFFSET+g_stDPO.m_nCurrentUTCAdjustment; /// The stack returns TAI in this field. Convert it to unix time
	int nElapsedMSec = elapsed_usec(stAPDUIdtf.m_tvTxTime, m_tvLastRxApduUTC) / 1000;

	if (g_stApp.m_stCfg.AppLogAllowINF())
	{

		char szTravel[32];
		const char * szPrefix = "";
		int nWide = 2;
		if( nElapsedMSec < 0 )	/// Have the sign in the right place: before the dot
		{	nElapsedMSec = -nElapsedMSec;
			szPrefix="-";
			nWide = 1;
		}
		sprintf( szTravel, "%s%*d.%03d", szPrefix, nWide, nElapsedMSec/1000, nElapsedMSec%1000);

#if 0
		if( g_stApp.m_stCfg.AppLogAllowDBG() )
		LOG("APDU(s):%s:%u -> %d Prio+Flg:%02X Security %d Len:%3d Travel:%s", GetHex( stAPDUIdtf.m_aucAddr, sizeof(stAPDUIdtf.m_aucAddr)),
			stAPDUIdtf.m_ucSrcTSAPID, stAPDUIdtf.m_ucDstTSAPID, stAPDUIdtf.m_ucPriorityAndFlags, stAPDUIdtf.m_ucSecurityCtrl, stAPDUIdtf.m_unDataLen, szTravel );
	else
#endif
			LOG("APDU(s):%s:%u -> %d Len:%3d Travel:%s", GetHex( stAPDUIdtf.m_aucAddr, sizeof(stAPDUIdtf.m_aucAddr)),
				stAPDUIdtf.m_ucSrcTSAPID, stAPDUIdtf.m_ucDstTSAPID, stAPDUIdtf.m_unDataLen, szTravel );
	}

//	if( stAPDUIdtf.m_tvTxTime.tv_usec > 1000000 ) LOG("WARNING APDU TX uSec too BIG: %u usec", stAPDUIdtf.m_tvTxTime.tv_usec);
	/// Extract all the concatenated GENERIC_ASL_SRVC structures in the APDU, one by one
	while(	(stAPDUIdtf.m_unDataLen - (pNext - pAPDUStart) > 0)	/// shortcut: avoid one call. ASLSRVC_GetGenericObject does the same validation at start.
		&&	(pNext = ASLSRVC_GetGenericObject( pCurrent = pNext, stAPDUIdtf.m_unDataLen - (pNext - pAPDUStart), &stRecvGenSrvc, stAPDUIdtf.m_aucAddr )) )
	{	uint16				unTxAPDULen, unAppHandle = 0;
		GENERIC_ASL_SRVC	stReqGenSrvc = {0,};
		const uint8 * 		pTxAPDU = NULL;

		++unObjectsInAPDU;
		if( (stRecvGenSrvc.m_ucType & (SRVC_RESPONSE << 7)) && (stRecvGenSrvc.m_ucType != SRVC_PUBLISH) )
		{	/// Search original request (always non-concatenated) by m_ucReqID/stAPDUIdtf  - only if it's a response
			pTxAPDU = ASLDE_SearchOriginalRequest(
				stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_ucReqID,
				stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_unSrcOID,
				stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_unDstOID,
				&stAPDUIdtf, &unTxAPDULen, &unAppHandle );
#if 0
			if( !pTxAPDU )
			{	LOG("ERROR processIsaPackets: no original request for APDU type %X reqId %d SrcOID %u DstOID %u ",
					stRecvGenSrvc.m_ucType, stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_ucReqID,
					stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_unSrcOID,stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_unDstOID );
			}
#endif
		}
		if( pTxAPDU )
		{	if( ASLSRVC_GetGenericObject( pTxAPDU, unTxAPDULen, &stReqGenSrvc, NULL ) )
			{	if(	( GET_ASL_SRVC_TYPE(stRecvGenSrvc.m_ucType) != GET_ASL_SRVC_TYPE(stReqGenSrvc.m_ucType) ) ||
					( stReqGenSrvc.m_stSRVC.m_stReadReq.m_unDstOID != stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_unSrcOID ) )
				{	/// Type/object validation. IT WILL NOT WORK for alerts
					LOG("ERROR processIsaPackets(H %u): APDU Rsp/Req mismatch reqId %d type (%X %X) OID (%d/%d). Possible Req ID overflow",
						unAppHandle, stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_ucReqID, stRecvGenSrvc.m_ucType, stReqGenSrvc.m_ucType,
						stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_unSrcOID, stReqGenSrvc.m_stSRVC.m_stReadReq.m_unDstOID);
					continue;
				}
			}
			else
			{	LOG("ERROR processIsaPackets(H %u) can't parse APDU with reqId %d", unAppHandle, stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_ucReqID);
				continue;
			}
		}
		else	/// Either expired request, or unsolicited data (request or publish or alert) or tunnel
		{
			if (SRVC_PUBLISH == stRecvGenSrvc.m_ucType)
			{	m_oPublishSvc.ProcessPublishAPDU( &stAPDUIdtf, &stRecvGenSrvc );
				bRet = true;	/// Call it again
			}
			else if (SRVC_TUNNEL_REQ == stRecvGenSrvc.m_ucType)
			{	m_oServerSvc.ProcessTunnelAPDU(&stAPDUIdtf,&stRecvGenSrvc);
				//m_oServerSvc.ProcessAPDU(0, &stRecvGenSrvc, NULL);
			}
			else if (SRVC_ALERT_REP == stRecvGenSrvc.m_ucType)
			{	m_oAlertSvc.ProcessAlertAPDU(&stAPDUIdtf,&stRecvGenSrvc);
				if(stRecvGenSrvc.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucCategory == ALERT_CAT_COMM_DIAG
				|| stRecvGenSrvc.m_stSRVC.m_stAlertRep.m_stAlertInfo.m_ucCategory == ALERT_CAT_SECURITY)
				{//ALERT CASCADING: compose a new alert to the SM containing this alert
					uint8_t	buffer[256];
					memcpy( buffer, stAPDUIdtf.m_aucAddr, sizeof(stAPDUIdtf.m_aucAddr) );
					memcpy( buffer+sizeof(stAPDUIdtf.m_aucAddr), pCurrent, pNext-pCurrent );
					ALERT stAlert ={0,
							ISA100_START_PORTS + stAPDUIdtf.m_ucDstTSAPID,
							stRecvGenSrvc.m_stSRVC.m_stAlertRep.m_unDstOID,
							{0,0},
							0,
							ALERT_CLASS_EVENT,
							ALARM_DIR_RET_OR_NO_ALARM,
							ALERT_CAT_PROCESS,
							101,//available for vendor-specific alerts
							ALERT_PR_MEDIUM_M,
							16+(pNext-pCurrent)};//size
					ARMO_AddAlertToQueue( &stAlert, buffer );
				}
				//m_oServerSvc.ProcessAPDU(0, &stRecvGenSrvc, NULL);
			}
			else
			{	LOG("WARNING processIsaPackets(H %u) unsolicited APDU type %X:%s reqID %u SOID %u DOID %u from %s:%u",
					unAppHandle, stRecvGenSrvc.m_ucType,
					getISA100SrvcTypeName( GET_ASL_SRVC_TYPE(stRecvGenSrvc.m_ucType) ),
					stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_ucReqID,
					stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_unSrcOID,
					stRecvGenSrvc.m_stSRVC.m_stReadRsp.m_unDstOID,
					GetHex( stAPDUIdtf.m_aucAddr,sizeof(stAPDUIdtf.m_aucAddr)), stAPDUIdtf.m_ucSrcTSAPID  );
			}
			/// Unsolicited don't go trough normal flow trough m_oIfaceOIDMgr.DispatchAPDU()
			/// because they need the whole stAPDUIdtf, not just unAppHandle
			/// TODO: we just added stAPDUIdtf into normal ProcessAPDU, maybe it's time to unify
			/// TODO ProcessPublishAPDU / ProcessTunnelAPDU / ProcessAlertAPDU with ProcessAPDU
			continue;
		}
		/// DISPATCH TO THE PROPER WORKER, which may send the response to the client
		m_oIfaceOIDMgr.DispatchAPDU( unAppHandle, &stAPDUIdtf, &stRecvGenSrvc, &stReqGenSrvc );
	}
	if( !unObjectsInAPDU )
		LOG("WARNING processIsaPackets: NO generic objects found in APDU" );
	if( unObjectsInAPDU > 1 )
		LOG("processIsaPackets found %u generic objects CONCATENATED in APDU", unObjectsInAPDU );

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Dispatch a timeout from ISA100 stack for a previously sent request
/// @param p_ushAppHandle - the application handle, used to find the original request
/// @remarks there are two ASLDE_SearchOriginalRequest, used on timeouts / responses
///
/// @note ProcessIsaTimeout does not need GENERIC_ASL_SRVC, p_ushAppHandle is enough
/// But m_oIfaceOIDMgr.DispatchISATimeout needs GENERIC_ASL_SRVC::object id.
/// @todo: keep the object ID in tracker and eliminate parameter GENERIC_ASL_SRVC from m_oIfaceOIDMgr.DispatchISATimeout
/// in order to get rid of tho stack calls: ASLDE_SearchOriginalRequest / ASLSRVC_GetGenericObject
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::DispatchISATimeout( uint16 p_ushAppHandle )
{
	GENERIC_ASL_SRVC stReqGenSrvc;
	APDU_IDTF stAPDUIdtf;
	/// Find the original request by app handle (always non-concatenated) and schedule it for deletion
	const uint8 * pTxAPDU = ASLDE_GetMyOriginalTxAPDU( p_ushAppHandle, ISA100_GW_UAP, &stAPDUIdtf );

	if( pTxAPDU && ASLSRVC_GetGenericObject( pTxAPDU, stAPDUIdtf.m_unDataLen, &stReqGenSrvc, NULL ) )
		m_oIfaceOIDMgr.DispatchISATimeout( p_ushAppHandle, &stReqGenSrvc );
	else
		LOG("ERROR CGwUAP::DispatchISATimeout(H %u): unsolicited timeout", p_ushAppHandle );
};

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> ISA) Generate a new application handle, send the request down ISA stack
/// @param p_pObj	Message to send to the stack, type GENERIC_ASL_SRVC::m_stSRVC
/// @param p_ucObjectType Message (object) type: SRVC_TYPE_READ / SRVC_TYPE_WRITE / SRVC_TYPE_EXECUTE or SRVC_READ_REQ / SRVC_EXEC_REQ / SRVC_EXEC_REQ
/// @param p_ucSrcTSAPID - usually ISA100_GW_UAP, but may be different for tunnel
/// @param p_ucDstTSAPID
/// @param p_unContractID
/// @param p_bNoRspExpected have double meaning: as parameter to ASLSRVC_AddGenericObject and as indication of wether to log or not
////////////////////////////////////////////////////////////////////////////////
uint16 CGwUAP::SendRequestToASL(void *		p_pObj,
								uint8		p_ucObjectType,
								uint8		p_ucSrcTSAPID,
								uint8		p_ucDstTSAPID,
								uint16		p_unContractID,
								bool		p_bNoRspExpected,
								uint8_t*	p_pucSFC)
{
	unsigned short ushAppHandle = NextAppHandle();
	uint8 ucRet = ASLSRVC_AddGenericObject( p_pObj, p_ucObjectType, 0, p_ucSrcTSAPID, p_ucDstTSAPID, ushAppHandle, NULL, p_unContractID, 0, true, p_bNoRspExpected );
	if(p_pucSFC)
		*p_pucSFC = ucRet;

	if( SFC_SUCCESS == ucRet )
	{
		if(p_bNoRspExpected)
		{
			LOG("SendRequestToASL(C %2u %s) reqID %3u H %u", p_unContractID, getISA100SrvcTypeName(GET_ASL_SRVC_TYPE(p_ucObjectType)), ((READ_REQ_SRVC*)p_pObj)->m_ucReqID, ushAppHandle);
		}
		return ushAppHandle;
	}	/// else ( SFC_SUCCESS != ucRet )
	if(p_bNoRspExpected)
	{
		LOG("ERROR SendRequestToASL(C %2u %s) ASLSRVC_AddGenericObject ret x%02X", p_unContractID, getISA100SrvcTypeName(GET_ASL_SRVC_TYPE(p_ucObjectType)), ucRet );
	}
	return 0;	/// Report error
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> ISA) Search in tracker old AppHhandle, generate a new application handle,
/// @brief send the request down ISA stack, add handle/session/transaction to tracker, remove old AppHhandle from tracker
/// @param p_pObj	Message to send to the stack, type GENERIC_ASL_SRVC::m_stSRVC
/// @param p_ucObjectType Message (object) type: SRVC_TYPE_READ / SRVC_TYPE_WRITE / SRVC_TYPE_EXECUTE or SRVC_READ_REQ / SRVC_EXEC_REQ / SRVC_EXEC_REQ
/// @param p_ucDstTSAPID
/// @param p_unContractID
/// @param p_unAppHandle - identifies nSessionID/nTransactionID/ucServiceType
/// @return ushAppHandle, or 0 if it cannot sent the request to ASL (no space available?)
/// @remarks Esentially this is a wrapper for ASLSRVC_AddGenericObject/m_tracker.AddMessage
////////////////////////////////////////////////////////////////////////////////
uint16 CGwUAP::SendRequestToASL(void *		p_pObj,
								uint8		p_ucObjectType,
								uint8		p_ucDstTSAPID,
								uint16		p_unContractID,
								uint16		p_unAppHandle )
{	/// TODO: TAKE CARE: DOUBLE SEARCH FOR MESSAGE is performed here in case of  RenewMessage
	/// TODO:  a possible fix it to bring RenewMessage here, after successfull SendRequestToASL
	MSG * pMSG = m_tracker.FindMessage( p_unAppHandle, "SendRequestToASL" );
	if(!pMSG)
	{	LOG("ERROR SendRequestToASL(H %u C %2u %X:%s): cannot renew inexistent AppH",	p_unAppHandle, p_unContractID, p_ucObjectType, getISA100SrvcTypeName(GET_ASL_SRVC_TYPE(p_ucObjectType)) );
		return 0;
	}
	return SendRequestToASL(p_pObj, p_ucObjectType, p_ucDstTSAPID, p_unContractID, pMSG->m_nSessionID, pMSG->m_TransactionID, pMSG->m_ServiceType, p_unAppHandle );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> ISA) Generate a new application handle, send the request down ISA stack, add handle/session/transaction to tracker
/// @param p_pObj	Message to send to the stack, type GENERIC_ASL_SRVC::m_stSRVC
/// @param p_ucObjectType Message (object) type: SRVC_TYPE_READ / SRVC_TYPE_WRITE / SRVC_TYPE_EXECUTE or SRVC_READ_REQ / SRVC_EXEC_REQ / SRVC_EXEC_REQ
/// @param p_ucDstTSAPID
/// @param p_unContractID
/// @param m_nSessionID
/// @param m_unTransactionID
/// @param m_ucServiceType	GW/HOST service type: SESSION_MANAGEMENT / LEASE_MANAGEMENT / TOPOLOGY_REPORT
/// @param p_unAppHandleOld if non-sero, indicate a req to renew this AppH, otherwise a new message is added in tracker
/// @return ushAppHandle, or 0 if it cannot sent the request to ASL (no space available?)
/// @remarks Esentially this is a wrapper for ASLSRVC_AddGenericObject/m_tracker.AddMessage or m_tracker.RenewMessage
////////////////////////////////////////////////////////////////////////////////
uint16 CGwUAP::SendRequestToASL(void *		p_pObj,
								uint8		p_ucObjectType,
								uint8		p_ucDstTSAPID,
								uint16		p_unContractID,
								uint32		p_nSessionID,
								uint32		p_unTransactionID,
								uint8		p_ucServiceType,
								uint16		p_unAppHandleOld, /*= 0*/
								uint8_t*	p_pucSFC  /*=NULL*/ )
{
	uint8 ucRet;
	unsigned short ushAppHandle = SendRequestToASL( p_pObj, p_ucObjectType, ISA100_GW_UAP, p_ucDstTSAPID, p_unContractID, false, &ucRet );
	
	if(p_pucSFC)
		*p_pucSFC = ucRet;

	if( SFC_SUCCESS == ucRet )
	{	unsigned unTracked;
		if( p_unAppHandleOld )	unTracked = m_tracker.RenewMessage( p_unAppHandleOld, ushAppHandle );
		else					unTracked = m_tracker.AddMessage( ushAppHandle, p_nSessionID, p_unTransactionID, p_ucServiceType );

		char szTmp[ 32 ] = { 0 };
		if(p_unAppHandleOld)
			sprintf( szTmp, "%u->", p_unAppHandleOld);

		LOG("SendRequestToASL(C %2u %s S %u T %u %s) reqID %3u H %s%u (%u pending%s)", p_unContractID,
			getISA100SrvcTypeName(GET_ASL_SRVC_TYPE(p_ucObjectType)), p_nSessionID, p_unTransactionID, getGSAPServiceName(p_ucServiceType),
			((READ_REQ_SRVC*)p_pObj)->m_ucReqID, szTmp, ushAppHandle, unTracked, unTracked ? "" : " tracker ERROR");
		return ushAppHandle;
	}	/// else ( SFC_SUCCESS != ucRet )
	LOG("ERROR SendRequestToASL(C %2u %s S %u T %u %s) ASLSRVC_AddGenericObject ret x%02X", p_unContractID,
		getISA100SrvcTypeName(GET_ASL_SRVC_TYPE(p_ucObjectType)), p_nSessionID, p_unTransactionID, getGSAPServiceName(p_ucServiceType), ucRet );
	return 0;	/// Report error, do not add to tracker
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW)  Find the GSAP on which the request associated with p_unAppHandle was made
/// @brief Validates the session ID in the process
/// @param p_unAppHandle Application handle identifying the REQUEST: session/transaction -> CGSAP
/// @param p_prMSG [OUT] Pointer to MSG, with session/transaction
/// @return associated CGSAP, if found and valid, NULL otherwise
/// @remarks Search in tracker, then search the session in session manager, then search in the CGSAP list
/// On CGSAP/Session error, make sure it does the sync
/// @remarks p_prMSG is guaranteed to point to a valid structure (but the values in the structure may be 0)
////////////////////////////////////////////////////////////////////////////////
CGSAP* CGwUAP::FindGSAP( uint16 p_unAppHandle, MSG *& p_prMSG )
{
	static MSG stMSG = { 0,0,0,0,{0},0 };
	p_prMSG = m_tracker.FindMessage( p_unAppHandle, "FindGSAP");
	if(!p_prMSG)
	{	p_prMSG = &stMSG;	/// Protect the caller
		return NULL;
	}
	return FindGSAP( p_prMSG->m_nSessionID );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Check the session validity in both SessionMgr and GSAP list. Return pointer to CGSAP if valid session is found, NULL otherwise. Return pointer to CGSAP if valid session is found, NULL otherwise
/// @brief Also resync session ID's in sess mgr and CGSAP list (delete)
/// @return CGSAP* Pointer to CGSAP handling the session, if session is valid
/// @retval NULL Session invalid or CGSAP handling it not found (connection closed). In this case session is also deleted from session mgr.
/// @remarks This method is called for both client REQUESTS and RESPONSES
/// @todo remove:  The method also syncronize session list within session mgr and GSAP's
/// @remarks Resons for desync:
/// 	- GSAP connection close: Sess Mgr has valid session, GSAP does not. will delete from Sess mgr
/// 	- Session expire/delete: GSAP has valid session, Sess Mgr does not. Will delete form GSAP
/// We will NOT send session expire confirm even if we have CGSAP* (example: publish
///		or session valid when request is sent, expired by the time confirm is received back).
///	When a session expires, we simply stop sending data to the client
////////////////////////////////////////////////////////////////////////////////
CGSAP * CGwUAP::FindGSAP( uint32 p_nSessionID )
{
	if( m_oSessMgmtSvc.IsValidSession( p_nSessionID ) )
	{
		CGSAP * pGSAP = m_oGSAPLst.FindGSAP( p_nSessionID );
		if( !pGSAP )		///< GSAP closed in the mean time
		{	/// DO NOT delete session here.
			LOG("CGwUAP: Valid session %u: INVALID GSAP.", p_nSessionID );
		}
		return pGSAP;
	}
	/// Since m_oSessMgmtSvc.IsValidSession deletes expired sessions there is no need to delete invalid session here
	LOG("CGwUAP::FindGSAP(S %u): INVALID", p_nSessionID );
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send confirm on SAP identified by p_nSessionID
/// @remarks usually needed by ProcessUserRequest (User -> GW) flow
////////////////////////////////////////////////////////////////////////////////
bool CGwUAP::SendConfirm( uint32 p_nSessionID, uint32 p_nTransactionID, uint8 p_ucServiceType, uint8* p_pMsgData, uint32 p_dwMsgDataLen, bool p_bRemoveFromTracker  )
{
	CGSAP* pGSAP = FindGSAP( p_nSessionID );
	if(pGSAP)
	{
		return pGSAP->SendConfirm( p_nSessionID, p_nTransactionID, p_ucServiceType, p_pMsgData, p_dwMsgDataLen, p_bRemoveFromTracker );
	}
	LOG("WARNING SendConfirm(S %u T %u) %s: Session/GSAP invalid, CONFIRM not sent", p_nSessionID, p_nTransactionID, getGSAPServiceName(p_ucServiceType) );
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send confirm on SAP identified by p_unAppHandle
/// @remarks usually needed by ProcessAPDU (ISA -> GW) flow
////////////////////////////////////////////////////////////////////////////////
bool CGwUAP::SendConfirm( uint16 p_unAppHandle, uint8* p_pMsgData, uint32 p_dwMsgDataLen, bool p_bRemoveFromTracker )
{	MSG   *	pMSG;
	CGSAP *	pGSAP;
	if( (pGSAP = FindGSAP( p_unAppHandle, pMSG )) )
	{
		return pGSAP->SendConfirm( pMSG->m_nSessionID, pMSG->m_TransactionID, pMSG->m_ServiceType, p_pMsgData, p_dwMsgDataLen, p_bRemoveFromTracker );
	}
	/// else
	/// PROGRAMMER ERROR: Tracker should never expire messages before stack.
	/// PROGRAMMER ERROR: Stack should not overflow RequestID
	LOG("ERROR SendConfirm(H %u): AppHandle not found in tracker, CONFIRM not sent", p_unAppHandle );
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Dump to log the whole object status
/// @remarks call Dump() for relevant member objects
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::Dump( void )
{
	LOG("******** GW status dump BEGIN ********");
	LOG("Join/UnJoin count: %u/%u. Now: %sJOINED", m_unJoinCount, m_unUnjoinCount, m_bJoined ? "":"NOT ");
	m_oGSAPLst.Dump();		/// Dump all CGSAP - user open connections
	m_oSessMgmtSvc.Dump();	/// Dump all sessions
	//m_oLeaseMgmtSvc.Dump(); already present in m_oIfaceOIDMgr
	m_tracker.Dump();		/// Dump Tracker - pending requests
	m_oIfaceOIDMgr.Dump();	/// Dump all CService* workers
	m_oYDevList.Dump();		/// Dump YGSAP Tag -> Address association table
	LOG("******** GW status dump END   ********");
}

////////////////////////////////////////////////////////////////////////////////
/// @author
/// @brief Notify contract created / deleted / failed
/// @param p_nContractId the contract id
/// @param p_pContract pointer to contract attributes
/// @remarks
///     p_pContract != NULL && p_nContractId != 0 contract created
///     p_pContract != NULL && p_nContractId == 0 contract creation failed
///     p_pContract == NULL && p_nContractId 1= 0 contract deleted
///     p_pContract == NULL && p_nContractId == 0 INVALID combination
/// Set GW-SM contract to INVALID_CONTRACTID on delete (p_pContract==NULL)
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::ContractNotification( uint16 p_nContractId, DMO_CONTRACT_ATTRIBUTE* p_pContract )
{
	if( p_pContract && p_nContractId )
		LOG("ContractNotification: ADD   id %u srcSAP %04X dev %s:%04X",
			p_nContractId, p_pContract->m_unSrcTLPort, GetHex(p_pContract->m_aDstAddr128, sizeof(IPV6_ADDR)), p_pContract->m_unDstTLPort );
	else if ( p_pContract && !p_nContractId )
		LOG("ContractNotification: FAIL  srcSAP %04X dev %s:%04X",
			p_pContract->m_unSrcTLPort, GetHex(p_pContract->m_aDstAddr128, sizeof(IPV6_ADDR)), p_pContract->m_unDstTLPort );
	else //( !p_pContract ) //&& p_nContractId
		LOG("ContractNotification: DELETE id %u", p_nContractId );

	/// SM-GW contract created
	if( p_pContract && p_nContractId
	&&	!memcmp(p_pContract->m_aDstAddr128, g_stApp.m_stCfg.getSM_IPv6(), sizeof(IPV6_ADDR))
	&&	p_pContract->m_unDstTLPort == ISA100_SMAP_PORT
	&&	p_pContract->m_unSrcTLPort == ISA100_APP_PORT )
	{
		m_oSystemReportSvc.SetSMContractID( p_nContractId );
		LOG("ContractNotification: GW->SM id %u created", p_nContractId);
		log2flash("ISA_GW GW->SM contractID %u", p_nContractId);
	}

	/// GW-SM contract erased (unjoin)
	if( !p_pContract
	&& ( p_nContractId == m_oSystemReportSvc.SMContractID() ) )
	{
		LOG("ContractNotification: GW->SM id %u erased", m_oSystemReportSvc.SMContractID());
		m_oSystemReportSvc.SetSMContractID( INVALID_CONTRACTID );
	}

	/// Pass all contracts to the lease management services
	m_oLeaseMgmtSvc.ContractNotification( p_nContractId, p_pContract );
	m_oAlertSvc.ContractNotification( p_nContractId, p_pContract );
};

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Notify join/unjoin
/// @param p_bJoined true when joined, false when unjoined
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::OnJoin( bool p_bJoined )
{
	if( p_bJoined )		++m_unJoinCount;
	else				++m_unUnjoinCount;
	m_bJoined = p_bJoined;
	LOG("OnJoin: %sJOIN (%u times)", p_bJoined?"":"UN",  p_bJoined ? m_unJoinCount: m_unUnjoinCount );

	if( !p_bJoined && m_oSystemReportSvc.SMContractID() )
	{
		log2flash("ISA_GW UNJoin: erase contract %u",  m_oSystemReportSvc.SMContractID());
		LOG("UNJOIN: GW->SM contractID %u erased", m_oSystemReportSvc.SMContractID());
		m_oSystemReportSvc.SetSMContractID( INVALID_CONTRACTID );
	}
	if( p_bJoined )
	{
		log2flash("ISA_GW Join");
	}
	/// ON unjoin erase all leases.
	m_oLeaseMgmtSvc.OnJoin( p_bJoined );
};
////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Request the GW-SM UAP contract to be used by topology and other system management services
/// @param none
/// @remarks main lop call this
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::requestSMContract( void )
{	DMO_CONTRACT_BANDWIDTH stBandwidth;
	stBandwidth.m_stAperiodic.m_nComittedBurst	= 100;
	stBandwidth.m_stAperiodic.m_nExcessBurst	= 100;
	stBandwidth.m_stAperiodic.m_ucMaxSendWindow	= 1;

	LOG("CGwUAP::requestSMContract REQUEST NEW contract");
	DMO_RequestNewContract( g_stApp.m_stCfg.getSM_IPv6(), ISA100_SMAP_PORT, ISA100_APP_PORT, 0xFFFFFFFF, SRVC_APERIODIC_COMM, 0, 1252, 0, &stBandwidth );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Claudiu Hobeanu
/// @brief	retrieve a tunnel object. the object is loaded if necessary
/// @param int p_nUapId -- UAP id valid 1-15
/// @return
/// @remarks
////////////////////////////////////////////////////////////////////////////////
CIsa100Object::Ptr CGwUAP::GetTunnelObject (int p_nUapId, int p_nObjectId)
{
	int nUapObjIndex = UAP_OBJ_INDEX(p_nUapId,p_nObjectId);
	CIsa100ObjectsMap::iterator it = m_oIsa100ObjectsMap.find(nUapObjIndex);

	if (it != m_oIsa100ObjectsMap.end())
	{
		CIsa100Object::Ptr pObj = it->second;

		if (pObj->GetObjectTypeId() != CIsa100Object::OBJECT_TYPE_ID_TUNNEL )
		{
			return CIsa100Object::Ptr();
		}
		return pObj;
	}
	//not found -> load a tunnel object

	CIsa100Object::Ptr pTunnel(new CTunnelObject(p_nUapId,p_nObjectId));
	m_oIsa100ObjectsMap.insert(std::make_pair(nUapObjIndex,pTunnel));
	LOG("CGwUAP::GetTunnelObject: Object not found, create new one with size:%i", sizeof(CTunnelObject) );

	return pTunnel;
}


	/// Update publish interval for all devices.
	/// Get data from CS SMO.GetContractsAndRoute (p_pCSData) and update it in publish cache
////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Update publish interval for all devices
/// @param p_pCSData SMO_METHOD_GET_CONTRACTS_AND_ROUTES data
/// @remarks Get data from CS SMO.GetContractsAndRoute (p_pCSData) and update it in publish cache
////////////////////////////////////////////////////////////////////////////////
void CGwUAP::UpdatePublishRate( const CClientServerService::CSData * p_pCSData )
{
	CClientServerService::TClientServerConfirmData_RX * pCnfrmR = (CClientServerService::TClientServerConfirmData_RX *) p_pCSData->m_pData;
	const SAPStruct::ContractsAndRoutes * pContractsAndRoutes = (const SAPStruct::ContractsAndRoutes *)pCnfrmR->m_aRspData;

	if( !pContractsAndRoutes->VALID( pCnfrmR->m_ushRspLen ) )
	{
		LOG("ERROR UpdatePublishRate: invalid ContractsAndRoutes data");
		return;
	}
	const SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes * pD = &pContractsAndRoutes->deviceList[0];

	unsigned unTotalContracts = 0, unPeriodicContracts2GW = 0;
	for(int i = 0; i < pContractsAndRoutes->numberOfDevices; ++i)
	{	unTotalContracts += pD->numberOfContracts;
		for(int j=0; j < pD->numberOfContracts; ++j)
		{	int k = 0;
			char szDeadline[32] = {0};	// deadline is expreaased in 10 ms slots, convert to milisec then to string
			if(pD->contractTable[j].serviceType == 0)
				sprintf( szDeadline, "D %d.%03d", pD->contractTable[j].deadline * 10 / 1000, pD->contractTable[j].deadline * 10 % 1000 );
			
			if( g_stApp.m_stCfg.AppLogAllowDBG() )
			{
				LOG("Contract %3u t %u %s:%u -> %s:%u %s %3d actv %d exp %d %s",
					pD->contractTable[j].contractID, pD->contractTable[j].serviceType,
					GetHex(pD->networkAddress, 16), pD->contractTable[j].sourceSAP,
					GetHex(pD->contractTable[j].destinationAddress, 16), pD->contractTable[j].destinationSAP,
					(pD->contractTable[j].serviceType == 0) ? "Period " : "CdtBrst",
	  				(pD->contractTable[j].serviceType == 0) ? pD->contractTable[j].period   : pD->contractTable[j].comittedBurst,
					pD->contractTable[j].activationTime, pD->contractTable[j].expirationTime, szDeadline);
			}

			if(	(pD->contractTable[j].serviceType == 0)	/// periodic
			&&	!memcmp( pD->contractTable[j].destinationAddress, g_stApp.m_stCfg.getGW_IPv6(), 16 ) /// with GW
			&&	(pD->contractTable[j].destinationSAP == ISA100_GW_UAP)	/// on the right TSAPID
			/// in theory it must be active, not expired, TODO check for expiry/activate also
			)
			{	++unPeriodicContracts2GW;
				if( g_stApp.m_stCfg.m_nDisplayReports || g_stApp.m_stCfg.AppLogAllowDBG() )
				{
					LOG("GPDU UpdatePublishRate C %3u %s:%u -> GW:UAP period %d phase %u %s",
						pD->contractTable[j].contractID, GetHex(pD->networkAddress, 16), pD->contractTable[j].sourceSAP,
						pD->contractTable[j].period, pD->contractTable[j].phase, szDeadline);
				}

				if( !m_oPublishSvc.GPDUStatsSetPublishRate(
						CPublishSubscribeService::PublishKey(pD->networkAddress, pD->contractTable[j].sourceSAP),
						pD->contractTable[j].period, pD->contractTable[j].phase, pD->contractTable[j].deadline) )
				{	/// This is possible if device just got its contract but did not publish yet
					/// It may as well be an error
					LOG( "WARNING GPDU UpdatePublishRate(%s): not yet in publish cache, cannot update period %u", GetHex(pD->networkAddress,16), pD->contractTable[j].period );
				}
				if( ++k > 1 )
					LOG("WARNING GPDU UpdatePublishRate(%s): Multiple publish contracts", GetHex(pD->networkAddress,16));
			}
		}
		pD = pD->NEXT();
	}
	LOG("GPDU UpdatePublishRate: total %u contracts (%u bytes), %u periodic to GW", unTotalContracts,
		unTotalContracts*sizeof(SAPStruct::ContractsAndRoutes::DeviceContractsAndRoutes::Contract), unPeriodicContracts2GW);
}
