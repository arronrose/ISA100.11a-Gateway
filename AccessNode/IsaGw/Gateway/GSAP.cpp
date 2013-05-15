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
/// @file GSAP.h
/// @author Marcel Ionescu
/// @brief Gateway Service Access Point interface. Handle a single client
////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include "../../Shared/Common.h"
#include "GSAP.h"
#include "GwApp.h"

////////////////////////////////////////////////////////////////////////////////
/// @class CGSAP
/// @brief Gateway Service Access Point. Base class for all protocol translators.
////////////////////////////////////////////////////////////////////////////////

CGSAP::CGSAP( void )
:m_dwRequestCount(0), m_dwIndicationCount(0), m_dwConfirmCount(0), m_dwResponseCount(0)
, m_dwTotalRxSize(0), m_dwTotalTxSize(0), m_aReqData(NULL), m_bIsValid(true)
{
	m_tLastRx = m_tLastRx = time( NULL ); ///< avoid immediate expiration
}

CGSAP::~CGSAP( void )
{
	clear();
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  Log one session id
/// @remarks use a signal to log the whole app status
/// @see CTcpGSAP::Dump
////////////////////////////////////////////////////////////////////////////////
static void sDumpSession( unsigned p_unSessionId )
{
	LOG( "   Session ID %u", p_unSessionId);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  Request the SessionManager to delete one session id
/// @param p_unSessionID - the session ID to delete 
/// @remarks use a signal to log the whole app status
/// @see CTcpGSAP::Dump
////////////////////////////////////////////////////////////////////////////////
static void sDeleteSession( unsigned p_unSessionID )
{
	bool bRet = g_pSessionMgr->DeleteSession( p_unSessionID);
	LOG( "CGSAP clear: Delete session ID %u (%s)", p_unSessionID, bRet ? "OK" : "NOT FOUND" );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  Log the object content
/// @remarks use a signal to log the whole app status
////////////////////////////////////////////////////////////////////////////////
void CGSAP::Dump( void )
{
	LOG(" GSAP activity: %s", Activity() );
	LOG(" GSAP status:   REQ %u CNF %u IND %u RSP %u Lifetime: %u Sessions: %u",
		m_dwRequestCount, m_dwConfirmCount, m_dwIndicationCount, m_dwResponseCount, (unsigned)m_uSec.GetElapsedSec(), m_lstSessions.size() );

	for_each( m_lstSessions.begin(), m_lstSessions.end(), sDumpSession );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  Return a static string formatted with last Tx/Rx (number of seconds in the past, or never)
/// @remarks 
////////////////////////////////////////////////////////////////////////////////
const char * CGSAP::Activity( void )
{	static char szRet[128];
	char tmpR[32], tmpT[32];
	
	if(m_tLastRx)	sprintf(tmpR, "%+ld", m_tLastRx-time(NULL));
	else			sprintf(tmpR, "never");
	if(m_tLastTx)	sprintf(tmpT, "%+ld", m_tLastTx-time(NULL));
	else			sprintf(tmpT, "never");
	sprintf(szRet,"Seconds(Rx %s Tx %s) Bytes(Rx %lu Tx %lu)", tmpR, tmpT, m_dwTotalRxSize, m_dwTotalTxSize );
	return szRet;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> User) Send Confirm to user, remove from Tracker
/// @param p_nSessionID		GS_Session_ID
/// @param p_nTransactionID GS_Transaction_ID
/// @param p_ucServiceType	The service type
/// @param p_pMsgData 		data starting with GS_Status, type-dependent. Does not include data CRC
/// @param p_dwMsgDataLen 	data len, type-dependent. Does not include data CRC
/// @retval true message was sent
/// @retval false message can't be sent, socket closed
/// @remarks Modify p_ucServiceType in CONFIRM, SendMessage, remove from tracker
/// if it's not SESSION_MANAGEMENT (which is not placed in Tracker in the first place)
////////////////////////////////////////////////////////////////////////////////
bool CGSAP::SendConfirm( uint32 p_nSessionID, uint32 p_nTransactionID, uint8 p_ucServiceType, uint8* p_pMsgData, uint32 p_dwMsgDataLen, bool p_bRemoveFromTracker /*= true*/ )
{
	bool bRet = SendMessage( p_nSessionID, p_nTransactionID, CONFIRM(p_ucServiceType), p_pMsgData, p_dwMsgDataLen );

	switch( REQUEST(p_ucServiceType) )
	{
		case SESSION_MANAGEMENT:///< SESSION_MANAGEMENT is not stored in Tracker, no need to remove
		case LEASE_MANAGEMENT:	///< LEASE_MANAGEMENT is not stored in Tracker, no need to remove
//		case PUBLISH:			///< Sent by CPublishSubscribeService
//		case WATCHDOG_TIMER:	///< Sent by CPublishSubscribeService
//		case SUBSCRIBE_TIMER:	///< Sent by CPublishSubscribeService
//	 	case ALERT_SUBSCRIBE:
//		case ALERT_NOTIFY:
		/// TODO Add here all other message types which are self-generated (without being initiated in on ProcessUserRequest)
			break;

		case CLIENT_SERVER:
//	 	case GATEWAY_CONFIGURATION_READ:
//		case GATEWAY_CONFIGURATION_WRITE:
//		case DEVICE_CONFIGURATION_READ:
//		case DEVICE_CONFIGURATION_WRITE:
		case DEVICE_LIST_REPORT:
		case TOPOLOGY_REPORT:
		case SCHEDULE_REPORT:
		case DEVICE_HEALTH_REPORT:
		case NEIGHBOR_HEALTH_REPORT:
		case NETWORK_HEALTH_REPORT:
		case TIME_SERVICE:
		case NETWORK_RESOURCE_REPORT:
		default:	/// All other services: remove from tracker

		/// If requested, remove regardless of SendMessage bRet code, we cannot do anything else with pair p_nSessionID/p_nTransactionID message anyway
		if( p_bRemoveFromTracker )
		{
			g_stApp.m_oGwUAP.m_tracker.RemoveMessage( p_nSessionID, p_nTransactionID );
		}
	}
	return bRet;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Search a session in internal list, NOT in session manager.
/// @retval true:  Session found in internal list
/// @retval false: Session not found in internal list
/// @remarks Return false if not found: session is not associated with this channel
////////////////////////////////////////////////////////////////////////////////
bool CGSAP::HasSession( uint32 p_unSessionID )
{
	for( TSessionLst::iterator it = m_lstSessions.begin(); it != m_lstSessions.end(); ++it )
	{
		if( *it == p_unSessionID )
		{
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Delete a session from internal list, NOT from session manager
/// @retval true:  Session found in internal list, erased
/// @retval false: Session not found in internal list
/// @remarks Does not verify the session manager
/// @remarks session appearing multiple times is inconsistency. Delete all and LOG warning in this case
////////////////////////////////////////////////////////////////////////////////
bool CGSAP::DelSession( uint32 p_unSessionID )
{	bool bFound = false;
	TSessionLst::iterator it = m_lstSessions.begin();
	while( it != m_lstSessions.end() )
	{
		if( *it == p_unSessionID )
		{
			if( bFound )
			{
				LOG("WARNING CGSAP::DelSession: session found/deleted more than once.");
			}
			it = m_lstSessions.erase( it );
			bFound =  true;
		}
		else
		{
			++it;
		}
	}
	if( bFound )
	{
		LOG("CGSAP::DelSession: session %d deleted. CGSAP(%s)", p_unSessionID, Identify() );
	}
	return bFound;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Clear the object, reset all buffers, invalidate the connection,
/// @brief erase sessions from both SessionManager and internal list
/// @return false - just a convenience for the callers to return dirrectly false
/// @remarks The container CGSAPList will delete CGSAP instances with IsValid() returning false
/// which means: after calling Close(), CTcpGSAP::IsValid() returns false,
/// therefore this instance will be destroyed by the container
/// @todo: initiate connection close here.
////////////////////////////////////////////////////////////////////////////////
bool CGSAP::clear( void )
{
	if( m_aReqData )
		delete m_aReqData;
	m_aReqData = NULL;
	m_bIsValid = false;

	LOG("CGSAP clear: erasing %u sessions", m_lstSessions.size());
	for_each( m_lstSessions.begin(), m_lstSessions.end(), sDeleteSession );
	m_lstSessions.clear();
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Unpack parameters, check CRC on header/data, convert header to host order,
///	@brief check session validity, dispatch the message to proper worker
/// @brief Direction: Client -> GW (request)
/// @see GwApp.h, TOPOLOGY_REPORT_REQUEST
/// @remarks The method expects a full message in m_stReqHeader/m_aReqData, both still in network order 
////////////////////////////////////////////////////////////////////////////////
void CGSAP::dispatchUserReq( void )
{
	if( !checkCrc( (byte*)&m_stReqHeader, sizeof(m_stReqHeader) )  )	///< header crc check
		///< Do not send answer back: the service might be wrong, we cannot rely on any header field sice the CRC is wrong.
		return;	///< Cannot delete the session here - since crc is wrong, the session ID might be wrong

	m_stReqHeader.NTOH();
	if(	(m_stReqHeader.m_nDataSize > sizeof(uint32))					///< data crc check only if there is data
	&&	!checkCrc( m_aReqData, m_stReqHeader.m_nDataSize ) ) 			///< data crc check
	{
		LOG("ERROR invalid request, IGNORE. Details below");
		LOG_HEX("hdr  ", (byte*)&m_stReqHeader, sizeof(m_stReqHeader) );
		LOG_HEX("data ", (byte*)m_aReqData, m_stReqHeader.m_nDataSize );
		//sDeleteSession( m_stReqHeader.m_nSessionID );	///< maybe is too radical to delete session on data CRC errors
		// clear() should be called here to re-sync. it erases all sessiond on this CGSAP
		return;
	}
	if( !reqSessionIsValid() )											///< session check
		return;	///< Invalid session cannot be deleted - to avoid deleting sessions open on other CGSAP's

#define SAP_IN_FORMAT "SAP_IN (S %2u T %5u) %s data(%d)"
	if( g_stApp.m_stCfg.AppLogAllowINF() )
	{
		LOG( SAP_IN_FORMAT " %s%s", m_stReqHeader.m_nSessionID, m_stReqHeader.m_unTransactionID,
			getGSAPServiceName( m_stReqHeader.m_ucServiceType ), m_stReqHeader.m_nDataSize,
			GET_HEX_LIMITED(m_aReqData, m_stReqHeader.m_nDataSize, (unsigned)g_stApp.m_stCfg.m_nMaxSapInDataLog ) );
	}
	else
	{
		LOG( SAP_IN_FORMAT, m_stReqHeader.m_nSessionID, m_stReqHeader.m_unTransactionID,
			getGSAPServiceName( m_stReqHeader.m_ucServiceType ), m_stReqHeader.m_nDataSize);
	}


	if( SESSION_MANAGEMENT == m_stReqHeader.m_ucServiceType )
	{
		processSessionManagementReq( );
		return;
	}
	if( !g_stApp.m_oGwUAP.DispatchUserRequest( &m_stReqHeader, m_aReqData ) )
	{	/// Do NOT remove the code below
		//byte ucStatus = G_STATUS_UNIMPLEMENTED;
		//SendConfirm(m_stReqHeader.m_nSessionID, m_stReqHeader.m_unTransactionID , m_stReqHeader.m_ucServiceType,  &ucStatus, sizeof(ucStatus) );
		/// DO NOTHING: invalid primitive may be attack
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Process a request for to session management object, send answer back
/// @brief Direction: Client to GW (request)
/// @remarks Check data CRC, extract parameters and convert byte order, call Session management class, send answer to the client
////////////////////////////////////////////////////////////////////////////////
void CGSAP::processSessionManagementReq( void )
{
	uint32 nSessionId	= m_stReqHeader.m_nSessionID;
	long nPeriod 		= 0;
	uint8_t status 		= m_stReqHeader.IsYGSAP() ? YGS_FAILURE : SESSION_FAIL_OTHER;;

	if( (m_stReqHeader.m_nDataSize < sizeof(TSessionMgmtReqData) + 4)) // 4: CRC
	{
		LOG("ERROR SESSION REQ (S %u T %u) buffer too small %d", m_stReqHeader.m_nSessionID, m_stReqHeader.m_unTransactionID, m_stReqHeader.m_nDataSize);
	}
	else
	{
		TSessionMgmtReqData * pData = (TSessionMgmtReqData*) m_aReqData;
		pData->NTOH();
		nPeriod = pData->m_nPeriod;
		status = g_pSessionMgr->RequestSession( nSessionId, nPeriod, pData->m_shNetworkId, m_stReqHeader.m_ucVersion );
	
		if ( 	!m_stReqHeader.m_nSessionID					/// Request for new session -> success
			&&	( 	( m_stReqHeader.IsYGSAP()
					&&  ( 	(status == YGS_SUCCESS)
						||	(status == YGS_SUCCESS_REDUCED) ) )
				||	( 	(status == SESSION_SUCCESS)
					||	(status == SESSION_SUCCESS_REDUCED) ) ) )
		{
			addSession( nSessionId );	///< Associate the session with this CGSAP (connection)
		}
	}
	TSessionMgmtRspData stRsp = { m_ucStatus: status, m_nPeriod: nPeriod };
	stRsp.HTON();
	SendConfirm( nSessionId, m_stReqHeader.m_unTransactionID, m_stReqHeader.m_ucServiceType, (byte*)&stRsp, sizeof(stRsp) );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Validate crc. On mismatch: close connection, log error
/// @param p_pData data, with CRC network order in last 4 bytes
/// @param p_dwDataLen Data len, must be > 4
/// @retval true:  CRC valid
/// @retval false: CRC invalid, connection closed 
/// @remarks Expect CRC to be network order. Expect p_dwDataLen > 4. Caller must ensure both.
////////////////////////////////////////////////////////////////////////////////
bool CGSAP::checkCrc( byte* p_pData, uint32 p_dwDataLen  )
{	uint32 nCrc;
	memcpy( &nCrc, p_pData + p_dwDataLen - sizeof(uint32), sizeof(uint32));
	bool bRet = (nCrc == crc( p_pData, p_dwDataLen - sizeof(uint32) ));
	if( !bRet )
	{	LOG("ERROR CGSAP::checkCrc rcv %08X comp %08X, IGNORE", nCrc, crc( p_pData, p_dwDataLen-sizeof(nCrc) ));
		LOG_HEX("ERROR Invalid CRC for", p_pData, p_dwDataLen );
		/// Potential change: Close connection on CRC errors to FORCE RE-SYNC.
		/// clear() does it, but erase all sessions which probably is too tough
		/// TAKE CARE: If clear is ever called here, DO NOT USE m_aReqData afterwards - it gets deleted by clear()
	}
	return bRet;
};

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Validate session ID. Return true if valid/false if invalid, send a message to client if invalid
/// @retval true:  Session is valid
/// @retval false: Session invalid, client notified
/// @remarks Expect header m_stReqHeader in host order. Expect p_dwDataLen > 4. Caller must ensure both.
/// @remarks This method is called for CLIENT REQUESTS to verify session id into session manager
/// @todo Make a similar check on messages from the field (CLEINT RESPONSES)
////////////////////////////////////////////////////////////////////////////////
bool CGSAP::reqSessionIsValid( void )
{	///The only case where invalid session ID is acceptable: req for a new session
	if(	(!m_stReqHeader.m_nSessionID && ( SESSION_MANAGEMENT == m_stReqHeader.m_ucServiceType ) )
	||	(g_pSessionMgr->IsValidSession( m_stReqHeader.m_nSessionID ) && HasSession( m_stReqHeader.m_nSessionID ) ))
	{
		return true;
	}
	
	// response data on invalid request
	byte invRsp[MAX_SIZE_FAIL_RES];
	// data size particular for each request type 
	int invRspSize = 1;

	// compute data size for each request type (same values for GSAP/YGSAP)
	switch( REQUEST(m_stReqHeader.m_ucServiceType) )
	{
		case SESSION_MANAGEMENT:
			invRspSize = sizeof(TSessionMgmtRspData);  // always 9
			break;
		case LEASE_MANAGEMENT:
			invRspSize = sizeof(CLeaseMgmtService::TLeaseMgmtConfirmData); // always 13
			break;
		case DEVICE_LIST_REPORT:
			invRspSize = DEV_LIST_FAIL_RES_SIZE;
			break;
		case TOPOLOGY_REPORT:
			invRspSize = TOPOLOGY_FAIL_RES_SIZE;
			break;
		case SCHEDULE_REPORT:
			invRspSize = SCHEDULE_FAIL_RES_SIZE;
			break;
		case DEVICE_HEALTH_REPORT:
			invRspSize = DEVICE_HEALTH_FAIL_RES_SIZE;
			break;
		case NEIGHBOR_HEALTH_REPORT:
			invRspSize = NEIGHBOR_HEALTH_FAIL_RES_SIZE;
			break;
		case NETWORK_HEALTH_REPORT:
			invRspSize = NETWORK_HEALTH_FAIL_RES_SIZE;
			break;
		case NETWORK_RESOURCE_REPORT:
			invRspSize = NETWORK_RESOURCE_FAIL_RES_SIZE;
			break;
		case TIME_SERVICE:
			invRspSize = TIME_FAIL_RES_SIZE;
			break;
		case CLIENT_SERVER:
			// TClientServerConfirmData_RX <=> TClientServerConfirmData_W on invalid response
			invRspSize = offsetof(CClientServerService::TClientServerConfirmData_RX, m_ucReqType); 
			break;
		case PUBLISH:
			break; // only status here
		case SUBSCRIBE:
			invRspSize = sizeof(CPublishSubscribeService::TSubscribeConfirmData) + 2; // variable size data > 2
			break;
		//case PUBLISH_TIMER:
		//case SUBSCRIBE_TIMER:
		//case WATCHDOG_TIMER:
		case BULK_TRANSFER_OPEN:
			invRspSize = sizeof(CBulkTransferService::TBulkOpenConfirmData);
			break;
		case BULK_TRANSFER_TRANSFER:				
		case BULK_TRANSFER_CLOSE:				
		case ALERT_SUBSCRIBE:				
		//case ALERT_NOTIFY:
		case GATEWAY_CONFIGURATION_READ:
		case GATEWAY_CONFIGURATION_WRITE:
		case DEVICE_CONFIGURATION_READ:
		case DEVICE_CONFIGURATION_WRITE:
			break; // only status here
		default:
			LOG("ERROR Invalid session %d (service %s), "/*send CONFIRM(err) and */"IGNORE. Details below", m_stReqHeader.m_nSessionID, getGSAPServiceName( m_stReqHeader.m_ucServiceType ) );
			LOG_HEX("hdr", (byte*)&m_stReqHeader, sizeof(m_stReqHeader) );
			return false;
			
	}
	// put statuses in response
	if( m_stReqHeader.IsYGSAP() )
	{
		// for all request types on YGSAP status = YGS_INVALID_SESSION
		invRsp[STATUS_IDX] = YGS_INVALID_SESSION;
	}
	else
	{
		switch( REQUEST(m_stReqHeader.m_ucServiceType) )
		{
			case SESSION_MANAGEMENT:		invRsp[STATUS_IDX] = SESSION_NOT_EXIST;		break;
			case LEASE_MANAGEMENT:			invRsp[STATUS_IDX] = LEASE_FAIL_OTHER;		break;
			case DEVICE_LIST_REPORT:
			case TOPOLOGY_REPORT:		
			case SCHEDULE_REPORT:		
			case DEVICE_HEALTH_REPORT:	
			case NEIGHBOR_HEALTH_REPORT:
			case NETWORK_HEALTH_REPORT: 
			case NETWORK_RESOURCE_REPORT:		invRsp[STATUS_IDX] = SYSTEM_REPORT_FAILURE;	break;
			case TIME_SERVICE:			invRsp[STATUS_IDX] = TIME_FAIL_OTHER;		break;
			case CLIENT_SERVER:			invRsp[STATUS_IDX] = CLIENTSERVER_FAIL_OTHER;	break;
			case PUBLISH:				invRsp[STATUS_IDX] = PUBLISH_FAIL_OTHER;	break;
			case SUBSCRIBE:				invRsp[STATUS_IDX] = PUBLISH_FAIL_OTHER;	break;
			//case PUBLISH_TIMER:			invRsp[STATUS_IDX] = 	break;
			//case SUBSCRIBE_TIMER:			invRsp[STATUS_IDX] = 	break;
			//case WATCHDOG_TIMER:			invRsp[STATUS_IDX] = 	break;
			case BULK_TRANSFER_OPEN:		invRsp[STATUS_IDX] = BULKO_FAIL_OTHER;	break;
			case BULK_TRANSFER_TRANSFER:		invRsp[STATUS_IDX] = BULKT_FAIL_OTHER;	break;
			case BULK_TRANSFER_CLOSE:		invRsp[STATUS_IDX] = BULKC_FAIL;	break;	///TAKE CARE: NON-STANDARD retcode. bulk close knows only BULKC_SUCCESS
			case ALERT_SUBSCRIBE:			invRsp[STATUS_IDX] = ALERT_FAIL_OTHER;	break;
			//case ALERT_NOTIFY:			invRsp[STATUS_IDX] = ;	break;
			case GATEWAY_CONFIGURATION_READ:	invRsp[STATUS_IDX] = 1 /*GWCONFIG_READ_FAIL_OTHER*/;	break;
			case GATEWAY_CONFIGURATION_WRITE:	invRsp[STATUS_IDX] = 2 /*GWCONFIG_WRITE_FAIL_OTHER*/;	break;
			case DEVICE_CONFIGURATION_READ:		invRsp[STATUS_IDX] = 1 /*DEVCONFIG_READ_FAIL_OTHER*/;	break;
			case DEVICE_CONFIGURATION_WRITE:	invRsp[STATUS_IDX] = 5 /*DEVONFIG_WRITE_FAIL_OTHER*/;	break;
			default:
				LOG("ERROR Invalid session %d (service %s), "/*send CONFIRM(err) and */"IGNORE. Details below", m_stReqHeader.m_nSessionID, getGSAPServiceName( m_stReqHeader.m_ucServiceType ) );
				LOG_HEX("hdr", (byte*)&m_stReqHeader, sizeof(m_stReqHeader) );
				return false;
				
		}
	}
	// after status fill data with 0
	memset(&invRsp[STATUS_IDX+1], 0, invRspSize - 1);
	LOG("ERROR Invalid session %d (service %s), send CONFIRM(err) and IGNORE. Details below", m_stReqHeader.m_nSessionID, getGSAPServiceName( m_stReqHeader.m_ucServiceType ) );
	LOG_HEX("hdr", (byte*)&m_stReqHeader, sizeof(m_stReqHeader) );
	/// TODO: send structures particular to each device type
	SendConfirm( m_stReqHeader.m_nSessionID, m_stReqHeader.m_unTransactionID, m_stReqHeader.m_ucServiceType, (byte*)&invRsp, invRspSize);
	return false;
}

