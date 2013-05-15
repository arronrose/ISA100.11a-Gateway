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
/// @file SystemReportSvc.cpp
/// @author Marcel Ionescu
/// @brief system Report services GSAP - implementation
////////////////////////////////////////////////////////////////////////////////
#include "../../Shared/Common.h"

#include "GwApp.h"
#include "SystemReportSvc.h"
#include "SAPStruct.h"

static bool isDeviceListFilterReq( const byte * p_pRequestDetails );

////////////////////////////////////////////////////////////////////////////////
/// @struct CSystemReportService::TGenerateReportRsp
/// @brief Response to Generate Report command
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Return the report size
/// @param p_ucServiceType - service type whose size is queried
////////////////////////////////////////////////////////////////////////////////
uint32	CSystemReportService::TGenerateReportRsp::ReportSize( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case DEVICE_LIST_REPORT:		return m_unDeviceListSize;
		case TOPOLOGY_REPORT:			return m_unTopologySize;
		case SCHEDULE_REPORT:			return m_unScheduleSize;
		case DEVICE_HEALTH_REPORT:		return m_unDeviceHealthSize;
		case NEIGHBOR_HEALTH_REPORT:	return m_unNeighborHealthSize;
		case NETWORK_HEALTH_REPORT:		return m_unNetworkHealthSize;
		case NETWORK_RESOURCE_REPORT:	return m_unNetworkResourceSize;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Return the report handler
/// @param p_ucServiceType - service type whose handler is queried
////////////////////////////////////////////////////////////////////////////////
uint32	CSystemReportService::TGenerateReportRsp::ReportHandler( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case DEVICE_LIST_REPORT:		return m_unDeviceListHandler;
		case TOPOLOGY_REPORT:			return m_unTopologyHandler;
		case SCHEDULE_REPORT:			return m_unScheduleHandler;
		case DEVICE_HEALTH_REPORT:		return m_unDeviceHealthHandler;
		case NEIGHBOR_HEALTH_REPORT:	return m_unNeighborHealthHandler;
		case NETWORK_HEALTH_REPORT:		return m_unNetworkHealthHandler;
		case NETWORK_RESOURCE_REPORT:	return m_unNetworkResourceHandler;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @class CSystemReportService
/// @brief System Report services
////////////////////////////////////////////////////////////////////////////////

/// leave the user-generated report to be the last, in order to have correct RTT
void CSystemReportService::reportFunc_newReport( byte p_ucServiceType, uint32 p_unAppHandle, uint32 p_p2 /*TGenerateReportRsp **/, uint32 p_p3 /* MSG* */ )
{	TGenerateReportRsp * pRspData = (TGenerateReportRsp *) p_p2;
	MSG* pMSG= (MSG*) p_p3;
	if( pRspData->ReportSize( p_ucServiceType ) && (pMSG->m_ServiceType != p_ucServiceType) )
		newReport( p_unAppHandle, p_ucServiceType, pRspData->ReportSize( p_ucServiceType ), pRspData->ReportHandler( p_ucServiceType ) );
}
////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Process an ADPU type
/// @brief CALL whenever an APDU for this class arrives (The method is called from CInterfaceObjMgr::DispatchAPDU trough Service::ProcessAPDU)
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_pAPDUIdtf - needed for device TX time with usec resolution
/// @param p_pRsp The response from field, unpacked
/// @param p_pReq The original request, unpacked
/// @retval true - field APDU dispatched ok
/// @retval false - field APDU not dispatched - one of the various verifications failed and was logged
/// @remarks only APDU's of type SRVC_EXEC_RSP are usefull - used for Topology and DevList
///   May return the response trough CGSAP::SendConfirm in case a full response is available(topology for now)
///   CALL whenever an APDU for this class arrives
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::ProcessAPDU( uint16 p_unAppHandle, APDU_IDTF* p_pAPDUIdtf, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	MSG * pMSG = g_stApp.m_oGwUAP.m_tracker.FindMessage( p_unAppHandle, "SystemReportService::ProcessAPDU" );
	if(!pMSG)
		return false;	/// we've got nothin' else to do

	if( !validateAPDU( p_unAppHandle, p_pRsp, p_pReq ) )
	{	/// Send confirm with error to GSAP indentified by p_unAppHandle.
		confirm2All_Error( pMSG->m_ServiceType, p_unAppHandle );
		return false;
	}

	switch( p_pReq->m_stSRVC.m_stExecReq.m_ucMethID )
	{
		case SMO_METHOD_GENERATE_REPORT:
		{
			TGenerateReportRsp * pRspData = (TGenerateReportRsp*)p_pRsp->m_stSRVC.m_stExecRsp.p_pRspData;
			if( p_pRsp->m_stSRVC.m_stExecRsp.m_unLen != sizeof (TGenerateReportRsp) )
			{	LOG( "WARNING SystemReportService::ProcessAPDU: SMO.generateReport: rsp size %d != %d", p_pRsp->m_stSRVC.m_stExecRsp.m_unLen, sizeof (TGenerateReportRsp) );
				return confirm2All_Error( pMSG->m_ServiceType, p_unAppHandle);
			}
			pRspData->NTOH();
			if( !pRspData->ReportSize(pMSG->m_ServiceType) || !pRspData->ReportHandler(pMSG->m_ServiceType) )
			{	LOG( "WARNING SystemReportService::ProcessAPDU: SMO.generateReport type %u (%u/%u): report not usable",
				   pMSG->m_ServiceType, pRspData->ReportSize(pMSG->m_ServiceType), pRspData->ReportHandler(pMSG->m_ServiceType));
				return confirm2All_Error( pMSG->m_ServiceType, p_unAppHandle);
			}
			if( m_dwMaxBlockSize && (m_dwMaxBlockSize != pRspData->m_unMaxBlockSize) )
			{
				LOG( "ERROR SystemReportService::ProcessAPDU: SM changed max block size %d -> %d. It could damage current transfers", m_dwMaxBlockSize, pRspData->m_unMaxBlockSize );
			}
			m_dwMaxBlockSize = pRspData->m_unMaxBlockSize;
			/// This approach is generic, better than using pMSG->m_ServiceType
			/// However, it is sensitive to a malfunctioning SM returning size !0 for services not requested
			for_each_report( &CSystemReportService::reportFunc_newReport, p_unAppHandle, (uint32)pRspData, (uint32)pMSG );
			/// leave the user-generated report to be the last, in order to have correct RTT
			newReport( p_unAppHandle, pMSG->m_ServiceType, pRspData->ReportSize(pMSG->m_ServiceType), pRspData->ReportHandler(pMSG->m_ServiceType) );
			return true;	//processed ok
		}

		case SMO_METHOD_GET_BLOCK:
		{
			TSMOGetBlockParameters *pReqData = (TSMOGetBlockParameters*)p_pReq->m_stSRVC.m_stExecReq.p_pReqData;	// get from request
			pReqData->NTOH();	/// it is stored network order in request
			if( pReqData->size != p_pRsp->m_stSRVC.m_stExecRsp.m_unLen )
			{	LOG( "WARNING SystemReportService::ProcessAPDU: SMO.getBlock rsp size %d != req %d", p_pRsp->m_stSRVC.m_stExecRsp.m_unLen, pReqData->size );
				return confirm2All_Error( pMSG->m_ServiceType, p_unAppHandle );
			}
			if( addReportBlock( pMSG->m_ServiceType, p_unAppHandle, pReqData->handler, pReqData->offset, p_pRsp->m_stSRVC.m_stExecRsp.m_unLen, p_pRsp->m_stSRVC.m_stExecRsp.p_pRspData ) )
			{	/// Report COMPLETE: Send report to all clients with a requests pending
				return confirm2All(p_unAppHandle);
			}
			break;
		}
		// default made IMPOSSIBLE by validateAPDU
	}
	return false;	// The topology is not yet complete
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Process a user request
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @retval true Topology is available and was sent to the client trough CGSAP::SendConfirm
/// @retval false Topology not available, request sent to SM
/// @remarks CALL on user requests
/// (The method is called from CInterfaceObjMgr::DispatchUserRequest)
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::ProcessUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{
	if( !validateUserRequest( p_pHdr, p_pData ) )
		return false;

	byte *pReqTranslatedData = NULL;
	CGSAP::TGSAPHdr hdr = *p_pHdr; //store it, in case we need it later untranslated
	int status = translateYGSAP(p_pHdr, p_pData, &pReqTranslatedData);
	
	if (p_pHdr->IsYGSAP())
	{
		switch(status)
		{
			case YGSAP_IN::STATUS_SUCCESS:
				break;
			case YGSAP_IN::STATUS_ALL_TAGS:
				if (p_pHdr->m_ucServiceType == DEVICE_LIST_REPORT)
				{
					break;
				}
			case YGSAP_IN::STATUS_INVALID_TAG_SELECTOR:
				confirm_Error( p_pHdr->m_ucServiceType, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, YGS_FAILURE );
				return false;
			case YGSAP_IN::STATUS_TAG_NOT_FOUND:
			default:
				LOG("WARNING SystemReportService::ProcessUserRequest: cannot translate tag, fail request");
				confirm_Error( p_pHdr->m_ucServiceType, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, YGS_TAG_NOT_FOUND );
				return false;
		
		}		
	}

	if( returnFromCache( p_pHdr, pReqTranslatedData ) )
		return true;

	if ( isRequestPending( p_pHdr->m_ucServiceType ) )	///< Request already pending, do not send a new request
	{
		if( !matchRequest2CacheData( p_pHdr->m_ucServiceType, pReqTranslatedData) )
		{	/// We do not support simultaneous req of the same type (NEIGHBOR_HEALTH_REPORT) but with different parameters
			/// In case of mismatch add to pipeline to be sent after the pending request response
			LOG("PIPELINE add  (S %2u T %5u) %s (Size %u) %s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,
				getGSAPServiceName( hdr.m_ucServiceType ), m_lstRequestPipeline.size(), GetHex( p_pData, hdr.m_nDataSize ));
			m_lstRequestPipeline.push_back( WaitingRequest(&hdr, p_pData) );
			return true;
		}

		/// ADD pending request of this type
		m_aReport[ idx(p_pHdr->m_ucServiceType) ].m_lstPendingRequests.push_back( PendingRequest(p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, (byte*)pReqTranslatedData, p_pHdr->m_nDataSize) );

		LOG("SystemReportService::ProcessUserRequest(S %u T %u) request in progress: total %d %s requests pending", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,
			m_aReport[ idx(p_pHdr->m_ucServiceType) ].m_lstPendingRequests.size(), getGSAPServiceNameNoDir( p_pHdr->m_ucServiceType ) );

		return true;	/// User request processed ok, regardless if the service was available or not
	}

	if( m_unSysMngContractID != INVALID_CONTRACTID )
	{	/// Request system report(m_ucServiceType) from SM SMO. Initiate system report(m_ucServiceType) transfer
		requestReport( p_pHdr, pReqTranslatedData );
	}
	else
	{
		LOG("WARN SystemReportService::ProcessUserRequest[%s] NO SM CONTRACT", getGSAPServiceNameNoDir( p_pHdr->m_ucServiceType ) );
		confirm_Error( p_pHdr->m_ucServiceType, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID );
	}
	return true;	/// User request processed ok, regardless if the service was available or not
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA -> GW) Process a ISA100 timeout of a req originated in this service
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_pOriginalReq the original request
/// @retval true - confirm sent to user
/// @retval false - confirm not sent to user
/// @remarks
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::ProcessISATimeout( uint16 p_unAppHandle, GENERIC_ASL_SRVC * p_pOriginalReq )
{
	LOG("SystemReportService::ProcessISATimeout(H %u): MethID %u", p_unAppHandle, p_pOriginalReq->m_stSRVC.m_stExecReq.m_ucMethID );
	MSG * pMSG = g_stApp.m_oGwUAP.m_tracker.FindMessage( p_unAppHandle, "SystemReportService::ProcessISATimeout" );
	if(!pMSG)
		return false;	/// we've got nothin' else to do

	switch( p_pOriginalReq->m_stSRVC.m_stExecReq.m_ucMethID )
	{	case SMO_METHOD_GENERATE_REPORT: case SMO_METHOD_GET_BLOCK:	break;
		default: LOG("ERROR SystemReportService::ProcessISATimeout(H %u): invalid MethodId %d. Clear anyway", p_unAppHandle, p_pOriginalReq->m_stSRVC.m_stExecReq.m_ucMethID );
	}
	return confirm2All_Error( pMSG->m_ServiceType, p_unAppHandle );
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
bool CSystemReportService::CanHandleServiceType( uint8 p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case DEVICE_LIST_REPORT:
		case TOPOLOGY_REPORT:
		case SCHEDULE_REPORT:
		case DEVICE_HEALTH_REPORT:
		case NEIGHBOR_HEALTH_REPORT:
		case NETWORK_HEALTH_REPORT:
		case NETWORK_RESOURCE_REPORT:
			return true;
	}
	return false;
};

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) find the index in m_aReport function by GSAP ServiceType
/// @param p_ucServiceType		GSAP service type
/// @return SRVC_IDX in m_aReport
/// @note the caller *MUST* ensure p_ucServiceType is in valid range
////////////////////////////////////////////////////////////////////////////////
CSystemReportService::SRVC_IDX CSystemReportService::idx( unsigned char p_ucServiceType ) const
{
	switch( p_ucServiceType )
	{
		case DEVICE_LIST_REPORT: 		return SRVC_IDX_DEVICE_LIST;
		case TOPOLOGY_REPORT: 			return SRVC_IDX_TOPOLOGY;
		case SCHEDULE_REPORT:			return SRVC_IDX_SCHEDULE;
		case DEVICE_HEALTH_REPORT:		return SRVC_IDX_DEVICE_HEALTH;
		case NEIGHBOR_HEALTH_REPORT:	return SRVC_IDX_NEIGHBOR_HEALTH;
		case NETWORK_HEALTH_REPORT:		return SRVC_IDX_NETWORK_HEALTH;
		case NETWORK_RESOURCE_REPORT:	return SRVC_IDX_NETWORK_RESOURCE;
	}
	LOG("ERROR SystemReportService::idx(%u): unknown GSAP service type. MALFUNCTION.", p_ucServiceType);
	return SRVC_IDX_MAX;	///It should NEVER get here
}

void CSystemReportService::reportFunc_clean( byte p_ucServiceType, uint32 /*p_p1*/, uint32 /*p_p2*/, uint32 /*p_p3*/  )
{	Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	clean( p_ucServiceType, pRep );
}

void CSystemReportService::SetSMContractID( uint16 p_unSysMngContractID /*= INVALID_CONTRACTID*/ )
{
	m_unSysMngContractID = p_unSysMngContractID;
	if( INVALID_CONTRACTID == m_unSysMngContractID )
	{	///reset pending requests and the state machines for all reports
		for_each_report( &CSystemReportService::reportFunc_clean );
	}
};

void CSystemReportService::reportFunc_dump( byte p_ucServiceType, uint32 /*p_p1*/, uint32 /*p_p2*/, uint32 /*p_p3*/  )
{
	Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	LOG(" %s pending(%s): %u req, ServiceH %4u size %4u/%-4u age %4d valid(%d): %s %s",
		getGSAPServiceNameNoDir( p_ucServiceType ), pRep->m_bRequestPending ? "yes": " no",
		pRep->m_lstPendingRequests.size(), pRep->m_unHandler, pRep->m_unReceived,
		pRep->m_unSize, age( p_ucServiceType ), g_stApp.m_stCfg.m_nSystemReportsCacheTimeout,
		isValid( p_ucServiceType, g_stApp.m_stCfg.m_nSystemReportsCacheTimeout)?"YES":"NO ",
		details( p_ucServiceType ) );
	if((g_stApp.m_stCfg.AppLogAllowINF() && ((p_ucServiceType == DEVICE_LIST_REPORT) || (p_ucServiceType == NETWORK_HEALTH_REPORT)))
	||	g_stApp.m_stCfg.AppLogAllowDBG() )
	{
		dumpReport( p_ucServiceType );
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Dump pipeline status to LOG
/// @remarks Called on Dump (on USR2)
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::dumpPipeline( void )
{
	for( TRequestPipeline::iterator it = m_lstRequestPipeline.begin(); it != m_lstRequestPipeline.end(); ++it )
	{	LOG("  PIPELINE: req (S %u T %u) %s: %s", it->m_oHdr.m_nSessionID, it->m_oHdr.m_unTransactionID,
			getGSAPServiceNameNoDir( it->m_oHdr.m_ucServiceType ), GetHex(it->m_pData, it->m_oHdr.m_nDataSize) );
	}
}


/** do not delete
void CSystemReportService::Report::Dump( void ) const
{
	for( TRequestList::const_iterator it = m_lstPendingRequests.begin(); it != m_lstPendingRequests.end(); ++it )
	{	/// Send to subsequent requestors, BUT DO not remove from tracker (subsequent requestors are not added to tracker)
		LOG( "  DEBUG  S %u T %u ServiceH %u size %u", it->m_nSessionID, it->m_dwTransactionID, m_unHandler, m_unSize);
	}
}
*/

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Dump object status to LOG
/// @remarks Called on USR2
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::Dump( void )
{
	LOG("%s: OID %u. Waiting in pipeline %u", Name(), m_ushInterfaceOID, m_lstRequestPipeline.size());
	dumpPipeline();
	for_each_report( &CSystemReportService::reportFunc_dump );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send the report received as parameter to ALL clients with pending requests including the first caller
/// @retval true always - help handling and returning in one instruction
/// @remarks different pending requests may have different details
/// (SCHEDULE_REPORT/NEIGHBOR_HEALTH_REPORT: device; DEVICE_HEALTH_REPORT: device list)
/// For each pending request, we format the response according to the request details
///
/// @note Auto-generated requests (i.e. without a corresponding SAP_IN) are tracked with m_nSessionID == 0
/// @note and m_dwTransactionID == 0. This method does not send SAP_OUT for messages with m_nSessionID == 0
///
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::confirm2All( uint16 p_unAppHandle )
{
	MSG * pMSG = g_stApp.m_oGwUAP.m_tracker.FindMessage( p_unAppHandle, "SystemReportService::confirm2All" );
	if(!pMSG)
		return false;	/// we've got nothin' else to do
	byte ucServiceType = pMSG->m_ServiceType;	/// pMsg may be deleted from tracker few lines below (m_tracker.RemoveMessage)
	Report	* pRep = &m_aReport[ idx( ucServiceType ) ];
	byte	* pBin = pRep->m_pBin;
	uint32	dwSize = pRep->m_unSize;
	byte	aucFormattedTmp [ dwSize ];	/// One schedule/device_health report cannot get bigger than this

	if( 0 != pMSG->m_nSessionID )	/// Normal flow for client requests (SAP_IN)
	{
		if( formatReport( ucServiceType, (const byte*&)pBin, dwSize, pRep->m_pExtraData, aucFormattedTmp ) )
			confirm( p_unAppHandle, pBin, dwSize );	///< Send to the original requestor
		else
			confirm_Error( ucServiceType, p_unAppHandle );
	}
	else	///< GW-generated message, we don't want SAP_OUT for (inexistent) original request
	{		/// Just need to remove from tracker
		g_stApp.m_oGwUAP.m_tracker.RemoveMessage( p_unAppHandle );	///TAKE CARE: pMSG(p_unAppHandle) is invalid now
	}

	for( TRequestList::const_iterator it = pRep->m_lstPendingRequests.begin(); it != pRep->m_lstPendingRequests.end(); ++it )
	{	pBin = pRep->m_pBin;		/// re-initialise, it was changed by previous formatReport() call
		dwSize = pRep->m_unSize;	/// re-initialise, it was changed by previous formatReport() call
		/// Send to subsequent requestors, BUT DO not remove from tracker (subsequent requestors are not added to tracker)
		if( formatReport( ucServiceType, (const byte*&)pBin, dwSize, it->m_pRequestData, aucFormattedTmp ) )
			confirm( ucServiceType, it->m_nSessionID, it->m_dwTransactionID, pBin, dwSize, false );
		else
			confirm_Error( ucServiceType, it->m_nSessionID, it->m_dwTransactionID );
	}
	pRep->m_lstPendingRequests.clear();
	pRep->m_bRequestPending = false;
	processPipeline( ucServiceType );	/// confirm to all matching, send next request to field
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send the device list CONFIRM with ERROR code to ALL clients with pending requests
/// @brief including the first caller - specified by p_unAppHandle
/// @param p_unAppHandle application handle identifying the first requestor
/// @param p_ucStatus device list confirm status to send. Must be some error
/// @retval true - confirm was sent to client
/// @retval false - confirm was not sent to client
/// @remarks
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::confirm2All_Error( uint8 p_ucServiceType, uint16 p_unAppHandle )
{
	confirm_Error( p_ucServiceType, p_unAppHandle ); ///< Send to the original requestor, remove from tracker

	Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	for( TRequestList::const_iterator it = pRep->m_lstPendingRequests.begin(); it != pRep->m_lstPendingRequests.end(); ++it )
	{	/// Send to subsequent requestors, BUT DO not remove from tracker (subsequent requestors are not added to tracker)
		confirm_Error( p_ucServiceType, it->m_nSessionID, it->m_dwTransactionID );
	}
	pRep->m_lstPendingRequests.clear();
	pRep->m_bRequestPending = false;
	processPipeline( p_ucServiceType );/// confirm to all matching, send next request to field
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send confirm device list/device list on connection GSAP, determined from session id, if connection is valid
/// @param p_nSessionID
/// @param p_TransactionID
/// @param p_ucServiceType
/// @retval true - the report was sent to client
/// @retval false - the report was not sent to client
/// @remarks
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::confirm( byte p_ucServiceType, unsigned p_nSessionID, uint32 p_TransactionID, byte * p_pBin, uint32 p_unSize, bool p_bRemoveFromTracker )
{	uint8 tmp[ 1 + p_unSize ];
 	tmp[ 0 ]= SYSTEM_REPORT_SUCCESS;	///< GS_Status on first byte. Same as YGS_SUCCESS
	memcpy( tmp+1, p_pBin, p_unSize );	// grr, data copy, should be avoided

	return g_stApp.m_oGwUAP.SendConfirm( p_nSessionID, p_TransactionID, p_ucServiceType, tmp, sizeof(tmp), p_bRemoveFromTracker );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send confirm device list/topology on connection GSAP determined from p_unAppHandle, if connection is valid
/// @param p_unAppHandle
/// @retval true - the device list was sent to client
/// @retval false - the device list was not sent to client
/// @remarks TAKE CARE: expects pMSG(p_unAppHandle)->m_nSessionID != 0 (guaranteed by the caller: confirm2All)
/// Messages registered in tracker with m_nSessionID==0 are self-generated and should not get SAP_OUT
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::confirm( uint16 p_unAppHandle, byte * p_pBin, uint32 p_unSize )
{	uint8 tmp[ 1 + p_unSize ];
 	tmp[ 0 ]= SYSTEM_REPORT_SUCCESS;	///< GS_Status on first byte. Same as YGS_SUCCESS
	memcpy( tmp+1, p_pBin, p_unSize );	// grr, data copy, should be avoided

	return g_stApp.m_oGwUAP.SendConfirm( p_unAppHandle, tmp, sizeof(tmp), true );	/// do remove from tracker
}

inline size_t CSystemReportService::minConfirmDataSize( byte p_ucServiceType ) const
{	switch(p_ucServiceType)
	{ 	case DEVICE_LIST_REPORT: case TOPOLOGY_REPORT: case NEIGHBOR_HEALTH_REPORT:
		case DEVICE_HEALTH_REPORT:		return 3;	///GS_Status(1)+numberOfDevices(2)/numberOfNeighbors(2)
		case SCHEDULE_REPORT:			return 4;	///GS_Status(1)+numberOfChannels(1)+numberOfDevices(2)
		case NETWORK_HEALTH_REPORT:		return 3 + sizeof(SAPStruct::NetworkHealthReportRsp::NetworkHealth);///GS_Status(1)+numberOfDevices(2)+sizeof(NetworkHealth)
		case NETWORK_RESOURCE_REPORT:	return 2 ;	///GS_Status(1)+SubnetNr(1)
	}
	return 0;
};


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send a confirm with status p_ucStatus (MUST be error) to client specified by p_unAppHandle
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID/ServiceType in tracker
/// @param p_ucStatus
/// @remarks  Send just the status, with an empty device list/topology
/// Session/transaction/service type are taken from tracker entry pointed by p_unAppHandle
/// TODO Check the structure is appropriate for ALL system reports, or change accordingly
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::confirm_Error( byte p_ucServiceType, uint16 p_unAppHandle )
{
	MSG * pMSG = g_stApp.m_oGwUAP.m_tracker.FindMessage( p_unAppHandle, "confirm_Error" );
	if( pMSG && (0 == pMSG->m_nSessionID) )
	{
		g_stApp.m_oGwUAP.m_tracker.RemoveMessage( p_unAppHandle );	///pMSG is invalid now
		return;	///< GW-generated message, we don't want SAP_OUT
	}

	size_t dwSize = minConfirmDataSize( p_ucServiceType );
	uint8  tmp[ dwSize ];
	memset( tmp+1, 0, dwSize-1);
	tmp [ 0 ] = g_pSessionMgr->IsYGSAPSession(pMSG->m_nSessionID) ? YGS_FAILURE : SYSTEM_REPORT_FAILURE;

	g_stApp.m_oGwUAP.SendConfirm( p_unAppHandle, tmp, dwSize, true );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send a confirm with type p_ucServiceType and status YGS_FAILURE or SYSTEM_REPORT_FAILURE to client specified by p_nSessionID
/// @param p_ucServiceType
/// @param p_nSessionID
/// @param p_TransactionID
/// @remarks  Send just the status, with an empty system report. Switch YGS_FAILURE / SYSTEM_REPORT_FAILURE according to session type (YGSAP/GSAP)
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::confirm_Error( byte p_ucServiceType, unsigned p_nSessionID, uint32 p_TransactionID )
{
	confirm_Error( p_ucServiceType, p_nSessionID, p_TransactionID, g_pSessionMgr->IsYGSAPSession(p_nSessionID) ? YGS_FAILURE : SYSTEM_REPORT_FAILURE );
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send a confirm with type p_ucServiceType and status p_ucStatus (MUST be error) to client specified by p_nSessionID
/// @param p_ucServiceType
/// @param p_nSessionID
/// @param p_TransactionID
/// @param p_ucStatus
/// @remarks  Send just the status, with an empty system report
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::confirm_Error( byte p_ucServiceType, unsigned p_nSessionID, uint32 p_TransactionID, uint8_t p_ucStatus )
{	size_t dwSize = minConfirmDataSize( p_ucServiceType ) ;
	uint8  tmp[ dwSize ];
	memset( tmp+1, 0, dwSize-1);
	tmp [ 0 ] = p_ucStatus;

	g_stApp.m_oGwUAP.SendConfirm( p_nSessionID, p_TransactionID, p_ucServiceType, tmp, sizeof(tmp), false );
}

#define SMO_REPORT_REQ_DEVICE_LIST			0x0001
#define SMO_REPORT_REQ_TOPOLOGY				0x0002
#define SMO_REPORT_REQ_SCHEDULE				0x0004
#define SMO_REPORT_REQ_DEVICE_HEALTH		0x0008
#define SMO_REPORT_REQ_NEIGHBOR_HEALTH		0x0010
#define SMO_REPORT_REQ_NETWORK_HEALTH		0x0020
#define SMO_REPORT_REQ_NETWORK_RESOURCE		0x0040

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> ISA100) Prepare to receive a new report: start timer, mark as pending, reset received bytes counter
/// @param p_ucServiceType	The service requestes
/// @param p_Bitmap		Bitmap specifying report type. Not the same as p_ucServiceType in auto-generated
/// @note return is on a branch only, break is still needed after it.
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::reportFunc_reset( byte p_ucServiceType, uint32 p_Bitmap, uint32 p_nSessionID, uint32 /*p_p3*/ )
{
	/// Reset only reports which are requested.
	switch( p_ucServiceType )
	{	case DEVICE_LIST_REPORT: 		if(!( p_Bitmap & SMO_REPORT_REQ_DEVICE_LIST))		return; break;
		case TOPOLOGY_REPORT:	 		if(!( p_Bitmap & SMO_REPORT_REQ_TOPOLOGY))			return; break;
		case SCHEDULE_REPORT:			if(!( p_Bitmap & SMO_REPORT_REQ_SCHEDULE)) 			return; break;
		case DEVICE_HEALTH_REPORT:		if(!( p_Bitmap & SMO_REPORT_REQ_DEVICE_HEALTH))		return; break;
		case NEIGHBOR_HEALTH_REPORT:	if(!( p_Bitmap & SMO_REPORT_REQ_NEIGHBOR_HEALTH))	return; break;
		case NETWORK_HEALTH_REPORT:		if(!( p_Bitmap & SMO_REPORT_REQ_NETWORK_HEALTH))	return; break;
		case NETWORK_RESOURCE_REPORT: 	if(!( p_Bitmap & SMO_REPORT_REQ_NETWORK_RESOURCE))	return; break;
	}
//	LOG("reportFunc_reset: %s serviceType %d, idx %d bitmap %02X session %d", getGSAPServiceNameNoDir(p_ucServiceType), p_ucServiceType, idx( p_ucServiceType ), p_Bitmap, p_nSessionID);

	Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	pRep->m_uSecGen.MarkStartTime();/// Mark time when report was requested, to measure the total RTT
	pRep->m_bRequestPending = true;	///< Report transfer in progress: all new requests will wait response to this one.
	pRep->m_nSessionID	= p_nSessionID;
	pRep->m_unReceived      = 0;	///< invalidate older report
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> ISA100) Send request to SM: SMO.generateReport
/// @param p_ucServiceType	The service requestes
/// @param p_nSessionID
/// @param p_TransactionID
/// @param p_pRequestData - it is non-NULL only for SCHEDULE_REPORT / NEIGHBOR_HEALTH / DEVICE_HEALTH_REPORT / DEVICE_LIST_REPORT (on YGSAP)
/// @param p_dwReqDataLen if data is not empty, it includes also the data CRC - 4 extra bytes
/// @retval TBD
/// @note parameter p_pRequestData is only used by NEIGHBOR_HEALTH_REPORT / DEVICE_HEALTH_REPORT and DEVICE_LIST_REPORT if YGSAP
/// @see TGenerateReportCmd in SystemReportSvc.h
/// @note in case of NETWORK_HEALTH_REPORT request contracts list before requesting
///		the report in order to compute GPDU statistics. Altough there is NO guarantee
///		the contract list will be returned before NETWORK_HEALTH_REPORT, there is
///		a good chance for it to happen
/// @remarks TODO: make here significant optimisations: request several reports at once.
/// @remarks DISABLED due to incorrect handling. TODO troubleshoot
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::requestReport( const CGSAP::TGSAPHdr * p_pHdr, const void * p_pRequestData )
{
	TGenerateReportCmd oCmd;
	memset( oCmd.m_aucDeviceNeighborHealth_NetworkAddress, 0, sizeof(oCmd.m_aucDeviceNeighborHealth_NetworkAddress) );
	switch( p_pHdr->m_ucServiceType )
	{	case DEVICE_LIST_REPORT: 	oCmd.m_ushReportsRequested = SMO_REPORT_REQ_DEVICE_LIST; break;
		case TOPOLOGY_REPORT:	 	oCmd.m_ushReportsRequested = SMO_REPORT_REQ_TOPOLOGY; break;
		case SCHEDULE_REPORT:		oCmd.m_ushReportsRequested = SMO_REPORT_REQ_SCHEDULE; break;
		case DEVICE_HEALTH_REPORT:	oCmd.m_ushReportsRequested = SMO_REPORT_REQ_DEVICE_HEALTH; break;
		case NEIGHBOR_HEALTH_REPORT:oCmd.m_ushReportsRequested = SMO_REPORT_REQ_NEIGHBOR_HEALTH;
			memcpy( oCmd.m_aucDeviceNeighborHealth_NetworkAddress, p_pRequestData, sizeof(oCmd.m_aucDeviceNeighborHealth_NetworkAddress) );
		break;
		case NETWORK_HEALTH_REPORT:		oCmd.m_ushReportsRequested = SMO_REPORT_REQ_NETWORK_HEALTH;
			g_stApp.m_oGwUAP.RequestContractList();/// Attempt to get the most recent contract list
		break;
		case NETWORK_RESOURCE_REPORT: 	oCmd.m_ushReportsRequested = SMO_REPORT_REQ_NETWORK_RESOURCE; break;
	}

	///DEBUG
	//oCmd.m_ushReportsRequested |= 0x006F;
	//oCmd.m_ushReportsRequested |= 0x007F; memcpy( oCmd.m_aucDeviceNeighborHealth_NetworkAddress, ??? , sizeof(oCmd.m_aucDeviceNeighborHealth_NetworkAddress) );

	unsigned nDataSize = p_pHdr->m_nDataSize;
	if( nDataSize < 16 )
	{
		nDataSize = 16;
		p_pRequestData = oCmd.m_aucDeviceNeighborHealth_NetworkAddress;
	}
	///DEBUG end

	oCmd.HTON();
	EXEC_REQ_SRVC stExecReq = { m_unSrcOID: m_ushInterfaceOID, m_unDstOID: SM_SMO_OBJ_ID,
		m_ucReqID: 0, m_ucMethID: SMO_METHOD_GENERATE_REPORT, m_unLen: sizeof( oCmd ), p_pReqData: (uint8*)&oCmd };

//	LOG( "requestReport(S %u T %u) %cGSAP Meth %u Reports %04X Addr %s", p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID,
//		p_pHdr->IsYGSAP() ? 'Y' : ' ', stExecReq.m_ucMethID, ntohs(oCmd.m_ushReportsRequested), GetHex( oCmd.m_aucDeviceNeighborHealth_NetworkAddress, 16 ) );
	/// Request a new REPORT
	if( g_stApp.m_oGwUAP.SendRequestToASL( &stExecReq, SRVC_EXEC_REQ, UAP_SMAP_ID, m_unSysMngContractID, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, p_pHdr->m_ucServiceType ) )
	{	//LOG("report reset %04x",  ntohs(oCmd.m_ushReportsRequested) );
		for_each_report( &CSystemReportService::reportFunc_reset, ntohs(oCmd.m_ushReportsRequested), p_pHdr->m_nSessionID );

		Report * pRep = &m_aReport[ idx(p_pHdr->m_ucServiceType) ];
		if(	(SCHEDULE_REPORT ==  p_pHdr->m_ucServiceType)
		||	(NEIGHBOR_HEALTH_REPORT ==  p_pHdr->m_ucServiceType)
		||	(DEVICE_HEALTH_REPORT == p_pHdr->m_ucServiceType)	/// this one has multiple addresses
		||	( (DEVICE_LIST_REPORT == p_pHdr->m_ucServiceType) && p_pHdr->IsYGSAP()))
		{	///TODO: currently nDataSize includes? 4 CRC bytes which should be stripped off
			if( pRep->m_pExtraData )
			{
				delete [] pRep->m_pExtraData;	/// delete old
				pRep->m_pExtraData = NULL ;
			}

			pRep->m_pExtraData = new byte[ nDataSize ];
			memcpy( pRep->m_pExtraData, p_pRequestData, nDataSize);
			if( g_stApp.m_stCfg.AppLogAllowINF() )
			{
				LOG("requestReport(%s) set extra data(%u) %s%s", getGSAPServiceNameNoDir(p_pHdr->m_ucServiceType), nDataSize, GET_HEX_LIMITED(pRep->m_pExtraData, nDataSize, (unsigned)g_stApp.m_stCfg.m_nMaxSapInDataLog ));
			}
		}
		if( (DEVICE_LIST_REPORT == p_pHdr->m_ucServiceType) && !p_pHdr->IsYGSAP())
		{	/// GSAP after YGSAP: reset m_pExtraData
			if( pRep->m_pExtraData )
			{
				delete [] pRep->m_pExtraData;
				pRep->m_pExtraData = NULL;
			}
		}
	}
	else
	{	/// Cannot send to ASL, report error. Do not attempt to remove from tracker, it's not there
		confirm_Error( p_pHdr->m_ucServiceType, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID );
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Extract device address from an YDeviceSpecifier.
/// @param  pRet [OUT] contains either m_aucNetworkAddress 
///			NULL if no tag provided or invalid tag selector (m_uchTargetSelector!= 0 or 1), 
/// @retval status: STATUS_INVALID_TAG_SELECTOR: invalid tag selector
///		STATUS_ALL_TAGS: if no tag provided - for services that require information for all devices
///		STATUS_TAG_NOT_FOUND: cannot find network address associated with tag in "tag-addr translation table"
///		STATUS_SUCCESS: device tag/m_aucNetworkAddress extracted with success
/// @remarks it uses either device tag (search in tag-addr translation table) or
/// 			dirrectly m_aucNetworkAddress, depending on m_uchTargetSelector
/// @remarks if ! Y_TARGET_SELECTOR_TAG it does not change p_pTag
////////////////////////////////////////////////////////////////////////////////
int YGSAP_IN::YDeviceSpecifier::ExtractDeviceAddress(uint8_t** p_pRet) const
{
	if( m_uchTargetSelector == Y_TARGET_SELECTOR_TAG )
	{
		if( !m_uchDeviceTagSize )	// no tag provided
		{
			*p_pRet = NULL;
			return STATUS_ALL_TAGS;
		}

		YGSAP_IN::DeviceTag::Ptr pDevTag( new YGSAP_IN::DeviceTag( m_uchDeviceTagSize, m_aucDeviceTag ) );
		*p_pRet = g_stApp.m_oGwUAP.YDevListFind( pDevTag );
		if(*p_pRet)
		{
			LOG("YGSAP ExtractDeviceAddress [%s] -> %s", (const char *)*pDevTag, GetHex(*p_pRet, 16));
			return STATUS_SUCCESS;
		}
		else
		{
			LOG("WARNING YGSAP ExtractDeviceAddress Tag [%s] has no associated address", (const char *)*pDevTag );
			return STATUS_TAG_NOT_FOUND;
		}
	}
	else if( m_uchTargetSelector == Y_TARGET_SELECTOR_ADDR )
	{
		LOG("YGSAP ExtractDeviceAddress(addr) -> %s", GetHex(m_aucNetworkAddress, 16));
		*p_pRet = (uint8_t*)m_aucNetworkAddress;
		return STATUS_SUCCESS;
	}
	//else
	LOG("WARNING YGSAP ExtractDeviceAddress: invalid target selector %u", m_uchTargetSelector );
	*p_pRet = NULL;
	return STATUS_INVALID_TAG_SELECTOR;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Translate YGSAP tag into device address
/// @param p_pHdr [in]	The service requestes
/// @param p_pData [in]
/// @param aReqTranslated [out] pointer to valid req details: either translated req, or original req if translation is not necessary
/// 				or NULL if translation fail, caller must return error to user
/// @retval status STATUS_SUCCESS: all tag selectors were valid / all tags has been found and translated into addresses,
/// 		STATUS_INVALID_TAG_SELECTOR: at least one invalid tag selector, 
///		STATUS_NO_TAG: some tags cannot be found,fail the whole report, customer cannot identify the device by it's addr
/// @note TAKE CARE set p_pHdr->m_nDataSize before EACH return. Caller will use it to learn the request size
////////////////////////////////////////////////////////////////////////////////
int CSystemReportService::translateYGSAP( CGSAP::TGSAPHdr * p_pHdr, const void * p_pData, byte **p_aReqTranslated)
{	// 16 bytes/device*200 = 3200 bytes + some crc
	static byte aReqTranslated[ 4*1024 ];
	*p_aReqTranslated = aReqTranslated;
	
	int status = YGSAP_IN::STATUS_SUCCESS;

	if( !p_pHdr->IsYGSAP() || (p_pHdr->m_nDataSize <= 4 ))
	{
		*p_aReqTranslated = (byte *)p_pData;
		return status;
	}
	uint8_t * pDeviceAddr = NULL;
;

	switch (p_pHdr->m_ucServiceType)
	{
		case DEVICE_LIST_REPORT: /// TODO: this should have special handling. it is valid to have no input
		case SCHEDULE_REPORT:
		case NEIGHBOR_HEALTH_REPORT:
		{	YGSAP_IN::YDeviceSpecifier* p = (YGSAP_IN::YDeviceSpecifier*)p_pData;
			status = p->ExtractDeviceAddress(&pDeviceAddr);
		}
		break;
		case DEVICE_HEALTH_REPORT:	/// this one has multiple addresses
		{	unsigned i = 0;
			GSAP_IN::TDeviceHealthReqData * pReq = (GSAP_IN::TDeviceHealthReqData*)(aReqTranslated);
			YGSAP_IN::YDeviceHealthReqData * p   = (YGSAP_IN::YDeviceHealthReqData*)p_pData;
			for( uint16 j = 0; j < ntohs(p->m_ushNumberOfDevices); ++j)
			{
				status = p->m_oDevice[j].ExtractDeviceAddress(&pDeviceAddr);
				if (status != YGSAP_IN::STATUS_SUCCESS)
				{
					p_pHdr->m_nDataSize = 0;
					*p_aReqTranslated = NULL;
					return status;
				}
				memcpy( pReq->m_aucNetworkAddress[i], pDeviceAddr, 16);
				++i;
			}
			pReq->m_ushNumberOfDevices = htons(i);
			p_pHdr->m_nDataSize = sizeof(pReq->m_ushNumberOfDevices) + i * sizeof( pReq->m_aucNetworkAddress[0]);
			return YGSAP_IN::STATUS_SUCCESS;	/// all tags TRANSLATED into addresses
		}
		default:
			*p_aReqTranslated = (byte *)p_pData;
			return YGSAP_IN::STATUS_SUCCESS; /// no translation required
	}
	/// We can only have DEVICE_LIST_REPORT / SCHEDULE_REPORT / NEIGHBOR_HEALTH_REPORT at this point	
	if (status != YGSAP_IN::STATUS_SUCCESS)
	{
		p_pHdr->m_nDataSize = 0;
		*p_aReqTranslated = NULL;
		return status;
	}
	memcpy( aReqTranslated, pDeviceAddr, 16);
	p_pHdr->m_nDataSize = 16;
	return YGSAP_IN::STATUS_SUCCESS; /// one tag TRANSLATED into addresses

}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Format the DEVICE_LIST_REPORT report for specific request details - extract only request devices from the report with all devices
/// @param p_pWholeReport		[in]  The whole device list report.
///								[out] The extracted report
/// @param p_dwWholeReportLen	[in]  The whole device list report len
///								[out] The extracted report len
/// @param p_pRequestDetails	Request details: currently if provided: device addr
/// @param p_pFormattedReport	Pointer to pre-allocated buffer for storage.
///								The buffer must be at least p_dwWholeReportLen big (yeah, THAT big!)
///								because there is no easy way to pre-calculate the required size
/// @retval true if a report was extracted/formatted, (or left unchanged if it does not require formatting)
/// @retval false if report cannot be extracted / formatted
/// @remarks
/// @remarks TAKE CARE: changes parameters p_pWholeReport & p_dwWholeReportLen
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::formatReportDeviceList( const byte *& p_pWholeReport, uint32& p_dwWholeReportLen, const byte * p_pRequestDetails, byte * p_pFormattedReport )
{	const SAPStruct::DeviceListRsp* pWholeRep   = (const SAPStruct::DeviceListRsp*) p_pWholeReport;
	const SAPStruct::DeviceListRsp::Device * pD = (const SAPStruct::DeviceListRsp::Device*) &pWholeRep->deviceList[0];
	SAPStruct::DeviceListRsp* pFormatted  = (SAPStruct::DeviceListRsp*)p_pFormattedReport;
	const byte * pNetworkAddress = (const byte *) p_pRequestDetails;

	if( !isDeviceListFilterReq( pNetworkAddress ) )/// no device tag: full report requested (either standard GSAP or YGSAP with no parameters)
	{	LOG("formatReport(DEVICE_LIST) no Tag in request: return whole report %u ", p_dwWholeReportLen );
		return true;
	}

	if( p_dwWholeReportLen < pWholeRep->SIZE() )	/// Invalid report
	{
		LOG("WARNING SystemReportService::formatReportDeviceList: invalid report size (%u > %u)", pWholeRep->SIZE(), p_dwWholeReportLen);
		return false;
	}
	/// Write only uint16 numberOfDevices; Take care if fixed part increases in size
	pFormatted->numberOfDevices = htons( 1 );	/// one device returned

	for( int i = 0; i < ntohs(pWholeRep->numberOfDevices); ++i )
	{
		if( !memcmp( pD->networkAddress, pNetworkAddress, 16 ))	/// found
		{
			memcpy( pFormatted->deviceList, pD, pD->SIZE() );

			LOG("formatReport(DEVICE_LIST) for %s: whole report %u, return %u", GetHex(pNetworkAddress, 16), p_dwWholeReportLen, pFormatted->SIZE());
			p_dwWholeReportLen = pFormatted->SIZE();
			p_pWholeReport     = (const byte *)p_pFormattedReport;
			return true;	/// job done
		}
		pD = pD->NEXT();
	}
	/// Report req device not found, altough translation tag/addr was available
	/// Device was present at req time, but just dissapear when the report was returned. Unlikely but possible
	LOG("ERROR formatReport(DEVICE_LIST) Addr %s not found in returned report", GetHex(pNetworkAddress, 16) );

	return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Format the SCHEDULE report for specific request details - extract only request devices from the report with all devices
/// @param p_pWholeReport		[in]  The whole schedule report.
///								[out] The extracted report
/// @param p_dwWholeReportLen	[in]  The whole schedule report len
///								[out] The extracted report len
/// @param p_pRequestDetails	Request details: currently NetworkAddress the device to look for
/// @param p_pFormattedReport	Pointer to pre-allocated buffer for storage.
///								The buffer must be at least p_dwWholeReportLen big (yeah, THAT big!)
///								because there is no easy way to pre-calculate the required size
/// @retval true if a report was extracted/formatted, (or left unchanged if it does not require formatting)
/// @retval false if report cannot be extracted / formatted
/// @remarks
/// @remarks TAKE CARE: changes parameters p_pWholeReport & p_dwWholeReportLen
/// @remarks
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::formatReportSchedule( const byte *& p_pWholeReport, uint32& p_dwWholeReportLen, const byte * p_pRequestDetails, byte * p_pFormattedReport )
{	const SAPStruct::ScheduleReportRsp* pWholeSched = (const SAPStruct::ScheduleReportRsp*)p_pWholeReport;
	SAPStruct::ScheduleReportRsp* pSchedule   = (SAPStruct::ScheduleReportRsp*)p_pFormattedReport;
	int i;

	if( p_dwWholeReportLen < pWholeSched->SIZE() )	/// Invalid schedule report
	{
		LOG("WARNING SystemReportService::formatReportSchedule: invalid report size (%u > %u)", pWholeSched->SIZE(), p_dwWholeReportLen);
		return false;
	}

	/// Copy channel list and channel/device numbers
	/// TODO: Optimisation: copy up to deviceSchedule[ 0 ]
	memcpy( p_pFormattedReport, pWholeSched, p_dwWholeReportLen);

	const SAPStruct::ScheduleReportRsp::DeviceSchedule * pD = pWholeSched->GET_DeviceSchedule_PTR();
	pSchedule->numberOfDevices = htons( 1 );	/// Only ONE device is returned
	/// Find the device in pDevice
	for( i = 0; i < ntohs(pWholeSched->numberOfDevices) ;++i )
	{
		if( !memcmp( pD->networkAddress, p_pRequestDetails, 16 ) )	/// Found
			break;
		pD = pD->NEXT();
	}
	if( i == ntohs(pWholeSched->numberOfDevices) )
	{	/// at end of list - device not found
		LOG("WARNING SystemReportService::formatReport(SCHEDULE): cannot extract [%s] from valid report", GetHex(p_pRequestDetails, 16) );
		return false;
	}

	memcpy( (void*)&pSchedule->GET_DeviceSchedule_PTR()[0], (const void*)pD, pD->SIZE() );
	LOG("formatReport(SCHEDULE) for %s: whole report %u, return %u ", GetHex(p_pRequestDetails, 16), p_dwWholeReportLen, pSchedule->SIZE());
	p_dwWholeReportLen = pSchedule->SIZE();
	p_pWholeReport     = (const byte *)pSchedule;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Format the DEVICE_HEALTH_REPORT report for specific request details - extract only request devices from the report with all devices
/// @param p_pWholeReport		[in]  The whole device health report.
///								[out] The extracted report
/// @param p_dwWholeReportLen	[in]  The whole device health report len
///								[out] The extracted report len
/// @param p_pRequestDetails	Request details: list of NetworkAddress for the devices to look for
/// @param p_pFormattedReport	Pointer to pre-allocated buffer for storage.
///								The buffer must be at least p_dwWholeReportLen big (yeah, THAT big!)
///								because there is no easy way to pre-calculate the required size
/// @retval true if a report was extracted/formatted, (or left unchanged if it does not require formatting)
/// @retval false if report cannot be extracted / formatted
/// @remarks
/// @remarks TAKE CARE: changes parameters p_pWholeReport & p_dwWholeReportLen
/// @remarks
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::formatReportDeviceHealth( const byte *& p_pWholeReport, uint32& p_dwWholeReportLen, const byte * p_pRequestDetails, byte * p_pFormattedReport )
{	const SAPStruct::DeviceHealthRsp* pWholeRep = (const SAPStruct::DeviceHealthRsp*)p_pWholeReport;
	const GSAP_IN::TDeviceHealthReqData* pReq = (const GSAP_IN::TDeviceHealthReqData*) p_pRequestDetails;
	SAPStruct::DeviceHealthRsp* pDeviceHealth = (SAPStruct::DeviceHealthRsp*)p_pFormattedReport;
	int i, j;
	uint16 ushDevicesFound = 0;

	if( p_dwWholeReportLen < pWholeRep->SIZE() )	/// Invalid report
	{
		LOG("WARNING SystemReportService::formatReport(DEVICEHEALTH): invalid report size (%u > %u)", pWholeRep->SIZE(), p_dwWholeReportLen);
		return false;
	}
	/// parse req list and intersect with report list 	(pReq pWholeRep)
	/// each matching device is copied into 			pDeviceHealth
	/// the intersection is the number of devices
	for( i = 0; i < ntohs(pReq->m_ushNumberOfDevices); ++i )
	{	for( j = 0; j < ntohs(pWholeRep->numberOfDevices); ++j )
		{	if( !memcmp( pWholeRep->deviceList[j].networkAddress, pReq->m_aucNetworkAddress[i], 16 ) )
			{	/// Found it, add it to resultset
				LOG("formatReport(DEVICEHEALTH): found %s", GetHex(pWholeRep->deviceList[j].networkAddress, 16));
				pDeviceHealth->deviceList[ ushDevicesFound++ ] = pWholeRep->deviceList[j];
				break;
			}
		}
	}
	pDeviceHealth->numberOfDevices = htons( ushDevicesFound );
	LOG("formatReport(DEVICEHEALTH): search %u in %u => found %u. Whole report size %u, return %u",
		ntohs(pReq->m_ushNumberOfDevices), ntohs(pWholeRep->numberOfDevices), ushDevicesFound, p_dwWholeReportLen, pDeviceHealth->SIZE());

	if( !ushDevicesFound )
		return false;	/// we should not return success with no devices in report

	p_dwWholeReportLen = pDeviceHealth->SIZE();
	p_pWholeReport     = (const byte *)pDeviceHealth;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Format the report for specific request details - extract only request devices from the report with all devices
/// @param p_ucServiceType
/// @param p_pRequestDetails	Request details: currently NetworkAddress the device to look for
/// @param p_pWholeReport		[in]  The whole schedule report.
///								[out] The extracted report
/// @param p_dwWholeReportLen	[in]  The whole schedule report len
///								[out] The extracted report len
/// @param p_pFormattedReport	Pointer to pre-allocated buffer for storage.
///								The buffer must be at least p_dwWholeReportLen big (yeah, THAT big!)
///								because there is no easy way to pre-calculate the required size
/// @retval true if a report was extracted/formatted, (or left unchanged if it does not require formatting)
/// @retval false if report cannot be extracted / formatted
/// @remarks Current implementation only extracts SCHEDULE_REPORT/DEVICE_HEALTH_REPORT and is some cases DEVICE_LIST_REPORT
/// @remarks
/// @remarks TAKE CARE: changes parameters p_pWholeReport & p_dwWholeReportLen
/// @remarks
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::formatReport( uint8 p_ucServiceType, const byte *& p_pWholeReport, uint32& p_dwWholeReportLen, const byte * p_pRequestDetails, byte * p_pFormattedReport )
{
	if( !p_pRequestDetails )
		return true;	/// No formatting is possible
	switch( p_ucServiceType )
	{	case DEVICE_LIST_REPORT:	return formatReportDeviceList(     p_pWholeReport, p_dwWholeReportLen, p_pRequestDetails, p_pFormattedReport );
		case SCHEDULE_REPORT:		return formatReportSchedule(       p_pWholeReport, p_dwWholeReportLen, p_pRequestDetails, p_pFormattedReport );
		case DEVICE_HEALTH_REPORT:	return formatReportDeviceHealth(   p_pWholeReport, p_dwWholeReportLen, p_pRequestDetails, p_pFormattedReport );
		default: return true;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA100 -> GW) Verify the APDU
/// @param TBD
/// @retval true	- the APDU is valid
/// @retval false	- the APDU is invalid
///@remarks we try to log detailed error cause
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::validateAPDU( uint16 p_unAppHandle, GENERIC_ASL_SRVC* p_pRsp, GENERIC_ASL_SRVC* p_pReq )
{
	if( SRVC_EXEC_RSP != p_pRsp->m_ucType ){							/// State machine reset below
		LOG("ERROR SystemReportService::ProcessAPDU(H %u): cannot handle ADPU type %X", p_unAppHandle, p_pRsp->m_ucType );
		return false;
	}
	else if( SM_SMO_OBJ_ID != p_pRsp->m_stSRVC.m_stExecRsp.m_unSrcOID ){	/// State machine reset below
		LOG("ERROR SystemReportService::ProcessAPDU(H %u): unknown ADPU SrcOID %d", p_unAppHandle, p_pRsp->m_stSRVC.m_stExecRsp.m_unSrcOID );
		return false;
	}
	else if( SFC_SUCCESS != p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC ){ 		/// State machine reset below
		LOG("ERROR SystemReportService::ProcessAPDU(H %u): m_ucSFC %d != SFC_SUCCESS", p_unAppHandle, p_pRsp->m_stSRVC.m_stExecRsp.m_ucSFC );
		return false;
	}
	else if(( SMO_METHOD_GENERATE_REPORT != p_pReq->m_stSRVC.m_stExecReq.m_ucMethID )
		&&	( SMO_METHOD_GET_BLOCK       != p_pReq->m_stSRVC.m_stExecReq.m_ucMethID ) ){
		LOG("ERROR SystemReportService::ProcessAPDU(H %u): invalid MethodId %d", p_unAppHandle, p_pReq->m_stSRVC.m_stExecReq.m_ucMethID );
		return false;
	}
	return true;	/// else is valid
}
////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Verify the user request: service type and data size. if necessary, does NTOH()
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @retval true	- the request is valid
/// @retval false	- the request is invalid
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::validateUserRequest( CGSAP::TGSAPHdr * p_pHdr, void * p_pData )
{	uint32 dwRequiredDataSize = 0;
	switch( p_pHdr->m_ucServiceType )
	{
		case DEVICE_LIST_REPORT:dwRequiredDataSize = p_pHdr->IsYGSAP()
				? sizeof(YGSAP_IN::YDeviceSpecifier) : 0;
			break;

		case SCHEDULE_REPORT: dwRequiredDataSize = p_pHdr->IsYGSAP()
				? sizeof(YGSAP_IN::YDeviceSpecifier) : sizeof(GSAP_IN::TScheduleReqData);
			break;

		case DEVICE_HEALTH_REPORT:
		{	// honestly, YDeviceHealthReqData and TDeviceHealthReqData have the same size
			size_t minSize = 4 + p_pHdr->IsYGSAP() ? sizeof(YGSAP_IN::YDeviceHealthReqData): sizeof(GSAP_IN::TDeviceHealthReqData); // 4: CRC size
			if( p_pHdr->m_nDataSize < minSize )
			{	LOG("ERROR SystemReportService::validateUserRequest[%s].first data size %d < %d",
					getGSAPServiceNameNoDir(p_pHdr->m_ucServiceType), p_pHdr->m_nDataSize, minSize );
				LOG_HEX("ERROR Data: ", (const unsigned char*)p_pData, p_pHdr->m_nDataSize);
				return false;
			}
			if( p_pHdr->IsYGSAP())
			{	const YGSAP_IN::YDeviceHealthReqData * pDeviceHealthYGSAP = (const YGSAP_IN::YDeviceHealthReqData*)p_pData;
				dwRequiredDataSize = pDeviceHealthYGSAP->SIZE();
			}
			else
			{	const GSAP_IN::TDeviceHealthReqData * pDeviceHealthGSAP   = (const GSAP_IN::TDeviceHealthReqData*) p_pData;
				dwRequiredDataSize = pDeviceHealthGSAP->SIZE();
			}
		}
		break;

		case NEIGHBOR_HEALTH_REPORT: dwRequiredDataSize = p_pHdr->IsYGSAP()
				? sizeof(YGSAP_IN::YDeviceSpecifier) : sizeof(GSAP_IN::TNeighborHealthReqData);
			break;

		case TOPOLOGY_REPORT:
		case NETWORK_HEALTH_REPORT:
		case NETWORK_RESOURCE_REPORT:
			break;
		default:	/// PROGRAMMER ERROR: We should never get to default
			LOG("ERROR SystemReportService::validateUserRequest: unknown/unimplemented service type %u", p_pHdr->m_ucServiceType );
			return false;/// Caller will SendConfirm with G_STATUS_UNIMPLEMENTED
	}
	if( p_pHdr->m_nDataSize < dwRequiredDataSize)
	{	LOG("ERROR SystemReportService::validateUserRequest[%s] data size %u < %u",
		   getGSAPServiceNameNoDir(p_pHdr->m_ucServiceType), p_pHdr->m_nDataSize, dwRequiredDataSize);
		LOG_HEX("ERROR Data: ", (const unsigned char*)p_pData, p_pHdr->m_nDataSize);
		return false;
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Check is there are pending requests for specified service. Reset pending req list if necessary
/// @param p_ucServiceType
/// @retval true	there are pending requests
/// @retval false	no  pending requests
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::isRequestPending( byte p_ucServiceType )
{ 	Report * pRep = &m_aReport[ idx(p_ucServiceType) ];
	if( pRep->m_bRequestPending && (pRep->m_uSecGen.GetElapsedMSec() > 1000*60*5))	/// 5 minutes
	{
		LOG("ERROR SystemReportService::isRequestPending: requests are pending for too long, RESET");
		for( TRequestList::const_iterator it = pRep->m_lstPendingRequests.begin(); it != pRep->m_lstPendingRequests.end(); ++it )
		{	/// Send to subsequent requestors, BUT DO not remove from tracker (subsequent requestors are not added to tracker)
			confirm_Error( p_ucServiceType, it->m_nSessionID, it->m_dwTransactionID );
		}
		pRep->m_lstPendingRequests.clear();
		pRep->m_bRequestPending = false;
	}
	return m_aReport[ idx( p_ucServiceType ) ].m_bRequestPending;
};

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Atempt to return the report from cache
/// @param p_pHdr		GSAP request header (has member m_nDataSize)
/// @param pData		GSAP request data (data size is in p_pHdr)
/// @retval true	data found in valid in cache, confirm sent to user
/// @retval false	data not found in cache or not valid -> confirm not sent to user, go the long way trough SM
/// @note expect user request to have enough bytes - mut call AFTER validateUserRequest
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::returnFromCache( CGSAP::TGSAPHdr * p_pHdr, const void * p_pData )
{
	if( !isValid( p_pHdr->m_ucServiceType, g_stApp.m_stCfg.m_nSystemReportsCacheTimeout ) )
		return false;
	/// The cache is valid at this point

	byte *  pBin   = m_aReport[ idx(p_pHdr->m_ucServiceType) ].m_pBin;
	uint32	unSize = m_aReport[ idx(p_pHdr->m_ucServiceType) ].m_unSize;
	byte	aucFormatted [ unSize ];	/// The formatted report cannot get bigger than the original report

	if( !matchRequest2CacheData( p_pHdr->m_ucServiceType, p_pData ) )
		return false;

	/// formatReport may change pBin to point to aucFormatted instead of m_aReport[ idx(p_pHdr->m_ucServiceType) ].m_pBin
	/// and also change unSize
	if( !formatReport( p_pHdr->m_ucServiceType, (const byte*&)pBin, unSize, (const byte *)p_pData, aucFormatted) )
		return false;

	LOG("SystemReportService: %s serve from CACHE (age: %d seconds)", getGSAPServiceNameNoDir( p_pHdr->m_ucServiceType  ), age( p_pHdr->m_ucServiceType ) );
	confirm( p_pHdr->m_ucServiceType, p_pHdr->m_nSessionID, p_pHdr->m_unTransactionID, pBin, unSize, false );
	return true;	/// User request processed ok, regardless if the service was available or not
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (USER -> GW) Match REQUEST and CACHE extra data
/// @param p_ucServiceType	Service type
/// @param p_pData			GSAP request data
/// @retval true	REQUEST/CACHE extra data match (also match if there is no data)
/// @retval false	REQUEST/CACHE extra data mismatch. Can happen only on NEIGHBOR_HEALTH_REPORT
/// @note expect user request to have enough bytes - mut call AFTER validateUserRequest
///
/// @note We request EXACT match in this implementation. Partial match implementations are possible
/// @note Only NEIGHBOR_HEALTH_REPORT check for parameter match - all other reports are sent by SM with all devices inside
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::matchRequest2CacheData( byte p_ucServiceType, const void * p_pData)
{
	switch( p_ucServiceType )
	{
		case NEIGHBOR_HEALTH_REPORT:
		{	const GSAP_IN::TNeighborHealthReqData * pReq   = (const GSAP_IN::TNeighborHealthReqData *) p_pData;
			const GSAP_IN::TNeighborHealthReqData * pCache = (const GSAP_IN::TNeighborHealthReqData *) m_aReport[ idx(p_ucServiceType) ].m_pExtraData;
			if( !pCache)
			{	LOG("matchRequest2CacheData[%s] NO CACHE", getGSAPServiceNameNoDir( p_ucServiceType ) );
				return false;
			}
			if( memcmp( pCache->m_aucNetworkAddress, pReq->m_aucNetworkAddress, 16 ) )
			{	//LOG("matchRequest2CacheData[%s] req/cache mismatch %s", getGSAPServiceNameNoDir( p_ucServiceType ), GetHex( pReq->m_aucNetworkAddress, 16 ) );
				return false;
			}
		}
		break;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Send confirm to all requestors with waiting requests in the pipeline and matching req details
/// @remarks the pipeline stores now only NEIGHBOR_HEALTH_REPORT so get out quick on other requests
/// @note call it only when there is no request pending for p_ucServiceType AND report is valid
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::sendConfirmToAllInPipeline( uint8 p_ucServiceType )
{	Report	* pRep = &m_aReport[ idx( p_ucServiceType ) ];
	if( (NEIGHBOR_HEALTH_REPORT != p_ucServiceType) || pRep->m_bRequestPending || !isValid(p_ucServiceType) )
		return;	/// this should never happen: call me only when there is no req pending

	GSAP_IN::TNeighborHealthReqData * pCachedReq = (GSAP_IN::TNeighborHealthReqData *) pRep->m_pExtraData;
	for( TRequestPipeline::iterator it = m_lstRequestPipeline.begin(); it != m_lstRequestPipeline.end(); )
	{	/// The report being confirmed is also found in pipeline - waiting to se sent
		/// Confirm dirrectly to speedup pipeline cleanup
		const GSAP_IN::TNeighborHealthReqData * pPielineReq = (const GSAP_IN::TNeighborHealthReqData*) it->m_pData;

//		LOG("PIPELINE DBG [%p %p] [%s %s]", pCachedReq, pPielineReq, GetHex( pCachedReq, 16), GetHex( pPielineReq, 16) );
		if( pCachedReq && pPielineReq && ( !memcmp( pCachedReq, pPielineReq, 16 ) ))	/// same extended parameters for request
		{	confirm( p_ucServiceType, it->m_oHdr.m_nSessionID, it->m_oHdr.m_unTransactionID, pRep->m_pBin, pRep->m_unSize, false );
			LOG("PIPELINE del  (S %2u T %5u) %s (Size %u) %s", it->m_oHdr.m_nSessionID, it->m_oHdr.m_unTransactionID,
				getGSAPServiceName( p_ucServiceType ), m_lstRequestPipeline.size(), GetHex(pPielineReq, 16));

			it = m_lstRequestPipeline.erase( it );
		}
		else
		{
			++it;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA100 -> GW) Verify the APDU. When request of type p_ucServiceType is fully received,
/// @brief send confirm to all in the pipeline then send next request of the same type from request pipeline.
/// @param p_ucServiceType	Service type
/// @note: TAKE CARE: pipeline knows to hold only NEIGHBOR_HEALTH_REPORT at this moment
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::processPipeline( uint8 p_ucServiceType )
{
	if( (DEVICE_HEALTH_REPORT != p_ucServiceType ) && (NEIGHBOR_HEALTH_REPORT != p_ucServiceType) )
		return;

	if( m_aReport[ idx( p_ucServiceType ) ].m_bRequestPending || !isValid(p_ucServiceType) )
		return;

	sendConfirmToAllInPipeline( p_ucServiceType );

	for( TRequestPipeline::iterator it = m_lstRequestPipeline.begin(); it != m_lstRequestPipeline.end(); ++it )
	{	if( p_ucServiceType == it->m_oHdr.m_ucServiceType )
		{	GSAP_IN::TNeighborHealthReqData * pPielineReq = (GSAP_IN::TNeighborHealthReqData *) it->m_pData;
			LOG("PIPELINE send (S %2u T %5u) %s (Size %u) %s",
				it->m_oHdr.m_nSessionID, it->m_oHdr.m_unTransactionID, getGSAPServiceName( it->m_oHdr.m_ucServiceType ),
					m_lstRequestPipeline.size(), GetHex(pPielineReq, 16));
			ProcessUserRequest( &(it->m_oHdr), it->m_pData );
			m_lstRequestPipeline.erase( it );
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA100 -> GW) when receiving a response to SMO.generateReport start the report transfer
/// @param p_unAppHandle
/// @param p_ucServiceType	Service type
/// @param p_unSize			Report size
/// @param p_unHandler		Report Handle. Do NOT  mistake with p_unAppHandle
/// @return true
/// @remarks Report create time is timestamped on requestReport
/// @note wo don't to populate m_pExtraData here (this may be a gw-generated call ).
/// m_pExtraData is only populated on user req and only used to respond to user requests
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::newReport( uint16 p_unAppHandle, byte p_ucServiceType, uint32 p_unSize, uint32 p_unHandler )
{	Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	if( g_stApp.m_stCfg.AppLogAllowINF() )
		LOG("newReport[%s] ServiceH %u Size %u MaxBlockSize %u",  getGSAPServiceNameNoDir( p_ucServiceType ), p_unHandler, p_unSize, m_dwMaxBlockSize);
	if( pRep->m_pBin )
		delete [] pRep->m_pBin;

	pRep->m_unSize		= p_unSize;
	pRep->m_unHandler	= p_unHandler;
	pRep->m_pBin		= new byte[ p_unSize ];
	pRep->m_unReceived	= 0;
	return reqReportBlock( p_unAppHandle, p_ucServiceType ); ///< SMO.getBlock
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (GW -> ISA100) Make a request to SM: SMO.getDevListBlock to get a block from a service
/// @param p_unHandle The service handler
/// @param p_pReport The report requested
/// @retval true
/// @remarks This request is part of a request chain
///
/// @note Auto-generated requests (i.e. without a corresponding SAP_IN) are tracked with m_nSessionID == 0
/// @note and m_dwTransactionID == 0. This method does not send SAP_OUT for messages with m_nSessionID == 0
///
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::reqReportBlock( uint16 p_unAppHandle, byte p_ucServiceType )
{	Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	TSMOGetBlockParameters reqParameters = { pRep->m_unHandler, pRep->m_unReceived, pRep->nextBlockSize( m_dwMaxBlockSize ) };

	EXEC_REQ_SRVC stExecReq = {	m_unSrcOID: m_ushInterfaceOID, m_unDstOID:SM_SMO_OBJ_ID,
		m_ucReqID: 0, m_ucMethID: SMO_METHOD_GET_BLOCK, m_unLen: sizeof( reqParameters ), p_pReqData: (byte*)&reqParameters };

	//LOG("reqReportBlock(H %u ServiceH %u) off %u size %u  Meth %u", p_unAppHandle, pRep->m_unHandler, reqParameters.offset, reqParameters.size, stExecReq.m_ucMethID);
	reqParameters.HTON();
	MSG * pMSG = g_stApp.m_oGwUAP.m_tracker.FindMessage( p_unAppHandle, "reqReportBlock" );
	if( !pMSG )
		return false;	/// The session/transaction info is lost, there is nothing to do

	bool bRet;
	/// Only one report will enter here, the one requested by user which triggered SMO.generateReport
	if( pMSG->m_ServiceType == p_ucServiceType )
		bRet = g_stApp.m_oGwUAP.SendRequestToASL( &stExecReq, SRVC_EXEC_REQ, UAP_SMAP_ID, m_unSysMngContractID, p_unAppHandle );
	else	/// This requests is auto-generated. Use  m_nSessionID == 0 and m_dwTransactionID == 0 to avoid sending SAP_OUT
			/// We do not take service type from p_unAppHandle -> pMSG because a SMO.generateReport
			/// may generate multiple SMO.getBlock out of which only one is of type pMSG->m_ServiceType (handled above)
			/// This is true only when requesting the first block (offset 0). The rest of the command
			/// chain SMO.getBlock will always use the branch above
			/// The RTT's corresponding to GW-generated comands handled here will be incorrect
			/// (smaller than reality with the amount of SMO.generateReport RTT)
		bRet = g_stApp.m_oGwUAP.SendRequestToASL( &stExecReq, SRVC_EXEC_REQ, UAP_SMAP_ID, m_unSysMngContractID, 0/*nSessionID*/, 0 /*nTransactionID*/, p_ucServiceType );///< 0, 0 indicates we don't want SAP_OUT
	if( !bRet )
	{
		if( pMSG->m_ServiceType == p_ucServiceType )
		{	/// Send confirm only when the user requested something
			confirm_Error( p_ucServiceType, p_unAppHandle );	/// will attempt to remove from tracker
		}	/// else is a GW-generated chain of commands
		clean( p_ucServiceType, pRep );
	}
	return bRet;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Cleanup the report, reset the state machine, free memory, send confirm( ERROR ) for all pending requests
/// @param p_ucServiceType The service type
/// @return none
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::clean( byte p_ucServiceType, Report * p_pRep  )
{
	LOG("SystemReportService::clean(%s) %u pending", getGSAPServiceNameNoDir( p_ucServiceType ), p_pRep->m_lstPendingRequests.size() );
	for( TRequestList::const_iterator it = p_pRep->m_lstPendingRequests.begin(); it != p_pRep->m_lstPendingRequests.end(); ++it )
	{	/// Send to subsequent requestors, BUT DO not remove from tracker (subsequent requestors are not added to tracker)
		confirm_Error( p_ucServiceType, it->m_nSessionID, it->m_dwTransactionID );
	}
	p_pRep->m_lstPendingRequests.clear();
	p_pRep->m_bRequestPending = false;
	p_pRep->m_unReceived = 0;
	if( p_pRep->m_pBin )
	{
		delete [] p_pRep->m_pBin;
		p_pRep->m_pBin = NULL;
	}
	if( p_pRep->m_pExtraData )
	{
		delete p_pRep->m_pExtraData;	/// delete old
		p_pRep->m_pExtraData = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief (ISA100 -> GW) Add a packet to report stored locally
/// @param p_ucServiceType
/// @param p_unAppHandle - the request handle, used to find SessionID/TransactionID in tracker
/// @param p_unHandler     - handler received, must match m_unHandler
/// @param p_unOffset      - offset received, always be multiple of m_dwMaxBlockSize
/// @param p_ushPacketSize - packet size received, must match m_dwMaxBlockSize on all
///                          packets except last, where p_ushPacketSize <= m_dwMaxBlockSize
/// @param p_pData         - packet data received
/// @retval true the topology is complete
/// @retval false the topology is incomplete
/// @remarks  CALL on response to SMO.getTopologyBlock
/// TODO
/// TODO parameter list is not ok, change it
/// TODO
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::addReportBlock( byte p_ucServiceType, uint16 p_unAppHandle, uint32 p_unHandler, uint32 p_unOffset, uint16 p_ushPacketSize, void * p_pData )
{	Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	if( g_stApp.m_stCfg.AppLogAllowINF() )
		LOG("addReportBlock(H %u ServiceH %u) off %u size %u", p_unAppHandle, p_unHandler, p_unOffset, p_ushPacketSize);
	if(		(p_unOffset % m_dwMaxBlockSize)				///< offset can only be multiple of m_dwMaxBlockSize
		||	((p_ushPacketSize < m_dwMaxBlockSize) && ( packets(p_unOffset) != (packets(pRep->m_unSize)-1) ))	/// Only last packet can be smaller than m_dwMaxBlockSize
		||	(p_ushPacketSize > m_dwMaxBlockSize) )		///< cannot receive more than requested (we req less on last packet, maybe we should check with the request)
	{
		LOG("ERROR SystemReportService::addReportBlock: invalid call off %u packRcv %u packMax %u packets(%u %u)",
			p_unOffset, p_ushPacketSize, m_dwMaxBlockSize, packets(p_unOffset), packets(pRep->m_unSize));
		/// just ignore the packet. Take care, this may lead to forever-pending requiest
		return false;/// Report incomplete
	}
	if( !pRep->m_pBin || (p_unHandler != pRep->m_unHandler) || (pRep->m_unReceived != p_unOffset))
	{	/// This is a block from a previously requested report, we cannot use it
		LOG("WARN SystemReportService:::addReportBlock: orphan block (ServiceH %u %u) (off %u %u). Ignored",
			pRep->m_unHandler, p_unHandler, pRep->m_unReceived, p_unOffset);
		return false;/// Report incomplete
	}

	memcpy( pRep->m_pBin + p_unOffset, p_pData, p_ushPacketSize);
	pRep->m_unReceived = p_unOffset + p_ushPacketSize;	/// m_unReceived == p_unOffset : check is done above

	if( pRep->m_unReceived == pRep->m_unSize )	/// report received complete
	{	/// Do NOT timestamp the report here (after it vas received complete). Do it when is generated (requestReport)
		LOG(/*"STATS: */"%s COMPLETE (ServiceH %4u AppH=%4d Size %5u) received in %.3f seconds %s",
			getGSAPServiceNameNoDir( p_ucServiceType ), p_unHandler, p_unAppHandle, pRep->m_unSize, pRep->m_uSecGen.GetElapsedSec(), details(p_ucServiceType));
		if( p_ucServiceType == DEVICE_LIST_REPORT)
		{	/// update the device tag/device address association list
			g_stApp.m_oGwUAP.YDevListRefresh( pRep->m_unSize, (const SAPStruct::DeviceListRsp*)pRep->m_pBin);
		}
		if( p_ucServiceType == NETWORK_HEALTH_REPORT)
		{	/// update GPDU statistics
			fillGPDUStats( pRep->m_pBin, pRep->m_unSize );
		}

		if(	g_stApp.m_stCfg.AppLogAllowDBG()
		||  (	g_stApp.m_stCfg.m_nDisplayReports
			&& (	(p_ucServiceType == DEVICE_LIST_REPORT)
				||	(p_ucServiceType == NETWORK_HEALTH_REPORT)
			   ) ) )
		{
			dumpReport( p_ucServiceType );
		}
		return true;	/// Report COMPLETE, return true
	}

	/// Request next block from SM
	reqReportBlock( p_unAppHandle, p_ucServiceType );	/// SMO.getBlock
	return false;	/// Report incomplete
}

void CSystemReportService::reportFunc_sessionDelete( byte p_ucServiceType, uint32 p_nSessionID, uint32 /*p_p2*/, uint32 /*p_p3*/ )
{
	Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	LOG("SystemReportService::ProcessSessionDelete(%u) search in %u pending %s requests", p_nSessionID,
		pRep->m_lstPendingRequests.size(), getGSAPServiceNameNoDir( p_ucServiceType ) );
	for( TRequestList::iterator it = pRep->m_lstPendingRequests.begin(); it != pRep->m_lstPendingRequests.end(); )
	{
		if( it->m_nSessionID  == p_nSessionID )
		{	/// TODO TBD if this is true: We simply erase without sending confirm: there is no session to confirm on
			LOG("SystemReportService::ProcessSessionDelete(S %u T %u)", p_nSessionID, it->m_dwTransactionID);
			it = pRep->m_lstPendingRequests.erase( it );
		}
		else
		{
			++it;
		}
	}
	if(pRep->m_nSessionID == p_nSessionID)
		pRep->m_bRequestPending = false;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Process session delete (expire or explicit delete) - deleting associated resources
/// @remarks Clean up pending requests generated on this session
/// @note: TAKE CARE: pipeline knows to hold only NEIGHBOR_HEALTH_REPORT at this moment
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::ProcessSessionDelete( unsigned  p_nSessionID )
{
	for( TRequestPipeline::iterator it = m_lstRequestPipeline.begin(); it != m_lstRequestPipeline.end(); )
	{	const GSAP_IN::TNeighborHealthReqData * pPielineReq = (const GSAP_IN::TNeighborHealthReqData *) it->m_pData;
		if( it->m_oHdr.m_nSessionID == p_nSessionID )
		{	LOG("PIPELINE del  (S %2u T %5u) %s (Size %u) %s", p_nSessionID, it->m_oHdr.m_unTransactionID,
				getGSAPServiceName( it->m_oHdr.m_ucServiceType ), m_lstRequestPipeline.size(),  GetHex(pPielineReq, 16));
			it = m_lstRequestPipeline.erase( it );
		}
		else
		{	++it;
		}
	}

	for_each_report( &CSystemReportService::reportFunc_sessionDelete, p_nSessionID );
}


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Iterates trough all reports and call a TPFunc3 for each
/// @param p_pFunc pointer to function type TPFunc3
/// @param p_p1 parameter to pass to p_pFunc
/// @param p_p2 parameter to pass to p_pFunc
/// @param p_p3 parameter to pass to p_pFunc
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::for_each_report( TPFunc3 p_pFunc, uint32 p_p1/*= 0*/, uint32 p_p2 /*= 0*/, uint32 p_p3 /*= 0*/ )
{
	for( byte i = DEVICE_LIST_REPORT; i <= NETWORK_RESOURCE_REPORT; ++i )
	{	if( i > NETWORK_HEALTH_REPORT )
			i = NETWORK_RESOURCE_REPORT;
		(this->*p_pFunc)( i, p_p1, p_p2, p_p3 );
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Return report details - number of devices in most cases
/// @param p_ucServiceType
////////////////////////////////////////////////////////////////////////////////
const char * CSystemReportService::details( byte p_ucServiceType ) const
{	static char szBuf[ 1024 ];
	const Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	szBuf[0] = 0;
	if (	!pRep->m_unReceived											// non-empty
		||	(pRep->m_unReceived != pRep->m_unSize) )
		return szBuf;

	SAPStruct::ScheduleReportRsp* pS = (SAPStruct::ScheduleReportRsp*)pRep->m_pBin;
	unsigned short sNrDev = ntohs(*(unsigned short *)pRep->m_pBin);
	switch(p_ucServiceType)
	{ 	case DEVICE_LIST_REPORT:		sprintf(szBuf, "Dev %u", sNrDev);	break;
		case TOPOLOGY_REPORT:			sprintf(szBuf, "Dev %u", sNrDev);	break;
		case SCHEDULE_REPORT:			sprintf(szBuf, "Dev %u Chan %u", pS->numberOfDevices, pS->numberOfChannels);	break;
		case DEVICE_HEALTH_REPORT:		sprintf(szBuf, "Dev %u", sNrDev);	break;
		case NEIGHBOR_HEALTH_REPORT:	sprintf(szBuf, "Nbr %u", sNrDev);	break;
		case NETWORK_HEALTH_REPORT:		sprintf(szBuf, "Dev %u", sNrDev);	break;
		case NETWORK_RESOURCE_REPORT:	sprintf(szBuf, "Net %u", *(unsigned char*)(pRep->m_pBin));	break;
	}
	return szBuf;
}


////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Display the whole report
/// @param p_ucServiceType
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::dumpReport( byte p_ucServiceType ) const
{	const Report * pRep = &m_aReport[ idx( p_ucServiceType ) ];
	if (	!pRep->m_unReceived											// non-empty
		||	(pRep->m_unReceived != pRep->m_unSize) )
		return false;	/// Nothin' to log

	/// Uncomment below to protect against untested parsing errors
	//return true;
	/// Uncomment above to protect against untested parsing errors

	switch(p_ucServiceType)
	{ 	case DEVICE_LIST_REPORT:
		{	const SAPStruct::DeviceListRsp* pDev = (const SAPStruct::DeviceListRsp*) pRep->m_pBin;
			const SAPStruct::DeviceListRsp::Device * pD = (const SAPStruct::DeviceListRsp::Device*) &pDev->deviceList[0];
			if(!pDev->VALID( pRep->m_unSize ))
				return false;

			if( isDeviceListFilterReq( pRep->m_pExtraData ) )
			{	LOG( "\tYGSAP REQ Device [%s]", GetHex( pRep->m_pExtraData, 16 ) );
			}

			for( int i = 0; i < ntohs(pDev->numberOfDevices); ++i )
			{
				LOG("\t%s EUI64 %s Battery %d (%s) %u:%s Type x%02X:%s", GetHex(pD->networkAddress, 16),
					GetHex(pD->uniqueDeviceID, 8),
					pD->powerSupplyStatus,
					( pD->powerSupplyStatus == 0 ) ? "  line" :
					( pD->powerSupplyStatus == 1 ) ? " > 75%" :
					( pD->powerSupplyStatus == 2 ) ? "25-75%" :
					( pD->powerSupplyStatus == 3 ) ? " < 25%" : "   unk",
					pD->joinStatus, getJoinStatusText(pD->joinStatus),
					ntohs(pD->deviceType), getCapabilitiesText(ntohs(pD->deviceType)) );
#if 1	//do not erase. Log manufacturer/model/revision/serialno
				char szTmp[5][256];
				memcpy(szTmp[0], pD->getManufacturerPTR()->data, pD->getManufacturerPTR()->size); szTmp[0][ pD->getManufacturerPTR()->size ] = 0;
				memcpy(szTmp[1], pD->getModelPTR()->data,        pD->getModelPTR()->size);        szTmp[1][ pD->getModelPTR()->size ] = 0;
				memcpy(szTmp[2], pD->getRevisionPTR()->data,     pD->getRevisionPTR()->size);     szTmp[2][ pD->getRevisionPTR()->size ] = 0;
				memcpy(szTmp[3], pD->getDeviceTagPTR()->data,    pD->getDeviceTagPTR()->size);    szTmp[3][ pD->getDeviceTagPTR()->size ] = 0;
				memcpy(szTmp[4], pD->getSerialNoPTR()->data,     pD->getSerialNoPTR()->size);     szTmp[4][ pD->getSerialNoPTR()->size ] = 0;
				LOG("\t\t(%s | %s | %s | %s | %s)",szTmp[0], szTmp[1], szTmp[2], szTmp[3], szTmp[4]);
#endif
				pD = pD->NEXT();
			}
			break;
		}
		case TOPOLOGY_REPORT:
		{	const SAPStruct::TopologyReportRsp* pTopology = (const SAPStruct::TopologyReportRsp*) pRep->m_pBin;
			const SAPStruct::TopologyReportRsp::Device * pD = (const SAPStruct::TopologyReportRsp::Device*) &pTopology->deviceList[0];
			if(!pTopology->VALID( pRep->m_unSize ))
				return false;

			for( int i = 0; i < ntohs(pTopology->numberOfDevices); ++i )
			{	LOG("\t%s Neighbors %u Graphs %u", GetHex(pD->networkAddress, 16), ntohs(pD->numberOfNeighbors), ntohs(pD->numberOfGraphs));
				int j;
				for( j = 0; j < ntohs(pD->numberOfNeighbors); ++j )
				{
					LOG("\t\tNeighbor %s IsClockSource: %s", GetHex(pD->neighborList[j].networkAddress, 16),
						(pD->neighborList[j].clockSource == 0) ? "NO" :
						(pD->neighborList[j].clockSource == 1) ? "YES, secondary" :
						(pD->neighborList[j].clockSource == 2) ? "YES, preferred" : "UNKNOWN");
				}
				const SAPStruct::TopologyReportRsp::Device::Graph* pDG = pD->GET_Graph_PTR();
				for( j = 0; j < ntohs(pD->numberOfGraphs); ++j )
				{	LOG("\t\tGraph ID %u members %u", ntohs(pDG->graphIdentifier), ntohs(pDG->numberOfMembers));
					for( int k = 0; k < ntohs(pDG->numberOfMembers); ++k )
					{	LOG("\t\t\tGraph member: %s", GetHex(pDG->graphMemberList[k], 16));
					}
					pDG = pDG->NEXT();
				}
				pD = pD->NEXT();
			}
			const SAPStruct::TopologyReportRsp::Backbone * pB = (const SAPStruct::TopologyReportRsp::Backbone *)pD;
			for( int i = 0; i < ntohs(pTopology->numberOfBackbones); ++i )
			{	LOG("\tBBR %s with SubnetID %u", GetHex(pB->networkAddress, 16), ntohs(pB->subnetID));
				pB = pB + 1;
			}
			break;
		}
		case SCHEDULE_REPORT:
		{	const SAPStruct::ScheduleReportRsp* pS = (const SAPStruct::ScheduleReportRsp*)pRep->m_pBin;
			const GSAP_IN::TScheduleReqData* p     = (const GSAP_IN::TScheduleReqData*)pRep->m_pExtraData;
			if(!pS->VALID( pRep->m_unSize ))
				return false;
			if( p )
				LOG( "\tREQ Device %s", GetHex( p->m_aucNetworkAddress, sizeof(p->m_aucNetworkAddress)));

			const SAPStruct::ScheduleReportRsp::DeviceSchedule * pD = pS->GET_DeviceSchedule_PTR();
			char szBuf[ 1024 ] = {0}, szTmp[32];
			int i;
			for( i = 0; i < pS->numberOfChannels; ++i )
			{	sprintf(szTmp, " %u:%02X", pS->channelList[i].channelNumber, pS->channelList[i].channelStatus);
				strcat( szBuf, szTmp );
			}
			LOG("\tChannels:%s", szBuf);
			const SAPStruct::ScheduleReportRsp::DeviceSchedule::Superframe * pSF;
			for( i = 0; i < ntohs(pS->numberOfDevices); ++i )
			{	//LOG("\tDEBUG %s", GetHex(pD, pD->SIZE()));
				LOG("\t%s Sf %u", GetHex(pD->networkAddress, 16), ntohs(pD->nrOfSuperframes) );
				pSF = (const SAPStruct::ScheduleReportRsp::DeviceSchedule::Superframe *) &pD->superframeList[0];
				for( int j = 0; j < ntohs(pD->nrOfSuperframes); ++j )
				{	LOG("\t\tSf_ID %u TimeSlots %4u Start %d Links %u", ntohs(pSF->superframeID),
						ntohs(pSF->numberOfTimeSlots), ntohl(pSF->startTime), ntohs(pSF->numberOfLinks) );
					for( int k = 0; k < ntohs(pSF->numberOfLinks); ++k )
					{	LOG("\t\t\t%s Idx %4u Period %4u Len %5u Chan %2u Dir %u Type %02X", GetHex(pSF->linkList[k].networkAddress, 16), ntohs(pSF->linkList[k].slotIndex), ntohs(pSF->linkList[k].linkPeriod), ntohs(pSF->linkList[k].slotLength), pSF->linkList[k].channelNumber, pSF->linkList[k].direction, pSF->linkList[k].linkType );
					}
					pSF = pSF->NEXT();
				}
				pD = pD->NEXT();
			}
			break;
		}
		case DEVICE_HEALTH_REPORT:
		{	const SAPStruct::DeviceHealthRsp* pD = (const SAPStruct::DeviceHealthRsp*)pRep->m_pBin;
			const GSAP_IN::TDeviceHealthReqData* p = (const GSAP_IN::TDeviceHealthReqData*)pRep->m_pExtraData;
			if(!pD->VALID( pRep->m_unSize ))
				return false;
			if( p )
			{	for( int i = 0; i < ntohs(p->m_ushNumberOfDevices); ++i)
				{	LOG( "\tREQ Device %s", GetHex( p->m_aucNetworkAddress[i], sizeof(p->m_aucNetworkAddress[0])));
				}
			}
			for( int i = 0; i < ntohs(pD->numberOfDevices); ++i )
			{	LOG("\t%s Tx %8u Rx %8u FailTx %6u FailRx %2u", GetHex(pD->deviceList[i].networkAddress, 16),
					pD->deviceList[i].tx, pD->deviceList[i].rx, pD->deviceList[i].failTx, pD->deviceList[i].failRx );
			}
			break;
		}
		case NEIGHBOR_HEALTH_REPORT:
		{	const SAPStruct::NeighborHealthRsp * pN = (const SAPStruct::NeighborHealthRsp*)pRep->m_pBin;
			const GSAP_IN::TNeighborHealthReqData * p = (const GSAP_IN::TNeighborHealthReqData*)pRep->m_pExtraData;
			if(!pN->VALID( pRep->m_unSize ))
				return false;
			if( p )
				LOG( "\tREQ Device %s", GetHex( p->m_aucNetworkAddress, sizeof(p->m_aucNetworkAddress)));
			for( int i = 0; i < ntohs(pN->numberOfNeighbors); ++i )
			{	LOG("\t%s Tx %8u Rx %8u FailTx %6u FailRx %2u SStrength %4d dBm SQuality %3u %s", GetHex(pN->neighborHealthList[i].networkAddress, 16),
					pN->neighborHealthList[i].tx, pN->neighborHealthList[i].rx, pN->neighborHealthList[i].failTx, pN->neighborHealthList[i].failRx,
					pN->neighborHealthList[i].signalStrength, pN->neighborHealthList[i].signalQuality,
					(pN->neighborHealthList[i].signalQuality < 63)  ? "poor" :
					(pN->neighborHealthList[i].signalQuality < 127) ? "fair" :
					(pN->neighborHealthList[i].signalQuality < 191) ? "good" :
																	  "exclnt" );
			}
			break;
		}
		case NETWORK_HEALTH_REPORT:
		{	const SAPStruct::NetworkHealthReportRsp* pN = (const SAPStruct::NetworkHealthReportRsp*)pRep->m_pBin;
			if(!pN->VALID( pRep->m_unSize ))
				return false;

			/// DPDUsSent counts only packets sent OK
			uint32_t unSendAttempts = pN->networkHealth.DPDUsSent + pN->networkHealth.DPDUsLost;
			
			LOG("\tNetId %3u Type %3u DevCount %4u Sent %8u Lost %7u PER %2d%% Latency %3u PathRel %3u DataRel %3u JoinNr %3u",
				pN->networkHealth.networkID, pN->networkHealth.networkType, pN->networkHealth.deviceCount,
				pN->networkHealth.DPDUsSent, pN->networkHealth.DPDUsLost,
				unSendAttempts ? (100 * pN->networkHealth.DPDUsLost / unSendAttempts) : 0,
				pN->networkHealth.GPDULatency,
				pN->networkHealth.GPDUPathReliability, pN->networkHealth.GPDUDataReliability, pN->networkHealth.joinCount);

			unsigned unDevicesNr = 0, unTotalJoinTime = 0, unMaxJoinTime = 0;	/// only for devices with JoinCount=1, compute average join time
			for( int i = 0; i < ntohs(pN->numberOfDevices); ++i )
			{	int nJoinSeconds = pN->deviceHealthList[i].startDate.seconds - pN->networkHealth.startDate.seconds;
				/// Consider only devices with a single join. For the rest, the join
				///	time is irrelevant because the report has last join time only
				if( 1 == pN->deviceHealthList[i].joinCount )
				{	++unDevicesNr;
					unTotalJoinTime += nJoinSeconds;
					unMaxJoinTime = _Max(unMaxJoinTime, (unsigned)nJoinSeconds);
				}
				/// DPDUsSent counts only packets sent OK
				unSendAttempts = pN->deviceHealthList[i].DPDUsSent + pN->deviceHealthList[i].DPDUsLost;
				LOG("\t%s Sent %8u Lost %7u PER %2d%% Latency %3u PathRel %3u DataRel %3u JoinNr %3u JoinIn %2d:%02d", GetHex(pN->deviceHealthList[i].networkAddress, 16),
					pN->deviceHealthList[i].DPDUsSent, pN->deviceHealthList[i].DPDUsLost,
					unSendAttempts ? (100 * pN->deviceHealthList[i].DPDUsLost / unSendAttempts) : 0,
					pN->deviceHealthList[i].GPDULatency, pN->deviceHealthList[i].GPDUPathReliability,
					pN->deviceHealthList[i].GPDUDataReliability, pN->deviceHealthList[i].joinCount,
	 				nJoinSeconds / 60, nJoinSeconds % 60 );
			}
			if(unDevicesNr)	/// avoid division by 0
			{	unTotalJoinTime /= unDevicesNr;
				unsigned unAvgjoinTime = unMaxJoinTime / unDevicesNr;
				LOG("\tSingle join devices JoinNr %3u average JoinIn %2d:%02d (%2d:%02d per device). Total single join time %2d:%02d",
					unDevicesNr, unTotalJoinTime/60, unTotalJoinTime%60, unAvgjoinTime/60, unAvgjoinTime%60, unMaxJoinTime/60, unMaxJoinTime%60);
			}

			break;
		}
		case NETWORK_RESOURCE_REPORT:
		{	const SAPStruct::NetworkResourceReportRsp* pN = (const SAPStruct::NetworkResourceReportRsp*)pRep->m_pBin;
			if(!pN->VALID( pRep->m_unSize ))
				return false;
			break;
		}
		default: return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Poll device list report for YGSAP
/// @note use m_nSessionID == 0 / m_dwTransactionID == 0 in call
/// @note Auto-generated requests (i.e. without a corresponding SAP_IN) are tracked with m_nSessionID == 0
/// @note and m_dwTransactionID == 0. The method confirm2all does not send SAP_OUT for messages with m_nSessionID == 0
////////////////////////////////////////////////////////////////////////////////
bool CSystemReportService::RequestDeviceList( void )
{
	if ( isRequestPending( DEVICE_LIST_REPORT ) )
		return true;

	/// Protocol version GSAP: no extra data for DEVICE_LIST_REPORT
	/// Session ID 0 is registered in tracker, but does not generate SAP_OUT (@see confirm2all / confirm_Error)
	CGSAP::TGSAPHdr hdr = { PROTO_VERSION_GSAP, DEVICE_LIST_REPORT, 0,0,0,0 };/// I don't care about crc
	LOG("RequestDeviceList polling DEVICE_LIST_REPORT");
	return requestReport( &hdr, NULL);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Indicate if DEVICE_LIST_REPORT request details contain a device
/// @param p_pRequestDetails	Request details: currently if provided: device addr
/// @return true if filter was requested, false if not
/// @remarks req detail may exist (@see requestReport, p_pRequestData assigned) but may be empty
////////////////////////////////////////////////////////////////////////////////
static bool isDeviceListFilterReq( const byte * p_pRequestDetails )
{	byte aEmptyAddr[16];
	memset( aEmptyAddr, 0, sizeof(aEmptyAddr) );
	/// formatReport() test also for !pNetworkAddress. leave it for safety
	return p_pRequestDetails && memcmp(p_pRequestDetails, aEmptyAddr, 16);
}

////////////////////////////////////////////////////////////////////////////////
/// @author Marcel Ionescu
/// @brief Update (fill) GPDU statistics into NETWORK_HEALTH_REPORT
/// @param p_pN	the NETWORK_HEALTH_REPORT data
/// @note Does not change GPDU statistics for devices with no publish.
/// We count on SM sending 0 in those fields
/// @note we return unGPDUPathReliability == GPDUPathReliability because they cannot be differentiated
/// @see e-mail on 1/27/2010 (thread start on 1/15/2010)
////////////////////////////////////////////////////////////////////////////////
void CSystemReportService::fillGPDUStats( uint8_t* p_pData, size_t p_unSize )
{
	SAPStruct::NetworkHealthReportRsp* pN = (SAPStruct::NetworkHealthReportRsp*) p_pData;
	unsigned unSumLatency = 0, unSumReliability = 0, unDeviceNumber = 0;/// devices with usable statistics
	uint8_t unGPDULatency, unGPDUPathReliability;

	if(!pN->VALID( p_unSize ))
		return;

	for( int i = 0; i < ntohs(pN->numberOfDevices); ++i )
	{	if( g_stApp.m_oGwUAP.GPDUStatsCompute( (const uint8_t*)pN->deviceHealthList[i].networkAddress, unGPDULatency, unGPDUPathReliability ) )
		{	++unDeviceNumber;
			unSumLatency     += unGPDULatency;
			unSumReliability += unGPDUPathReliability;
			pN->deviceHealthList[i].GPDULatency			= unGPDULatency;
			pN->deviceHealthList[i].GPDUPathReliability	= unGPDUPathReliability;
			pN->deviceHealthList[i].GPDUDataReliability	= unGPDUPathReliability;/// This is correct, Path/Data reliability cannot be differentiated
		}
	}
	if( unDeviceNumber )
	{	pN->networkHealth.GPDULatency			= unSumLatency / unDeviceNumber;
		pN->networkHealth.GPDUPathReliability	= unSumReliability / unDeviceNumber;
		pN->networkHealth.GPDUDataReliability	= unSumReliability / unDeviceNumber; /// This is correct, Path/Data reliability cannot be differentiated
	}
}
