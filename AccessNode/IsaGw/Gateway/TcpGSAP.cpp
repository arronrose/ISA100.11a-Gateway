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
/// @file TcpGSAP.cpp
/// @author Marcel Ionescu
/// @brief TCP Gateway Service Access Point implementation - the TCP server; GSAP serialiser
////////////////////////////////////////////////////////////////////////////////

#include "../../Shared/Common.h"
#include "GwApp.h"
#include "TcpGSAP.h"

////////////////////////////////////////////////////////////////////////////////
/// @class CTcpGSAP
////////////////////////////////////////////////////////////////////////////////
CTcpGSAP::CTcpGSAP( CTcpSocket * p_pTcpSocket )
: m_pTcpSocket(p_pTcpSocket), m_dwReceived(0), m_ucStatus(GSAP_WAIT_HDR)
{}

CTcpGSAP::~CTcpGSAP( void )
{
	Dump();
	clear();
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief 	Non-blocking read whatever client data is available.
/// @brief 	When full message is assembled, unpack parameters, convert to host order and dispatch to proper worker.
/// @retval true  A complete message was dispatched
/// @retval false The message is incomplete
/// @remarks Can read and assemble both partial header and partial data blocks
////////////////////////////////////////////////////////////////////////////////
bool CTcpGSAP::ProcessData( void )
{
	byte * pDest 	  = (m_ucStatus == GSAP_WAIT_HDR) ? (byte*)&m_stReqHeader  : m_aReqData;
	uint32 dwExpected = (m_ucStatus == GSAP_WAIT_HDR) ? sizeof( m_stReqHeader ) : ntohl(m_stReqHeader.m_nDataSize) ;

	if ( m_dwReceived > dwExpected)	///PROGRAMMER ERROR, we should never see this message
	{	LOG("ERROR CTcpGSAP(%d)::ProcessData: OUT OF SYNC %d > expected %d", (int)*m_pTcpSocket, m_dwReceived, dwExpected);
		return ((m_bIsValid = false));	/// This instance will be killed by GSAP manager - CGSAPList
	}
	time_t tNow = time(NULL);
	if( ( (tNow - m_tLastRx) > g_stApp.m_stCfg.m_nTCPInactivity )
	&& 	( (tNow - m_tLastTx) > g_stApp.m_stCfg.m_nTCPInactivity ) )
	{
		LOG("CTcpGSAP %s INACTIIVE %s", Identify(), Activity());
		log2flash("WARN ISA_GW: Closing inactive connection %s", Identify() );
		return ((m_bIsValid = false));	/// This instance will be killed by GSAP manager - CGSAPList
	}

	int nLen = dwExpected - m_dwReceived;
	if( nLen > 0 )
	{	/// only do read if we actually expect more data
		if(!m_pTcpSocket->Recv( 0, (void*)(pDest + m_dwReceived), nLen ))
		{	/// No data, no work. This is not an error, do not close the connection
			return false;
		}
	}	/// else there is no data section
	if( g_stApp.m_stCfg.AppLogAllowDBG() )
	{
		LOG("CTcpGSAP(%d)::ProcessData %d bytes (total %d/%d)", (int)*m_pTcpSocket, nLen, m_dwReceived+nLen, dwExpected);
	}

	switch( m_ucStatus )
	{
		case GSAP_WAIT_HDR:
			if ( (m_dwReceived += nLen) == dwExpected)
			{	uint32 unDataSize = ntohl( m_stReqHeader.m_nDataSize );

				m_ucStatus = unDataSize ? GSAP_WAIT_DATA : GSAP_MSG_READY;

				if(unDataSize > (uint32)g_stApp.m_stCfg.m_nTCPMaxSize)
				{
					LOG("ERROR CTcpGSAP(%d)::ProcessData: msg size in header: %dB -> HUGE, potential attack", (int)*m_pTcpSocket, unDataSize );
					log2flash("WARN ISA_GW: Potential attack from %s", Identify() );
					m_ucStatus = GSAP_WAIT_HDR;
					return ((m_bIsValid = false));	/// This instance will be killed by GSAP manager - CGSAPList
				}
				if(unDataSize)
				{
					m_aReqData = new byte[ unDataSize ];
				}
				m_dwReceived = 0;
			}
		break;
		case GSAP_WAIT_DATA:
			if ( (m_dwReceived += nLen) == dwExpected)
			{
				m_ucStatus = GSAP_MSG_READY;
				m_dwReceived = 0;
			}
		break;
		default:
			LOG("ERROR CTcpGSAP(%d)::ProcessData: unexpected status %u", m_ucStatus);
			return false;
	}
	if( m_ucStatus == GSAP_MSG_READY )
	{
		dispatchUserReq();

		if( nLen)
		{
			delete [] m_aReqData;	///< data processed, reclaim space
		}
		m_aReqData = NULL;
		m_ucStatus = GSAP_WAIT_HDR;
		++m_dwRequestCount;
		m_tLastRx = time(NULL);
		m_dwTotalRxSize += m_dwReceived + sizeof(m_stReqHeader); // m_dwReceived == ntohl( m_stReqHeader.m_nDataSize )
		m_dwReceived = 0;

		return true;	/// full message received and processed
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> User) Pack parameters and send the message to the client.
/// @brief TAKE CARE: This is low-level method, DO NOT USE: use CGSAP::SendConfirm instead
/// @param p_nSessionID		GS_Session_ID
/// @param p_nTransactionID GS_Transaction_ID
/// @param p_ucServiceType	The service type
/// @param p_pMsgData 		data starting with GS_Status, type-dependent. Does not include data CRC
/// @param p_dwMsgDataLen 	data len, type-dependent. Does not include data CRC
/// @retval true message was sent
/// @retval false message can't be sent, socket closed
/// @remarks the caller must check the session on both messages from the field and client
////////////////////////////////////////////////////////////////////////////////
bool CTcpGSAP::SendMessage( uint32 p_nSessionID, uint32 p_nTransactionID, uint8 p_ucServiceType, uint8* p_pMsgData, uint32 p_dwMsgDataLen )
{	/// TAKE CARE: we rely on last received header (m_stReqHeader) to determine protocol type when answering back.
	/// Not good, in case the client switch protocol types on the same session or use different
	/// protocols on separate sessions (both cases are forbidden)
	/// TODO: check for a single protocol type on each GSAP

	size_t unSize =  sizeof(TGSAPHdr) + p_dwMsgDataLen + sizeof(uint32) ;
	char buf[  unSize ];
	char* pCrc = buf + sizeof(TGSAPHdr) + p_dwMsgDataLen;

	TGSAPHdr * pstHeader = (TGSAPHdr *)&buf;
	pstHeader->m_ucVersion		= m_stReqHeader.IsYGSAP() ? PROTO_VERSION_YGSAP : PROTO_VERSION_GSAP;
	pstHeader->m_ucServiceType	= p_ucServiceType;
	pstHeader->m_nSessionID		= p_nSessionID;
	pstHeader->m_unTransactionID= p_nTransactionID;
	pstHeader->m_nDataSize		= p_dwMsgDataLen + sizeof(uint32); //crc
	pstHeader->m_nCrc			= 0;
	pstHeader->HTON();
	pstHeader->m_nCrc			= crc( (byte*)pstHeader, offsetof(TGSAPHdr, m_nCrc) );
	memcpy( buf + sizeof(TGSAPHdr), p_pMsgData, p_dwMsgDataLen );
	uint32 c				= crc( p_pMsgData, p_dwMsgDataLen ) ;
	memcpy(pCrc, &c, sizeof c);

	char szStatus[ 32 ];
	char szTransaction[ 16 ];
	szStatus[ 0 ] = szTransaction[ 0 ] = 0;

	switch( REQUEST(p_ucServiceType) )
	{
		case PUBLISH:
		case WATCHDOG_TIMER:
		case SUBSCRIBE_TIMER:
		case ALERT_NOTIFY:
			if( IS_INDICATION(p_ucServiceType) )
			{	/// HACK: mask displayed transaction ID - just for pretty log. The user gets the correct id.
				if (g_stApp.m_stCfg.AppLogAllowINF())
				{					
					sprintf( szTransaction, "| %4u", (unsigned)ntohl(pstHeader->m_unTransactionID) & 0x7FFFFFFF  );
				}
				szStatus[0] = 0; ///< Publish indications do not have status
			}
			break;
		default:	/// ALL other services have status and regular transaction id reported
			snprintf( szStatus, sizeof(szStatus), " %d:%s", *(byte*)p_pMsgData, getGSAPStatusName( pstHeader->m_ucVersion, pstHeader->m_ucServiceType, *(byte*)p_pMsgData ) );
			szStatus[ sizeof(szStatus) - 1 ] = 0;
			sprintf(  szTransaction, " %5u", (unsigned)ntohl(pstHeader->m_unTransactionID) );
	}

#define SAP_OUT_FORMAT "SAP_OUT(S %2u T%s) %s%s data(%d)"
	if( g_stApp.m_stCfg.AppLogAllowINF()
	&&	(PUBLISH         != REQUEST(p_ucServiceType))
	&&	(SUBSCRIBE_TIMER != REQUEST(p_ucServiceType)) )
	{
		LOG( SAP_OUT_FORMAT " %s%s", ntohl(pstHeader->m_nSessionID), szTransaction,
			getGSAPServiceName( pstHeader->m_ucServiceType ), szStatus, p_dwMsgDataLen,
			GET_HEX_LIMITED(p_pMsgData, p_dwMsgDataLen, (unsigned)g_stApp.m_stCfg.m_nMaxSapOutDataLog ) );
	}
	else
	{
		if (g_stApp.m_stCfg.AppLogAllowINF())
		{
			LOG( SAP_OUT_FORMAT, ntohl(pstHeader->m_nSessionID), szTransaction,
				getGSAPServiceName( pstHeader->m_ucServiceType ), szStatus, p_dwMsgDataLen );
		}
	}

	if( IS_INDICATION(p_ucServiceType) )
		++ m_dwIndicationCount;
	else if( IS_CONFIRM(p_ucServiceType) )
		++m_dwConfirmCount;
	else if( IS_RESPONSE(p_ucServiceType) )
		++m_dwResponseCount;

	m_tLastTx = time(NULL);
	m_dwTotalTxSize += unSize;

	return m_pTcpSocket->Send( buf, sizeof(buf) );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief  Log the object content
/// @remarks use a signal to log the whole app status
////////////////////////////////////////////////////////////////////////////////
void CTcpGSAP::Dump( void )
{
	CGSAP::Dump();
	LOG("  TcpGSAP: %s rcv %u state: %s", Identify(), m_dwReceived,
		( m_ucStatus == GSAP_WAIT_HDR )  ? "WAIT_HDR"  :
		( m_ucStatus == GSAP_WAIT_DATA ) ? "WAIT_DATA" :
		( m_ucStatus == GSAP_MSG_READY ) ? "MSG_READY" : "STATUS_UNK" );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Clear the object, reset all buffers, close the socket, invalidate the connection
/// @return false - just a convenience for the callers to return dirrectly false
/// @remarks The container CGSAPList will delete CGSAP instances with IsValid() returning false
/// which means: after calling Close(), CTcpGSAP::IsValid() returns false,
/// therefore this instance will be destroyed by the container
////////////////////////////////////////////////////////////////////////////////
bool CTcpGSAP::clear( void )
{
	if( m_pTcpSocket)
		delete m_pTcpSocket;
	m_pTcpSocket	= NULL;

	m_ucStatus = GSAP_WAIT_HDR;
	m_dwReceived	= 0;
	return CGSAP::clear();
}


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Identify CTcpGSAP using ip, port, socket
/// @return Statically allocatted buffer with the string
/// @remarks The buffer will contain INVALID in case the socket is invalid
////////////////////////////////////////////////////////////////////////////////
const char * CTcpGSAP::Identify( void )
{	static char sBuf [ 128 ];
	if( m_pTcpSocket )
		sprintf(sBuf, "%s:%u fd %d", m_pTcpSocket->GetIPString(), m_pTcpSocket->GetPort(), (int)*m_pTcpSocket );
	else
		sprintf(sBuf, "INVALID");

	return sBuf;
}

